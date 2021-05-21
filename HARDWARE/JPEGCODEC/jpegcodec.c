#include "jpegcodec.h"
#include "usart.h"
#include "delay.h"
#include "malloc.h"
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

void (*jpeg_in_callback)(void);		//JPEG DMA����ص�����
void (*jpeg_out_callback)(void);	//JPEG DMA��� �ص�����
void (*jpeg_eoc_callback)(void);	//JPEG ������� �ص�����
void (*jpeg_hdp_callback)(void);	//JPEG Header������� �ص�����


JPEG_HandleTypeDef  JPEG_Handler;    		//JPEG���
DMA_HandleTypeDef   JPEGDMAIN_Handler;		//JPEG��������DMA
DMA_HandleTypeDef   JPEGDMAOUT_Handler;		//JPEG�������DMA

//JPEG�淶(ISO/IEC 10918-1��׼)������������
//��ȡJPEGͼƬ����ʱ��Ҫ�õ�
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

//JPEGӲ����������&���DMA����
//meminaddr:JPEG����DMA�洢����ַ. 
//memoutaddr:JPEG���DMA�洢����ַ. 
//meminsize:����DMA���ݳ���,0~262143,���ֽ�Ϊ��λ
//memoutsize:���DMA���ݳ���,0~262143,���ֽ�Ϊ��λ 
void JPEG_IN_OUT_DMA_Init(u32 meminaddr,u32 memoutaddr,u32 meminsize,u32 memoutsize)
{ 
	if(meminsize%4)meminsize+=4-meminsize%4;	//��չ��4�ı���
	meminsize/=4;								//����4
	if(memoutsize%4)memoutsize+=4-memoutsize%4;	//��չ��4�ı���
	memoutsize/=4;								//����4 
	
	__HAL_RCC_DMA2_CLK_ENABLE();                                    	//ʹ��DMA2ʱ��
	
	//JPEG��������DMAͨ������
    JPEGDMAIN_Handler.Instance=DMA2_Stream0;                          	//DMA2������0                   
    JPEGDMAIN_Handler.Init.Channel=DMA_CHANNEL_9;                    	//ͨ��9
    JPEGDMAIN_Handler.Init.Direction=DMA_MEMORY_TO_PERIPH;            	//�洢��������
    JPEGDMAIN_Handler.Init.PeriphInc=DMA_PINC_DISABLE;                	//���������ģʽ
    JPEGDMAIN_Handler.Init.MemInc=DMA_MINC_ENABLE;                     	//�洢������ģʽ
    JPEGDMAIN_Handler.Init.PeriphDataAlignment=DMA_PDATAALIGN_WORD;   	//�������ݳ���:32λ
    JPEGDMAIN_Handler.Init.MemDataAlignment=DMA_MDATAALIGN_WORD;       	//�洢�����ݳ���:32λ
    JPEGDMAIN_Handler.Init.Mode=DMA_NORMAL;                         	//��ͨģʽ
    JPEGDMAIN_Handler.Init.Priority=DMA_PRIORITY_HIGH;                	//�����ȼ�
    JPEGDMAIN_Handler.Init.FIFOMode=DMA_FIFOMODE_ENABLE;              	//ʹ��FIFO
    JPEGDMAIN_Handler.Init.FIFOThreshold=DMA_FIFO_THRESHOLD_FULL; 		//ȫFIFO
    JPEGDMAIN_Handler.Init.MemBurst=DMA_MBURST_INC4;                	//�洢��4��������ͻ������
    JPEGDMAIN_Handler.Init.PeriphBurst=DMA_PBURST_INC4;             	//����4��������ͻ������
    HAL_DMA_Init(&JPEGDMAIN_Handler);	                                //��ʼ��DMA
    
    //�ڿ���DMA֮ǰ��ʹ��__HAL_UNLOCK()����һ��DMA,��ΪHAL_DMA_Statrt()HAL_DMAEx_MultiBufferStart()
    //����������һ��ʼҪ��ʹ��__HAL_LOCK()����DMA,������__HAL_LOCK()���жϵ�ǰ��DMA״̬�Ƿ�Ϊ����״̬�������
    //����״̬�Ļ���ֱ�ӷ���HAL_BUSY�������ᵼ�º���HAL_DMA_Statrt()��HAL_DMAEx_MultiBufferStart()������DMA����
    //����ֱ�ӱ�������DMAҲ�Ͳ�������������Ϊ�˱���������������������DMA֮ǰ�ȵ���__HAL_UNLOC()�Ƚ���һ��DMA��
    __HAL_UNLOCK(&JPEGDMAIN_Handler);
	HAL_DMA_Start(&JPEGDMAIN_Handler,meminaddr,(u32)&JPEG->DIR,meminsize);//����DMA	
	__HAL_DMA_ENABLE_IT(&JPEGDMAIN_Handler,DMA_IT_TC);    				//������������ж�
	HAL_NVIC_SetPriority(DMA2_Stream0_IRQn,2,3);  						//��ռ2�������ȼ�3����2 
    HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);        						//ʹ���ж�

	//JPEG�������DMAͨ������
    JPEGDMAOUT_Handler.Instance=DMA2_Stream1;                          	//DMA2������1                 
    JPEGDMAOUT_Handler.Init.Channel=DMA_CHANNEL_9;                    	//ͨ��9
    JPEGDMAOUT_Handler.Init.Direction=DMA_PERIPH_TO_MEMORY;            	//���赽�洢��
    JPEGDMAOUT_Handler.Init.PeriphInc=DMA_PINC_DISABLE;                	//���������ģʽ
    JPEGDMAOUT_Handler.Init.MemInc=DMA_MINC_ENABLE;                     //�洢������ģʽ
    JPEGDMAOUT_Handler.Init.PeriphDataAlignment=DMA_PDATAALIGN_WORD;   	//�������ݳ���:32λ
    JPEGDMAOUT_Handler.Init.MemDataAlignment=DMA_MDATAALIGN_WORD;      	//�洢�����ݳ���:32λ
    JPEGDMAOUT_Handler.Init.Mode=DMA_NORMAL;                         	//��ͨģʽ
    JPEGDMAOUT_Handler.Init.Priority=DMA_PRIORITY_VERY_HIGH;            //�������ȼ�
    JPEGDMAOUT_Handler.Init.FIFOMode=DMA_FIFOMODE_ENABLE;              	//ʹ��FIFO
    JPEGDMAOUT_Handler.Init.FIFOThreshold=DMA_FIFO_THRESHOLD_FULL; 		//ȫFIFO
    JPEGDMAOUT_Handler.Init.MemBurst=DMA_MBURST_INC4;                	//�洢��4��������ͻ������
    JPEGDMAOUT_Handler.Init.PeriphBurst=DMA_PBURST_INC4;             	//����4��������ͻ������
    HAL_DMA_Init(&JPEGDMAOUT_Handler);	                                //��ʼ��DMA
    
    //�ڿ���DMA֮ǰ��ʹ��__HAL_UNLOCK()����һ��DMA,��ΪHAL_DMA_Statrt()HAL_DMAEx_MultiBufferStart()
    //����������һ��ʼҪ��ʹ��__HAL_LOCK()����DMA,������__HAL_LOCK()���жϵ�ǰ��DMA״̬�Ƿ�Ϊ����״̬�������
    //����״̬�Ļ���ֱ�ӷ���HAL_BUSY�������ᵼ�º���HAL_DMA_Statrt()��HAL_DMAEx_MultiBufferStart()������DMA����
    //����ֱ�ӱ�������DMAҲ�Ͳ�������������Ϊ�˱���������������������DMA֮ǰ�ȵ���__HAL_UNLOC()�Ƚ���һ��DMA��
    __HAL_UNLOCK(&JPEGDMAOUT_Handler);
	HAL_DMA_Start(&JPEGDMAOUT_Handler,(u32)&JPEG->DOR,memoutaddr,memoutsize);//����DMA	
	__HAL_DMA_ENABLE_IT(&JPEGDMAOUT_Handler,DMA_IT_TC);    				//������������ж�
	HAL_NVIC_SetPriority(DMA2_Stream1_IRQn,2,3);  						//��ռ2�������ȼ�3����2
    HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);        						//ʹ���ж�
} 



