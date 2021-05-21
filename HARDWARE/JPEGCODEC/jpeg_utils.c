#include "jpeg_utils.h"
#include "jpeg_utils_tbl.h" 
//////////////////////////////////////////////////////////////////////////////////  
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK STM32������
//�ṩSTM32F7 JPEGӲ������  MCU(Minimum Coded Unit)�鵽RGB����ɫת������  
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//��������:2016/7/26
//�汾��V1.0
//ע��:YCbCr��RGB��ת��,û�жԲ���MCU������������ȵ�ͼ��������,����,��ͼƬ�Ŀ��
//���ǵ���MCU�������ص�������ʱ,����ͼƬ������һЩ����.����������,�뱣֤ͼƬ���
//ΪMCU�������ص�����������,��һ����16�ı���.
//********************************************************************************
//�޸�˵��
//��
//////////////////////////////////////////////////////////////////////////////////  

//��Χ�޶��궨�� 
//��ʹ��RGB565���ʱ:CLAMP_LUT��һ��16λ��ѯ��,����λ�ͷ�Χ����.
//��ʹ��RGB888/ARGB888���ʱ:CLAMP_LUT��һ��8λ��ѯ��,����Χ����.
//���:jpeg_utils_tbl.h
#define CLAMP(value) 			CLAMP_LUT[value]			//��ɫ��ѯ�� 


JPEG_MCU_RGB_ConvertorTypeDef JPEG_ConvertorParams;			//��ɫת����������ṹ��


//��YCbCr 4:2:0 Blocksת����RGB����
//һ��MCU BLOCK����:4 8x8 blocks of Y + 1 8x8 block of Cb + 1 8x8 block of Cr
//pInBuffer:ָ������� YCbCr blocks������
//pOutBuffer:ָ������� RGB888/ARGB8888֡������
//BlockIndex:����buf����ĵ�һ��MCU����
//DataCount:���뻺�����Ĵ�С
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
#if JPEG_BYTES_PER_PIXEL==4		//ARGB_8888��ɫ��ʽ   				
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
#elif JPEG_BYTES_PER_PIXEL==3	//RGB_888��ɫ��ʽ  
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
#elif JPEG_BYTES_PER_PIXEL==2	//RGB_565��ɫ��ʽ 
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
//��YCbCr 4:2:2 Blocksת����RGB����
//һ��MCU BLOCK����:2 8x8 blocks of Y + 1 8x8 block of Cb + 1 8x8 block of Cr
//pInBuffer:ָ������� YCbCr blocks������
//pOutBuffer:ָ������� RGB888/ARGB8888֡������
//BlockIndex:����buf����ĵ�һ��MCU����
//DataCount:���뻺�����Ĵ�С
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
#if JPEG_BYTES_PER_PIXEL==4		//ARGB_8888��ɫ��ʽ    
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
#elif JPEG_BYTES_PER_PIXEL==3	//RGB_888��ɫ��ʽ  
						ycomp=(int)(*(pLum+j));
						pOutAddr[JPEG_RED_OFFSET/8]=CLAMP(ycomp+c_red);
						pOutAddr[JPEG_GREEN_OFFSET/8]=CLAMP(ycomp+c_green);
						pOutAddr[JPEG_BLUE_OFFSET/8]=CLAMP(ycomp+c_blue);
						ycomp=(int)(*(pLum+j+1));
						pOutAddr[3+JPEG_RED_OFFSET/8]=CLAMP(ycomp+c_red);
						pOutAddr[3+JPEG_GREEN_OFFSET/8]=CLAMP(ycomp+c_green);
						pOutAddr[3+JPEG_BLUE_OFFSET/8]=CLAMP(ycomp+c_blue);
