/**
  ******************************************************************************
  * @file    gpio_ctrl.c
  * @brief   GPIO 初始化与控制 - 中药机硬件引脚 (寄存器直接操作版)
  ******************************************************************************
  */

#include "gpio_ctrl.h"

/**
  * @brief  初始化所有 GPIO
  */
void GPIO_Init_All(void)
{
    /* 使能各端口时钟 */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN |
                    RCC_APB2ENR_IOPCEN | RCC_APB2ENR_IOPDEN |
                    RCC_APB2ENR_IOPEEN | RCC_APB2ENR_AFIOEN;

    /* 禁用 JTAG，保留 SWD，释放 PA15/PB3/PB4 */
    AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_JTAGDISABLE;

    /* 辅助宏: 配置 GPIO 模式 (CNF[1:0] MODE[1:0])
       输入浮空:  0100 (0x4)
       推挽输出50MHz: 0011 (0x3)
    */

    /* ================================================================
       RS485 方向控制引脚 (输出)
       ================================================================ */
    /* PD3(/RE), PD4(DE) -> 推挽输出 */
    GPIOD->CRL &= ~(GPIO_CRL_CNF3 | GPIO_CRL_MODE3 |
                     GPIO_CRL_CNF4 | GPIO_CRL_MODE4);
    GPIOD->CRL |= (0x3 << (3 * 4)) | (0x3 << (4 * 4));  /* 推挽输出 50MHz */
    GPIOD->BRR = GPIO_BRR_BR3;  /* /RE = 0 (接收) */
    GPIOD->BRR = GPIO_BRR_BR4;  /* DE  = 0 */

    /* PE14(DE), PE15(/RE) -> 推挽输出 */
    GPIOE->CRH &= ~(GPIO_CRH_CNF14 | GPIO_CRH_MODE14 |
                     GPIO_CRH_CNF15 | GPIO_CRH_MODE15);
    GPIOE->CRH |= (0x3 << ((14 - 8) * 4)) | (0x3 << ((15 - 8) * 4));
    GPIOE->BRR = GPIO_BRR_BR14;
    GPIOE->BRR = GPIO_BRR_BR15;

    /* ================================================================
       传感器输入引脚 (浮空输入)
       ================================================================ */
    /* PD0(低), PD6(中), PD12(高) -> 浮空输入 */
    GPIOD->CRL &= ~(GPIO_CRL_CNF0 | GPIO_CRL_MODE0 |
                     GPIO_CRL_CNF6 | GPIO_CRL_MODE6);
    GPIOD->CRL |= (0x4 << (0 * 4)) | (0x4 << (6 * 4));

    GPIOD->CRH &= ~(GPIO_CRH_CNF12 | GPIO_CRH_MODE12);
    GPIOD->CRH |= (0x4 << ((12 - 8) * 4));

    /* PC3(入药), PC4(门), PC12, PC13 -> 浮空输入 */
    GPIOC->CRL &= ~(GPIO_CRL_CNF3 | GPIO_CRL_MODE3 |
                     GPIO_CRL_CNF4 | GPIO_CRL_MODE4);
    GPIOC->CRL |= (0x4 << (3 * 4)) | (0x4 << (4 * 4));

    GPIOC->CRH &= ~(GPIO_CRH_CNF12 | GPIO_CRH_MODE12 |
                     GPIO_CRH_CNF13 | GPIO_CRH_MODE13);
    GPIOC->CRH |= (0x4 << ((12 - 8) * 4)) | (0x4 << ((13 - 8) * 4));

    /* ================================================================
       电机控制输出引脚 (推挽输出)
       ================================================================ */
    /* DC1: PA8, PA9 */
    GPIOA->CRH &= ~(GPIO_CRH_CNF8 | GPIO_CRH_MODE8 |
                     GPIO_CRH_CNF9 | GPIO_CRH_MODE9);
    GPIOA->CRH |= (0x3 << ((8 - 8) * 4)) | (0x3 << ((9 - 8) * 4));
    GPIOA->BRR = GPIO_BRR_BR8 | GPIO_BRR_BR9;

    /* DC2: PC6, PC7 */
    GPIOC->CRL &= ~(GPIO_CRL_CNF6 | GPIO_CRL_MODE6 |
                     GPIO_CRL_CNF7 | GPIO_CRL_MODE7);
    GPIOC->CRL |= (0x3 << (6 * 4)) | (0x3 << (7 * 4));
    GPIOC->BRR = GPIO_BRR_BR6 | GPIO_BRR_BR7;

    /* DC3: PB0, PB1 */
    GPIOB->CRL &= ~(GPIO_CRL_CNF0 | GPIO_CRL_MODE0 |
                     GPIO_CRL_CNF1 | GPIO_CRL_MODE1);
    GPIOB->CRL |= (0x3 << (0 * 4)) | (0x3 << (1 * 4));
    GPIOB->BRR = GPIO_BRR_BR0 | GPIO_BRR_BR1;

    /* DC4: PD8, PD9 -- 与旧版一致 */
    GPIOD->CRH &= ~(GPIO_CRH_CNF8 | GPIO_CRH_MODE8 |
                     GPIO_CRH_CNF9 | GPIO_CRH_MODE9);
    GPIOD->CRH |= (0x3 << ((8 - 8) * 4)) | (0x3 << ((9 - 8) * 4));
    GPIOD->BRR = GPIO_BRR_BR8 | GPIO_BRR_BR9;

    /* 小绞龙: PA0(SV=TIM5_CH1, 复用推挽), PA10(EN) */
    GPIOA->CRL &= ~(GPIO_CRL_CNF0 | GPIO_CRL_MODE0);
    GPIOA->CRL |= (0xB << (0 * 4));  /* PA0: AF push-pull 50MHz */
    GPIOA->CRH &= ~(GPIO_CRH_CNF10 | GPIO_CRH_MODE10);
    GPIOA->CRH |= (0x3 << ((10 - 8) * 4));  /* PA10: 推挽输出 50MHz */
    GPIOA->BRR = GPIO_BRR_BR0 | GPIO_BRR_BR10;

    /* 小绞龙: PD1(F/R), PD5(BRK) */
    GPIOD->CRL &= ~(GPIO_CRL_CNF1 | GPIO_CRL_MODE1 |
                     GPIO_CRL_CNF5 | GPIO_CRL_MODE5);
    GPIOD->CRL |= (0x3 << (1 * 4)) | (0x3 << (5 * 4));
    GPIOD->BRR = GPIO_BRR_BR1 | GPIO_BRR_BR5;

    /* 大绞龙: PA1(SV=TIM2_CH2, 复用推挽), PA3(EN) */
    GPIOA->CRL &= ~(GPIO_CRL_CNF1 | GPIO_CRL_MODE1 |
                     GPIO_CRL_CNF3 | GPIO_CRL_MODE3);
    GPIOA->CRL |= (0xB << (1 * 4)) | (0x3 << (3 * 4));  /* PA1: AF push-pull 50MHz */
    GPIOA->BRR = GPIO_BRR_BR1 | GPIO_BRR_BR3;

    /* 大绞龙: PB6(BRK), PD2(F/R) */
    GPIOB->CRL &= ~(GPIO_CRL_CNF6 | GPIO_CRL_MODE6);
    GPIOB->CRL |= (0x3 << (6 * 4));
    GPIOB->BRR = GPIO_BRR_BR6;
    GPIOB->BSRR = GPIO_BSRR_BS6;  /* BRK 默认置位 (刹车) */
    GPIOD->CRL &= ~(GPIO_CRL_CNF2 | GPIO_CRL_MODE2);
    GPIOD->CRL |= (0x3 << (2 * 4));  /* PD2: 推挽输出 50MHz */
    GPIOD->BRR = GPIO_BRR_BR2;

    /* ================================================================
       LED 输出
       ================================================================ */
    /* PE10, PE11 -> 推挽输出 */
    GPIOE->CRH &= ~(GPIO_CRH_CNF10 | GPIO_CRH_MODE10 |
                     GPIO_CRH_CNF11 | GPIO_CRH_MODE11);
    GPIOE->CRH |= (0x3 << ((10 - 8) * 4)) | (0x3 << ((11 - 8) * 4));
    GPIOE->BSRR = GPIO_BSRR_BS10 | GPIO_BSRR_BS11;  /* 默认灭 */
}