//DMA2_Stream0�жϷ�����
//����Ӳ��JPEG����ʱ�����������
void DMA2_Stream0_IRQHandler(void)
{ 
    if(__HAL_DMA_GET_FLAG(&JPEGDMAIN_Handler,DMA_FLAG_TCIF0_4)!=RESET) //DMA�������
    {
        __HAL_DMA_CLEAR_FLAG(&JPEGDMAIN_Handler,DMA_FLAG_TCIF0_4);     //���DMA��������жϱ�־λ
		JPEG->CR&=~(1<<11);			//�ر�JPEG��DMA IN
		__HAL_JPEG_DISABLE_IT(&JPEG_Handler,JPEG_IT_IFT|JPEG_IT_IFNF|JPEG_IT_OFT|\
							JPEG_IT_OFNE|JPEG_IT_EOC|JPEG_IT_HPD);		//�ر�JPEG�ж�,��ֹ�����.		
        if(jpeg_in_callback!=NULL)jpeg_in_callback();					//ִ�лص�����
		__HAL_JPEG_ENABLE_IT(&JPEG_Handler,JPEG_IT_EOC|JPEG_IT_HPD);	//ʹ��EOC��HPD�ж�.
    }      		 											 
}  

//DMA2_Stream1�жϷ�����
//����Ӳ��JPEG����������������
void DMA2_Stream1_IRQHandler(void)
{     
    if(__HAL_DMA_GET_FLAG(&JPEGDMAOUT_Handler,DMA_FLAG_TCIF1_5)!=RESET) //DMA�������
    {
        __HAL_DMA_CLEAR_FLAG(&JPEGDMAOUT_Handler,DMA_FLAG_TCIF1_5);     //���DMA��������жϱ�־λ
		JPEG->CR&=~(1<<12);												//�ر�JPEG��DMA OUT
		__HAL_JPEG_DISABLE_IT(&JPEG_Handler,JPEG_IT_IFT|JPEG_IT_IFNF|JPEG_IT_OFT|\
							JPEG_IT_OFNE|JPEG_IT_EOC|JPEG_IT_HPD);		//�ر�JPEG�ж�,��ֹ�����.	
        if(jpeg_out_callback!=NULL)jpeg_out_callback();					//ִ�лص�����
		__HAL_JPEG_ENABLE_IT(&JPEG_Handler,JPEG_IT_EOC|JPEG_IT_HPD);	//ʹ��EOC��HPD�ж�.
    }     		 											 
}  

