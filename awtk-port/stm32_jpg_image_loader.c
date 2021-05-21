#include "tkc/mem.h"
#include "tkc/utils.h"
#include "jpegcodec.h"
#include "stm32_jpg_image_loader.h"

static jpeg_codec_typedef hjpgd;  	//JPEGӲ������ṹ��
static uint8_t s_inbuf_0[JPEG_DMA_INBUF_LEN];	//�� DMA ��������ʹ�õĻ���
static uint8_t s_inbuf_1[JPEG_DMA_INBUF_LEN]; //�� DMA ��������ʹ�õĻ���

//JPEG����������,�ص�����,���ڻ�ȡJPEG�ļ�ԭʼ����
//ÿ��JPEG DMA IN BUFΪ�յ�ʱ��,���øú���
static void jpeg_dma_in_callback(void) { 
	hjpgd.inbuf[hjpgd.inbuf_read_ptr].sta = 0;	//��buf�Ѿ���������
	hjpgd.inbuf[hjpgd.inbuf_read_ptr].size = 0;	//��buf�Ѿ��������� 
	hjpgd.inbuf_read_ptr++;					//ָ����һ��buf
	if(hjpgd.inbuf_read_ptr >= JPEG_DMA_INBUF_NB) {
		hjpgd.inbuf_read_ptr=0;				//����
	}
	if(hjpgd.inbuf[hjpgd.inbuf_read_ptr].sta==0) {//����Чbuf
		JPEG_IN_DMA_Pause();					//��ͣ��ȡ����
		hjpgd.indma_pause=1;					//��ͣ��ȡ����
	} else {												//��Ч��buf
		JPEG_IN_DMA_Resume((u32)hjpgd.inbuf[hjpgd.inbuf_read_ptr].buf,hjpgd.inbuf[hjpgd.inbuf_read_ptr].size);//������һ��DMA����
	}
}

//JPEG���������(YCBCR)�ص�����,�������YCbCr������
static void jpeg_dma_out_callback(void) {	  
	u32 *pdata = 0; 
	hjpgd.outbuf[hjpgd.outbuf_write_ptr].sta = 1;	//��buf����
	hjpgd.outbuf[hjpgd.outbuf_write_ptr].size = JPEG_DMA_OUTBUF_LEN-(DMA2_Stream1->NDTR<<2);//��buf�������ݵĳ���
	if(hjpgd.state == JPEG_STATE_FINISHED){		//����ļ��Ѿ��������,��Ҫ��ȡDOR��������(<=32�ֽ�)
		pdata = (u32*)(hjpgd.outbuf[hjpgd.outbuf_write_ptr].buf + hjpgd.outbuf[hjpgd.outbuf_write_ptr].size);
		while(JPEG->SR&(1<<4)){
			*pdata = JPEG->DOR;
			pdata++;
			hjpgd.outbuf[hjpgd.outbuf_write_ptr].size += 4; 
		}
	}  
	hjpgd.outbuf_write_ptr++;					//ָ����һ��buf
	if(hjpgd.outbuf_write_ptr>=JPEG_DMA_OUTBUF_NB) {
		hjpgd.outbuf_write_ptr = 0;			//����
	}
	if(hjpgd.outbuf[hjpgd.outbuf_write_ptr].sta==1)//����Чbuf
	{
		JPEG_OUT_DMA_Pause();						//��ͣ�������
		hjpgd.outdma_pause = 1;					//��ͣ�������
	}else{														//��Ч��buf
		JPEG_OUT_DMA_Resume((u32)hjpgd.outbuf[hjpgd.outbuf_write_ptr].buf, JPEG_DMA_OUTBUF_LEN);//������һ��DMA����
	}
}

