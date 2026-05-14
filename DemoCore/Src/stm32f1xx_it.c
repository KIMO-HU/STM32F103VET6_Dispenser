/**
  ******************************************************************************
  * @file    stm32f1xx_it.c
  * @brief   中断服务程序
  ******************************************************************************
  */

#include "stm32f103xe.h"

/* 外部声明 SysTick 计数器 (定义在 main.c) */
extern volatile uint32_t sysTickCount;

/**
  * @brief  NMI 处理程序
  */
void NMI_Handler(void)
{
    while (1) { }
}

/**
  * @brief  Hard Fault 处理程序
  */
void HardFault_Handler(void)
{
    while (1) { }
}

/**
  * @brief  Mem Manage 处理程序
  */
void MemManage_Handler(void)
{
    while (1) { }
}

/**
  * @brief  Bus Fault 处理程序
  */
void BusFault_Handler(void)
{
    while (1) { }
}

/**
  * @brief  Usage Fault 处理程序
  */
void UsageFault_Handler(void)
{
    while (1) { }
}

/**
  * @brief  SVC 处理程序
  */
void SVC_Handler(void)
{
}

/**
  * @brief  Debug Monitor 处理程序
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  PendSV 处理程序
  */
void PendSV_Handler(void)
{
}

/**
  * @brief  SysTick 处理程序
  *         每 1ms 中断一次
  */
void SysTick_Handler(void)
{
    sysTickCount++;
}
