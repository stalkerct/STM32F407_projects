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
#ifndef __BSP_UART_H__
#define __BSP_UART_H__

#include "stm32f4xx.h"

/*
 * 串口1 驱动（PA9 = TX，PA10 = RX）
 *
 * 【重要】本模块已经把 C 库的 printf 重定向到了 USART1，
 *         所以工程里任意位置直接调用 printf() 即可，无需额外配置。
 *         前提是先调用 uart1_init()。
 *
 * 【注意】printf 是阻塞的，而且不可重入：
 *         不要在中段里调用（比如定时器回调、按键扫描里打印调试信息）。
 */
void uart1_init(uint32_t __Baud);

/*
 * USART1 中断服务函数，实现在本模块的 .c 文件里。
 * 名称必须与启动文件向量表里的符号一字不差，且不能加 static，
 * 否则会静默落到启动文件的 Default_Handler 死循环里。
 */
void USART1_IRQHandler(void);

#endif