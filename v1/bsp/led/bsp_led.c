/**
 * @file    bsp_led.c
 * @brief   板载 4 个 LED 的驱动（LED1~LED4 -> PD3~PD6）
 */
#include "bsp_led.h"

/*
 * 原理图：LED1~LED4 的阳极经 R14~R17(1K) 接 VCC+3.3V，阴极接 PD3~PD6
 * 因此 IO 输出低电平时 LED 点亮，输出高电平时 LED 熄灭
 */
#define LED_GPIO_CLK        RCC_AHB1Periph_GPIOD
#define LED_GPIO_PORT       GPIOD
#define LED_GPIO_PIN_ALL    (GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6)

/* LED 编号(1~4)与 IO 引脚的对应关系，下标为 编号 - 1 */
static const uint16_t led_pin[LED_NUM] =
{
    GPIO_Pin_3,     /* 1 -> LED1 */
    GPIO_Pin_4,     /* 2 -> LED2 */
    GPIO_Pin_5,     /* 3 -> LED3 */
    GPIO_Pin_6      /* 4 -> LED4 */
};

/**
 * @brief  初始化板载 LED 所用的 IO
 * @note   配完 GPIO 会顺手把所有灯熄掉，避免上电时残留亮灯
 */
void bsp_led_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd(LED_GPIO_CLK, ENABLE);

    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin   = LED_GPIO_PIN_ALL;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;      /* 普通 GPIO 输出（非复用） */
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;      /* 推挽 */
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_Init(LED_GPIO_PORT, &GPIO_InitStructure);

    bsp_led_all_off();
}

/**
 * @brief  设置指定 LED 的亮灭
 * @param  led   1 ~ 4，1 对应 LED1；越界直接返回，不做任何动作
 * @param  state LED_ON 点亮，LED_OFF 熄灭
 */
void bsp_led_set(uint8_t led, led_state_t state)
{
    if (led < 1 || led > LED_NUM)
    {
        return;
    }

    /* 硬件是低电平点亮，所以“点亮”对应 ResetBits、“熄灭”对应 SetBits */
    if (state == LED_ON)
    {
        GPIO_ResetBits(LED_GPIO_PORT, led_pin[led - 1]);
    }
    else
    {
        GPIO_SetBits(LED_GPIO_PORT, led_pin[led - 1]);
    }
}

/** @brief 点亮指定 LED
 *  @param led 1 ~ 4，1 对应 LED1 */
void bsp_led_on(uint8_t led) { bsp_led_set(led, LED_ON); }

/** @brief 熄灭指定 LED
 *  @param led 1 ~ 4，1 对应 LED1 */
void bsp_led_off(uint8_t led) { bsp_led_set(led, LED_OFF); }

/** @brief 翻转指定 LED 的亮灭状态
 *  @param led 1 ~ 4，1 对应 LED1 */
void bsp_led_toggle(uint8_t led)
{
    if (led < 1 || led > LED_NUM)
    {
        return;
    }

    GPIO_ToggleBits(LED_GPIO_PORT, led_pin[led - 1]);
}

/** @brief 点亮全部 LED */
void bsp_led_all_on(void)
{
    uint8_t i;

    for (i = 1; i <= LED_NUM; i++)
    {
        bsp_led_on(i);
    }
}

/** @brief 熄灭全部 LED */
void bsp_led_all_off(void)
{
    uint8_t i;

    for (i = 1; i <= LED_NUM; i++)
    {
        bsp_led_off(i);
    }
}

/** @brief 翻转全部 LED（一次性改 4 个引脚，比循环调用效率高） */
void bsp_led_all_toggle(void)
{
    GPIO_ToggleBits(LED_GPIO_PORT, LED_GPIO_PIN_ALL);
}