//JPEG�����ļ�������ɻص�����
static void jpeg_endofcovert_callback(void) { 
	hjpgd.state = JPEG_STATE_FINISHED;			//���JPEG�������
}
//JPEG header�����ɹ��ص�����
static void jpeg_hdrover_callback(void) {
	bitmap_t* image = (bitmap_t*)hjpgd.ctx;
	hjpgd.state=JPEG_STATE_HEADEROK;			//HEADER��ȡ�ɹ�
	JPEG_Get_Info(&hjpgd);						//��ȡJPEG�����Ϣ,������С,ɫ�ʿռ�,������
	JPEG_GetDecodeColorConvertFunc(&hjpgd.Conf, &hjpgd.ycbcr2rgb, &hjpgd.total_blks);//��ȡJPEGɫ��ת������,�Լ���MCU��
	
	if (image != NULL && hjpgd.Conf.ImageWidth > 0 && hjpgd.Conf.ImageHeight > 0) {
		image->w = hjpgd.Conf.ImageWidth;
    image->h = hjpgd.Conf.ImageHeight; 
    image->flags = BITMAP_FLAG_IMMUTABLE;
    bitmap_set_line_length(image, 0);
#if LCD_PIXFORMAT == LCD_PIXFORMAT_ARGB8888 
    image->format = BITMAP_FMT_BGRA8888;
#else
    image->format = BITMAP_FMT_BGR565;
#endif
    bitmap_alloc_data(image);
	}
}

static ret_t jpeg_deinit(jpeg_codec_typedef *tjpeg) {
	int i = 0;
	for(i = 0; i < JPEG_DMA_OUTBUF_NB; i++) {
		if (tjpeg->outbuf[i].buf != NULL) {
			TKMEM_FREE(tjpeg->outbuf[i].buf);
		}
	}
	return RET_OK;
}

static ret_t jpeg_init(jpeg_codec_typedef *tjpeg, bitmap_t *image) {
	int i = 0;
	memset(tjpeg, 0x0, sizeof(jpeg_codec_typedef));
	tjpeg->inbuf[0].buf = s_inbuf_0;
	tjpeg->inbuf[1].buf = s_inbuf_1;

	for(i = 0; i < JPEG_DMA_OUTBUF_NB; i++) {
		tjpeg->outbuf[i].buf = TKMEM_ALLOC(JPEG_DMA_OUTBUF_LEN + 32);//�п��ܻ����Ҫ32�ֽ��ڴ�
		if(tjpeg->outbuf[i].buf == NULL) {
			jpeg_deinit(tjpeg);
			return_value_if_fail(tjpeg->outbuf[i].buf != NULL, RET_OOM);
		}   
	}
	tjpeg->ctx = image;
	return RET_OK;
}

static ret_t image_loader_get_yuv_and_set_image_data(uint8_t* buff, int32_t size, bitmap_t *image) {
	uint8_t* d = NULL;
	uint32_t timecnt = 0;
	uint32_t mcublkindex = 0;  
	return_value_if_fail(buff != NULL && image != NULL, RET_BAD_PARAMS);

	while(1) {
    SCB_CleanInvalidateDCache();				//���D catch
		if(hjpgd.inbuf[hjpgd.inbuf_write_ptr].sta == 0 && size > 0)	{ //��bufΪ��
			int s = tk_min(size, JPEG_DMA_INBUF_LEN);
			hjpgd.inbuf[hjpgd.inbuf_write_ptr].sta = 1;
			hjpgd.inbuf[hjpgd.inbuf_write_ptr].size = s;
			memcpy(hjpgd.inbuf[hjpgd.inbuf_write_ptr].buf, buff, s);	
			buff += s;
			size -= s;
			if (size <= 0) {
				timecnt = 0;
			}
			if(hjpgd.indma_pause == 1 && hjpgd.inbuf[hjpgd.inbuf_read_ptr].sta == 1)//֮ǰ����ͣ����,��������
			{
				JPEG_IN_DMA_Resume((u32)hjpgd.inbuf[hjpgd.inbuf_read_ptr].buf, hjpgd.inbuf[hjpgd.inbuf_read_ptr].size);	//������һ��DMA����
				hjpgd.indma_pause = 0;
			}
			hjpgd.inbuf_write_ptr++;
			if(hjpgd.inbuf_write_ptr >= JPEG_DMA_INBUF_NB) {
				hjpgd.inbuf_write_ptr = 0;
			}
		}
		if(hjpgd.outbuf[hjpgd.outbuf_read_ptr].sta == 1){	//buf����������Ҫ����
			if (d == NULL) {
				d = bitmap_lock_buffer_for_write(image);
			}
			if (d != NULL) {
				mcublkindex += hjpgd.ycbcr2rgb(hjpgd.outbuf[hjpgd.outbuf_read_ptr].buf, d, mcublkindex, hjpgd.outbuf[hjpgd.outbuf_read_ptr].size); 
				hjpgd.outbuf[hjpgd.outbuf_read_ptr].sta = 0;	//���bufΪ��
				hjpgd.outbuf[hjpgd.outbuf_read_ptr].size = 0;	//���������
				hjpgd.outbuf_read_ptr++;
				if(hjpgd.outbuf_read_ptr>=JPEG_DMA_OUTBUF_NB)hjpgd.outbuf_read_ptr=0;//���Ʒ�Χ
				if(mcublkindex==hjpgd.total_blks)
				{
					break;
				}
		  }
		} else if(hjpgd.outdma_pause==1&&hjpgd.outbuf[hjpgd.outbuf_write_ptr].sta==0)	{	//out��ͣ,�ҵ�ǰwritebuf�Ѿ�Ϊ����,��ָ�out���
			JPEG_OUT_DMA_Resume((u32)hjpgd.outbuf[hjpgd.outbuf_write_ptr].buf, JPEG_DMA_OUTBUF_LEN);//������һ��DMA����
			hjpgd.outdma_pause=0;
		}
		timecnt++; 
		if(size <= 0)//�ļ�������,��ʱ�˳�,��ֹ��ѭ��
		{
			if(hjpgd.state==JPEG_STATE_NOHEADER)break;	//����ʧ����
			if(timecnt>0X3FFF)break;					//��ʱ�˳�
		}
	} 
	
	return bitmap_unlock_buffer(image);
}

