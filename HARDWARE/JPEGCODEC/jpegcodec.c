#include "jpegcodec.h"
#include "usart.h"
#include "delay.h"
#include "malloc.h"
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

void (*jpeg_in_callback)(void);		//JPEG DMA输入回调函数
void (*jpeg_out_callback)(void);	//JPEG DMA输出 回调函数
void (*jpeg_eoc_callback)(void);	//JPEG 解码完成 回调函数
void (*jpeg_hdp_callback)(void);	//JPEG Header解码完成 回调函数


JPEG_HandleTypeDef  JPEG_Handler;    		//JPEG句柄
DMA_HandleTypeDef   JPEGDMAIN_Handler;		//JPEG数据输入DMA
DMA_HandleTypeDef   JPEGDMAOUT_Handler;		//JPEG数据输出DMA

//JPEG规范(ISO/IEC 10918-1标准)的样本量化表
//获取JPEG图片质量时需要用到
const u8 JPEG_LUM_QuantTable[JPEG_QUANT_TABLE_SIZE] = 
{
	16,11,10,16,24,40,51,61,12,12,14,19,26,58,60,55,
	14,13,16,24,40,57,69,56,14,17,22,29,51,87,80,62,
	18,22,37,56,68,109,103,77,24,35,55,64,81,104,113,92,
	49,64,78,87,103,121,120,101,72,92,95,98,112,100,103,99
};
const u8 JPEG_ZIGZAG_ORDER[JPEG_QUANT_TABLE_SIZE]=
{
	0,1,8,16,9,2,3,10,17,24,32,25,18,11,4,5,
	12,19,26,33,40,48,41,34,27,20,13,6,7,14,21,28,
	35,42,49,56,57,50,43,36,29,22,15,23,30,37,44,51,
	58,59,52,45,38,31,39,46,53,60,61,54,47,55,62,63
}; 

