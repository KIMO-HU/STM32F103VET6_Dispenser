/**
  ******************************************************************************
  * @file    stm32f1xx_it.c
  * @brief   中断服务程序 (裸机版)
  ******************************************************************************
  */

#include "stm32f103xe.h"
#include "system_clock.h"

/* 外部声明 SysTick 计数器 */
extern volatile uint32_t sysTickCount;

void NMI_Handler(void)
{
    while (1) { }
}

void HardFault_Handler(void)
{
    while (1) { }
}

void MemManage_Handler(void)
{
    while (1) { }
}

void BusFault_Handler(void)
{
    while (1) { }
}

void UsageFault_Handler(void)
{
    while (1) { }
}

void SVC_Handler(void)
{
}

void DebugMon_Handler(void)
{
}

void PendSV_Handler(void)
{
}

/**
  * @brief  SysTick 处理程序 - 每 1ms 中断一次
  */
void SysTick_Handler(void)
{
    sysTickCount++;
}
