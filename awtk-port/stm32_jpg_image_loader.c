#include "tkc/mem.h"
#include "tkc/utils.h"
#include "jpegcodec.h"
#include "stm32_jpg_image_loader.h"

static jpeg_codec_typedef hjpgd;  	//JPEG硬件解码结构体
static uint8_t s_inbuf_0[JPEG_DMA_INBUF_LEN];	//给 DMA 传入数据使用的缓存
static uint8_t s_inbuf_1[JPEG_DMA_INBUF_LEN]; //给 DMA 传入数据使用的缓存

//JPEG输入数据流,回调函数,用于获取JPEG文件原始数据
//每当JPEG DMA IN BUF为空的时候,调用该函数
static void jpeg_dma_in_callback(void) { 
	hjpgd.inbuf[hjpgd.inbuf_read_ptr].sta = 0;	//此buf已经处理完了
	hjpgd.inbuf[hjpgd.inbuf_read_ptr].size = 0;	//此buf已经处理完了 
	hjpgd.inbuf_read_ptr++;					//指向下一个buf
	if(hjpgd.inbuf_read_ptr >= JPEG_DMA_INBUF_NB) {
		hjpgd.inbuf_read_ptr=0;				//归零
	}
	if(hjpgd.inbuf[hjpgd.inbuf_read_ptr].sta==0) {//无有效buf
		JPEG_IN_DMA_Pause();					//暂停读取数据
		hjpgd.indma_pause=1;					//暂停读取数据
	} else {												//有效的buf
		JPEG_IN_DMA_Resume((u32)hjpgd.inbuf[hjpgd.inbuf_read_ptr].buf,hjpgd.inbuf[hjpgd.inbuf_read_ptr].size);//继续下一次DMA传输
	}
}

//JPEG输出数据流(YCBCR)回调函数,用于输出YCbCr数据流
static void jpeg_dma_out_callback(void) {	  
	u32 *pdata = 0; 
	hjpgd.outbuf[hjpgd.outbuf_write_ptr].sta = 1;	//此buf已满
	hjpgd.outbuf[hjpgd.outbuf_write_ptr].size = JPEG_DMA_OUTBUF_LEN-(DMA2_Stream1->NDTR<<2);//此buf里面数据的长度
	if(hjpgd.state == JPEG_STATE_FINISHED){		//如果文件已经解码完成,需要读取DOR最后的数据(<=32字节)
		pdata = (u32*)(hjpgd.outbuf[hjpgd.outbuf_write_ptr].buf + hjpgd.outbuf[hjpgd.outbuf_write_ptr].size);
		while(JPEG->SR&(1<<4)){
			*pdata = JPEG->DOR;
			pdata++;
			hjpgd.outbuf[hjpgd.outbuf_write_ptr].size += 4; 
		}
	}  
	hjpgd.outbuf_write_ptr++;					//指向下一个buf
	if(hjpgd.outbuf_write_ptr>=JPEG_DMA_OUTBUF_NB) {
		hjpgd.outbuf_write_ptr = 0;			//归零
	}
	if(hjpgd.outbuf[hjpgd.outbuf_write_ptr].sta==1)//无有效buf
	{
		JPEG_OUT_DMA_Pause();						//暂停输出数据
		hjpgd.outdma_pause = 1;					//暂停输出数据
	}else{														//有效的buf
		JPEG_OUT_DMA_Resume((u32)hjpgd.outbuf[hjpgd.outbuf_write_ptr].buf, JPEG_DMA_OUTBUF_LEN);//继续下一次DMA传输
	}
}

