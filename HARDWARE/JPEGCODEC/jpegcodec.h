#ifndef __JPEGCODEC_H
#define __JPEGCODEC_H
#include "sys.h"
#include "jpeg_utils.h"
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32开发板 
//JPEG硬件编解码器 驱动代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2016/7/22
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2014-2024
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 	
	
#define JPEG_DMA_INBUF_LEN			4096			//单个DMA IN  BUF的大小
#define JPEG_DMA_OUTBUF_LEN  		768*10 			//单个DMA OUT BUF的大小
#define JPEG_DMA_INBUF_NB			2				//DMA IN  BUF的个数
#define JPEG_DMA_OUTBUF_NB			2				//DMA OUT BUF的个数

//JPEG数据缓冲结构体
typedef struct
{
    u8 sta;			//状态:0,无数据;1,有数据.
    u8 *buf;		//JPEG数据缓冲区
    u16 size; 		//JPEG数据长度 
}jpeg_databuf_type; 

#define JPEG_STATE_NOHEADER		0					//HEADER未读取,初始状态
#define JPEG_STATE_HEADEROK		1					//HEADER读取成功
#define JPEG_STATE_FINISHED		2					//解码完成


//jpeg编解码控制结构体
typedef struct
{ 
	JPEG_ConfTypeDef	Conf;             			//当前JPEG文件相关参数
	jpeg_databuf_type inbuf[JPEG_DMA_INBUF_NB];		//DMA IN buf
	jpeg_databuf_type outbuf[JPEG_DMA_OUTBUF_NB];	//DMA OUT buf
	vu8 inbuf_read_ptr;								//DMA IN buf当前读取位置
	vu8 inbuf_write_ptr;							//DMA IN buf当前写入位置
	vu8 indma_pause;								//输入DMA暂停状态标识
	vu8 outbuf_read_ptr;							//DMA OUT buf当前读取位置
	vu8 outbuf_write_ptr;							//DMA OUT buf当前写入位置
	vu8 outdma_pause;								//输入DMA暂停状态标识
	vu8 state;										//解码状态;0,未识别到Header;1,识别到了Header;2,解码完成;
	u32 blkindex;									//当前block编号
	u32 total_blks;									//jpeg文件总block数
	u32 (*ycbcr2rgb)(u8 *,u8 *,u32 ,u32);			//颜色转换函数指针,原型请参考:JPEG_YCbCrToRGB_Convert_Function
	void* ctx;
}jpeg_codec_typedef;

//DMA回调函数
extern void (*jpeg_in_callback)(void);				//JPEG DMA输入回调函数
extern void (*jpeg_out_callback)(void);				//JPEG DMA输出 回调函数
extern void (*jpeg_eoc_callback)(void);				//JPEG 解码完成 回调函数
extern void (*jpeg_hdp_callback)(void);				//JPEG Header解码完成 回调函数


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