/**
  * @brief  设置 RS485 方向
  * @param  uart_idx: UART_IDX_USART3 或 UART_IDX_UART4
  * @param  tx_mode: 1=发送, 0=接收
  */
void RS485_SetDirection(uint8_t uart_idx, uint8_t tx_mode)
{
    if (uart_idx == UART_IDX_UART4) {
        if (tx_mode) {
            GPIOD->BSRR = GPIO_BSRR_BS4;  /* DE=1 */
            GPIOD->BSRR = GPIO_BSRR_BS3;  /* /RE=1 */
        } else {
            GPIOD->BRR = GPIO_BRR_BR4;    /* DE=0 */
            GPIOD->BRR = GPIO_BRR_BR3;    /* /RE=0 */
        }
    } else {
        if (tx_mode) {
            GPIOE->BSRR = GPIO_BSRR_BS14; /* DE=1 */
            GPIOE->BSRR = GPIO_BSRR_BS15; /* /RE=1 */
        } else {
            GPIOE->BRR = GPIO_BRR_BR14;   /* DE=0 */
            GPIOE->BRR = GPIO_BRR_BR15;   /* /RE=0 */
        }
    }
}

/* ================================================================
   传感器读取函数
   ================================================================ */
uint8_t SENSOR_ReadLow(void)
{
    return (GPIOD->IDR & GPIO_IDR_IDR0) ? 1 : 0;
}

