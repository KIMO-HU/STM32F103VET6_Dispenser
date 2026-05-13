/**
  ******************************************************************************
  * @file    usart_ctrl.h
  * @brief   UART/RS485 通信控制头文件
  ******************************************************************************
  */

#ifndef __USART_CTRL_H__
#define __USART_CTRL_H__

#include "stm32f103xe.h"
#include <stdint.h>

/* UART 索引 */
#define UART_IDX_USART3     0
#define UART_IDX_UART4      1

/* 接收缓冲区大小 */
#define UART_RX_BUF_SIZE    256
#define UART_TX_BUF_SIZE    256

/* 接收状态 */
typedef struct {
    uint8_t  rx_buf[UART_RX_BUF_SIZE];
    volatile uint16_t rx_head;
    volatile uint16_t rx_tail;
    volatile uint8_t  rx_flag;
    uint8_t  tx_buf[UART_TX_BUF_SIZE];
    volatile uint16_t tx_head;
    volatile uint16_t tx_tail;
} uart_ctrl_t;

extern uart_ctrl_t g_uart4_ctrl;

/* 函数声明 */
void USART3_Init(uint32_t baudrate);
void UART4_Init(uint32_t baudrate);

void UART_PutChar(uint8_t uart_idx, uint8_t ch);
void UART_PutBuf(uint8_t uart_idx, uint8_t *buf, uint16_t len);
int  UART_GetChar(uint8_t uart_idx, uint8_t *ch);
uint16_t UART_RxCount(uint8_t uart_idx);
void UART_FlushRx(uint8_t uart_idx);

#endif /* __USART_CTRL_H__ */
