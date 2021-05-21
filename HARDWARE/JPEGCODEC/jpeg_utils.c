#include "jpeg_utils.h"
#include "jpeg_utils_tbl.h" 
//////////////////////////////////////////////////////////////////////////////////  
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32开发板
//提供STM32F7 JPEG硬件解码  MCU(Minimum Coded Unit)块到RGB的颜色转换函数  
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2016/7/26
//版本：V1.0
//注意:YCbCr到RGB的转换,没有对不是MCU像素整数倍宽度的图像做处理,所以,当图片的宽度
//不是单个MCU所含像素的整数倍时,会在图片左侧出现一些条纹.这是正常的,请保证图片宽度
//为MCU所含像素的整数倍即可,即一般是16的倍数.
//********************************************************************************
//修改说明
//无
//////////////////////////////////////////////////////////////////////////////////  

//范围限定宏定义 
//当使用RGB565输出时:CLAMP_LUT是一个16位查询表,带移位和范围限制.
//当使用RGB888/ARGB888输出时:CLAMP_LUT是一个8位查询表,带范围限制.
//详见:jpeg_utils_tbl.h
#define CLAMP(value) 			CLAMP_LUT[value]			//颜色查询表 


JPEG_MCU_RGB_ConvertorTypeDef JPEG_ConvertorParams;			//颜色转换所需参数结构体


