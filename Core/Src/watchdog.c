/**
  ******************************************************************************
  * @file    watchdog.c
  * @brief   独立看门狗 (IWDG) 寄存器级驱动
  *          与旧版 software_V1.0 (board.c) 中的 IWDG_Config 保持一致
  *
  *  IWDG 配置参数:
  *    - 时钟源: LSI (内部低速 RC, ~40kHz)
  *    - 预分频器: /64  (IWDG_PR = 4)
  *    - 重装载值: 1250 (IWDG_RLR = 1250)
  *    - 超时时间: Tout = ((4 * 2^PR) * RLR) / 40kHz
  *                = ((4 * 2^4) * 1250) / 40000
  *                = (64 * 1250) / 40000
  *                = 80000 / 40000 = 2.0 秒
  *
  *  寄存器操作:
  *    IWDG_KR (Key Register):
  *      - 0x5555: 允许访问 PR 和 RLR 寄存器
  *      - 0xAAAA: 重装载计数器 (喂狗)
  *      - 0xCCCC: 启动看门狗
  *    IWDG_PR (Prescaler Register): 预分频值 0~7 (/4 ~ /256)
  *    IWDG_RLR (Reload Register): 12位重装载值 0~0xFFF
  *    IWDG_SR (Status Register): 位0=PVU(预分频更新中), 位1=RVU(重装载更新中)
  ******************************************************************************
  */

#include "watchdog.h"

/* IWDG 寄存器定义 (如果 CMSIS 头文件未包含) */
#ifndef IWDG_BASE
#define IWDG_BASE           0x40003000
#define IWDG                ((IWDG_TypeDef *)IWDG_BASE)
typedef struct {
    volatile uint32_t KR;
    volatile uint32_t PR;
    volatile uint32_t RLR;
    volatile uint32_t SR;
} IWDG_TypeDef;
#endif

/* IWDG 键值 */
#define IWDG_KEY_ENABLE     0x5555  /* 允许写 PR/RLR */
#define IWDG_KEY_RELOAD     0xAAAA  /* 重装载 (喂狗) */
#define IWDG_KEY_START      0xCCCC  /* 启动看门狗 */

/* IWDG 预分频值 */
#define IWDG_PRESCALER_4    0
#define IWDG_PRESCALER_8    1
#define IWDG_PRESCALER_16   2
#define IWDG_PRESCALER_32   3
#define IWDG_PRESCALER_64   4
#define IWDG_PRESCALER_128  5
#define IWDG_PRESCALER_256  6

/* 旧版配置: PR=64, RLR=1250, 超时~2秒 */
#define IWDG_PRESCALER      IWDG_PRESCALER_64
#define IWDG_RELOAD_VALUE   1250

/**
  * @brief  等待 IWDG 寄存器更新完成
  */
static void iwdg_wait_ready(void)
{
    /* 等待 PVU 和 RVU 位清零 */
    while (IWDG->SR & 0x03);
}

void IWDG_Init(void)
{
    /* 1. 允许写 PR 和 RLR 寄存器 */
    IWDG->KR = IWDG_KEY_ENABLE;

    /* 2. 设置预分频器 = /64 */
    iwdg_wait_ready();
    IWDG->PR = IWDG_PRESCALER;

    /* 3. 设置重装载值 = 1250 */
    iwdg_wait_ready();
    IWDG->RLR = IWDG_RELOAD_VALUE;

    /* 4. 先喂一次狗 (将 RLR 值加载到计数器) */
    IWDG->KR = IWDG_KEY_RELOAD;

    /* 5. 启动看门狗 */
    IWDG->KR = IWDG_KEY_START;
}

void IWDG_Feed(void)
{
    /* 写 0xAAAA 到 KR 寄存器重装载计数器 */
    IWDG->KR = IWDG_KEY_RELOAD;
}
