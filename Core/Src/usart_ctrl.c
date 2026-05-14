/**
  ******************************************************************************
  * @file    usart_ctrl.c
  * @brief   UART4/RS485 与 USART3 初始化及通信驱动 (寄存器直接操作版)
  ******************************************************************************
  */

#include "usart_ctrl.h"
#include "gpio_ctrl.h"
#include "system_clock.h"

uart_ctrl_t g_uart4_ctrl = {0};
static uart_ctrl_t g_usart3_ctrl = {0};

/**
  * @brief  USART3 初始化 (PB10=TX, PB11=RX)
  */
void USART3_Init(uint32_t baudrate)
{
    uint32_t apbclock = 36000000;  /* APB1 = 36MHz */
    uint32_t mantissa;
    uint32_t fraction;

    /* 使能时钟 */
    RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;

    /* PB10 TX: 复用推挽输出 50MHz */
    GPIOB->CRH &= ~(GPIO_CRH_CNF10 | GPIO_CRH_MODE10);
    GPIOB->CRH |= (0xB << ((10 - 8) * 4));  /* CNF=10(AF), MODE=11(50MHz) */

    /* PB11 RX: 浮空输入 */
    GPIOB->CRH &= ~(GPIO_CRH_CNF11 | GPIO_CRH_MODE11);
    GPIOB->CRH |= (0x4 << ((11 - 8) * 4));  /* CNF=01, MODE=00 */

    /* 波特率计算 */
    uint32_t div = (apbclock + baudrate / 2) / baudrate;
    mantissa = div / 16;
    fraction = div % 16;

    USART3->BRR = (mantissa << 4) | (fraction & 0x0F);

    /* 8N1, 使能 TX/RX, 使能 RXNE 中断 */
    USART3->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE | USART_CR1_UE;

    /* NVIC: USART3_IRQn = 39 */
    NVIC->ISER[1] = (1 << (39 - 32));
}

/**
  * @brief  UART4 初始化 (PC10=TX, PC11=RX)
  */
void UART4_Init(uint32_t baudrate)
{
    uint32_t apbclock = 36000000;  /* APB1 = 36MHz */
    uint32_t mantissa;
    uint32_t fraction;

    /* 使能时钟 */
    RCC->APB1ENR |= RCC_APB1ENR_UART4EN;
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;

    /* PC10 TX: 复用推挽输出 50MHz */
    GPIOC->CRH &= ~(GPIO_CRH_CNF10 | GPIO_CRH_MODE10);
    GPIOC->CRH |= (0xB << ((10 - 8) * 4));

    /* PC11 RX: 浮空输入 */
    GPIOC->CRH &= ~(GPIO_CRH_CNF11 | GPIO_CRH_MODE11);
    GPIOC->CRH |= (0x4 << ((11 - 8) * 4));

    /* 波特率计算 */
    uint32_t div = (apbclock + baudrate / 2) / baudrate;
    mantissa = div / 16;
    fraction = div % 16;

    UART4->BRR = (mantissa << 4) | (fraction & 0x0F);

    /* 8N1, 使能 TX/RX, 使能 RXNE 中断 */
    UART4->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE | USART_CR1_UE;

    /* NVIC: UART4_IRQn = 52 */
    NVIC->ISER[1] = (1 << (52 - 32));
}

/**
  * @brief  发送单字节
  */
void UART_PutChar(uint8_t uart_idx, uint8_t ch)
{
    USART_TypeDef *USARTx = (uart_idx == UART_IDX_UART4) ? UART4 : USART3;
    while (!(USARTx->SR & USART_SR_TXE));
    USARTx->DR = ch;
}

/**
  * @brief  发送缓冲区
  */
void UART_PutBuf(uint8_t uart_idx, uint8_t *buf, uint16_t len)
{
    RS485_SetDirection(uart_idx, 1);
    delay_us(20);

    for (uint16_t i = 0; i < len; i++) {
        UART_PutChar(uart_idx, buf[i]);
    }

    USART_TypeDef *USARTx = (uart_idx == UART_IDX_UART4) ? UART4 : USART3;
    while (!(USARTx->SR & USART_SR_TC));
    delay_us(80);
    RS485_SetDirection(uart_idx, 0);
}

/**
  * @brief  获取接收缓冲区字节数
  */
uint16_t UART_RxCount(uint8_t uart_idx)
{
    uart_ctrl_t *ctrl = (uart_idx == UART_IDX_UART4) ? &g_uart4_ctrl : &g_usart3_ctrl;
    uint16_t count;
    uint16_t head = ctrl->rx_head;
    uint16_t tail = ctrl->rx_tail;
    if (head >= tail) count = head - tail;
    else count = UART_RX_BUF_SIZE - tail + head;
    return count;
}

/**
  * @brief  读取单字节
  * @retval 0:无数据, 1:成功
  */
int UART_GetChar(uint8_t uart_idx, uint8_t *ch)
{
    uart_ctrl_t *ctrl = (uart_idx == UART_IDX_UART4) ? &g_uart4_ctrl : &g_usart3_ctrl;
    if (ctrl->rx_head == ctrl->rx_tail) return 0;
    *ch = ctrl->rx_buf[ctrl->rx_tail];
    ctrl->rx_tail = (ctrl->rx_tail + 1) % UART_RX_BUF_SIZE;
    return 1;
}

/**
  * @brief  清空接收缓冲区
  */
void UART_FlushRx(uint8_t uart_idx)
{
    uart_ctrl_t *ctrl = (uart_idx == UART_IDX_UART4) ? &g_uart4_ctrl : &g_usart3_ctrl;
    ctrl->rx_head = 0;
    ctrl->rx_tail = 0;
}

/* ================================================================
   中断处理 (定义在 stm32f1xx_it.c 中会被覆盖，此处保留备用)
   ================================================================ */
void USART3_IRQHandler(void)
{
    if (USART3->SR & USART_SR_RXNE) {
        uint8_t ch = (uint8_t)(USART3->DR & 0xFF);
        uint16_t next = (g_usart3_ctrl.rx_head + 1) % UART_RX_BUF_SIZE;
        if (next != g_usart3_ctrl.rx_tail) {
            g_usart3_ctrl.rx_buf[g_usart3_ctrl.rx_head] = ch;
            g_usart3_ctrl.rx_head = next;
        }
    }
}

void UART4_IRQHandler(void)
{
    if (UART4->SR & USART_SR_RXNE) {
        uint8_t ch = (uint8_t)(UART4->DR & 0xFF);
        uint16_t next = (g_uart4_ctrl.rx_head + 1) % UART_RX_BUF_SIZE;
        if (next != g_uart4_ctrl.rx_tail) {
            g_uart4_ctrl.rx_buf[g_uart4_ctrl.rx_head] = ch;
            g_uart4_ctrl.rx_head = next;
        }
    }
}