//将YCbCr 4:2:0 Blocks转换到RGB像素
//一个MCU BLOCK包含:4 8x8 blocks of Y + 1 8x8 block of Cb + 1 8x8 block of Cr
//pInBuffer:指向输入的 YCbCr blocks缓冲区
//pOutBuffer:指向输出的 RGB888/ARGB8888帧缓冲区
//BlockIndex:输入buf里面的第一个MCU块编号
//DataCount:输入缓冲区的大小
u32 JPEG_MCU_YCbCr420_ARGB_ConvertBlocks(u8 *pInBuffer,u8 *pOutBuffer,u32 BlockIndex,u32 DataCount)
{  
	u32 numberMCU;
	u32 i,j,k, currentMCU, xRef,yRef;
	u32 refline; 
	int ycomp, crcomp, cbcomp;
	int c_red, c_blue, c_green;
	u8 *pOutAddr, *pOutAddr2;
	u8 *pChrom,*pLum;  
	numberMCU=DataCount/YCBCR_420_BLOCK_SIZE;
	currentMCU=BlockIndex; 
	while(currentMCU<(numberMCU+BlockIndex))
	{
		xRef=((currentMCU*16)/JPEG_ConvertorParams.WidthExtend)*16;
		yRef=((currentMCU*16)%JPEG_ConvertorParams.WidthExtend);
		refline=JPEG_ConvertorParams.ScaledWidth*xRef+(JPEG_BYTES_PER_PIXEL*yRef);
		currentMCU++;
		pChrom=pInBuffer+256;	//pChroma=pInBuffer+4*64
		pLum=pInBuffer;
		for(i=0;i<16;i+=2)
		{
			if(i==8)pLum=pInBuffer+128;
			if(refline<JPEG_ConvertorParams.ImageSize_Bytes)
			{
				pOutAddr=pOutBuffer+refline;
				pOutAddr2=pOutAddr+JPEG_ConvertorParams.ScaledWidth;
				for(k=0;k<2;k++)
				{
					for(j=0;j<8;j+=2)
					{     
						cbcomp=(int)(*(pChrom));
						c_blue=(int)(*(CB_BLUE_LUT+cbcomp));
						crcomp=(int)(*(pChrom+64));
						c_red=(int)(*(CR_RED_LUT+crcomp));
						c_green=(((int)(*(CR_GREEN_LUT+crcomp))+(int)(*(CB_GREEN_LUT+cbcomp)))>>16); 
#if JPEG_BYTES_PER_PIXEL==4		//ARGB_8888颜色格式   				
						ycomp=(int)(*(pLum+j)); 
						*(vu32*)pOutAddr=
						(CLAMP(ycomp+c_red)<<JPEG_RED_OFFSET)|\
						(CLAMP(ycomp+c_green)<<JPEG_GREEN_OFFSET)|\
						(CLAMP(ycomp+c_blue)<<JPEG_BLUE_OFFSET); 
						ycomp=(int)(*(pLum+j+1)); 
						*((vu32*)(pOutAddr+4))=
						(CLAMP(ycomp+c_red)<<JPEG_RED_OFFSET)|\
						(CLAMP(ycomp+c_green)<<JPEG_GREEN_OFFSET)|\
						(CLAMP(ycomp+c_blue)<<JPEG_BLUE_OFFSET); 
						ycomp=(int)(*(pLum+j+8)); 
						*(vu32*)pOutAddr2=
						(CLAMP(ycomp+c_red)<<JPEG_RED_OFFSET)|\
						(CLAMP(ycomp+c_green)<<JPEG_GREEN_OFFSET)|\
						(CLAMP(ycomp+c_blue)<<JPEG_BLUE_OFFSET); 
						ycomp=(int)(*(pLum+j+8+1)); 
						*((vu32*)(pOutAddr2+4))=
						(CLAMP(ycomp+c_red)<<JPEG_RED_OFFSET)|\
						(CLAMP(ycomp+c_green)<<JPEG_GREEN_OFFSET)|\
						(CLAMP(ycomp+c_blue)<<JPEG_BLUE_OFFSET); 
#elif JPEG_BYTES_PER_PIXEL==3	//RGB_888颜色格式  
						ycomp=(int)(*(pLum+j)); 
						pOutAddr[JPEG_RED_OFFSET/8]=CLAMP(ycomp+c_red);
						pOutAddr[JPEG_GREEN_OFFSET/8]=CLAMP(ycomp+c_green);
						pOutAddr[JPEG_BLUE_OFFSET/8]=CLAMP(ycomp+c_blue); 
						ycomp=(int)(*(pLum+j+1)); 
						pOutAddr[3+JPEG_RED_OFFSET/8]=CLAMP(ycomp+c_red);
						pOutAddr[3+JPEG_GREEN_OFFSET/8]=CLAMP(ycomp+c_green);
						pOutAddr[3+JPEG_BLUE_OFFSET/8]=CLAMP(ycomp+c_blue); 
						ycomp=(int)(*(pLum+j+8)); 
						pOutAddr2[JPEG_RED_OFFSET/8] = CLAMP(ycomp + c_red);
						pOutAddr2[JPEG_GREEN_OFFSET/8] = CLAMP(ycomp + c_green);
						pOutAddr2[JPEG_BLUE_OFFSET/8] = CLAMP(ycomp + c_blue); 
						ycomp=(int)(*(pLum+j+8+1));   
						pOutAddr2[3+JPEG_RED_OFFSET/8]=CLAMP(ycomp+c_red);
						pOutAddr2[3+JPEG_GREEN_OFFSET/8]=CLAMP(ycomp+c_green);
						pOutAddr2[3+JPEG_BLUE_OFFSET/8]=CLAMP(ycomp+c_blue);
#elif JPEG_BYTES_PER_PIXEL==2	//RGB_565颜色格式 
						ycomp=(int)(*(pLum+j));   
						*(vu16*)pOutAddr=CLAMP(ycomp+c_red)|CLAMP(ycomp+c_green)|CLAMP(ycomp+c_blue);
						ycomp=(int)(*(pLum+j+1)); 
						*((vu16*)pOutAddr+1)=CLAMP(ycomp+c_red)|CLAMP(ycomp+c_green)|CLAMP(ycomp+c_blue);
						ycomp=(int)(*(pLum+j+8)); 
						*(vu16*)pOutAddr2=CLAMP(ycomp+c_red)|CLAMP(ycomp+c_green)|CLAMP(ycomp+c_blue);
 						ycomp=(int)(*(pLum+j+8+1));   
						*((vu16*)pOutAddr2+1)=CLAMP(ycomp+c_red)|CLAMP(ycomp+c_green)|CLAMP(ycomp+c_blue); 
#endif  
						pOutAddr+=JPEG_BYTES_PER_PIXEL*2;
						pOutAddr2+=JPEG_BYTES_PER_PIXEL*2;
						pChrom++;
					}
					pLum+=64;                  
				} 
				pLum=pLum-128+16; 
				refline+=JPEG_ConvertorParams.ScaledWidth*2; 
			}
		} 
		pInBuffer+=YCBCR_420_BLOCK_SIZE;
	}
	return numberMCU;
} 
//将YCbCr 4:2:2 Blocks转换到RGB像素
//一个MCU BLOCK包含:2 8x8 blocks of Y + 1 8x8 block of Cb + 1 8x8 block of Cr
//pInBuffer:指向输入的 YCbCr blocks缓冲区
//pOutBuffer:指向输出的 RGB888/ARGB8888帧缓冲区
//BlockIndex:输入buf里面的第一个MCU块编号
//DataCount:输入缓冲区的大小
u32 JPEG_MCU_YCbCr422_ARGB_ConvertBlocks(u8 *pInBuffer,u8 *pOutBuffer,u32 BlockIndex,u32 DataCount)
{  
	static vu16 tempadd=0;
	u32 numberMCU;
	u32 i,j,k,currentMCU,xRef,yRef;
	u32 refline; 
	int ycomp,crcomp,cbcomp;
	int c_red,c_blue,c_green;
	u8 *pOutAddr;
	u8 *pChrom,*pLum;
	numberMCU=DataCount/YCBCR_422_BLOCK_SIZE;
	currentMCU=BlockIndex;
	while(currentMCU<(numberMCU+BlockIndex))
	{
		xRef=((currentMCU*16)/JPEG_ConvertorParams.WidthExtend)*8;
		yRef=((currentMCU*16)%JPEG_ConvertorParams.WidthExtend);
		refline=JPEG_ConvertorParams.ScaledWidth*xRef+(JPEG_BYTES_PER_PIXEL*yRef);
		currentMCU++;
		pChrom=pInBuffer+128;	//pChroma=pInBuffer+2*64
		pLum=pInBuffer;
		for(i=0;i<8;i++)
		{
			if(refline<JPEG_ConvertorParams.ImageSize_Bytes)
			{
				pOutAddr=pOutBuffer+refline;
				for(k=0;k<2;k++)
				{
					for(j=0;j<8;j+=2)
					{
						cbcomp=(int)(*(pChrom));
						c_blue=(int)(*(CB_BLUE_LUT+cbcomp));
						crcomp=(int)(*(pChrom+64));
						c_red=(int)(*(CR_RED_LUT+crcomp));
						c_green=((int)(*(CR_GREEN_LUT+crcomp))+(int)(*(CR_GREEN_LUT+cbcomp)))>>16;
#if JPEG_BYTES_PER_PIXEL==4		//ARGB_8888颜色格式    
						ycomp=(int)(*(pLum+j)); 
						*(vu32*)pOutAddr=
						(CLAMP(ycomp+c_red)<<JPEG_RED_OFFSET)|\
						(CLAMP(ycomp+c_green)<<JPEG_GREEN_OFFSET)|\
						(CLAMP(ycomp+c_blue)<<JPEG_BLUE_OFFSET); 
						ycomp=(int)(*(pLum+j+1)); 
						*((vu32*)(pOutAddr+4))=
						(CLAMP(ycomp+c_red)<<JPEG_RED_OFFSET)|\
						(CLAMP(ycomp+c_green)<<JPEG_GREEN_OFFSET)|\
						(CLAMP(ycomp+c_blue)<<JPEG_BLUE_OFFSET);   
#elif JPEG_BYTES_PER_PIXEL==3	//RGB_888颜色格式  
						ycomp=(int)(*(pLum+j));
						pOutAddr[JPEG_RED_OFFSET/8]=CLAMP(ycomp+c_red);
						pOutAddr[JPEG_GREEN_OFFSET/8]=CLAMP(ycomp+c_green);
						pOutAddr[JPEG_BLUE_OFFSET/8]=CLAMP(ycomp+c_blue);
						ycomp=(int)(*(pLum+j+1));
						pOutAddr[3+JPEG_RED_OFFSET/8]=CLAMP(ycomp+c_red);
						pOutAddr[3+JPEG_GREEN_OFFSET/8]=CLAMP(ycomp+c_green);
						pOutAddr[3+JPEG_BLUE_OFFSET/8]=CLAMP(ycomp+c_blue);
#elif JPEG_BYTES_PER_PIXEL==2	//RGB_565颜色格式  
						ycomp=(int)(*(pLum+j)); 
						*(vu16*)pOutAddr=CLAMP(ycomp+c_red)|CLAMP(ycomp+c_green)|CLAMP(ycomp+c_blue); 
						ycomp=(int)(*(pLum+j+1)); 
						*((vu16*)pOutAddr+1)=CLAMP(ycomp+c_red)|CLAMP(ycomp+c_green)|CLAMP(ycomp+c_blue); 
#endif
						pOutAddr+=JPEG_BYTES_PER_PIXEL*2;
						pChrom++;
					}
					pLum+=64;                   
				}
				pLum=pLum-128+8;
				refline+=JPEG_ConvertorParams.ScaledWidth;        
			}
		}   
		pInBuffer+=YCBCR_422_BLOCK_SIZE;
	}
	return numberMCU;
}
//将YCbCr 4:4:4 Blocks转换到RGB像素
//一个MCU BLOCK包含:1 8x8 blocks of Y + 1 8x8 block of Cb + 1 8x8 block of Cr
//pInBuffer:指向输入的 YCbCr blocks缓冲区
//pOutBuffer:指向输出的 RGB888/ARGB8888帧缓冲区
//BlockIndex:输入buf里面的第一个MCU块编号
//DataCount:输入缓冲区的大小
u32 JPEG_MCU_YCbCr444_ARGB_ConvertBlocks(u8 *pInBuffer,u8 *pOutBuffer,u32 BlockIndex,u32 DataCount)
{  
	u32 numberMCU;
	u32 i,j, currentMCU, xRef,yRef; 
	u32 refline; 
	int ycomp, crcomp, cbcomp; 
	int c_red, c_blue, c_green; 
	u8 *pOutAddr;
	u8 *pChrom, *pLum; 
	numberMCU=DataCount/YCBCR_444_BLOCK_SIZE;
	currentMCU=BlockIndex;
	while(currentMCU<(numberMCU+BlockIndex))
	{ 
		xRef=((currentMCU*8)/JPEG_ConvertorParams.WidthExtend)*8;
		yRef=((currentMCU*8)%JPEG_ConvertorParams.WidthExtend);
		refline=JPEG_ConvertorParams.ScaledWidth*xRef+(JPEG_BYTES_PER_PIXEL*yRef);
		currentMCU++;
		pChrom=pInBuffer+64;//pChroma=pInBuffer+4*64 
		pLum=pInBuffer;

		for(i=0;i<8;i++)
		{
			if(refline<JPEG_ConvertorParams.ImageSize_Bytes)
			{
				pOutAddr=pOutBuffer+refline;
				for(j=0;j<8;j++)
				{
					cbcomp=(int)(*pChrom);
					c_blue=(int)(*(CB_BLUE_LUT+cbcomp)); 
					crcomp=(int)(*(pChrom+64));
					c_red=(int)(*(CR_RED_LUT+crcomp));    
					c_green=((int)(*(CR_GREEN_LUT+crcomp))+(int)(*(CB_GREEN_LUT+cbcomp)))>>16; 
#if JPEG_BYTES_PER_PIXEL==4		//ARGB_8888颜色格式    
					ycomp=(int)(*(pLum+j)); 
					*(vu32*)pOutAddr=
					(CLAMP(ycomp+c_red)<<JPEG_RED_OFFSET)|\
					(CLAMP(ycomp+c_green)<<JPEG_GREEN_OFFSET)|\
					(CLAMP(ycomp+c_blue)<<JPEG_BLUE_OFFSET);    
#elif JPEG_BYTES_PER_PIXEL==3	//RGB_888颜色格式   
					ycomp=(int)(*(pLum+j)); 
					pOutAddr[JPEG_RED_OFFSET/8]=CLAMP(ycomp+c_red);
					pOutAddr[JPEG_GREEN_OFFSET/8]=CLAMP(ycomp+c_green);
					pOutAddr[JPEG_BLUE_OFFSET/8]=CLAMP(ycomp+c_blue);   
#elif JPEG_BYTES_PER_PIXEL==2	//RGB_565颜色格式 
					ycomp=(int)(*(pLum+j)); 
					*(vu16*)pOutAddr=CLAMP(ycomp+c_red)|CLAMP(ycomp+c_green)|CLAMP(ycomp+c_blue); 
#endif   			 
					pOutAddr+=JPEG_BYTES_PER_PIXEL; 
					pChrom++;
				}
				pLum+=8;
				refline+=JPEG_ConvertorParams.ScaledWidth;      
			}
		}   
		pInBuffer+=YCBCR_444_BLOCK_SIZE;
	}
	return numberMCU;
}
//将Y Gray Blocks转换到RGB像素
//一个MCU BLOCK包含:1 8x8 blocks of Y
//pInBuffer:指向输入的 YCbCr blocks缓冲区
//pOutBuffer:指向输出的 RGB888/ARGB8888帧缓冲区
//BlockIndex:输入buf里面的第一个MCU块编号
//DataCount:输入缓冲区的大小
u32 JPEG_MCU_Gray_ARGB_ConvertBlocks(u8 *pInBuffer,u8 *pOutBuffer,u32 BlockIndex,u32 DataCount)
{
	u32 numberMCU;
	u32  currentMCU, xRef,yRef;
	u32 refline;
	u32 i,j, ySample;
	u8 *pOutAddr,*pLum; 
	numberMCU=DataCount/GRAY_444_BLOCK_SIZE;
	currentMCU=BlockIndex; 
	while(currentMCU<(numberMCU+BlockIndex))
	{
		xRef=((currentMCU*8)/JPEG_ConvertorParams.WidthExtend)*8;
		yRef=((currentMCU*8)%JPEG_ConvertorParams.WidthExtend);
		refline=JPEG_ConvertorParams.ScaledWidth*xRef+(JPEG_BYTES_PER_PIXEL*yRef);
		currentMCU++;
		pLum=pInBuffer;
		for(i=0;i<8;i++)
		{
			pOutAddr=pOutBuffer+refline;
			if(refline<JPEG_ConvertorParams.ImageSize_Bytes)
			{
				for(j=0;j<8;j++)
				{
					ySample=(u32)(*pLum); 
#if JPEG_BYTES_PER_PIXEL==4		//ARGB_8888颜色格式    
					*(vu32*)pOutAddr=ySample|(ySample<<8)|(ySample<<16); 
#elif JPEG_BYTES_PER_PIXEL==3	//RGB_888颜色格式  
					pOutAddr[0]=ySample;
					pOutAddr[1]=ySample;
					pOutAddr[2]=ySample; 
#elif JPEG_BYTES_PER_PIXEL==2	//RGB_565颜色格式
					*(vu16*)pOutAddr=(ySample&0XF8)<<8|(ySample&0XFC)<<3|ySample>>3;
#endif   
					pOutAddr+=JPEG_BYTES_PER_PIXEL;
					pLum++;
				}
				refline+=JPEG_ConvertorParams.ScaledWidth;    
			}
		} 
		pInBuffer+=GRAY_444_BLOCK_SIZE;    
	}
	return numberMCU;
}

