#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "sdram.h"
#include "tftlcd.h"
#include "ltdc.h"
#include "mpu.h"
#include "usmart.h"
#include "malloc.h"
#include "touch.h"

#include "base/lcd.h"
#include "tkc/mem.h"
#include "lcd/lcd_mem_bgr565.h"
#include "lcd/lcd_mem_bgra8888.h"

extern int gui_app_start(int lcd_w, int lcd_h);

#define FB_ADDR (uint8_t*)0XC0000000

static uint8_t* s_framebuffers[2];

lcd_t* stm32f767_create_lcd(wh_t w, wh_t h) {
  lcd_t* lcd = NULL;
  uint32_t size = w * h * lcdltdc.pixsize;
  s_framebuffers[0] = FB_ADDR;
  s_framebuffers[1] = FB_ADDR + size;

#if LCD_PIXFORMAT == LCD_PIXFORMAT_ARGB8888 
  lcd = lcd_mem_bgra8888_create_double_fb(w, h, s_framebuffers[0], s_framebuffers[1]);
#else
  lcd = lcd_mem_bgr565_create_double_fb(w, h, s_framebuffers[0], s_framebuffers[1]);
#endif /*LCD_PIXFORMAT*/
	
  return lcd;
}

int main(void) {   
  Write_Through();                //Cahceǿ��͸д
  MPU_Memory_Protection();        //������ش洢����
  Cache_Enable();                 //��L1-Cache
      
	Stm32_Clock_Init(432,25,2,9);   //����ʱ��,216Mhz 
	HAL_Init();				        //��ʼ��HAL��
  delay_init(216);                //��ʱ��ʼ��
	uart_init(115200);		        //���ڳ�ʼ��
  KEY_Init();                     //������ʼ��
  LED_Init();                     //��ʼ��LED
  SDRAM_Init();                   //SDRAM��ʼ��
	TFTLCD_Init();                  //��ʼ��LCD
  TP_Init();				        //��������ʼ��

  return gui_app_start(lcdltdc.pwidth, lcdltdc.pheight);
}

