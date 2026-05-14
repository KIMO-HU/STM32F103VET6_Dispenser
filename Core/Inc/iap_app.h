/**
  ******************************************************************************
  * @file    iap_app.h
  * @brief   App 侧 IAP 接口 - 触发进入 Bootloader 升级模式
  ******************************************************************************
  */

#ifndef __IAP_APP_H__
#define __IAP_APP_H__

#include <stdint.h>

/* Bootloader 标志地址 (与 Bootloader 一致) */
#define IAP_FLAG_ADDR       0x08007FF0
#define IAP_MAGIC_ENTER     0x5A5A5A5A

/**
  * @brief  触发系统复位并进入 Bootloader IAP 模式
  * @note   调用后不会返回，系统立即复位
  */
void IAP_TriggerBootloader(void);

/**
  * @brief  直接跳转到 Bootloader (不复位)
  * @note   慎用：可能因外设状态不一致导致问题
  */
void IAP_JumpToBootloader(void);

#endif /* __IAP_APP_H__ */