//将CMYK Blocks转换到RGB像素
//一个MCU BLOCK包含: 1 Cyan 8x8  block + 1 Magenta 8x8 block + 1 Yellow 8x8 block + 1 Key 8x8 block.
//pInBuffer:指向输入的 YCbCr blocks缓冲区
//pOutBuffer:指向输出的 RGB888/ARGB8888帧缓冲区
//BlockIndex:输入buf里面的第一个MCU块编号
//DataCount:输入缓冲区的大小
u32 JPEG_MCU_YCCK_ARGB_ConvertBlocks(u8 *pInBuffer,u8 *pOutBuffer,u32 BlockIndex,u32 DataCount)
{  
	u32 numberMCU;
	u32 i,j, currentMCU, xRef,yRef;
	u32 refline;
	int color_k;
	int c_red, c_blue, c_green;
	u8 *pOutAddr,*pChrom;
	numberMCU=DataCount/CMYK_444_BLOCK_SIZE;
	currentMCU=BlockIndex;
	while(currentMCU<(numberMCU+BlockIndex))
	{
		xRef=((currentMCU*8)/JPEG_ConvertorParams.WidthExtend)*8;
		yRef=((currentMCU*8)%JPEG_ConvertorParams.WidthExtend);
		refline=JPEG_ConvertorParams.ScaledWidth*xRef+(JPEG_BYTES_PER_PIXEL*yRef);
		currentMCU++;
		pChrom=pInBuffer;
		for(i=0;i<8;i++)
		{
			if(refline<JPEG_ConvertorParams.ImageSize_Bytes)
			{
				pOutAddr=pOutBuffer+refline;     
				for(j=0;j<8;j++)
				{      
					color_k=(int)(*(pChrom+192));
					c_red=(color_k*((int)(*pChrom)))/255;
					c_green=(color_k*(int)(*(pChrom+64)))/255;
					c_blue=(color_k*(int)(*(pChrom+128)))/255;
#if JPEG_BYTES_PER_PIXEL==4		//ARGB_8888颜色格式    
					*(vu32*)pOutAddr=
					(c_red<<JPEG_RED_OFFSET)|\
					(c_green<<JPEG_GREEN_OFFSET)|\
					(c_blue<<JPEG_BLUE_OFFSET);
#elif JPEG_BYTES_PER_PIXEL==3	//RGB_888颜色格式  
					pOutAddr[JPEG_RED_OFFSET/8]=c_red;
					pOutAddr[JPEG_GREEN_OFFSET/8]=c_green;
					pOutAddr[JPEG_BLUE_OFFSET/8]=c_blue;  
#elif JPEG_BYTES_PER_PIXEL==2	//RGB_565颜色格式 
					*(vu16*)pOutAddr=(c_red&0XF8)<<8|(c_green&0XFC)<<3|c_blue>>3;
#endif      
					pOutAddr+=JPEG_BYTES_PER_PIXEL; 
					pChrom++;
				} 
				refline+=JPEG_ConvertorParams.ScaledWidth;        
			}
		}    
		pInBuffer+=CMYK_444_BLOCK_SIZE;
	}
	return numberMCU;
}