//JPEG硬件解码输入&输出DMA配置
//meminaddr:JPEG输入DMA存储器地址. 
//memoutaddr:JPEG输出DMA存储器地址. 
//meminsize:输入DMA数据长度,0~262143,以字节为单位
//memoutsize:输出DMA数据长度,0~262143,以字节为单位 
void JPEG_IN_OUT_DMA_Init(u32 meminaddr,u32 memoutaddr,u32 meminsize,u32 memoutsize)
{ 
	if(meminsize%4)meminsize+=4-meminsize%4;	//扩展到4的倍数
	meminsize/=4;								//除以4
	if(memoutsize%4)memoutsize+=4-memoutsize%4;	//扩展到4的倍数
	memoutsize/=4;								//除以4 
	
	__HAL_RCC_DMA2_CLK_ENABLE();                                    	//使能DMA2时钟
	
	//JPEG数据输入DMA通道配置
    JPEGDMAIN_Handler.Instance=DMA2_Stream0;                          	//DMA2数据流0                   
    JPEGDMAIN_Handler.Init.Channel=DMA_CHANNEL_9;                    	//通道9
    JPEGDMAIN_Handler.Init.Direction=DMA_MEMORY_TO_PERIPH;            	//存储器到外设
    JPEGDMAIN_Handler.Init.PeriphInc=DMA_PINC_DISABLE;                	//外设非增量模式
    JPEGDMAIN_Handler.Init.MemInc=DMA_MINC_ENABLE;                     	//存储器增量模式
    JPEGDMAIN_Handler.Init.PeriphDataAlignment=DMA_PDATAALIGN_WORD;   	//外设数据长度:32位
    JPEGDMAIN_Handler.Init.MemDataAlignment=DMA_MDATAALIGN_WORD;       	//存储器数据长度:32位
    JPEGDMAIN_Handler.Init.Mode=DMA_NORMAL;                         	//普通模式
    JPEGDMAIN_Handler.Init.Priority=DMA_PRIORITY_HIGH;                	//高优先级
    JPEGDMAIN_Handler.Init.FIFOMode=DMA_FIFOMODE_ENABLE;              	//使能FIFO
    JPEGDMAIN_Handler.Init.FIFOThreshold=DMA_FIFO_THRESHOLD_FULL; 		//全FIFO
    JPEGDMAIN_Handler.Init.MemBurst=DMA_MBURST_INC4;                	//存储器4节拍增量突发传输
    JPEGDMAIN_Handler.Init.PeriphBurst=DMA_PBURST_INC4;             	//外设4节拍增量突发传输
    HAL_DMA_Init(&JPEGDMAIN_Handler);	                                //初始化DMA
    
    //在开启DMA之前先使用__HAL_UNLOCK()解锁一次DMA,因为HAL_DMA_Statrt()HAL_DMAEx_MultiBufferStart()
    //这两个函数一开始要先使用__HAL_LOCK()锁定DMA,而函数__HAL_LOCK()会判断当前的DMA状态是否为锁定状态，如果是
    //锁定状态的话就直接返回HAL_BUSY，这样会导致函数HAL_DMA_Statrt()和HAL_DMAEx_MultiBufferStart()后续的DMA配置
    //程序直接被跳过！DMA也就不能正常工作，为了避免这种现象，所以在启动DMA之前先调用__HAL_UNLOC()先解锁一次DMA。
    __HAL_UNLOCK(&JPEGDMAIN_Handler);
	HAL_DMA_Start(&JPEGDMAIN_Handler,meminaddr,(u32)&JPEG->DIR,meminsize);//开启DMA	
	__HAL_DMA_ENABLE_IT(&JPEGDMAIN_Handler,DMA_IT_TC);    				//开启传输完成中断
	HAL_NVIC_SetPriority(DMA2_Stream0_IRQn,2,3);  						//抢占2，子优先级3，组2 
    HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);        						//使能中断

	//JPEG数据输出DMA通道配置
    JPEGDMAOUT_Handler.Instance=DMA2_Stream1;                          	//DMA2数据流1                 
    JPEGDMAOUT_Handler.Init.Channel=DMA_CHANNEL_9;                    	//通道9
    JPEGDMAOUT_Handler.Init.Direction=DMA_PERIPH_TO_MEMORY;            	//外设到存储器
    JPEGDMAOUT_Handler.Init.PeriphInc=DMA_PINC_DISABLE;                	//外设非增量模式
    JPEGDMAOUT_Handler.Init.MemInc=DMA_MINC_ENABLE;                     //存储器增量模式
    JPEGDMAOUT_Handler.Init.PeriphDataAlignment=DMA_PDATAALIGN_WORD;   	//外设数据长度:32位
    JPEGDMAOUT_Handler.Init.MemDataAlignment=DMA_MDATAALIGN_WORD;      	//存储器数据长度:32位
    JPEGDMAOUT_Handler.Init.Mode=DMA_NORMAL;                         	//普通模式
    JPEGDMAOUT_Handler.Init.Priority=DMA_PRIORITY_VERY_HIGH;            //极高优先级
    JPEGDMAOUT_Handler.Init.FIFOMode=DMA_FIFOMODE_ENABLE;              	//使能FIFO
    JPEGDMAOUT_Handler.Init.FIFOThreshold=DMA_FIFO_THRESHOLD_FULL; 		//全FIFO
    JPEGDMAOUT_Handler.Init.MemBurst=DMA_MBURST_INC4;                	//存储器4节拍增量突发传输
    JPEGDMAOUT_Handler.Init.PeriphBurst=DMA_PBURST_INC4;             	//外设4节拍增量突发传输
    HAL_DMA_Init(&JPEGDMAOUT_Handler);	                                //初始化DMA
    
    //在开启DMA之前先使用__HAL_UNLOCK()解锁一次DMA,因为HAL_DMA_Statrt()HAL_DMAEx_MultiBufferStart()
    //这两个函数一开始要先使用__HAL_LOCK()锁定DMA,而函数__HAL_LOCK()会判断当前的DMA状态是否为锁定状态，如果是
    //锁定状态的话就直接返回HAL_BUSY，这样会导致函数HAL_DMA_Statrt()和HAL_DMAEx_MultiBufferStart()后续的DMA配置
    //程序直接被跳过！DMA也就不能正常工作，为了避免这种现象，所以在启动DMA之前先调用__HAL_UNLOC()先解锁一次DMA。
    __HAL_UNLOCK(&JPEGDMAOUT_Handler);
	HAL_DMA_Start(&JPEGDMAOUT_Handler,(u32)&JPEG->DOR,memoutaddr,memoutsize);//开启DMA	
	__HAL_DMA_ENABLE_IT(&JPEGDMAOUT_Handler,DMA_IT_TC);    				//开启传输完成中断
	HAL_NVIC_SetPriority(DMA2_Stream1_IRQn,2,3);  						//抢占2，子优先级3，组2
    HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);        						//使能中断
} 



