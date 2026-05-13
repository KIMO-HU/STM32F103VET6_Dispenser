/**
  ******************************************************************************
  * @file    system_clock.c
  * @brief   系统时钟配置 - HSE 8MHz -> SYSCLK 72MHz
  ******************************************************************************
  */

#include "system_clock.h"

volatile uint32_t sysTickCount = 0;

/**
  * @brief  系统时钟配置
  *         HSE = 8MHz -> PLL x9 -> SYSCLK = 72MHz
  *         AHB = 72MHz, APB1 = 36MHz, APB2 = 72MHz
  */
void SystemClock_Config(void)
{
    /* 1. 使能 HSE 并等待就绪 */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));

    /* 2. 设置 Flash 预取和等待状态 (2 wait states for 72MHz) */
    FLASH->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY_2;

    /* 3. 配置 PLL: HSE (不分频) * 9 = 72MHz */
    RCC->CFGR &= ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL);
    RCC->CFGR |= RCC_CFGR_PLLSRC
              |  RCC_CFGR_PLLXTPRE_HSE
              |  RCC_CFGR_PLLMULL9;

    /* 4. 配置总线分频器 */
    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);
    RCC->CFGR |= RCC_CFGR_HPRE_DIV1     /* AHB = 72MHz */
              |  RCC_CFGR_PPRE1_DIV2    /* APB1 = 36MHz */
              |  RCC_CFGR_PPRE2_DIV1;   /* APB2 = 72MHz */

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
void SysTick_Init(void)
{
    SysTick->LOAD = (SystemCoreClock / 1000) - 1;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}

/**
  * @brief  毫秒延时 (基于 SysTick)
  */
void delay_ms(uint32_t ms)
{
    uint32_t start = sysTickCount;
    while ((sysTickCount - start) < ms);
}

/**
  * @brief  微秒延时 (粗略循环)
  * @note   72MHz 下，约 72 个周期 1us
  */
void delay_us(uint32_t us)
{
    volatile uint32_t count;
    while (us--) {
        count = 12;  /* 根据实际时钟调整 */
        while (count--);
    }
}
