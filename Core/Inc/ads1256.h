/**
  ******************************************************************************
  * @file    ads1256.h
  * @brief   ADS1256 24位 ADC 驱动头文件 (软件 SPI)
  *          称重传感器: LC1030 -> ADS1256 -> STM32F103 SPI2 (GPIO 模拟)
  ******************************************************************************
  */

#ifndef __ADS1256_H__
#define __ADS1256_H__

#include <stdint.h>

/* ==================================================================
   引脚定义 (基于旧版代码硬件连接)
   ================================================================== */
#define ADS1256_CS_PORT     GPIOB
#define ADS1256_CS_PIN      (1U << 12)      /* PB12: 片选 */

#define ADS1256_SCLK_PORT   GPIOB
#define ADS1256_SCLK_PIN    (1U << 13)      /* PB13: 时钟 */

#define ADS1256_DOUT_PORT   GPIOB
#define ADS1256_DOUT_PIN    (1U << 14)      /* PB14: MISO (ADC -> MCU) */

#define ADS1256_DIN_PORT    GPIOB
#define ADS1256_DIN_PIN     (1U << 15)      /* PB15: MOSI (MCU -> ADC) */

#define ADS1256_DRDY_PORT   GPIOD
#define ADS1256_DRDY_PIN    (1U << 10)      /* PD10: 数据就绪 */

/* ==================================================================
   寄存器地址
   ================================================================== */
#define REG_STATUS          0x00
#define REG_MUX             0x01
#define REG_ADCON           0x02
#define REG_DRATE           0x03
#define REG_IO              0x04
#define REG_OFC0            0x05
#define REG_OFC1            0x06
#define REG_OFC2            0x07
#define REG_FSC0            0x08
#define REG_FSC1            0x09
#define REG_FSC2            0x0A

/* ==================================================================
   命令定义
   ================================================================== */
#define CMD_WAKEUP          0x00
#define CMD_RDATA           0x01
#define CMD_RDATAC          0x03
#define CMD_SDATAC          0x0F
#define CMD_RREG            0x10
#define CMD_WREG            0x50
#define CMD_SELFCAL         0xF0
#define CMD_SELFOCAL        0xF1
#define CMD_SELFGCAL        0xF2
#define CMD_SYSOCAL         0xF3
#define CMD_SYSGCAL         0xF4
#define CMD_SYNC            0xFC
#define CMD_STANDBY         0xFD
#define CMD_RESET           0xFE

/* ==================================================================
   增益与数据率
   ================================================================== */
#define PGA_1               0x00
#define PGA_2               0x01
#define PGA_4               0x02
#define PGA_8               0x03
#define PGA_16              0x04
#define PGA_32              0x05
#define PGA_64              0x06

#define DRATE_30000         0xF0
#define DRATE_15000         0xE0
#define DRATE_7500          0xD0
#define DRATE_3750          0xC0
#define DRATE_2000          0xB0
#define DRATE_1000          0xA0
#define DRATE_500           0x92
#define DRATE_100           0x82
#define DRATE_60            0x72
#define DRATE_50            0x63
#define DRATE_30            0x53
#define DRATE_25            0x43
#define DRATE_15            0x33
#define DRATE_10            0x23
#define DRATE_5             0x13
#define DRATE_2_5           0x02

/* ==================================================================
   通道选择 (MUX 寄存器)
   ================================================================== */
#define CH_AIN0             0x08    /* AIN0 + AINCOM */
#define CH_AIN1             0x18    /* AIN1 + AINCOM */
#define CH_AIN2             0x28    /* AIN2 + AINCOM */
#define CH_AIN3             0x38    /* AIN3 + AINCOM */

/* ==================================================================
   函数声明
   ================================================================== */
void ADS1256_Init(void);
int32_t ADS1256_ReadRaw(void);
uint16_t ADS1256_ReadWeight(void);

void ADS1256_SetZeroCalibration(int32_t adc_zero);
void ADS1256_SetFullScaleCalibration(int32_t adc_full, uint16_t weight_full);

int32_t  ADS1256_GetZeroCalibration(void);
int32_t  ADS1256_GetFullCalibration(void);
uint16_t ADS1256_GetFullWeight(void);
uint8_t  ADS1256_IsCalibrated(void);

void ADS1256_WriteReg(uint8_t reg_id, uint8_t value);
uint8_t ADS1256_ReadReg(uint8_t reg_id);
void ADS1256_WriteCmd(uint8_t cmd);

#endif /* __ADS1256_H__ */