//DMA2_Stream0中断服务函数
//处理硬件JPEG解码时输入的数据流
void DMA2_Stream0_IRQHandler(void)
{ 
    if(__HAL_DMA_GET_FLAG(&JPEGDMAIN_Handler,DMA_FLAG_TCIF0_4)!=RESET) //DMA传输完成
    {
        __HAL_DMA_CLEAR_FLAG(&JPEGDMAIN_Handler,DMA_FLAG_TCIF0_4);     //清除DMA传输完成中断标志位
		JPEG->CR&=~(1<<11);			//关闭JPEG的DMA IN
		__HAL_JPEG_DISABLE_IT(&JPEG_Handler,JPEG_IT_IFT|JPEG_IT_IFNF|JPEG_IT_OFT|\
							JPEG_IT_OFNE|JPEG_IT_EOC|JPEG_IT_HPD);		//关闭JPEG中断,防止被打断.		
        if(jpeg_in_callback!=NULL)jpeg_in_callback();					//执行回调函数
		__HAL_JPEG_ENABLE_IT(&JPEG_Handler,JPEG_IT_EOC|JPEG_IT_HPD);	//使能EOC和HPD中断.
    }      		 											 
}  

//DMA2_Stream1中断服务函数
//处理硬件JPEG解码后输出的数据流
void DMA2_Stream1_IRQHandler(void)
{     
    if(__HAL_DMA_GET_FLAG(&JPEGDMAOUT_Handler,DMA_FLAG_TCIF1_5)!=RESET) //DMA传输完成
    {
        __HAL_DMA_CLEAR_FLAG(&JPEGDMAOUT_Handler,DMA_FLAG_TCIF1_5);     //清除DMA传输完成中断标志位
		JPEG->CR&=~(1<<12);												//关闭JPEG的DMA OUT
		__HAL_JPEG_DISABLE_IT(&JPEG_Handler,JPEG_IT_IFT|JPEG_IT_IFNF|JPEG_IT_OFT|\
							JPEG_IT_OFNE|JPEG_IT_EOC|JPEG_IT_HPD);		//关闭JPEG中断,防止被打断.	
        if(jpeg_out_callback!=NULL)jpeg_out_callback();					//执行回调函数
		__HAL_JPEG_ENABLE_IT(&JPEG_Handler,JPEG_IT_EOC|JPEG_IT_HPD);	//使能EOC和HPD中断.
    }     		 											 
}  

//JPEG解码中断服务函数
void JPEG_IRQHandler(void)
{
	if(__HAL_JPEG_GET_FLAG(&JPEG_Handler,JPEG_FLAG_HPDF)!=RESET)//JPEG Header解码完成		
	{ 
		jpeg_hdp_callback();
		__HAL_JPEG_DISABLE_IT(&JPEG_Handler,JPEG_IT_HPD);		//禁止Jpeg Header解码完成中断		
		__HAL_JPEG_CLEAR_FLAG(&JPEG_Handler,JPEG_FLAG_HPDF);	//清除HPDF位(header解码完成位)			
	}
	if(__HAL_JPEG_GET_FLAG(&JPEG_Handler,JPEG_FLAG_EOCF)!=RESET) //JPEG解码完成   
	{
		JPEG_DMA_Stop();
		jpeg_eoc_callback();
		__HAL_JPEG_CLEAR_FLAG(&JPEG_Handler,JPEG_FLAG_EOCF);	//清除EOC位(解码完成位)	
		__HAL_DMA_DISABLE(&JPEGDMAIN_Handler);					//关闭JPEG数据输入DMA
		__HAL_DMA_DISABLE(&JPEGDMAOUT_Handler);					//关闭JPEG数据输出DMA 
	}
}

//初始化硬件JPEG内核
//tjpeg:jpeg编解码控制结构体
//返回值:0,成功;
//    其他,失败
u8 JPEG_Core_Init(jpeg_codec_typedef *tjpeg)
{
//	u8 i;
	
	JPEG_Handler.Instance=JPEG;
  HAL_JPEG_Init(&JPEG_Handler);   	//初始化JPEG
	
//	for(i=0;i<JPEG_DMA_INBUF_NB;i++)
//	{
//		tjpeg->inbuf[i].buf=mymalloc(SRAMIN,JPEG_DMA_INBUF_LEN);
//		if(tjpeg->inbuf[i].buf==NULL)
//		{
//			JPEG_Core_Destroy(tjpeg);
//			return 1;
//		}   
//	} 
//	for(i=0;i<JPEG_DMA_OUTBUF_NB;i++)
//	{
//		tjpeg->outbuf[i].buf=mymalloc(SRAMIN,JPEG_DMA_OUTBUF_LEN+32);//有可能会多需要32字节内存
//		if(tjpeg->outbuf[i].buf==NULL)		
//		{
//			JPEG_Core_Destroy(tjpeg);
//			return 1;
//		}   
//	}
	return 0;
}

