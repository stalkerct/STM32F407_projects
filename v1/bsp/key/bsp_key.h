#ifndef __BSP_KEY_H__
#define __BSP_KEY_H__

#include "stm32f4xx.h"

/* 板载按键数量 */
#define KEY_NUM     4

/* 按键消抖时间(ms) */
#define KEY_DEBOUNCE_MS     10

/*
 * key 取值 1 ~ 4，依次对应 PD13、PD12、PD11、PD14；越界的编号返回 0。
 *
 * 【调用契约】bsp_key_scan() 的消抖用的是 bsp_tim 模块的毫秒计数，所以：
 *   1. 必须先调用 bsp_tim_init()；
 *   2. bsp_tim 必须保持运行，一旦 bsp_tim_stop()，按键就会失灵。
 *
 * bsp_key_read() / bsp_key_scan() 都不阻塞，可以在主循环里反复调用。
 */
void    bsp_key_init(void);
uint8_t bsp_key_read(uint8_t key);
uint8_t bsp_key_scan(void);

#endif