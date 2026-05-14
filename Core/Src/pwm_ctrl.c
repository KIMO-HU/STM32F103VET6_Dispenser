/**
  ******************************************************************************
  * @file    pwm_ctrl.c
  * @brief   PWM 调速控制实现 - TIM2_CH2 (PA1) + TIM5_CH1 (PA0)
  * @note    APB1=36MHz, PSC=8 -> 4MHz, ARR=4095 -> ~976Hz PWM
  ******************************************************************************
  */

#include "pwm_ctrl.h"

/* ================================================================
   私有宏定义
   ================================================================ */
#define TIM_CCMR_OCxM_PWM1      0x06u   /* 110: PWM 模式 1 */
#define TIM_CCMR_OCxM_PWM2      0x07u   /* 111: PWM 模式 2 */

/* ================================================================
   本地辅助函数: 初始化 TIM2 CH2 (PA1)
   ================================================================ */
static void TIM2_PWM_Init(void)
{
    /* 1. 使能 TIM2 时钟 */
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    /* 2. 配置 TIM2 时基 */
    TIM2->PSC = PWM_PRESCALER;          /* 预分频: 36MHz/(8+1)=4MHz */
    TIM2->ARR = PWM_PERIOD;             /* 自动重装载值: 4095 */
    TIM2->CNT = 0;                      /* 清零计数器 */

    /* 3. 配置 CH2 为 PWM 模式 1, 预装载使能 */
    /* CCMR1: OC2M[2:0]=110 (PWM1), OC2PE=1 */
    TIM2->CCMR1 &= ~(TIM_CCMR1_OC2M_Msk | TIM_CCMR1_OC2PE);
    TIM2->CCMR1 |= (TIM_CCMR_OCxM_PWM1 << TIM_CCMR1_OC2M_Pos) | TIM_CCMR1_OC2PE;

    /* 4. 配置 CH2 输出极性: 高电平有效, 使能输出 */
    TIM2->CCER |= TIM_CCER_CC2E;

    /* 5. 使能自动重装载预装载 */
    TIM2->CR1 |= TIM_CR1_ARPE;

    /* 6. 产生更新事件, 装载寄存器 */
    TIM2->EGR |= TIM_EGR_UG;

    /* 7. 清除更新标志 */
    TIM2->SR &= ~TIM_SR_UIF;

    /* 8. 初始占空比为 0 */
    TIM2->CCR2 = 0;

    /* 9. 使能计数器 */
    TIM2->CR1 |= TIM_CR1_CEN;
}

/* ================================================================
   本地辅助函数: 初始化 TIM5 CH1 (PA0)
   ================================================================ */
static void TIM5_PWM_Init(void)
{
    /* 1. 使能 TIM5 时钟 */
    RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;

    /* 2. 配置 TIM5 时基 */
    TIM5->PSC = PWM_PRESCALER;          /* 预分频: 36MHz/(8+1)=4MHz */
    TIM5->ARR = PWM_PERIOD;             /* 自动重装载值: 4095 */
    TIM5->CNT = 0;                      /* 清零计数器 */

    /* 3. 配置 CH1 为 PWM 模式 1, 预装载使能 */
    /* CCMR1: OC1M[2:0]=110 (PWM1), OC1PE=1 */
    TIM5->CCMR1 &= ~(TIM_CCMR1_OC1M_Msk | TIM_CCMR1_OC1PE);
    TIM5->CCMR1 |= (TIM_CCMR_OCxM_PWM1 << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE;

    /* 4. 配置 CH1 输出极性: 高电平有效, 使能输出 */
    TIM5->CCER |= TIM_CCER_CC1E;

    /* 5. 使能自动重装载预装载 */
    TIM5->CR1 |= TIM_CR1_ARPE;

    /* 6. 产生更新事件, 装载寄存器 */
    TIM5->EGR |= TIM_EGR_UG;

    /* 7. 清除更新标志 */
    TIM5->SR &= ~TIM_SR_UIF;

    /* 8. 初始占空比为 0 */
    TIM5->CCR1 = 0;

    /* 9. 使能计数器 */
    TIM5->CR1 |= TIM_CR1_CEN;
}

/* ================================================================
   接口函数实现
   ================================================================ */

/**
  * @brief  初始化 TIM2 和 TIM5 的 PWM 输出
  */
void PWM_Init(void)
{
    TIM2_PWM_Init();
    TIM5_PWM_Init();
}

/**
  * @brief  设置指定 PWM 通道的占空比
  * @param  ch: PWM_CH_SMALL_AUGER 或 PWM_CH_BIG_AUGER
  * @param  duty: 占空比值, 范围 0 ~ PWM_PERIOD
  */
void PWM_SetDuty(pwm_channel_t ch, uint16_t duty)
{
    if (duty > PWM_PERIOD) {
        duty = PWM_PERIOD;
    }

    if (ch == PWM_CH_SMALL_AUGER) {
        TIM5->CCR1 = duty;
    } else if (ch == PWM_CH_BIG_AUGER) {
        TIM2->CCR2 = duty;
    }
}

/**
  * @brief  停止指定 PWM 通道输出 (占空比置 0)
  * @param  ch: PWM_CH_SMALL_AUGER 或 PWM_CH_BIG_AUGER
  */
void PWM_Stop(pwm_channel_t ch)
{
    PWM_SetDuty(ch, 0);
}

/**
  * @brief  停止所有 PWM 输出
  */
void PWM_StopAll(void)
{
    TIM5->CCR1 = 0;
    TIM2->CCR2 = 0;
}
