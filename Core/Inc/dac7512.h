/**
  ******************************************************************************
  * @file    dac7512.h
  * @brief   DAC7512 双路模拟输出驱动 - SPI1(小绞龙) + SPI3(大绞龙)
  * @note    DAC7512: 12-bit DAC, 16-bit SPI frame (D15-D14=control, D13-D12=power, D11-D0=data)
  *          数据格式: 0x0XXX = 正常模式写入DAC寄存器并更新输出
  ******************************************************************************
  */

#ifndef __DAC7512_H
#define __DAC7512_H

#include "stm32f103xe.h"
#include <stdint.h>

/* ================================================================
   通道选择
   ================================================================ */
typedef enum {
    DAC_CH_SMALL_AUGER = 0,     /* SPI1 -> 小绞龙速度模拟输出 */
    DAC_CH_BIG_AUGER   = 1,     /* SPI3 -> 大绞龙速度模拟输出 */
} dac_channel_t;

/* ================================================================
   接口函数
   ================================================================ */

/**
  * @brief  初始化 SPI1 和 SPI3, 配置为硬件 SPI 主机模式
  * @note   SPI1: APB2=72MHz, 分频/4=18MHz; SPI3: APB1=36MHz, 分频/4=9MHz
  *         Mode0(CPOL=0,CPHA=0), 16-bit, MSB-first, 软件NSS
  */
void DAC7512_Init(void);

/**
  * @brief  向指定 DAC 通道写入 12-bit 数据
  * @param  ch: DAC_CH_SMALL_AUGER 或 DAC_CH_BIG_AUGER
  * @param  value: 12-bit DAC 值 (0x000~0xFFF)
  * @note   内部自动添加控制位 (00) 和电源模式位 (00) -> 帧格式: 0000 + D11-D0
  */
void DAC7512_Write(dac_channel_t ch, uint16_t value);

/**
  * @brief  设置指定 DAC 通道输出为 0
  * @param  ch: DAC_CH_SMALL_AUGER 或 DAC_CH_BIG_AUGER
  */
void DAC7512_SetZero(dac_channel_t ch);

/**
  * @brief  两路 DAC 同时置 0
  */
void DAC7512_SetZeroAll(void);

#endif /* __DAC7512_H */
