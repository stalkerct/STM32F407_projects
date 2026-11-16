/**
 * @file    bsp_key.c
 * @brief   板载 4 个按键的驱动（KEY1~KEY4 -> PD13/PD12/PD11/PD14），带非阻塞消抖
 */
#include "bsp_key.h"
#include "bsp_tim.h"

/*
 * 原理图：SW2~SW5 一端接 PD11~PD14，另一端接 GND，
 * 同时 PD11~PD14 各有 R18~R22(10K) 上拉到 VCC+3.3V
 * 因此 IO 读到低电平表示按键按下，读到高电平表示按键松开
 */
#define KEY_GPIO_CLK        RCC_AHB1Periph_GPIOD
#define KEY_GPIO_PORT       GPIOD
#define KEY_GPIO_PIN_ALL    (GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14)
#define KEY_PRESS_LEVEL     Bit_RESET       /* 按下时读到的电平：低 */

/* 按键编号(1~4)与 IO 引脚的对应关系，下标为 编号 - 1。
 * 注意顺序不是 11/12/13/14，而是按板子丝印 KEY1~KEY4 排的 */
static const uint16_t key_pin[KEY_NUM] =
{
    GPIO_Pin_13,    /* KEY1 -> PD13 */
    GPIO_Pin_12,    /* KEY2 -> PD12 */
    GPIO_Pin_11,    /* KEY3 -> PD11 */
    GPIO_Pin_14     /* KEY4 -> PD14 */
};

/**
 * @brief  初始化板载按键所用的 IO
 * @note   板上已有 10K 外部上拉，这里再开内部上拉只是双保险
 */
void bsp_key_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd(KEY_GPIO_CLK, ENABLE);

    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin   = KEY_GPIO_PIN_ALL;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;   /* 内部上拉，与外部 10K 上拉一致 */
    GPIO_Init(KEY_GPIO_PORT, &GPIO_InitStructure);
}

/**
 * @brief  读取指定按键的当前状态（不含消抖，直接反映引脚电平）
 * @param  key 1 ~ 4，1 对应 PD13；越界返回 0
 * @retval 1 表示按下，0 表示松开（编号越界时同样返回 0）
 */
uint8_t bsp_key_read(uint8_t key)
{
    if (key < 1 || key > KEY_NUM)
    {
        return 0;
    }

    if (GPIO_ReadInputDataBit(KEY_GPIO_PORT, key_pin[key - 1]) == KEY_PRESS_LEVEL)
    {
        return 1;
    }

    return 0;
}

/**
 * @brief  扫描按键（带消抖，同一按键按住期间只在按下瞬间上报一次）
 * @note   非阻塞：内部不做任何等待，每次调用只花几微秒，可以在主循环里反复调用。
 *         消抖不用 delay 空等，而是记住电平变化的时刻，
 *         等它稳定够 KEY_DEBOUNCE_MS 毫秒才认账。
 * @note   时间基准来自定时器模块，所以调用前必须先 bsp_tim_init()，
 *         而且定时器要保持运行（停了毫秒计数就不走了，按键会失灵）。
 * @note   多个键同时按下时，只上报编号最小的那个。
 * @retval 按下的按键编号 1 ~ 4，没有新按键按下时返回 0
 */
uint8_t bsp_key_scan(void)
{
    static uint8_t  key_raw  = 0;   /* 上一次读到的原始键号，没按键时为 0 */
    static uint32_t key_time = 0;   /* key_raw 发生变化的时刻 */
    static uint8_t  key_last = 0;   /* 已经上报过的键号 */

    uint32_t tick = bsp_tim_get_tick();
    uint8_t  now  = 0;
    uint8_t  i;

    /* 读出当前的原始状态，多个键同时按下时取编号最小的那个 */
    for (i = 1; i <= KEY_NUM; i++)
    {
        if (bsp_key_read(i))
        {
            now = i;
            break;
        }
    }

    /* 状态刚发生变化，先记下时刻，等它稳定下来再说 */
    if (now != key_raw)
    {
        key_raw  = now;
        key_time = tick;
        return 0;
    }

    /* 状态没变，但还没稳定够消抖时间。
     * 这里 tick 和 key_time 都是无符号数，即使 tick 溢出回绕，
     * tick - key_time 也依然等于真实经过的毫秒数（模 2^32 运算保证），
     * 所以不需要专门处理约 49.7 天一次的溢出 */
    if ((tick - key_time) < KEY_DEBOUNCE_MS)
    {
        return 0;
    }

    /* 状态已经稳定了：松手就重置上报记录，否则在按下瞬间上报一次 */
    if (now == 0)
    {
        key_last = 0;       /* 松手了，允许同一个键下次再触发 */
        return 0;
    }

    if (now != key_last)    /* 新按下的键，只在按下的瞬间上报一次 */
    {
        key_last = now;
        return now;
    }

    return 0;
}