static ret_t image_loader_stm32_jpg_load(image_loader_t *l, const asset_info_t *asset, bitmap_t *image) {
	uint32_t i = 0;
	int32_t size = 0;
	uint8_t* buff = NULL;
	return_value_if_fail(l != NULL && asset != NULL && image != NULL, RET_BAD_PARAMS);
  if (asset->subtype != ASSET_TYPE_IMAGE_JPG) {
    return RET_NOT_IMPL;
  }
	size = asset->size;
	buff = (uint8_t*)asset->data; 

	jpeg_init(&hjpgd, image);
	JPEG_Core_Init(&hjpgd);						//��ʼ��JPEG�ں�
	JPEG_Decode_Init(&hjpgd);						//��ʼ��Ӳ��JPEG������

	for(i = 0; i < JPEG_DMA_INBUF_NB && size > 0; i++) {
		int s = tk_min(size, JPEG_DMA_INBUF_LEN);
		hjpgd.inbuf[i].sta = 1;
		hjpgd.inbuf[i].size = s;
		memcpy(hjpgd.inbuf[i].buf, buff, s);	
		buff += s;
		size -= s;
		if (size <= 0) {
			break;
		}
	}
	
	JPEG_IN_OUT_DMA_Init((u32)hjpgd.inbuf[0].buf,(u32)hjpgd.outbuf[0].buf,hjpgd.inbuf[0].size,JPEG_DMA_OUTBUF_LEN);//����DMA
	jpeg_in_callback = jpeg_dma_in_callback;			//JPEG DMA��ȡ���ݻص�����
	jpeg_out_callback = jpeg_dma_out_callback; 		//JPEG DMA������ݻص�����
	jpeg_eoc_callback = jpeg_endofcovert_callback;	//JPEG ��������ص�����
	jpeg_hdp_callback = jpeg_hdrover_callback; 		//JPEG Header������ɻص�����
	
	JPEG_DMA_Start();								//����DMA���� 
	
	image_loader_get_yuv_and_set_image_data(buff, size, image);
	jpeg_deinit(&hjpgd);
	JPEG_Core_Destroy(&hjpgd);
  return RET_OK;
}

static const image_loader_t stm32_jpg_image_loader = {.load = image_loader_stm32_jpg_load};

image_loader_t *image_loader_stm32_jpg() {
  return (image_loader_t *)&stm32_jpg_image_loader;
}

