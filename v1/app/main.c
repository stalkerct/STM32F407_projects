/*
 * 立创开发板·硬件驱动库·扩展板硬件驱动合集
 * 立创开发板：www.lckfb.com
 * 技术支持：常驻论坛，任何技术问题欢迎随时交流学习
 * 技术论坛：https://oshwhub.com/forum
 * 关注bilibili账号：【立创开发板】，掌握我们的最新动态！
 * 不靠卖板赚钱，以培养中国工程师为己任
 *

 Change Logs:
 * Date           Author       Notes
 * 2026-11-10     LCKFB-LP     定时器(TIM3)驱动流水灯例程
 */
#include "board.h"
#include "bsp_uart.h"
#include "bsp_led.h"
#include "bsp_key.h"
#include "bsp_tim.h"
#include <stdio.h>

/*
 * 硬件连接：
 *   LED1 ~ LED4 -> PD3  ~ PD6   低电平点亮
 *   KEY1 ~ KEY4 -> PD13/PD12/PD11/PD14  按下为低电平
 *
 * 本例程：
 *   TIM3 硬件每 1ms 中断一次，回调每 500ms 执行一次（500ms 这个周期
 *   由 bsp_tim 内部软件分频得到），在回调里换一个灯点亮，形成流水灯。
 *   换灯交给定时器做，主循环就空出来了；按键扫描也改成了非阻塞式
 *   （消抖靠定时器的毫秒计数判断），所以主循环里一次 delay 都没有，
 *   后面想加别的事情随时可以加。
 *
 *   KEY1 -> 暂停 / 继续流水（只是让回调不再换灯，定时器本身一直在跑，
 *           因为按键消抖要靠它的毫秒计数）
 *   KEY2 -> 切换流水方向
 */

/* 流水灯当前点亮的灯号(1~4)，只在中断里改动 */
static uint8_t led_index = 1;

/* 流水方向：1 从左往右，-1 从右往左；主循环和中断都要访问，所以要 volatile */
static volatile int8_t led_dir = 1;

/* 流水开关：1 正常流水，0 停在当前这个灯上；主循环和中断都要访问 */
static volatile uint8_t led_run = 1;

/**
 -  @brief  定时器周期回调，在 TIM3 中断里每 500ms 执行一次
 -  @note   中断里只做点灯这种很快的事情，不要放 printf 之类的耗时操作
 -  @param  None
 -  @retval None
*/
static void led_flow_callback(void)
{
	if (led_run == 0)             /* 暂停时什么都不做，灯停在原处 */
	{
		return;
	}

	bsp_led_all_off();
	bsp_led_on(led_index);

	led_index = (uint8_t)(led_index + led_dir);

	if (led_index > LED_NUM)        /* 走到头就绕回第一个 */
	{
		led_index = 1;
	}
	else if (led_index < 1)
	{
		led_index = LED_NUM;
	}
}

int main(void)
{
	uint8_t key;

	board_init();

	uart1_init(115200U);

	bsp_led_init();               //使用板载LED
	bsp_key_init();               //使用板载按键

	/* 硬件 1ms 中断一次，回调每 500ms 执行一次，在回调里换灯 */
	bsp_tim_init(300U);
	bsp_tim_set_callback(led_flow_callback);

	printf("LED TIM demo start.\r\n");
	printf("KEY1: pause/resume    KEY2: change direction\r\n");

	while(1)
	{
		key = bsp_key_scan();

		if (key == 1U)            /* KEY1 暂停 / 继续 */
		{
			led_run = (uint8_t)(!led_run);

			printf("tick=%lu ms, %s\r\n",
			       (unsigned long)bsp_tim_get_tick(), led_run ? "run" : "pause");
		}
		else if (key == 2U)       /* KEY2 切换方向 */
		{
			led_dir = (int8_t)(-led_dir);

			printf("tick=%lu ms, dir=%d\r\n",
			       (unsigned long)bsp_tim_get_tick(), (int)led_dir);
		}
	}
}
