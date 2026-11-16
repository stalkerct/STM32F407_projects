/**
 * @file    bsp_tim.c
 * @brief   TIM3 定时器模块：1ms 硬件时基 + 可配置的周期回调
 */
#include "bsp_tim.h"

/*
 * 硬件说明：
 *   TIM3 挂在 APB1 总线上。以系统主频 168MHz 为例，APB1 分频后 PCLK1 = 42MHz，
 *   因为 APB1 预分频系数不为 1，定时器时钟要再 x2，即 84MHz。
 *   实际值由 bsp_tim_get_clock() 在运行时算出来，不是写死的。
 */
#define TIM_MODULE_CLK          RCC_APB1Periph_TIM3
#define TIM_MODULE              TIM3
#define TIM_MODULE_IRQ          TIM3_IRQn
#define TIM_MODULE_IRQ_PRE      0       /* 抢占优先级 = 0 */
#define TIM_MODULE_IRQ_SUB      2       /* 子优先级 = 2（数值越大优先级越低） */

/* TIM3 的 ARR 是 16 位寄存器，一个周期最多能数 65536 下 */
#define TIM_ARR_MAX             65536U

/* 下面几个变量都会被中断和主循环同时访问，所以都加 volatile。
 * 其中 tim_callback 是主循环写、中断读，属于同一类共享变量，同样需要 volatile */
static volatile uint32_t tim_period_ms  = 1;    /* 回调周期(ms) */
static volatile uint32_t tim_tick_ms    = 0;    /* 开机以来的毫秒计数，每 1ms 加 1 */
static volatile uint32_t tim_elapsed_ms = 0;    /* 距离上次回调已经过了多少 ms */
static volatile bsp_tim_callback_t tim_callback = 0;  /* 周期回调函数，0 表示未注册 */

/**
 * @brief  算出当前 TIM3 的时钟频率
 * @note   APB1 预分频系数为 1 时定时器时钟等于 PCLK1，
 *         否则定时器时钟是 PCLK1 的 2 倍。
 * @retval TIM3 的时钟频率(Hz)
 */
static uint32_t bsp_tim_get_clock(void)
{
    RCC_ClocksTypeDef clocks;
    uint32_t ppre1;

    RCC_GetClocksFreq(&clocks);

    /* RCC_CFGR 的 PPRE1 位域取值 0~3 表示不分频，4~7 依次表示 2/4/8/16 分频。
     * 该位域位于 bit[12:10]，所以右移 10 位 */
    ppre1 = (RCC->CFGR & RCC_CFGR_PPRE1) >> 10;

    if (ppre1 < 4U)
    {
        return clocks.PCLK1_Frequency;
    }

    return clocks.PCLK1_Frequency * 2U;
}

/**
 * @brief  初始化 TIM3 并启动，period_ms 是回调周期
 * @note   定时器硬件固定 1ms 中断一次，回调周期由软件分频得到
 *         （中断里数够 period_ms 次才调一次回调）。
 *         硬件之所以固定 1ms，是为了让 bsp_tim_get_tick() 有真正的
 *         毫秒分辨率 —— bsp_key_scan() 的消抖就靠它，如果硬件中断
 *         周期比消抖时间还长，按键会明显迟滞。
 * @note   1ms 对应多少个计数，由 预分频(PSC) 和 重装载值(ARR) 凑出来，
 *         本函数会自动挑一个合适的 PSC，保证 ARR 不超过 16 位。
 * @note   本函数返回时定时器已经在跑了，但回调可能还没注册；
 *         中断里有 if (tim_callback != 0) 保护，所以不会跑飞。
 * @param  period_ms 回调周期，单位 ms，传 0 按 1ms 处理
 */
