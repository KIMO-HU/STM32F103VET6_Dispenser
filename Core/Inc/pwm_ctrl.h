/**
  ******************************************************************************
  * @file    pwm_ctrl.h
  * @brief   PWM 调速控制 - TIM2_CH2 (PA1=大绞龙SV) + TIM5_CH1 (PA0=小绞龙SV)
  ******************************************************************************
  */

#ifndef __PWM_CTRL_H
#define __PWM_CTRL_H

#include "stm32f103xe.h"
#include <stdint.h>

/* ================================================================
   PWM 参数定义
   ================================================================ */
#define PWM_FREQ_HZ             1000u       /* PWM 频率 1kHz */
#define PWM_PERIOD              4095u       /* ARR 值, 对应 12bit 精度 (0x0FFF) */
#define PWM_PRESCALER           8u          /* 36MHz / 9 = 4MHz -> 4MHz/4096 = ~976Hz */

/* ================================================================
   通道选择
   ================================================================ */
typedef enum {
    PWM_CH_SMALL_AUGER = 0,     /* TIM5_CH1 -> PA0 小绞龙 SV */
    PWM_CH_BIG_AUGER   = 1,     /* TIM2_CH2 -> PA1 大绞龙 SV */
} pwm_channel_t;

/* ================================================================
   接口函数
   ================================================================ */

/**
  * @brief  初始化 TIM2 和 TIM5 的 PWM 输出
  * @note   PA0(TIM5_CH1) 和 PA1(TIM2_CH2) 需预先配置为复用推挽输出
  */
void PWM_Init(void);

/**
  * @brief  设置指定 PWM 通道的占空比
  * @param  ch: PWM_CH_SMALL_AUGER 或 PWM_CH_BIG_AUGER
  * @param  duty: 占空比值, 范围 0 ~ PWM_PERIOD (0=0%, PWM_PERIOD=100%)
  * @note   直接对应 dispenser 参数中的 speed 值 (0x0000~0x0FFF)
  */
void PWM_SetDuty(pwm_channel_t ch, uint16_t duty);

/**
  * @brief  停止指定 PWM 通道输出 (占空比置 0)
  * @param  ch: PWM_CH_SMALL_AUGER 或 PWM_CH_BIG_AUGER
  */
void PWM_Stop(pwm_channel_t ch);

/**
  * @brief  停止所有 PWM 输出
  */
void PWM_StopAll(void);

#endif /* __PWM_CTRL_H */
