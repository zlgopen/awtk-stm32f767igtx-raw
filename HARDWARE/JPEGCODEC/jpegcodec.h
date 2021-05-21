#ifndef __JPEGCODEC_H
#define __JPEGCODEC_H
#include "sys.h"
#include "jpeg_utils.h"
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK STM32������ 
//JPEGӲ��������� ��������	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//��������:2016/7/22
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2014-2024
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 	
	
#define JPEG_DMA_INBUF_LEN			4096			//����DMA IN  BUF�Ĵ�С
#define JPEG_DMA_OUTBUF_LEN  		768*10 			//����DMA OUT BUF�Ĵ�С
#define JPEG_DMA_INBUF_NB			2				//DMA IN  BUF�ĸ���
#define JPEG_DMA_OUTBUF_NB			2				//DMA OUT BUF�ĸ���

//JPEG���ݻ���ṹ��
typedef struct
{
    u8 sta;			//״̬:0,������;1,������.
    u8 *buf;		//JPEG���ݻ�����
    u16 size; 		//JPEG���ݳ��� 
}jpeg_databuf_type; 

#define JPEG_STATE_NOHEADER		0					//HEADERδ��ȡ,��ʼ״̬
#define JPEG_STATE_HEADEROK		1					//HEADER��ȡ�ɹ�
#define JPEG_STATE_FINISHED		2					//�������


//jpeg�������ƽṹ��
typedef struct
{ 
	JPEG_ConfTypeDef	Conf;             			//��ǰJPEG�ļ���ز���
	jpeg_databuf_type inbuf[JPEG_DMA_INBUF_NB];		//DMA IN buf
	jpeg_databuf_type outbuf[JPEG_DMA_OUTBUF_NB];	//DMA OUT buf
	vu8 inbuf_read_ptr;								//DMA IN buf��ǰ��ȡλ��
	vu8 inbuf_write_ptr;							//DMA IN buf��ǰд��λ��
	vu8 indma_pause;								//����DMA��ͣ״̬��ʶ
	vu8 outbuf_read_ptr;							//DMA OUT buf��ǰ��ȡλ��
	vu8 outbuf_write_ptr;							//DMA OUT buf��ǰд��λ��
	vu8 outdma_pause;								//����DMA��ͣ״̬��ʶ
	vu8 state;										//����״̬;0,δʶ��Header;1,ʶ����Header;2,�������;
	u32 blkindex;									//��ǰblock���
	u32 total_blks;									//jpeg�ļ���block��
	u32 (*ycbcr2rgb)(u8 *,u8 *,u32 ,u32);			//��ɫת������ָ��,ԭ����ο�:JPEG_YCbCrToRGB_Convert_Function
	void* ctx;
}jpeg_codec_typedef;

//DMA�ص�����
extern void (*jpeg_in_callback)(void);				//JPEG DMA����ص�����
extern void (*jpeg_out_callback)(void);				//JPEG DMA��� �ص�����
extern void (*jpeg_eoc_callback)(void);				//JPEG ������� �ص�����
extern void (*jpeg_hdp_callback)(void);				//JPEG Header������� �ص�����


void JPEG_IN_OUT_DMA_Init(u32 meminaddr,u32 memoutaddr,u32 meminsize,u32 memoutsize);
u8 JPEG_Core_Init(jpeg_codec_typedef *tjpeg);
void JPEG_Core_Destroy(jpeg_codec_typedef *tjpeg);
void JPEG_Decode_Init(jpeg_codec_typedef *tjpeg);
void JPEG_DMA_Start(void);
void JPEG_DMA_Stop(void);
void JPEG_IN_DMA_Pause(void);
void JPEG_IN_DMA_Resume(u32 memaddr,u32 memlen);
void JPEG_OUT_DMA_Pause(void);
void JPEG_OUT_DMA_Resume(u32 memaddr,u32 memlen);
void JPEG_Get_Info(jpeg_codec_typedef *tjpeg);
u8 JPEG_Get_Quality(void);

#endif