uint8_t SENSOR_ReadMid(void)
{
    return (GPIOD->IDR & GPIO_IDR_IDR6) ? 1 : 0;
}

uint8_t SENSOR_ReadHigh(void)
{
    return (GPIOD->IDR & GPIO_IDR_IDR12) ? 1 : 0;
}

uint8_t SENSOR_ReadInput(void)
{
    return (GPIOC->IDR & GPIO_IDR_IDR3) ? 1 : 0;
}

uint8_t SENSOR_ReadDoor(void)
{
    return (GPIOC->IDR & GPIO_IDR_IDR4) ? 1 : 0;
}

/* ================================================================
   电机控制函数
   ================================================================ */
void MOTOR_DC1_Set(uint8_t in1, uint8_t in2)
{
    if (in1) GPIOA->BSRR = GPIO_BSRR_BS8;  else GPIOA->BRR = GPIO_BRR_BR8;
    if (in2) GPIOA->BSRR = GPIO_BSRR_BS9;  else GPIOA->BRR = GPIO_BRR_BR9;
}

void MOTOR_DC2_Set(uint8_t in1, uint8_t in2)
{
    if (in1) GPIOC->BSRR = GPIO_BSRR_BS6;  else GPIOC->BRR = GPIO_BRR_BR6;
    if (in2) GPIOC->BSRR = GPIO_BSRR_BS7;  else GPIOC->BRR = GPIO_BRR_BR7;
}

void MOTOR_DC3_Set(uint8_t in1, uint8_t in2)
{
    if (in1) GPIOB->BSRR = GPIO_BSRR_BS0;  else GPIOB->BRR = GPIO_BRR_BR0;
    if (in2) GPIOB->BSRR = GPIO_BSRR_BS1;  else GPIOB->BRR = GPIO_BRR_BR1;
}

void MOTOR_DC4_Set(uint8_t in1, uint8_t in2)
{
    if (in1) GPIOD->BSRR = GPIO_BSRR_BS8;  else GPIOD->BRR = GPIO_BRR_BR8;
    if (in2) GPIOD->BSRR = GPIO_BSRR_BS9;  else GPIOD->BRR = GPIO_BRR_BR9;
}

/* ================================================================
   绞龙控制函数
   ================================================================ */
void AUGER_S_Set(uint8_t en, uint8_t fr, uint8_t brk)
{
    if (en)  GPIOA->BSRR = GPIO_BSRR_BS10; else GPIOA->BRR = GPIO_BRR_BR10;
    if (fr)  GPIOD->BSRR = GPIO_BSRR_BS1;  else GPIOD->BRR = GPIO_BRR_BR1;
    if (brk) GPIOD->BSRR = GPIO_BSRR_BS5;  else GPIOD->BRR = GPIO_BRR_BR5;
}

void AUGER_B_Set(uint8_t en, uint8_t fr, uint8_t brk)
{
    if (en)  GPIOA->BSRR = GPIO_BSRR_BS3;  else GPIOA->BRR = GPIO_BRR_BR3;
    if (fr)  GPIOD->BSRR = GPIO_BSRR_BS2;  else GPIOD->BRR = GPIO_BRR_BR2;
    if (brk) GPIOB->BSRR = GPIO_BSRR_BS6;  else GPIOB->BRR = GPIO_BRR_BR6;
}

/* ================================================================
   LED 控制 (低电平点亮)
   ================================================================ */
void LED_Set(uint8_t led_idx, uint8_t state)
{
    uint16_t pin = (led_idx == 0) ? GPIO_BSRR_BS10 : GPIO_BSRR_BS11;
    uint16_t pin_r = (led_idx == 0) ? GPIO_BRR_BR10 : GPIO_BRR_BR11;
    if (state) GPIOE->BRR = pin_r;   /* 点亮 */
    else       GPIOE->BSRR = pin;     /* 熄灭 */
}