//JPEG底层驱动，时钟使能
//此函数会被HAL_JPEG_Init()调用
//hsdram:JPEG句柄
void HAL_JPEG_MspInit(JPEG_HandleTypeDef *hjpeg)
{
    __HAL_RCC_JPEG_CLK_ENABLE();            //使能JPEG时钟
    
    HAL_NVIC_SetPriority(JPEG_IRQn,1,3);    //JPEG中断优先级
    HAL_NVIC_EnableIRQ(JPEG_IRQn);
}

//关闭硬件JPEG内核,并释放内存
//tjpeg:jpeg编解码控制结构体
void JPEG_Core_Destroy(jpeg_codec_typedef *tjpeg)
{
//	u8 i; 
	JPEG_DMA_Stop();//停止DMA传输
//	for(i=0;i<JPEG_DMA_INBUF_NB;i++)myfree(SRAMIN,tjpeg->inbuf[i].buf);		//释放内存
//	for(i=0;i<JPEG_DMA_OUTBUF_NB;i++)myfree(SRAMIN,tjpeg->outbuf[i].buf);	//释放内存
}

//初始化硬件JPEG解码器
//tjpeg:jpeg编解码控制结构体
void JPEG_Decode_Init(jpeg_codec_typedef *tjpeg)
{ 
	u8 i;
	tjpeg->inbuf_read_ptr=0;
	tjpeg->inbuf_write_ptr=0;
	tjpeg->indma_pause=0;
	tjpeg->outbuf_read_ptr=0;
	tjpeg->outbuf_write_ptr=0;	
	tjpeg->outdma_pause=0;	
	tjpeg->state=JPEG_STATE_NOHEADER;	//图片解码结束标志
	tjpeg->blkindex=0;					//当前MCU编号
	tjpeg->total_blks=0;				//总MCU数目
	for(i=0;i<JPEG_DMA_INBUF_NB;i++)
	{
		tjpeg->inbuf[i].sta=0;
		tjpeg->inbuf[i].size=0;
	}
	for(i=0;i<JPEG_DMA_OUTBUF_NB;i++)
	{
		tjpeg->outbuf[i].sta=0;
		tjpeg->outbuf[i].size=0;
	}	
	JPEG->CONFR1|=JPEG_CONFR1_DE;			//使能硬件JPEG解码模式
	__HAL_JPEG_ENABLE_IT(&JPEG_Handler,JPEG_IT_HPD);//使能Jpeg Header解码完成中断
	__HAL_JPEG_ENABLE_IT(&JPEG_Handler,JPEG_IT_EOC);//使能解码完成中断  			
	JPEG->CONFR0|=JPEG_CONFR0_START;			//使能JPEG编解码进程 		
}

//启动JPEG DMA解码过程
void JPEG_DMA_Start(void)
{ 
	__HAL_DMA_ENABLE(&JPEGDMAIN_Handler);	//打开JPEG数据输入DMA
	__HAL_DMA_ENABLE(&JPEGDMAOUT_Handler); 	//打开JPEG数据输出DMA 
	JPEG->CR|=3<<11; 						//JPEG IN&OUT DMA使能	
}

//停止JPEG DMA解码过程
void JPEG_DMA_Stop(void)
{
	JPEG->CR&=~(3<<11); 					//JPEG IN&OUT DMA禁止 
	JPEG->CONFR0&=~(1<<0);					//停止JPEG编解码进程 
	__HAL_JPEG_DISABLE_IT(&JPEG_Handler,JPEG_IT_IFT|JPEG_IT_IFNF|JPEG_IT_OFT|\
							JPEG_IT_OFNE|JPEG_IT_EOC|JPEG_IT_HPD);		//关闭所有中断  
					
	JPEG->CFR=3<<5;							//清空标志  
}

//暂停DMA IN过程
void JPEG_IN_DMA_Pause(void)
{  
	JPEG->CR&=~(1<<11);			//暂停JPEG的DMA IN
}

//恢复DMA IN过程(为了效率，直接操作寄存器！)
//memaddr:存储区首地址
//memlen:要传输数据长度(以字节为单位)
void JPEG_IN_DMA_Resume(u32 memaddr,u32 memlen)
{  
	if(memlen%4)memlen+=4-memlen%4;//扩展到4的倍数
	memlen/=4;					//除以4
	DMA2->LIFCR|=0X3D<<6*0;		//清空通道0上所有中断标志
	DMA2_Stream0->M0AR=memaddr;	//设置存储器地址
	DMA2_Stream0->NDTR=memlen;	//传输长度为memlen
	DMA2_Stream0->CR|=1<<0;		//开启DMA2,Stream0 
	JPEG->CR|=1<<11; 			//恢复JPEG DMA IN 
}

