#include "sys.h"
#include "usart.h"
#if SYSTEM_SUPPORT_OS
#include "includes.h"
#endif

/*
 * USART1 驱动
 *
 * 该文件承担两个任务：
 * 1. 通过 printf 重定向，把调试信息和遥测数据送到串口
 * 2. 通过接收中断把上位机命令组装成完整字符串
 *
 * 本工程串口通信采用“命令 CRLF 结束”的文本协议，因此接收状态机只需要
 * 判断是否已经依次收到 '\r' 和 '\n'，即可将一帧命令交给主循环处理。
 */

#if defined(__GNUC__) && !defined(__CC_ARM) && !(__ARMCC_VERSION >= 6000000)
/* GCC 下通过重写 _write() 实现 printf -> USART1。 */
int _write(int fd, char *ptr, int len)
{
	(void)fd;
	for(int i = 0; i < len; i++)
	{
		while((USART1->SR & 0X40) == 0);
		USART1->DR = (u8)ptr[i];
	}
	return len;
}
#else
/* AC6 / AC5 编译器下关闭半主机并重定向 fputc。 */
#if __ARMCC_VERSION >= 6000000
__asm(".global __use_no_semihosting");
#elif defined(__CC_ARM)
#pragma import(__use_no_semihosting)
struct __FILE
{
	int handle;
};
#endif

FILE __stdout;
void _sys_exit(int x)
{
	x = x;
}
void _ttywrch(int ch)
{
	ch = ch;
}
int fputc(int ch, FILE *f)
{
	while((USART1->SR&0X40)==0);
	USART1->DR = (u8) ch;
	return ch;
}
#endif

#if EN_USART1_RX
u8 USART_RX_BUF[USART_REC_LEN];
u16 USART_RX_STA=0;

/*
 * USART1 初始化
 *
 * 接口配置为：
 * - PA9  -> USART1_TX
 * - PA10 -> USART1_RX
 * - 8 数据位，1 停止位，无校验
 * - 开启 RXNE 接收中断
 */
void uart_init(u32 bound){
  GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);

	GPIO_PinAFConfig(GPIOA,GPIO_PinSource9,GPIO_AF_USART1);
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource10,GPIO_AF_USART1);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOA,&GPIO_InitStructure);

	USART_InitStructure.USART_BaudRate = bound;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  USART_Init(USART1, &USART_InitStructure);

  USART_Cmd(USART1, ENABLE);

#if EN_USART1_RX
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =3;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

#endif

}


/*
 * USART1 接收中断
 *
 * 接收状态机规则：
 * 1. 普通字符先进入 USART_RX_BUF
 * 2. 收到 '\r' 后，把 bit14 置 1，表示正在等待 '\n'
 * 3. 若下一字节是 '\n'，则把 bit15 置 1，表示一帧接收完成
 * 4. 如果 '\r' 之后不是 '\n'，认为这一帧格式错误，直接清零重收
 *
 * 这样设计的好处是中断里只做轻量级收字节，不做字符串解析，主循环再去
 * 调用 parse_command() 即可，结构清晰，也更安全。
 */
void USART1_IRQHandler(void)
{
	u8 Res;
#if SYSTEM_SUPPORT_OS
	OSIntEnter();
#endif
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		Res =USART_ReceiveData(USART1);

		if((USART_RX_STA&0x8000)==0)
		{
			if(USART_RX_STA&0x4000)
			{
				if(Res!=0x0a)USART_RX_STA=0;
				else USART_RX_STA|=0x8000;
			}
			else
				{
					if(Res==0x0d)USART_RX_STA|=0x4000;
					else
					{
						/* 低 14 位作为接收长度计数器。 */
						USART_RX_BUF[USART_RX_STA&0X3FFF]=Res ;
						USART_RX_STA++;
						if(USART_RX_STA>(USART_REC_LEN-1))USART_RX_STA=0;
					}
				}
		}
  }
#if SYSTEM_SUPPORT_OS
	OSIntExit();
#endif
}
#endif
