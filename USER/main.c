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
#include "base/mem.h"
#include "lcd/lcd_mem_rgb565.h"
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
  lcd = lcd_mem_rgb565_create_double_fb(w, h, s_framebuffers[0], s_framebuffers[1]);
#endif /*LCD_PIXFORMAT*/
	
  return lcd;
}

int main(void) {   
  Write_Through();                //Cahce强制透写
  MPU_Memory_Protection();        //保护相关存储区域
  Cache_Enable();                 //打开L1-Cache
      
	Stm32_Clock_Init(432,25,2,9);   //设置时钟,216Mhz 
	HAL_Init();				        //初始化HAL库
  delay_init(216);                //延时初始化
	uart_init(115200);		        //串口初始化
  KEY_Init();                     //按键初始化
  LED_Init();                     //初始化LED
  SDRAM_Init();                   //SDRAM初始化
	TFTLCD_Init();                  //初始化LCD
  TP_Init();				        //触摸屏初始化

  return gui_app_start(lcdltdc.pwidth, lcdltdc.pheight);
}