//JPEG�����жϷ�����
void JPEG_IRQHandler(void)
{
	if(__HAL_JPEG_GET_FLAG(&JPEG_Handler,JPEG_FLAG_HPDF)!=RESET)//JPEG Header�������		
	{ 
		jpeg_hdp_callback();
		__HAL_JPEG_DISABLE_IT(&JPEG_Handler,JPEG_IT_HPD);		//��ֹJpeg Header��������ж�		
		__HAL_JPEG_CLEAR_FLAG(&JPEG_Handler,JPEG_FLAG_HPDF);	//���HPDFλ(header�������λ)			
	}
	if(__HAL_JPEG_GET_FLAG(&JPEG_Handler,JPEG_FLAG_EOCF)!=RESET) //JPEG�������   
	{
		JPEG_DMA_Stop();
		jpeg_eoc_callback();
		__HAL_JPEG_CLEAR_FLAG(&JPEG_Handler,JPEG_FLAG_EOCF);	//���EOCλ(�������λ)	
		__HAL_DMA_DISABLE(&JPEGDMAIN_Handler);					//�ر�JPEG��������DMA
		__HAL_DMA_DISABLE(&JPEGDMAOUT_Handler);					//�ر�JPEG�������DMA 
	}
}

//��ʼ��Ӳ��JPEG�ں�
//tjpeg:jpeg�������ƽṹ��
//����ֵ:0,�ɹ�;
//    ����,ʧ��
u8 JPEG_Core_Init(jpeg_codec_typedef *tjpeg)
{
//	u8 i;
	
	JPEG_Handler.Instance=JPEG;
  HAL_JPEG_Init(&JPEG_Handler);   	//��ʼ��JPEG
	
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
//		tjpeg->outbuf[i].buf=mymalloc(SRAMIN,JPEG_DMA_OUTBUF_LEN+32);//�п��ܻ����Ҫ32�ֽ��ڴ�
//		if(tjpeg->outbuf[i].buf==NULL)		
//		{
//			JPEG_Core_Destroy(tjpeg);
//			return 1;
//		}   
//	}
	return 0;
}

