/**
  ******************************************************************************
  * @file    led_indicator.h
  * @brief   料位LED指示灯控制 (基于旧版 protocolSAC.c LED控制逻辑)
  *          通过 USART3 发送协议帧控制外部LED颜色，直观显示当前料位状态
  ******************************************************************************
  */

#ifndef __LED_INDICATOR_H__
#define __LED_INDICATOR_H__

#include <stdint.h>

/**
  * @brief  初始化LED指示器 (无需额外初始化，依赖USART3)
  */
void LED_Indicator_Init(void);

/**
  * @brief  根据当前料位状态更新LED颜色
  *         仅在料位状态变化且未出药时发送更新帧
  */
void LED_Indicator_Update(void);

/**
  * @brief  强制发送LED颜色帧 (用于测试)
  * @param  r, g, b: RGB颜色值
  * @param  dim: 1=使用RUO弱化亮度(旧版实际使用), 0=全亮度
  */
void LED_Indicator_SendColor(uint8_t r, uint8_t g, uint8_t b, uint8_t dim);

#endif /* __LED_INDICATOR_H__ */
