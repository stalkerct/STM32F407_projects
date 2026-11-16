#ifndef __BSP_KEY_H__
#define __BSP_KEY_H__

#include "stm32f4xx.h"

/* 板载按键数量 */
#define KEY_NUM     4

/* 按键消抖时间(ms) */
#define KEY_DEBOUNCE_MS     10

/*
 * 外部可调用函数
 * key 取值 1 ~ 4，依次对应 PD13、PD12、PD11、PD14
 *
 * 注意：bsp_key_scan() 的消抖用的是定时器模块的毫秒计数，
 *       所以调用它之前必须先 bsp_tim_init()，而且定时器要保持运行。
 */
void    bsp_key_init(void);
uint8_t bsp_key_read(uint8_t key);
uint8_t bsp_key_scan(void);

#endif