//JPEG�ײ�������ʱ��ʹ��
//�˺����ᱻHAL_JPEG_Init()����
//hsdram:JPEG���
void HAL_JPEG_MspInit(JPEG_HandleTypeDef *hjpeg)
{
    __HAL_RCC_JPEG_CLK_ENABLE();            //ʹ��JPEGʱ��
    
    HAL_NVIC_SetPriority(JPEG_IRQn,1,3);    //JPEG�ж����ȼ�
    HAL_NVIC_EnableIRQ(JPEG_IRQn);
}

//�ر�Ӳ��JPEG�ں�,���ͷ��ڴ�
//tjpeg:jpeg�������ƽṹ��
void JPEG_Core_Destroy(jpeg_codec_typedef *tjpeg)
{
//	u8 i; 
	JPEG_DMA_Stop();//ֹͣDMA����
//	for(i=0;i<JPEG_DMA_INBUF_NB;i++)myfree(SRAMIN,tjpeg->inbuf[i].buf);		//�ͷ��ڴ�
//	for(i=0;i<JPEG_DMA_OUTBUF_NB;i++)myfree(SRAMIN,tjpeg->outbuf[i].buf);	//�ͷ��ڴ�
}

//��ʼ��Ӳ��JPEG������
//tjpeg:jpeg�������ƽṹ��
void JPEG_Decode_Init(jpeg_codec_typedef *tjpeg)
{ 
	u8 i;
	tjpeg->inbuf_read_ptr=0;
	tjpeg->inbuf_write_ptr=0;
	tjpeg->indma_pause=0;
	tjpeg->outbuf_read_ptr=0;
	tjpeg->outbuf_write_ptr=0;	
	tjpeg->outdma_pause=0;	
	tjpeg->state=JPEG_STATE_NOHEADER;	//ͼƬ���������־
	tjpeg->blkindex=0;					//��ǰMCU���
	tjpeg->total_blks=0;				//��MCU��Ŀ
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
	JPEG->CONFR1|=JPEG_CONFR1_DE;			//ʹ��Ӳ��JPEG����ģʽ
	__HAL_JPEG_ENABLE_IT(&JPEG_Handler,JPEG_IT_HPD);//ʹ��Jpeg Header��������ж�
	__HAL_JPEG_ENABLE_IT(&JPEG_Handler,JPEG_IT_EOC);//ʹ�ܽ�������ж�  			
	JPEG->CONFR0|=JPEG_CONFR0_START;			//ʹ��JPEG�������� 		
}

//����JPEG DMA�������
void JPEG_DMA_Start(void)
{ 
	__HAL_DMA_ENABLE(&JPEGDMAIN_Handler);	//��JPEG��������DMA
	__HAL_DMA_ENABLE(&JPEGDMAOUT_Handler); 	//��JPEG�������DMA 
	JPEG->CR|=3<<11; 						//JPEG IN&OUT DMAʹ��	
}

//ֹͣJPEG DMA�������
void JPEG_DMA_Stop(void)
{
	JPEG->CR&=~(3<<11); 					//JPEG IN&OUT DMA��ֹ 
	JPEG->CONFR0&=~(1<<0);					//ֹͣJPEG�������� 
	__HAL_JPEG_DISABLE_IT(&JPEG_Handler,JPEG_IT_IFT|JPEG_IT_IFNF|JPEG_IT_OFT|\
							JPEG_IT_OFNE|JPEG_IT_EOC|JPEG_IT_HPD);		//�ر������ж�  
					
	JPEG->CFR=3<<5;							//��ձ�־  
}

