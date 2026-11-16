#ifndef __BSP_LED_H__
#define __BSP_LED_H__

#include "stm32f4xx.h"

/* 板载 LED 数量 */
#define LED_NUM     4

/* LED 亮灭状态 */
typedef enum
{
    LED_OFF = 0,
    LED_ON  = 1
} led_state_t;

/*
 * 外部可调用函数
 * led 取值 1 ~ 4，1 对应原理图上的 LED1，依次类推
 */
void bsp_led_init(void);
void bsp_led_set(uint8_t led, led_state_t state);
void bsp_led_on(uint8_t led);
void bsp_led_off(uint8_t led);
void bsp_led_toggle(uint8_t led);
void bsp_led_all_on(void);
void bsp_led_all_off(void);
void bsp_led_all_toggle(void);

#endif