#elif JPEG_BYTES_PER_PIXEL==2	//RGB_565��ɫ��ʽ  
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
//��YCbCr 4:4:4 Blocksת����RGB����
//һ��MCU BLOCK����:1 8x8 blocks of Y + 1 8x8 block of Cb + 1 8x8 block of Cr
//pInBuffer:ָ������� YCbCr blocks������
//pOutBuffer:ָ������� RGB888/ARGB8888֡������
//BlockIndex:����buf����ĵ�һ��MCU����
//DataCount:���뻺�����Ĵ�С
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
#if JPEG_BYTES_PER_PIXEL==4		//ARGB_8888��ɫ��ʽ    
					ycomp=(int)(*(pLum+j)); 
					*(vu32*)pOutAddr=
					(CLAMP(ycomp+c_red)<<JPEG_RED_OFFSET)|\
					(CLAMP(ycomp+c_green)<<JPEG_GREEN_OFFSET)|\
					(CLAMP(ycomp+c_blue)<<JPEG_BLUE_OFFSET);    
#elif JPEG_BYTES_PER_PIXEL==3	//RGB_888��ɫ��ʽ   
					ycomp=(int)(*(pLum+j)); 
					pOutAddr[JPEG_RED_OFFSET/8]=CLAMP(ycomp+c_red);
					pOutAddr[JPEG_GREEN_OFFSET/8]=CLAMP(ycomp+c_green);
					pOutAddr[JPEG_BLUE_OFFSET/8]=CLAMP(ycomp+c_blue);   
#elif JPEG_BYTES_PER_PIXEL==2	//RGB_565��ɫ��ʽ 
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
//��Y Gray Blocksת����RGB����
//һ��MCU BLOCK����:1 8x8 blocks of Y
//pInBuffer:ָ������� YCbCr blocks������
//pOutBuffer:ָ������� RGB888/ARGB8888֡������
//BlockIndex:����buf����ĵ�һ��MCU����
//DataCount:���뻺�����Ĵ�С
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
#if JPEG_BYTES_PER_PIXEL==4		//ARGB_8888��ɫ��ʽ    
					*(vu32*)pOutAddr=ySample|(ySample<<8)|(ySample<<16); 
#elif JPEG_BYTES_PER_PIXEL==3	//RGB_888��ɫ��ʽ  
					pOutAddr[0]=ySample;
					pOutAddr[1]=ySample;
					pOutAddr[2]=ySample; 
#elif JPEG_BYTES_PER_PIXEL==2	//RGB_565��ɫ��ʽ
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

//��CMYK Blocksת����RGB����
//һ��MCU BLOCK����: 1 Cyan 8x8  block + 1 Magenta 8x8 block + 1 Yellow 8x8 block + 1 Key 8x8 block.
//pInBuffer:ָ������� YCbCr blocks������
//pOutBuffer:ָ������� RGB888/ARGB8888֡������
//BlockIndex:����buf����ĵ�һ��MCU����
//DataCount:���뻺�����Ĵ�С
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
#if JPEG_BYTES_PER_PIXEL==4		//ARGB_8888��ɫ��ʽ    
					*(vu32*)pOutAddr=
					(c_red<<JPEG_RED_OFFSET)|\
					(c_green<<JPEG_GREEN_OFFSET)|\
					(c_blue<<JPEG_BLUE_OFFSET);
#elif JPEG_BYTES_PER_PIXEL==3	//RGB_888��ɫ��ʽ  
					pOutAddr[JPEG_RED_OFFSET/8]=c_red;
					pOutAddr[JPEG_GREEN_OFFSET/8]=c_green;
					pOutAddr[JPEG_BLUE_OFFSET/8]=c_blue;  
#elif JPEG_BYTES_PER_PIXEL==2	//RGB_565��ɫ��ʽ 
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

