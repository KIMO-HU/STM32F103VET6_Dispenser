/**
  ******************************************************************************
  * @file    watchdog.h
  * @brief   独立看门狗 (IWDG) 驱动头文件
  *          基于旧版软件 board.c 中的 IWDG_Config 实现
  *          超时时间: ~2秒 (PR=64, RLR=1250, 40kHz LSI)
  ******************************************************************************
  */

#ifndef __WATCHDOG_H__
#define __WATCHDOG_H__

#include <stdint.h>

/**
  * @brief  初始化独立看门狗 (IWDG)
  *         配置: 预分频=/64, 重装载值=1250, 超时~2秒
  */
void IWDG_Init(void);

/**
  * @brief  喂狗 (重装载计数器)
  *         必须在看门狗超时前调用，建议在主循环中定期调用
  */
void IWDG_Feed(void);

#endif /* __WATCHDOG_H__ */
