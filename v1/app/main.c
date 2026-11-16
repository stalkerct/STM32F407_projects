/*
 * 定时器(TIM3) 驱动流水灯例程
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
 *   LED1 ~ LED4 -> PD3  ~ PD6            低电平点亮
 *   KEY1 ~ KEY4 -> PD13/PD12/PD11/PD14   按下为低电平
 *
 * 本例程：
 *   TIM3 硬件每 1ms 中断一次，回调每 300ms 执行一次（300ms 这个周期
 *   由 bsp_tim 内部软件分频得到），在回调里换一个灯点亮，形成流水灯。
 *   换灯交给定时器做，主循环就空出来了；按键扫描也改成了非阻塞式
 *   （消抖靠定时器的毫秒计数判断），所以主循环里一次 delay 都没有，
 *   后面想加别的事情随时可以加。
 *
 *   KEY1 -> 暂停 / 继续流水（只是让回调不再换灯，定时器本身一直在跑，
 *           因为按键消抖要靠它的毫秒计数）
 *   KEY2 -> 切换流水方向
 *
 * 【初始化顺序不要随便调】
 *   1. board_init() 必须最先调用，它是延时和计时的基础；
 *   2. bsp_tim_init() 必须在第一次 bsp_key_scan() 之前，
 *      因为按键消抖依赖 bsp_tim 提供的毫秒计数；
 *   3. bsp_tim_set_callback() 要在 bsp_tim_init() 之后尽早调用 ——
 *      init 返回时定时器就已经在产生中断了，只是回调还是空指针。
 */

/* 流水灯当前点亮的灯号(1~4)。
 * 只有 led_flow_callback() 读写它，主循环完全不碰，所以不需要 volatile。 */
static uint8_t led_index = 1;

/* 流水方向：1 从左往右，-1 从右往左。
 * 主循环写、中断读，属于跨上下文的共享变量，必须 volatile 防止编译器缓存。 */
static volatile int8_t led_dir = 1;

/* 流水开关：1 正常流水，0 停在当前这个灯上。
 * 同样是主循环写、中断读，必须 volatile。 */
static volatile uint8_t led_run = 1;

/**
 * @brief  定时器周期回调，在 TIM3 中断里每 300ms 执行一次
 * @note   本函数运行在中断上下文（用 MSP，不是主循环的栈），所以：
 *         1. 不能放 printf —— 既耗时又不可重入（主循环里也在用 printf）；
 *         2. 不能调 delay_xxx —— 会把整个中断活锁住；
 *         3. 只做寄存器级的快操作，几微秒内必须返回。
 *         下面只改了几个变量和 4 个 GPIO，是合规的。
 */
static void led_flow_callback(void)
{
    if (led_run == 0)
    {
        return;                     /* 暂停中：灯停在原处，什么都不做 */
    }

    bsp_led_all_off();              /* 先全灭再点亮下一盏，保证同时只有一盏灯亮 */
    bsp_led_on(led_index);

    led_index = (uint8_t)(led_index + led_dir);     /* 按当前方向挪一格 */

    /* 有效范围是 1 ~ LED_NUM，越界就绕到另一头，形成循环流水 */
    if (led_index > LED_NUM)
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

    board_init();                   /* 向量表基址 + SysTick 计时源 */

    uart1_init(115200U);            /* 串口1；printf 已在本模块内部重定向过来 */

    /* 硬件 1ms 中断一次，回调每 300ms 执行一次 */
    bsp_tim_init(300U);

    bsp_led_init();
    bsp_key_init();

    /* 注册换灯回调。bsp_tim_init() 返回时定时器已经在跑了，
     * 所以这一步要尽早做；注册之前中断里是空转的，不会跑飞 */
    bsp_tim_set_callback(led_flow_callback);

    printf("LED TIM demo start.\r\n");
    printf("KEY1: pause/resume    KEY2: change direction\r\n");

    while (1)
    {
        /* 非阻塞扫描：没有按键时立刻返回 0，不会卡住主循环 */
        key = bsp_key_scan();

        if (key == 1U)              /* KEY1：暂停 / 继续 */
        {
            led_run = (uint8_t)(!led_run);

            printf("tick=%lu ms, %s\r\n",
                   (unsigned long)bsp_tim_get_tick(), led_run ? "run" : "pause");
        }
        else if (key == 2U)         /* KEY2：切换流水方向 */
        {
            led_dir = (int8_t)(-led_dir);

            printf("tick=%lu ms, dir=%d\r\n",
                   (unsigned long)bsp_tim_get_tick(), (int)led_dir);
        }
    }
}
