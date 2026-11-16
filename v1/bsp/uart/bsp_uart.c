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
 
#include "bsp_uart.h" 
#include "stdio.h"

void uart1_init(uint32_t __Baud)
{
	GPIO_InitTypeDef GPIO_InitStructure;	

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);	

	GPIO_PinAFConfig(GPIOA,GPIO_PinSource9,GPIO_AF_USART1);   /* PA9 复用为 USART1_TX */
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource10,GPIO_AF_USART1);  /* PA10 复用为 USART1_RX */

	GPIO_StructInit(&GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin           = GPIO_Pin_9;   /* TX */
	GPIO_InitStructure.GPIO_Mode          = GPIO_Mode_AF;   /* 复用模式：引脚控制权交给 USART1 外设 */
	GPIO_InitStructure.GPIO_Speed         = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType         = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd          = GPIO_PuPd_UP;
	GPIO_Init(GPIOA,&GPIO_InitStructure);

	GPIO_StructInit(&GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin           = GPIO_Pin_10;  /* RX */
	GPIO_InitStructure.GPIO_Mode          = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed         = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType         = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd          = GPIO_PuPd_UP;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
  
	USART_InitTypeDef USART_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE); /* 使能 USART1 时钟（USART1 挂在 APB2 上） */

	USART_DeInit(USART1);   /* 把所有 USART1 寄存器恢复为复位默认值 */

	USART_StructInit(&USART_InitStructure);
	USART_InitStructure.USART_BaudRate              = __Baud;   /* 波特率由调用者传入 */
	USART_InitStructure.USART_WordLength            = USART_WordLength_8b;   /* 8 位数据 */
	USART_InitStructure.USART_StopBits              = USART_StopBits_1;   /* 1 位停止位 */
	USART_InitStructure.USART_Parity                = USART_Parity_No ;   /* 无校验 */
	USART_InitStructure.USART_Mode                  = USART_Mode_Rx | USART_Mode_Tx; /* 收发都使能；printf 只用到 Tx */
	USART_InitStructure.USART_HardwareFlowControl   = USART_HardwareFlowControl_None; /* 无硬件流控，与 USB-TTL / 串口助手对接的通用配置 */
	USART_Init(USART1,&USART_InitStructure);   /* 上面三行合起来即 8N1 格式 */
	
	USART_ClearFlag(USART1,USART_FLAG_RXNE);   /* 清掉上电残留的 RXNE，防止一开中断就误触发 */

	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE); /* 使能 RXNE（接收）中断。注意本工程只接收不处理，见 ISR 里的说明 */

	USART_Cmd(USART1,ENABLE);   /* 使能 USART1，到这一步 printf 才真正可用 */
	
	NVIC_InitTypeDef NVIC_InitStructure;

	NVIC_InitStructure.NVIC_IRQChannel                    = USART1_IRQn; /* USART1 中断通道 */
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority  = 0;   /* 抢占优先级 0 */
	NVIC_InitStructure.NVIC_IRQChannelSubPriority         = 1;   /* 子优先级 1。本工程未调用 NVIC_PriorityGroupConfig()，分组沿用复位默认值（子优先级位数为 0），此项实际不生效 */
	NVIC_InitStructure.NVIC_IRQChannelCmd                 = ENABLE; /* 使能该中断通道 */
	NVIC_Init(&NVIC_InitStructure);
		
}

#if !defined(__MICROLIB)
/* ==== printf 重定向：绕开半主机（semihosting） ====
 * 如果链接时找不到 _sys_exit / __stdout，C 库会去调用调试器代管的半主机接口，
 * 表现就是「接着调试器能跑，脱机烧录后卡死」。下面这些定义就是为了绕开它。
 * 使用 MicroLIB 时这些已由库提供，所以整体用 #if 排除掉。 */
#if (__ARMCLIB_VERSION <= 6000000)
/* AC5（即 __ARMCLIB_VERSION 较小）下 __FILE 结构体不是现成的，需要自己定义 */
struct __FILE
{
	int handle;
};
#endif

FILE __stdout;

/* _sys_exit 只是为了让链接器满意，避免引入半主机依赖；x = x 是为了消掉未使用参数警告 */
void _sys_exit(int x)
{
	x = x;
}
#endif

/* 把 C 库的 printf 重定向到 USART1：库每输出一个字符都会调用一次 fputc */
int fputc(int ch, FILE *f)
{
    USART_SendData(USART1, (uint8_t)ch);
	
	while( RESET == USART_GetFlagStatus(USART1, USART_FLAG_TXE) ){}
	
    return ch;
}



/* ==== 串口1 中断服务函数 ====
 * 目前只接收、不处理，收到的字节直接丢弃；保留接收中断是为了以后扩展命令解析。
 * 关于清标志：F4 的 RXNE 有两种清法 —— 读 USART_DR，或往 SR 的对应位写 0。
 * 本库的 USART_ClearITPendingBit() 走的是后者（写 SR），
 * 所以 DR 里的数据并没有被读走，也就等于被丢弃了。
 * 如果既不读 DR 也不清标志，退出中断后会立刻再次进入，形成死循环。 */
void USART1_IRQHandler(void)
{
	if(USART_GetITStatus(USART1, USART_IT_RXNE) == SET) /* 确认是 RXNE（接收数据寄存器非空）触发的 */
	{
		/* 回显测试：收到什么就发回什么。需要用的时候把下面这一行取消注释 */
		//USART_SendData(USART1,USART_ReceiveData(USART1));
		
		
		USART_ClearITPendingBit(USART1, USART_IT_RXNE); /* 清标志位 */
	}  
}