void bsp_tim_init(uint32_t period_ms)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    uint32_t tim_clk;
    uint32_t total;     /* 1ms 一共要数多少下 */
    uint32_t psc;
    uint32_t arr;

    if (period_ms == 0U)
    {
        period_ms = 1U;
    }

    tim_clk = bsp_tim_get_clock();

    /* 硬件固定 1ms 中断一次，先算出 1ms 对应多少个计数 */
    total = tim_clk / 1000U;

    /* ARR 装不下就加大预分频，直到 1ms 的计数能放进 16 位。
     * 注意 arr 这里用的是整数除法，会截断：主频不能整除时会带来
     * 最多 PSC+1 个定时器时钟周期的误差。以 84MHz/1ms(84000 计数) 为例
     * 是整除的，没有误差 */
    psc = (uint32_t)((total + TIM_ARR_MAX - 1U) / TIM_ARR_MAX) - 1U;
    arr = (uint32_t)(total / (psc + 1U)) - 1U;

    tim_period_ms  = period_ms;
    tim_elapsed_ms = 0;

    RCC_APB1PeriphClockCmd(TIM_MODULE_CLK, ENABLE);

    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
    TIM_TimeBaseStructure.TIM_Prescaler         = (uint16_t)psc;
    TIM_TimeBaseStructure.TIM_CounterMode       = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_Period            = arr;
    TIM_TimeBaseStructure.TIM_ClockDivision     = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM_MODULE, &TIM_TimeBaseStructure);

    TIM_ClearFlag(TIM_MODULE, TIM_FLAG_Update);
    TIM_ITConfig(TIM_MODULE, TIM_IT_Update, ENABLE);

    /* 注意：本工程没有调用过 NVIC_PriorityGroupConfig()，
     * 优先级分组沿用复位默认值（4 位全部给抢占优先级、子优先级位数为 0），
     * 因此下面的 IPRE/IPSUB 组合实际等价于「抢占优先级 0」，
     * IPSUB 那个 2 暂时不会生效。需要真正的优先级区分请先在 board_init() 里配置分组 */
    NVIC_InitStructure.NVIC_IRQChannel                   = TIM_MODULE_IRQ;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = TIM_MODULE_IRQ_PRE;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = TIM_MODULE_IRQ_SUB;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* 注意时序：这一句执行完定时器就开始产生中断了，
     * 而此时回调可能还是空指针（由调用方随后通过 bsp_tim_set_callback() 注册）。
     * 中断里有 if (tim_callback != 0) 保护，所以中间这段窗口是安全的 */
    TIM_Cmd(TIM_MODULE, ENABLE);
}

/**
 * @brief  注册周期回调函数
 * @note   回调在中断上下文里执行，里面不要放耗时或不可重入的代码
 *         （printf、delay_xxx、动态内存分配都不行）。
 * @note   tim_callback 是 32 位变量且地址对齐，赋值是单条存储指令，
 *         主循环里调用本函数不需要关中断保护。
 * @param  callback 回调函数指针，传 0 表示取消回调
 */
void bsp_tim_set_callback(bsp_tim_callback_t callback)
{
    tim_callback = callback;
}

/**
 * @brief  启动定时器
 */
void bsp_tim_start(void)
{
    TIM_Cmd(TIM_MODULE, ENABLE);
}

/**
 * @brief  停止定时器
 * @note   停了之后 bsp_tim_get_tick() 的毫秒计数也一起停住，
 *         依赖它的功能（比如 bsp_key_scan 的消抖）会跟着失效，
 *         一般只在确实不需要计时的时候才调用。
 */
void bsp_tim_stop(void)
{
    TIM_Cmd(TIM_MODULE, DISABLE);
}

/**
 * @brief  读取开机以来的毫秒数
 * @note   32 位无符号，约 49.7 天溢出回绕一次；
 *         做时间差时用无符号相减即可，溢出后依然正确。
 * @retval 毫秒计数
 */
uint32_t bsp_tim_get_tick(void)
{
    return tim_tick_ms;
}

/**
 * @brief  读取当前的回调周期
 * @retval 回调周期，单位 ms
 */
uint32_t bsp_tim_get_period(void)
{
    return tim_period_ms;
}

/**
 * @brief  TIM3 中断服务函数，硬件每 1ms 进来一次
 * @note   毫秒计数每 1ms 加 1，回调则按 tim_period_ms 软件分频之后才执行。
 * @note   本函数的实现靠符号名替换启动文件向量表里的弱符号，
 *         所以函数名和签名都不能改，也不能加 static。
 */
void TIM3_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM_MODULE, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM_MODULE, TIM_IT_Update);   /* 先清标志，避免退出后立刻重入 */

        tim_tick_ms++;                          /* 硬件固定 1ms 一次，直接加 1 */

        tim_elapsed_ms++;

        if (tim_elapsed_ms >= tim_period_ms)    /* 数够一个周期才回调 */
        {
            tim_elapsed_ms = 0;

            if (tim_callback != 0)
            {
                tim_callback();
            }
        }
    }
}