//��ͣDMA IN����
void JPEG_IN_DMA_Pause(void)
{  
	JPEG->CR&=~(1<<11);			//��ͣJPEG��DMA IN
}

//�ָ�DMA IN����(Ϊ��Ч�ʣ�ֱ�Ӳ����Ĵ�����)
//memaddr:�洢���׵�ַ
//memlen:Ҫ�������ݳ���(���ֽ�Ϊ��λ)
void JPEG_IN_DMA_Resume(u32 memaddr,u32 memlen)
{  
	if(memlen%4)memlen+=4-memlen%4;//��չ��4�ı���
	memlen/=4;					//����4
	DMA2->LIFCR|=0X3D<<6*0;		//���ͨ��0�������жϱ�־
	DMA2_Stream0->M0AR=memaddr;	//���ô洢����ַ
	DMA2_Stream0->NDTR=memlen;	//���䳤��Ϊmemlen
	DMA2_Stream0->CR|=1<<0;		//����DMA2,Stream0 
	JPEG->CR|=1<<11; 			//�ָ�JPEG DMA IN 
}

//��ͣDMA OUT����
void JPEG_OUT_DMA_Pause(void)
{  
	JPEG->CR&=~(1<<12);			//��ͣJPEG��DMA OUT
}

//�ָ�DMA OUT����(Ϊ��Ч�ʣ�ֱ�Ӳ����Ĵ���)
//memaddr:�洢���׵�ַ
//memlen:Ҫ�������ݳ���(���ֽ�Ϊ��λ)
void JPEG_OUT_DMA_Resume(u32 memaddr,u32 memlen)
{  
	if(memlen%4)memlen+=4-memlen%4;//��չ��4�ı���
	memlen/=4;					//����4
	DMA2->LIFCR|=0X3D<<6*1;		//���ͨ��1�������жϱ�־
	DMA2_Stream1->M0AR=memaddr;	//���ô洢����ַ
	DMA2_Stream1->NDTR=memlen;	//���䳤��Ϊmemlen
	DMA2_Stream1->CR|=1<<0;		//����DMA2,Stream1 
	JPEG->CR|=1<<12; 			//�ָ�JPEG DMA OUT 
}

//��ȡͼ����Ϣ
//tjpeg:jpeg����ṹ��
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
	tjpeg->Conf.ImageHeight=(JPEG->CONFR1&0XFFFF0000)>>16;	//���ͼ��߶�
	tjpeg->Conf.ImageWidth=(JPEG->CONFR3&0XFFFF0000)>>16;	//���ͼ����
	if((tjpeg->Conf.ColorSpace==JPEG_YCBCR_COLORSPACE)||(tjpeg->Conf.ColorSpace==JPEG_CMYK_COLORSPACE))
	{
		yblockNb  =(JPEG->CONFR4&(0XF<<4))>>4;
		cBblockNb =(JPEG->CONFR5&(0XF<<4))>>4;
		cRblockNb =(JPEG->CONFR6&(0XF<<4))>>4;
		if((yblockNb==1)&&(cBblockNb==0)&&(cRblockNb==0))tjpeg->Conf.ChromaSubsampling=JPEG_422_SUBSAMPLING; //16x8 block
		else if((yblockNb==0)&&(cBblockNb==0)&&(cRblockNb==0))tjpeg->Conf.ChromaSubsampling=JPEG_444_SUBSAMPLING;
		else if((yblockNb==3)&&(cBblockNb==0)&&(cRblockNb==0))tjpeg->Conf.ChromaSubsampling = JPEG_420_SUBSAMPLING;
		else tjpeg->Conf.ChromaSubsampling=JPEG_444_SUBSAMPLING; 
	}else tjpeg->Conf.ChromaSubsampling=JPEG_444_SUBSAMPLING;		//Ĭ����4:4:4 
	tjpeg->Conf.ImageQuality=0;	//ͼ����������������ͼƬ����ĩβ,�տ�ʼ��ʱ��,���޷���ȡ��,����ֱ������Ϊ0
}

//�õ�jpegͼ������
//�ڽ�����ɺ�,���Ե��ò������ȷ�Ľ��.
//����ֵ:ͼ������,0~100.
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






