//暂停DMA OUT过程
void JPEG_OUT_DMA_Pause(void)
{  
	JPEG->CR&=~(1<<12);			//暂停JPEG的DMA OUT
}

//恢复DMA OUT过程(为了效率，直接操作寄存器)
//memaddr:存储区首地址
//memlen:要传输数据长度(以字节为单位)
void JPEG_OUT_DMA_Resume(u32 memaddr,u32 memlen)
{  
	if(memlen%4)memlen+=4-memlen%4;//扩展到4的倍数
	memlen/=4;					//除以4
	DMA2->LIFCR|=0X3D<<6*1;		//清空通道1上所有中断标志
	DMA2_Stream1->M0AR=memaddr;	//设置存储器地址
	DMA2_Stream1->NDTR=memlen;	//传输长度为memlen
	DMA2_Stream1->CR|=1<<0;		//开启DMA2,Stream1 
	JPEG->CR|=1<<12; 			//恢复JPEG DMA OUT 
}

//获取图像信息
//tjpeg:jpeg解码结构体
void JPEG_Get_Info(jpeg_codec_typedef *tjpeg)
{ 
	u32 yblockNb,cBblockNb,cRblockNb; 
	switch(JPEG->CONFR1&0X03)
	{
		case 0://grayscale,1 color component
			tjpeg->Conf.ColorSpace=JPEG_GRAYSCALE_COLORSPACE;
			break;
		case 2://YUV/RGB,3 color component
			tjpeg->Conf.ColorSpace=JPEG_YCBCR_COLORSPACE;
			break;	
		case 3://CMYK,4 color component
			tjpeg->Conf.ColorSpace=JPEG_CMYK_COLORSPACE;
			break;			
	}
	tjpeg->Conf.ImageHeight=(JPEG->CONFR1&0XFFFF0000)>>16;	//获得图像高度
	tjpeg->Conf.ImageWidth=(JPEG->CONFR3&0XFFFF0000)>>16;	//获得图像宽度
	if((tjpeg->Conf.ColorSpace==JPEG_YCBCR_COLORSPACE)||(tjpeg->Conf.ColorSpace==JPEG_CMYK_COLORSPACE))
	{
		yblockNb  =(JPEG->CONFR4&(0XF<<4))>>4;
		cBblockNb =(JPEG->CONFR5&(0XF<<4))>>4;
		cRblockNb =(JPEG->CONFR6&(0XF<<4))>>4;
		if((yblockNb==1)&&(cBblockNb==0)&&(cRblockNb==0))tjpeg->Conf.ChromaSubsampling=JPEG_422_SUBSAMPLING; //16x8 block
		else if((yblockNb==0)&&(cBblockNb==0)&&(cRblockNb==0))tjpeg->Conf.ChromaSubsampling=JPEG_444_SUBSAMPLING;
		else if((yblockNb==3)&&(cBblockNb==0)&&(cRblockNb==0))tjpeg->Conf.ChromaSubsampling = JPEG_420_SUBSAMPLING;
		else tjpeg->Conf.ChromaSubsampling=JPEG_444_SUBSAMPLING; 
	}else tjpeg->Conf.ChromaSubsampling=JPEG_444_SUBSAMPLING;		//默认用4:4:4 
	tjpeg->Conf.ImageQuality=0;	//图像质量参数在整个图片的最末尾,刚开始的时候,是无法获取的,所以直接设置为0
}

//得到jpeg图像质量
//在解码完成后,可以调用并获得正确的结果.
//返回值:图像质量,0~100.
u8 JPEG_Get_Quality(void)
{
	u32 quality=0;
	u32 quantRow,quantVal,scale,i,j;
	u32 *tableAddress=(u32*)JPEG->QMEM0; 
	i=0;
	while(i<JPEG_QUANT_TABLE_SIZE)
	{
		quantRow=*tableAddress;
		for(j=0;j<4;j++)
		{
			quantVal=(quantRow>>(8*j))&0xFF;
			if(quantVal==1)quality+=100;	//100% 
			else
			{
				scale=(quantVal*100)/((u32)JPEG_LUM_QuantTable[JPEG_ZIGZAG_ORDER[i+j]]);
				if(scale<=100)quality+=(200-scale)/2;  
				else quality+=5000/scale;      
			}      
		} 
		i+=4;
		tableAddress++;    
	} 
	return (quality/((u32)64));   
}






































