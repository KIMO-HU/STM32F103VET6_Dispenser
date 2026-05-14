/**
  ******************************************************************************
  * @file    flash.c
  * @brief   STM32F103 Flash 操作驱动 (寄存器级)
  *          注意: 擦除/编程函数需在 RAM 中执行，或确保执行时不在被擦除的 Bank 中
  ******************************************************************************
  */

#include "flash.h"

#define FLASH_PRG_KEY1      0x45670123U
#define FLASH_PRG_KEY2      0xCDEF89ABU
#define FLASH_TIMEOUT_VAL       0x00050000U

/**
  * @brief  解锁 Flash
  */
void FLASH_Unlock(void)
{
    if (FLASH->CR & FLASH_CR_LOCK) {
        FLASH->KEYR = FLASH_PRG_KEY1;
        FLASH->KEYR = FLASH_PRG_KEY2;
    }
}

/**
  * @brief  锁定 Flash
  */
void FLASH_Lock(void)
{
    FLASH->CR |= FLASH_CR_LOCK;
}

/**
  * @brief  清空错误标志
  */
void FLASH_ClearFlags(void)
{
    FLASH->SR = FLASH_SR_EOP | FLASH_SR_WRPRTERR | FLASH_SR_PGERR;
}

/**
  * @brief  获取 Flash 状态
  */
flash_status_t FLASH_GetStatus(void)
{
    if (FLASH->SR & FLASH_SR_BSY) {
        return FLASH_BUSY;
    }
    if (FLASH->SR & FLASH_SR_PGERR) {
        return FLASH_ERROR;
    }
    if (FLASH->SR & FLASH_SR_WRPRTERR) {
        return FLASH_ERROR;
    }
    return FLASH_OK;
}

/**
  * @brief  等待 Flash 操作完成
  */
flash_status_t FLASH_WaitForLastOperation(uint32_t timeout)
{
    flash_status_t status = FLASH_BUSY;
    while (timeout--) {
        status = FLASH_GetStatus();
        if (status != FLASH_BUSY) {
            break;
        }
    }
    if (status == FLASH_BUSY) {
        return FLASH_ERROR;
    }
    if (FLASH->SR & (FLASH_SR_PGERR | FLASH_SR_WRPRTERR)) {
        FLASH_ClearFlags();
        return FLASH_ERROR;
    }
    FLASH->SR |= FLASH_SR_EOP;  /* 清除 EOP */
    return FLASH_OK;
}

/**
  * @brief  擦除一页 (2KB)
  * @param  page_addr: 页起始地址，必须是 0x08000000 + n*2048
  */
flash_status_t FLASH_ErasePage(uint32_t page_addr)
{
    flash_status_t status;

    status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VAL);
    if (status != FLASH_OK) {
        return status;
    }

    FLASH->CR |= FLASH_CR_PER;          /* 页擦除使能 */
    FLASH->AR  = page_addr;             /* 设置页地址 */
    FLASH->CR |= FLASH_CR_STRT;         /* 开始擦除 */

    status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VAL);

    FLASH->CR &= ~FLASH_CR_PER;         /* 清除页擦除位 */

    return status;
}

/**
  * @brief  写入半字 (16-bit)
  */
flash_status_t FLASH_ProgramHalfWord(uint32_t addr, uint16_t data)
{
    flash_status_t status;

    status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VAL);
    if (status != FLASH_OK) {
        return status;
    }

    FLASH->CR |= FLASH_CR_PG;           /* 编程使能 */
    *(__IO uint16_t *)addr = data;      /* 写入数据 */

    status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VAL);

    FLASH->CR &= ~FLASH_CR_PG;          /* 清除编程位 */

    return status;
}

/**
  * @brief  写入字 (32-bit)，分两次半字写入
  */
flash_status_t FLASH_ProgramWord(uint32_t addr, uint32_t data)
{
    flash_status_t status;

    status = FLASH_ProgramHalfWord(addr, (uint16_t)(data & 0xFFFF));
    if (status != FLASH_OK) return status;

    status = FLASH_ProgramHalfWord(addr + 2, (uint16_t)((data >> 16) & 0xFFFF));

    return status;
}
