/**
 * file:   main_loop_stm32_raw.c
 * author: li xianjing <xianjimli@hotmail.com>
 * brief:  main loop for stm32
 *
 * copyright (c) 2018 - 2018 Guangzhou ZHIYUAN Electronics Co.,Ltd.
 *
 * this program is distributed in the hope that it will be useful,
 * but without any warranty; without even the implied warranty of
 * merchantability or fitness for a particular purpose.  see the
 * license file for more details.
 *
 */

/**
 * history:
 * ================================================================
 * 2018-05-11 li xianjing <xianjimli@hotmail.com> created
 *
 */

#include "key.h"
#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "tftlcd.h"
#include "ltdc.h"
#include "string.h"
#include "sdram.h"
#include "malloc.h"
#include "string.h"
#include "ltdc.h"
#include "touch.h"
#include "stm32_jpg_image_loader.h"

#include "base/g2d.h"
#include "base/idle.h"
#include "base/timer.h"
#include "main_loop/main_loop_simple.h"

uint8_t platform_disaptch_input(main_loop_t* loop) {
  int x = 0;
  int y = 0;
  static int prev_key = 0;
  uint8_t key = KEY_Scan(0);

  switch (key) {
  case KEY0_PRES: {
    key = TK_KEY_RIGHT;
    break;
  }
  case KEY1_PRES: {
    key = TK_KEY_DOWN;
    break;
  }
  case KEY2_PRES: {
    key = TK_KEY_RETURN;
    break;
  }
  case WKUP_PRES: {
    key = TK_KEY_UP;
    break;
  }
  default: { key = 0; }
  }

  if (key != prev_key) {
    main_loop_post_key_event(main_loop(), FALSE, prev_key);
    main_loop_post_key_event(main_loop(), TRUE, key);
  } else {
    main_loop_post_key_event(main_loop(), TRUE, key);
  }
  prev_key = key;

  tp_dev.scan(0);

  x = tp_dev.x[0];
  y = tp_dev.y[0];

  if (tp_dev.sta & TP_PRES_DOWN) {
    main_loop_post_pointer_event(loop, TRUE, x, y);
  } else {
    main_loop_post_pointer_event(loop, FALSE, x, y);
  }

  return 0;
}

extern lcd_t* stm32f767_create_lcd(wh_t w, wh_t h);

lcd_t* platform_create_lcd(wh_t w, wh_t h) {
	/* 由于 flash 不够大，所以屏蔽了硬件解码 jpg，如果有需要的话，可以把 awtk-port\stm32_jpg_image_loader.c 中的文件加入编译，以及打开下面的注册代码 */
	//image_loader_register(image_loader_stm32_jpg());
  return stm32f767_create_lcd(w, h);
}

#include "main_loop/main_loop_raw.inc"
