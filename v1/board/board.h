/*
 * 立创开发板软硬件资料与相关扩展板软硬件资料官网全部开源
 * 开发板官网：www.lckfb.com
 * 技术支持常驻论坛，任何技术问题欢迎随时交流学习
 * 立创论坛：https://oshwhub.com/forum
 * 关注bilibili账号：【立创开发板】，掌握我们的最新动态！
 * 不靠卖板赚钱，以培养中国工程师为己任
 * 

 Change Logs:
 * Date           Author       Notes
 * 2024-03-07     LCKFB-LP    first version
 */
#ifndef __BOARD_H__
#define __BOARD_H__

#include "stm32f4xx.h"

/* 板级初始化：设定向量表基址 + 启动 SysTick 计时源。
 * 延时函数依赖它，所以必须先调用 board_init() 再用 delay_xxx()。 */
void board_init(void);

/* 延时函数，全部是轮询阻塞式实现，不要在中断里调用。
 *   delay_us()  与 delay_1us() 等价，微秒级
 *   delay_1ms() 毫秒级
 * 注意：早期声明过的 delay_ms() 已废弃（.c 里没有实现），统一改用 delay_1ms()。 */
void delay_us(uint32_t _us);
void delay_1ms(uint32_t ms);
void delay_1us(uint32_t us);

#endif