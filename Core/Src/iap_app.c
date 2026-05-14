/**
  ******************************************************************************
  * @file    iap_app.c
  * @brief   App 侧 IAP 实现
  *          通过写入 Flash 标志并复位，通知 Bootloader 进入升级模式
  ******************************************************************************
  */

#include "iap_app.h"
#include "stm32f103xe.h"

/* Flash 解锁序列 */
#define FLASH_PRG_KEY1      0x45670123U
#define FLASH_PRG_KEY2      0xCDEF89ABU

/* 本地 Flash 辅助函数 (不依赖 Bootloader 的 flash.c) */
static void flash_unlock(void)
{
    if (FLASH->CR & FLASH_CR_LOCK) {
        FLASH->KEYR = FLASH_PRG_KEY1;
        FLASH->KEYR = FLASH_PRG_KEY2;
    }
}

static void flash_lock(void)
{
    FLASH->CR |= FLASH_CR_LOCK;
}

static void flash_wait_busy(void)
{
    while (FLASH->SR & FLASH_SR_BSY);
}

static void flash_erase_page(uint32_t addr)
{
    flash_wait_busy();
    FLASH->SR = FLASH_SR_EOP | FLASH_SR_WRPRTERR | FLASH_SR_PGERR;
    FLASH->CR |= FLASH_CR_PER;
    FLASH->AR = addr;
    FLASH->CR |= FLASH_CR_STRT;
    flash_wait_busy();
    FLASH->CR &= ~FLASH_CR_PER;
}

static void flash_program_word(uint32_t addr, uint32_t data)
{
    flash_wait_busy();
    FLASH->SR = FLASH_SR_EOP | FLASH_SR_WRPRTERR | FLASH_SR_PGERR;
    FLASH->CR |= FLASH_CR_PG;
    *(__IO uint16_t *)addr = (uint16_t)(data & 0xFFFF);
    flash_wait_busy();
    *(__IO uint16_t *)(addr + 2) = (uint16_t)((data >> 16) & 0xFFFF);
    flash_wait_busy();
    FLASH->CR &= ~FLASH_CR_PG;
}

/**
  * @brief  写入 IAP 标志并复位系统
  */
void IAP_TriggerBootloader(void)
{
    __disable_irq();

    /* 写入强制升级标志 */
    flash_unlock();
    flash_erase_page(IAP_FLAG_ADDR & ~(2048 - 1));
    flash_program_word(IAP_FLAG_ADDR, IAP_MAGIC_ENTER);
    flash_lock();

    /* 系统复位 */
    NVIC_SystemReset();

    while (1);
}

/**
  * @brief  直接跳转到 Bootloader (不复位)
  */
void IAP_JumpToBootloader(void)
{
    uint32_t bl_addr = 0x08000000;

    __disable_irq();

    /* 关闭 SysTick */
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;

    /* 重置向量表 */
    SCB->VTOR = bl_addr;

    /* 设置栈顶 */
    uint32_t sp = *(volatile uint32_t *)bl_addr;
    uint32_t pc = *(volatile uint32_t *)(bl_addr + 4);

    __set_MSP(sp);

    __asm volatile ("BX %0" :: "r"(pc));

    while (1);
}
