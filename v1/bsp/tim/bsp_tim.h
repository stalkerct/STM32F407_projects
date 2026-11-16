#ifndef __BSP_TIM_H__
#define __BSP_TIM_H__

#include "stm32f4xx.h"

/*
 * 通用定时器 TIM3 模块（挂在 APB1 总线上）
 *
 * 作用：
 *   1. 定时器硬件固定每 1ms 中断一次，中断里累加毫秒计数，
 *      所以 bsp_tim_get_tick() 是真正的毫秒时间基准，
 *      可以拿来做消抖、超时判断这些需要”知道过了多久”的事情；
 *   2. 支持注册一个周期回调函数，流水灯换灯这类
 *      需要”每隔一段时间做一次”的事情都写在回调里，
 *      回调周期由软件分频得到，跟 1ms 的硬件节拍互不影响。
 *
 * 定时器时钟不是写死的，bsp_tim_init() 会根据当前 RCC 的实际配置算出来，
 * 以后把主频改成别的频率，本模块不用改。
 */

/* 周期回调函数类型 */
typedef void (*bsp_tim_callback_t)(void);

/*
 * 外部可调用函数
 */
void     bsp_tim_init(uint32_t period_ms);                  /* 初始化并启动，period_ms 为回调周期(ms) */
void     bsp_tim_set_callback(bsp_tim_callback_t callback); /* 注册周期回调，传 0 表示取消 */
void     bsp_tim_start(void);                               /* 启动定时器 */
void     bsp_tim_stop(void);                                /* 停止定时器 */
uint32_t bsp_tim_get_tick(void);                            /* 读取开机以来的毫秒数（1ms 分辨率） */
uint32_t bsp_tim_get_period(void);                          /* 读取当前的回调周期(ms) */

/* TIM3 中断服务函数，已经在本模块的 .c 文件里实现 */
void TIM3_IRQHandler(void);

#endif