//JPEG整个文件解码完成回调函数
static void jpeg_endofcovert_callback(void) { 
	hjpgd.state = JPEG_STATE_FINISHED;			//标记JPEG解码完成
}
//JPEG header解析成功回调函数
static void jpeg_hdrover_callback(void) {
	bitmap_t* image = (bitmap_t*)hjpgd.ctx;
	hjpgd.state=JPEG_STATE_HEADEROK;			//HEADER获取成功
	JPEG_Get_Info(&hjpgd);						//获取JPEG相关信息,包括大小,色彩空间,抽样等
	JPEG_GetDecodeColorConvertFunc(&hjpgd.Conf, &hjpgd.ycbcr2rgb, &hjpgd.total_blks);//获取JPEG色彩转换函数,以及总MCU数
	
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
		tjpeg->outbuf[i].buf = TKMEM_ALLOC(JPEG_DMA_OUTBUF_LEN + 32);//有可能会多需要32字节内存
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
    SCB_CleanInvalidateDCache();				//清空D catch
		if(hjpgd.inbuf[hjpgd.inbuf_write_ptr].sta == 0 && size > 0)	{ //有buf为空
			int s = tk_min(size, JPEG_DMA_INBUF_LEN);
			hjpgd.inbuf[hjpgd.inbuf_write_ptr].sta = 1;
			hjpgd.inbuf[hjpgd.inbuf_write_ptr].size = s;
			memcpy(hjpgd.inbuf[hjpgd.inbuf_write_ptr].buf, buff, s);	
			buff += s;
			size -= s;
			if (size <= 0) {
				timecnt = 0;
			}
			if(hjpgd.indma_pause == 1 && hjpgd.inbuf[hjpgd.inbuf_read_ptr].sta == 1)//之前是暂停的了,继续传输
			{
				JPEG_IN_DMA_Resume((u32)hjpgd.inbuf[hjpgd.inbuf_read_ptr].buf, hjpgd.inbuf[hjpgd.inbuf_read_ptr].size);	//继续下一次DMA传输
				hjpgd.indma_pause = 0;
			}
			hjpgd.inbuf_write_ptr++;
			if(hjpgd.inbuf_write_ptr >= JPEG_DMA_INBUF_NB) {
				hjpgd.inbuf_write_ptr = 0;
			}
		}
		if(hjpgd.outbuf[hjpgd.outbuf_read_ptr].sta == 1){	//buf里面有数据要处理
			if (d == NULL) {
				d = bitmap_lock_buffer_for_write(image);
			}
			if (d != NULL) {
				mcublkindex += hjpgd.ycbcr2rgb(hjpgd.outbuf[hjpgd.outbuf_read_ptr].buf, d, mcublkindex, hjpgd.outbuf[hjpgd.outbuf_read_ptr].size); 
				hjpgd.outbuf[hjpgd.outbuf_read_ptr].sta = 0;	//标记buf为空
				hjpgd.outbuf[hjpgd.outbuf_read_ptr].size = 0;	//数据量清空
				hjpgd.outbuf_read_ptr++;
				if(hjpgd.outbuf_read_ptr>=JPEG_DMA_OUTBUF_NB)hjpgd.outbuf_read_ptr=0;//限制范围
				if(mcublkindex==hjpgd.total_blks)
				{
					break;
				}
		  }
		} else if(hjpgd.outdma_pause==1&&hjpgd.outbuf[hjpgd.outbuf_write_ptr].sta==0)	{	//out暂停,且当前writebuf已经为空了,则恢复out输出
			JPEG_OUT_DMA_Resume((u32)hjpgd.outbuf[hjpgd.outbuf_write_ptr].buf, JPEG_DMA_OUTBUF_LEN);//继续下一次DMA传输
			hjpgd.outdma_pause=0;
		}
		timecnt++; 
		if(size <= 0)//文件结束后,及时退出,防止死循环
		{
			if(hjpgd.state==JPEG_STATE_NOHEADER)break;	//解码失败了
			if(timecnt>0X3FFF)break;					//超时退出
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
	JPEG_Core_Init(&hjpgd);						//初始化JPEG内核
	JPEG_Decode_Init(&hjpgd);						//初始化硬件JPEG解码器

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
	
	JPEG_IN_OUT_DMA_Init((u32)hjpgd.inbuf[0].buf,(u32)hjpgd.outbuf[0].buf,hjpgd.inbuf[0].size,JPEG_DMA_OUTBUF_LEN);//配置DMA
	jpeg_in_callback = jpeg_dma_in_callback;			//JPEG DMA读取数据回调函数
	jpeg_out_callback = jpeg_dma_out_callback; 		//JPEG DMA输出数据回调函数
	jpeg_eoc_callback = jpeg_endofcovert_callback;	//JPEG 解码结束回调函数
	jpeg_hdp_callback = jpeg_hdrover_callback; 		//JPEG Header解码完成回调函数
	
	JPEG_DMA_Start();								//启动DMA传输 
	
	image_loader_get_yuv_and_set_image_data(buff, size, image);
	jpeg_deinit(&hjpgd);
	JPEG_Core_Destroy(&hjpgd);
  return RET_OK;
}

static const image_loader_t stm32_jpg_image_loader = {.load = image_loader_stm32_jpg_load};

image_loader_t *image_loader_stm32_jpg() {
  return (image_loader_t *)&stm32_jpg_image_loader;
}

