/**
  ******************************************************************************
  * @file    flash.h
  * @brief   STM32F103 Flash 操作驱动 (寄存器级)
  ******************************************************************************
  */

#ifndef __FLASH_H__
#define __FLASH_H__

#include <stdint.h>
#include "stm32f103xe.h"

/* Flash 状态 */
typedef enum {
    FLASH_OK = 0,
    FLASH_ERROR,
    FLASH_TIMEOUT,
    FLASH_BUSY,
} flash_status_t;

/* 解锁/锁定 */
void FLASH_Unlock(void);
void FLASH_Lock(void);

/* 擦除页 (2KB/页) */
flash_status_t FLASH_ErasePage(uint32_t page_addr);

/* 编程 */
flash_status_t FLASH_ProgramHalfWord(uint32_t addr, uint16_t data);
flash_status_t FLASH_ProgramWord(uint32_t addr, uint32_t data);

/* 等待操作完成 */
flash_status_t FLASH_WaitForLastOperation(uint32_t timeout);

/* 获取状态 */
flash_status_t FLASH_GetStatus(void);

/* 清空所有错误标志 */
void FLASH_ClearFlags(void);

#endif /* __FLASH_H__ */
