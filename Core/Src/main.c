/**
  ******************************************************************************
  * @file    main.c
  * @brief   STM32F103VET6 基础示例程序
  *          功能：LED 闪烁 (PC13 - 大多数开发板的板载LED)
  *          时钟配置：外部晶振 8MHz -> SYSCLK 72MHz
  ******************************************************************************
  */

#include "stm32f103xe.h"

/* 简单延时计数器 (基于 SysTick) */
volatile uint32_t sysTickCount = 0;

void SysTick_Handler(void);

static void delay_ms(uint32_t ms)
{
    uint32_t start = sysTickCount;
    while ((sysTickCount - start) < ms);
}

/**
  * @brief  系统时钟配置
  *         HSE = 8MHz -> PLL x9 -> SYSCLK = 72MHz
  *         AHB = 72MHz, APB1 = 36MHz, APB2 = 72MHz
  */
static void SystemClock_Config(void)
{
    /* 1. 使能 HSE 并等待就绪 */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));

    /* 2. 设置 Flash 预取和等待状态 (2 wait states for 72MHz) */
    FLASH->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY_2;

    /* 3. 配置 PLL: HSE (不分频) * 9 = 72MHz */
    RCC->CFGR &= ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL);
    RCC->CFGR |= RCC_CFGR_PLLSRC        /* HSE 作为 PLL 输入源 */
              |  RCC_CFGR_PLLXTPRE_HSE  /* HSE 不分频 */
              |  RCC_CFGR_PLLMULL9;     /* PLL 倍频 9 */

    /* 4. 配置总线分频器 */
    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);
    RCC->CFGR |= RCC_CFGR_HPRE_DIV1     /* AHB = SYSCLK = 72MHz */
              |  RCC_CFGR_PPRE1_DIV2    /* APB1 = HCLK/2 = 36MHz (max) */
              |  RCC_CFGR_PPRE2_DIV1;   /* APB2 = HCLK = 72MHz */

    /* 5. 使能 PLL 并等待就绪 */
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    /* 6. 切换系统时钟到 PLL */
    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);

    /* 7. 更新 SystemCoreClock 变量 */
    SystemCoreClock = 72000000;
}

/**
  * @brief  配置 SysTick 为 1ms 中断
  */
static void SysTick_Init(void)
{
    /* 配置 SysTick 每 1ms 中断一次 (HCLK = 72MHz) */
    SysTick->LOAD = (SystemCoreClock / 1000) - 1;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}

/**
  * @brief  GPIO 初始化
  *         PC13 - 推挽输出 (LED)
 */
static void GPIO_Init(void)
{
    /* 使能 GPIOC 时钟 */
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;

    /* PC13 配置为通用推挽输出，2MHz */
    GPIOC->CRH &= ~(GPIO_CRH_CNF13 | GPIO_CRH_MODE13);
    GPIOC->CRH |= GPIO_CRH_MODE13_1;  /* 10: Output mode, max speed 2 MHz */
}

/**
  * @brief  主函数
  */
int main(void)
{
    /* 配置系统时钟为 72MHz */
    SystemClock_Config();

    /* 初始化 SysTick (必须在时钟配置之后) */
    SysTick_Init();

    /* 初始化 GPIO */
    GPIO_Init();

    /* 主循环: LED 闪烁，周期 1秒 (亮500ms, 灭500ms) */
    while (1)
    {
        /* LED 亮 (PC13 输出低电平，大多数板子的LED是低电平点亮) */
        GPIOC->BSRR = GPIO_BSRR_BR13;
        delay_ms(500);

        /* LED 灭 (PC13 输出高电平) */
        GPIOC->BSRR = GPIO_BSRR_BS13;
        delay_ms(500);
    }
}

/**
  * @brief  错误处理函数
  */
void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
        /* 错误时快速闪烁 LED */
        GPIOC->ODR ^= GPIO_ODR_ODR13;
        for (volatile int i = 0; i < 20000; i++);
    }
}
