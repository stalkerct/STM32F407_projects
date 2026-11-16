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
#include <board.h>

/* 预留的系统节拍变量，目前工程里没有用到，留着是为了以后做软件定时器。
 * __IO 是 CMSIS 对 volatile 的封装，防止编译器把它优化掉。 */
static __IO uint32_t g_system_tick = 0;


/**
 * @brief  板级初始化：设定向量表基地址，并把 SysTick 配成延时用的计时源
 * @note   本函数没有配置 NVIC 优先级分组，所以中断优先级沿用芯片复位默认值：
 *         4 位全部作为抢占优先级、子优先级位数 = 0。
 *         也就是说各模块里写的“子优先级(SubPriority)”目前并不生效，
 *         需要真正的优先级区分，得先调用 NVIC_PriorityGroupConfig()。
 */
void board_init(void)
{
    /* 系统控制块 SCB：设定向量表基地址。
     * 注意动的是 SCB（系统控制块），不是 NVIC，两者是不同的外设 */
#define NVIC_VTOR_MASK              0x3FFFFF80
#ifdef  VECT_TAB_RAM
    /* 向量表放到 RAM 起始 0x10000000（一般只在调试场景用） */
    SCB->VTOR  = (0x10000000 & NVIC_VTOR_MASK);
#else  /* VECT_TAB_FLASH  */
    /* 向量表放到 Flash 起始 0x08000000。
     * 从 Flash 启动时硬件默认就是这个值，所以这一行平时是冗余的；
     * 只有做了 Bootloader + APP 分区，或把向量表搬到 RAM 时才必须改，
     * 改错了会让所有中断跳到错误的表里（表现为中断静默失效）。 */
    SCB->VTOR  = (0x08000000 & NVIC_VTOR_MASK);
#endif

    /* SysTick 在这里只当“自由运行的递减计数器”用，故意不开中断：
     * LOAD 取 0xFFFF，让它每 65536 个 HCLK 回绕一次，
     * delay_us() 靠轮询 VAL 的两次差值累加出精确的微秒数。 */
    SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);
    SysTick->LOAD = 0xFFFF;                     /* 重装载值，不是“清空计数器” */
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;   /* 开始递减计数 */
}

/**
 * @brief  基于内核 SysTick 的微秒级延时（轮询实现，会一直占用 CPU）
 * @note   前提是 board_init() 已经把 SysTick 配成自由运行计数器。
 *         VAL 从 LOAD 一直递减到 0 然后回绕，所以两次差值要分两种情形累计。
 * @note   ticks 那行是 uint32_t 乘法，_us 超过约 2500 万会溢出；
 *         需要更长的延时请用 delay_1ms() 分段调用。
 * @param  _us 要延时的微秒数
 */
void delay_us(uint32_t _us)
{
    uint32_t ticks;
    uint32_t told, tnow, tcnt = 0;

    /* 需要的时钟数 = 微秒数 x 每微秒的 HCLK 数（168MHz 时每微秒 168 个） */
    ticks = _us * (SystemCoreClock / 1000000);

    told = SysTick->VAL;                        /* 记下起点 */

    while (1)
    {
        tnow = SysTick->VAL;                    /* 反复刷新当前值 */

        if (tnow != told)
        {
            /* VAL 是递减的，正常情形 tnow < told；
             * 若 tnow > told 说明中途回绕过一次，得把 LOAD 补上 */
            if (tnow < told)
                tcnt += told - tnow;
            else
                tcnt += SysTick->LOAD - tnow + told;

            told = tnow;

            if (tcnt >= ticks)                  /* 累计够时钟数就退出 */
                break;
        }
    }
}

/**
 * @brief  毫秒级延时（内部就是 delay_us 放大 1000 倍）
 * @param  ms 要延时的毫秒数
 */
void delay_1ms(uint32_t ms) { delay_us(ms * 1000); }

/**
 * @brief  微秒级延时，等价于 delay_us()
 * @note   保留这个别名是为了和 delay_1ms() 成对使用，语义更清楚
 * @param  us 要延时的微秒数
 */
void delay_1us(uint32_t us) { delay_us(us); }