//获取YCbCr到RGB颜色转换函数和总的MCU Block数目.
//pJpegInfo:JPEG文件信息结构体
//pFunction:JPEG_YCbCrToRGB_Convert_Function的函数指针,根据jpeg图像参数,指向不同的颜色转换函数.
//ImageNbMCUs:总的MCU块数目
//返回值:0,正常;
//       1,失败;
u8 JPEG_GetDecodeColorConvertFunc(JPEG_ConfTypeDef *pJpegInfo, JPEG_YCbCrToRGB_Convert_Function *pFunction, u32 *ImageNbMCUs)
{
	u32 hMCU, vMCU;
	JPEG_ConvertorParams.ColorSpace=pJpegInfo->ColorSpace;				//色彩空间
	JPEG_ConvertorParams.ImageWidth=pJpegInfo->ImageWidth;				//图像宽度
	JPEG_ConvertorParams.ImageHeight=pJpegInfo->ImageHeight;			//图像高度
	JPEG_ConvertorParams.ImageSize_Bytes=pJpegInfo->ImageWidth*pJpegInfo->ImageHeight*JPEG_BYTES_PER_PIXEL;	//转换后的图像总字节数
	JPEG_ConvertorParams.ChromaSubsampling=pJpegInfo->ChromaSubsampling;//抽样情况
	if(JPEG_ConvertorParams.ColorSpace==JPEG_YCBCR_COLORSPACE)			//YCBCR颜色空间
	{
		if(JPEG_ConvertorParams.ChromaSubsampling==JPEG_420_SUBSAMPLING)
		{
			*pFunction=JPEG_MCU_YCbCr420_ARGB_ConvertBlocks;			//使用YCbCr420转换
			JPEG_ConvertorParams.LineOffset=JPEG_ConvertorParams.ImageWidth%16;
			if(JPEG_ConvertorParams.LineOffset!=0)
			{
				JPEG_ConvertorParams.LineOffset=16-JPEG_ConvertorParams.LineOffset;
			}
			JPEG_ConvertorParams.H_factor=16;
			JPEG_ConvertorParams.V_factor=16;
		}else if(JPEG_ConvertorParams.ChromaSubsampling==JPEG_422_SUBSAMPLING)
		{
			*pFunction=JPEG_MCU_YCbCr422_ARGB_ConvertBlocks;			//使用YCbCr422转换
 			JPEG_ConvertorParams.LineOffset=JPEG_ConvertorParams.ImageWidth%16;
			if(JPEG_ConvertorParams.LineOffset!=0)
			{
				JPEG_ConvertorParams.LineOffset=16-JPEG_ConvertorParams.LineOffset;
			}
			JPEG_ConvertorParams.H_factor=16;
			JPEG_ConvertorParams.V_factor=8;
		}else		//4:4:4
		{
			*pFunction=JPEG_MCU_YCbCr444_ARGB_ConvertBlocks;			//使用YCbCr444转换
 			JPEG_ConvertorParams.LineOffset=JPEG_ConvertorParams.ImageWidth%8;
			if(JPEG_ConvertorParams.LineOffset!=0)
			{
				JPEG_ConvertorParams.LineOffset=8-JPEG_ConvertorParams.LineOffset;
			}
			JPEG_ConvertorParams.H_factor=8;
			JPEG_ConvertorParams.V_factor=8;
		}
	}else if(JPEG_ConvertorParams.ColorSpace==JPEG_GRAYSCALE_COLORSPACE)//GrayScale颜色空间
	{
		*pFunction=JPEG_MCU_Gray_ARGB_ConvertBlocks;					//使用Y Gray转换
		JPEG_ConvertorParams.LineOffset=JPEG_ConvertorParams.ImageWidth%8;
		if(JPEG_ConvertorParams.LineOffset!=0)
		{
			JPEG_ConvertorParams.LineOffset=8-JPEG_ConvertorParams.LineOffset;
		}
		JPEG_ConvertorParams.H_factor=8;
		JPEG_ConvertorParams.V_factor=8;
	}else if(JPEG_ConvertorParams.ColorSpace==JPEG_CMYK_COLORSPACE)		//CMYK颜色空间
	{
		*pFunction=JPEG_MCU_YCCK_ARGB_ConvertBlocks;					//使用CMYK颜色转换
		JPEG_ConvertorParams.LineOffset=JPEG_ConvertorParams.ImageWidth%8; 
		if(JPEG_ConvertorParams.LineOffset!=0)
		{
			JPEG_ConvertorParams.LineOffset=8-JPEG_ConvertorParams.LineOffset;
		}
		JPEG_ConvertorParams.H_factor=8;
		JPEG_ConvertorParams.V_factor=8;
	}else return 0X01;	//不支持的颜色空间  
	JPEG_ConvertorParams.WidthExtend=JPEG_ConvertorParams.ImageWidth+JPEG_ConvertorParams.LineOffset;
	JPEG_ConvertorParams.ScaledWidth=JPEG_BYTES_PER_PIXEL*JPEG_ConvertorParams.ImageWidth;
	hMCU=(JPEG_ConvertorParams.ImageWidth/JPEG_ConvertorParams.H_factor);
	if((JPEG_ConvertorParams.ImageWidth%JPEG_ConvertorParams.H_factor)!=0)hMCU++;	//+1 for horizenatl incomplete MCU   
	vMCU=(JPEG_ConvertorParams.ImageHeight/JPEG_ConvertorParams.V_factor);
	if((JPEG_ConvertorParams.ImageHeight%JPEG_ConvertorParams.V_factor)!=0)vMCU++;	//+1 for vertical incomplete MCU  
	JPEG_ConvertorParams.MCU_Total_Nb=(hMCU*vMCU);
	*ImageNbMCUs=JPEG_ConvertorParams.MCU_Total_Nb;
	return 0X00;
}