//��ȡYCbCr��RGB��ɫת���������ܵ�MCU Block��Ŀ.
//pJpegInfo:JPEG�ļ���Ϣ�ṹ��
//pFunction:JPEG_YCbCrToRGB_Convert_Function�ĺ���ָ��,����jpegͼ�����,ָ��ͬ����ɫת������.
//ImageNbMCUs:�ܵ�MCU����Ŀ
//����ֵ:0,����;
//       1,ʧ��;
u8 JPEG_GetDecodeColorConvertFunc(JPEG_ConfTypeDef *pJpegInfo, JPEG_YCbCrToRGB_Convert_Function *pFunction, u32 *ImageNbMCUs)
{
	u32 hMCU, vMCU;
	JPEG_ConvertorParams.ColorSpace=pJpegInfo->ColorSpace;				//ɫ�ʿռ�
	JPEG_ConvertorParams.ImageWidth=pJpegInfo->ImageWidth;				//ͼ����
	JPEG_ConvertorParams.ImageHeight=pJpegInfo->ImageHeight;			//ͼ��߶�
	JPEG_ConvertorParams.ImageSize_Bytes=pJpegInfo->ImageWidth*pJpegInfo->ImageHeight*JPEG_BYTES_PER_PIXEL;	//ת�����ͼ�����ֽ���
	JPEG_ConvertorParams.ChromaSubsampling=pJpegInfo->ChromaSubsampling;//�������
	if(JPEG_ConvertorParams.ColorSpace==JPEG_YCBCR_COLORSPACE)			//YCBCR��ɫ�ռ�
	{
		if(JPEG_ConvertorParams.ChromaSubsampling==JPEG_420_SUBSAMPLING)
		{
			*pFunction=JPEG_MCU_YCbCr420_ARGB_ConvertBlocks;			//ʹ��YCbCr420ת��
			JPEG_ConvertorParams.LineOffset=JPEG_ConvertorParams.ImageWidth%16;
			if(JPEG_ConvertorParams.LineOffset!=0)
			{
				JPEG_ConvertorParams.LineOffset=16-JPEG_ConvertorParams.LineOffset;
			}
			JPEG_ConvertorParams.H_factor=16;
			JPEG_ConvertorParams.V_factor=16;
		}else if(JPEG_ConvertorParams.ChromaSubsampling==JPEG_422_SUBSAMPLING)
		{
			*pFunction=JPEG_MCU_YCbCr422_ARGB_ConvertBlocks;			//ʹ��YCbCr422ת��
 			JPEG_ConvertorParams.LineOffset=JPEG_ConvertorParams.ImageWidth%16;
			if(JPEG_ConvertorParams.LineOffset!=0)
			{
				JPEG_ConvertorParams.LineOffset=16-JPEG_ConvertorParams.LineOffset;
			}
			JPEG_ConvertorParams.H_factor=16;
			JPEG_ConvertorParams.V_factor=8;
		}else		//4:4:4
		{
			*pFunction=JPEG_MCU_YCbCr444_ARGB_ConvertBlocks;			//ʹ��YCbCr444ת��
 			JPEG_ConvertorParams.LineOffset=JPEG_ConvertorParams.ImageWidth%8;
			if(JPEG_ConvertorParams.LineOffset!=0)
			{
				JPEG_ConvertorParams.LineOffset=8-JPEG_ConvertorParams.LineOffset;
			}
			JPEG_ConvertorParams.H_factor=8;
			JPEG_ConvertorParams.V_factor=8;
		}
	}else if(JPEG_ConvertorParams.ColorSpace==JPEG_GRAYSCALE_COLORSPACE)//GrayScale��ɫ�ռ�
	{
		*pFunction=JPEG_MCU_Gray_ARGB_ConvertBlocks;					//ʹ��Y Grayת��
		JPEG_ConvertorParams.LineOffset=JPEG_ConvertorParams.ImageWidth%8;
		if(JPEG_ConvertorParams.LineOffset!=0)
		{
			JPEG_ConvertorParams.LineOffset=8-JPEG_ConvertorParams.LineOffset;
		}
		JPEG_ConvertorParams.H_factor=8;
		JPEG_ConvertorParams.V_factor=8;
	}else if(JPEG_ConvertorParams.ColorSpace==JPEG_CMYK_COLORSPACE)		//CMYK��ɫ�ռ�
	{
		*pFunction=JPEG_MCU_YCCK_ARGB_ConvertBlocks;					//ʹ��CMYK��ɫת��
		JPEG_ConvertorParams.LineOffset=JPEG_ConvertorParams.ImageWidth%8; 
		if(JPEG_ConvertorParams.LineOffset!=0)
		{
			JPEG_ConvertorParams.LineOffset=8-JPEG_ConvertorParams.LineOffset;
		}
		JPEG_ConvertorParams.H_factor=8;
		JPEG_ConvertorParams.V_factor=8;
	}else return 0X01;	//��֧�ֵ���ɫ�ռ�  
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














