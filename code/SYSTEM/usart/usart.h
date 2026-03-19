#ifndef __USART_H
#define __USART_H
#include "stdio.h"
#include "stm32f4xx_conf.h"
#include "sys.h"

/*
 * USART1 接收缓存长度
 *
 * 本工程采用文本协议，单条命令很短，200 字节已足够覆盖：
 * TEMP:xx.xx / START / LIMIT:xx.xx / GET 等控制命令。
 */
#define USART_REC_LEN 200

/* 是否使能 USART1 接收中断。 */
#define EN_USART1_RX 1

/*
 * 接收缓冲区与接收状态字说明
 *
 * USART_RX_STA 各位含义：
 * - bit15：1 表示已经收到完整一帧
 * - bit14：1 表示已经收到 '\r'
 * - bit13~0：表示当前缓冲区中已接收的有效字节数
 */
extern u8 USART_RX_BUF[USART_REC_LEN];
extern u16 USART_RX_STA;

/* 初始化 USART1，配置为 8N1，并打开接收中断。 */
void uart_init(u32 bound);
#endif


