/**
  ******************************************************************************
  * @file    stm32f1xx_it.h
  * @brief   中断服务程序头文件
  ******************************************************************************
  */

#ifndef __STM32F1XX_IT_H
#define __STM32F1XX_IT_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "stm32f103xe.h"

/* Exported functions prototypes ---------------------------------------------*/
void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __STM32F1XX_IT_H */

/* 外部声明 SysTick 计数器 */
extern volatile uint32_t sysTickCount;
