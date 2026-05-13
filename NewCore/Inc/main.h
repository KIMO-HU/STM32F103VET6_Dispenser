/**
  ******************************************************************************
  * @file    main.h
  * @brief   中药机主程序头文件
  ******************************************************************************
  */

#ifndef __MAIN_H__
#define __MAIN_H__

#include "stm32f103xe.h"
#include "stdint.h"
#include "system_clock.h"
#include "gpio_ctrl.h"
#include "usart_ctrl.h"
#include "protocol_sac.h"
#include "dispenser.h"

/* 软件版本 */
#define FW_VERSION_MAJOR    1
#define FW_VERSION_MINOR    0
#define FW_VERSION_PATCH    0

/* 错误处理 */
void Error_Handler(void);

#endif /* __MAIN_H__ */
