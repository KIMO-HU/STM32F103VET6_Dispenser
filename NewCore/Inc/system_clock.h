/**
  ******************************************************************************
  * @file    system_clock.h
  * @brief   系统时钟配置头文件
  ******************************************************************************
  */

#ifndef __SYSTEM_CLOCK_H__
#define __SYSTEM_CLOCK_H__

#include <stdint.h>
#include "stm32f103xe.h"

void SystemClock_Config(void);
void SysTick_Init(void);
void delay_ms(uint32_t ms);
void delay_us(uint32_t us);

extern volatile uint32_t sysTickCount;

#endif /* __SYSTEM_CLOCK_H__ */
