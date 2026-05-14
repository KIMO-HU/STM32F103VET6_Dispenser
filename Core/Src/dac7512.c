/**
  ******************************************************************************
  * @file    dac7512.c
  * @brief   DAC7512 双路模拟输出驱动实现
  * @note    使用硬件 SPI1 (PA4/PA5/PA7) 和 SPI3 (PA15/PB3/PB5)
  *          SYNC 由软件 GPIO 控制 (PA4=DAC1_CS, PA15=DAC2_CS)
  ******************************************************************************
  */

#include "dac7512.h"

/* ================================================================
   引脚定义 (与 gpio_ctrl.h 保持一致)
   ================================================================ */
#define DAC1_CS_PORT            GPIOA
#define DAC1_CS_PIN             (1u << 4)       /* PA4: SPI1_NSS/SYNC */
#define DAC1_SCLK_PIN           (1u << 5)       /* PA5: SPI1_SCLK */
#define DAC1_MISO_PIN           (1u << 6)       /* PA6: SPI1_MISO (不用) */
#define DAC1_MOSI_PIN           (1u << 7)       /* PA7: SPI1_MOSI */

#define DAC2_CS_PORT            GPIOA
#define DAC2_CS_PIN             (1u << 15)      /* PA15: SPI3_NSS/SYNC */
#define DAC2_SCLK_PIN           (1u << 3)       /* PB3: SPI3_SCLK */
#define DAC2_MISO_PIN           (1u << 4)       /* PB4: SPI3_MISO (不用) */
#define DAC2_MOSI_PIN           (1u << 5)       /* PB5: SPI3_MOSI */

/* ================================================================
   本地辅助宏
   ================================================================ */
#define DAC1_CS_LOW()           (DAC1_CS_PORT->BRR  = DAC1_CS_PIN)
#define DAC1_CS_HIGH()          (DAC1_CS_PORT->BSRR = DAC1_CS_PIN)

#define DAC2_CS_LOW()           (DAC2_CS_PORT->BRR  = DAC2_CS_PIN)
#define DAC2_CS_HIGH()          (DAC2_CS_PORT->BSRR = DAC2_CS_PIN)

/* DAC7512 控制字: D15-D14=00(写入并更新), D13-D12=00(正常模式) */
#define DAC7512_CMD_NORMAL      0x0000u

/* ================================================================
   本地函数: 初始化 SPI1
   ================================================================ */
static void SPI1_Init_HW(void)
{
    /* 1. 使能 SPI1 时钟 (APB2) */
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    /* 2. 配置 GPIO
       PA4: 普通推挽输出 (软件 CS/SYNC)
       PA5: 复用推挽输出 (SCLK)
       PA6: 输入浮空 (MISO, 不用)
       PA7: 复用推挽输出 (MOSI)
    */
    GPIOA->CRL &= ~(GPIO_CRL_CNF4 | GPIO_CRL_MODE4 |
                     GPIO_CRL_CNF5 | GPIO_CRL_MODE5 |
                     GPIO_CRL_CNF6 | GPIO_CRL_MODE6 |
                     GPIO_CRL_CNF7 | GPIO_CRL_MODE7);
    /* PA4: 推挽输出 50MHz (0x3) */
    /* PA5: AF 推挽输出 50MHz (0xB) */
    /* PA6: 输入浮空 (0x4) */
    /* PA7: AF 推挽输出 50MHz (0xB) */
    GPIOA->CRL |= (0x3 << (4 * 4)) | (0xB << (5 * 4)) |
                   (0x4 << (6 * 4)) | (0xB << (7 * 4));

    /* CS 默认高电平 (SYNC 空闲高) */
    GPIOA->BSRR = GPIO_BSRR_BS4;

    /* 3. 配置 SPI1 CR1
       - CPHA=0, CPOL=0 (Mode 0)
       - MSTR=1 (主机)
       - BR[2:0]=001 (fPCLK/4 = 72MHz/4 = 18MHz)
       - SPE=0 (先不使能)
       - LSBFIRST=0 (MSB first)
       - SSI=1, SSM=1 (软件 NSS)
       - DFF=1 (16-bit)
    */
    SPI1->CR1 = 0;
    SPI1->CR1 |= SPI_CR1_MSTR              /* 主机模式 */
              |  (0x01 << SPI_CR1_BR_Pos)   /* BR=001, /4 */
              |  SPI_CR1_SSI               /* 内部从机选择=1 */
              |  SPI_CR1_SSM               /* 软件从机管理 */
              |  SPI_CR1_DFF;              /* 16-bit 数据帧 */

    /* 4. 清除状态标志 */
    SPI1->SR = 0;

    /* 5. 使能 SPI1 */
    SPI1->CR1 |= SPI_CR1_SPE;
}

/* ================================================================
   本地函数: 初始化 SPI3
   ================================================================ */
static void SPI3_Init_HW(void)
{
    /* 1. 使能 SPI3 时钟 (APB1) */
    RCC->APB1ENR |= RCC_APB1ENR_SPI3EN;

    /* 2. 配置 GPIO
       PA15: 普通推挽输出 (软件 CS/SYNC)
       PB3:  复用推挽输出 (SCLK)
       PB4:  输入浮空 (MISO, 不用)
       PB5:  复用推挽输出 (MOSI)
    */
    /* PA15 在 CRH 中 (Pin 15) */
    GPIOA->CRH &= ~(GPIO_CRH_CNF15 | GPIO_CRH_MODE15);
    GPIOA->CRH |= (0x3 << ((15 - 8) * 4));  /* 推挽输出 50MHz */

    GPIOB->CRL &= ~(GPIO_CRL_CNF3 | GPIO_CRL_MODE3 |
                     GPIO_CRL_CNF4 | GPIO_CRL_MODE4 |
                     GPIO_CRL_CNF5 | GPIO_CRL_MODE5);
    /* PB3: AF 推挽输出 50MHz (0xB) */
    /* PB4: 输入浮空 (0x4) */
    /* PB5: AF 推挽输出 50MHz (0xB) */
    GPIOB->CRL |= (0xB << (3 * 4)) | (0x4 << (4 * 4)) | (0xB << (5 * 4));

    /* CS 默认高电平 */
    GPIOA->BSRR = GPIO_BSRR_BS15;

    /* 3. 配置 SPI3 CR1
       SPI3 在 APB1 (36MHz), BR=001 -> 36MHz/4 = 9MHz
    */
    SPI3->CR1 = 0;
    SPI3->CR1 |= SPI_CR1_MSTR              /* 主机模式 */
              |  (0x01 << SPI_CR1_BR_Pos)   /* BR=001, /4 */
              |  SPI_CR1_SSI               /* 内部从机选择=1 */
              |  SPI_CR1_SSM               /* 软件从机管理 */
              |  SPI_CR1_DFF;              /* 16-bit 数据帧 */

    /* 4. 清除状态标志 */
    SPI3->SR = 0;

    /* 5. 使能 SPI3 */
    SPI3->CR1 |= SPI_CR1_SPE;
}

/* ================================================================
   本地函数: 通过 SPI1 发送 16-bit 数据
   ================================================================ */
static void SPI1_Send16(uint16_t data)
{
    /* 等待发送缓冲区空 */
    while ((SPI1->SR & SPI_SR_TXE) == 0) {}

    /* 写入数据寄存器 */
    SPI1->DR = data;

    /* 等待接收缓冲区非空 (16-bit 传输完成后 RXNE 会置位) */
    while ((SPI1->SR & SPI_SR_RXNE) == 0) {}

    /* 读取 DR 清除 RXNE */
    (void)SPI1->DR;

    /* 等待 SPI 不忙 */
    while (SPI1->SR & SPI_SR_BSY) {}
}

/* ================================================================
   本地函数: 通过 SPI3 发送 16-bit 数据
   ================================================================ */
static void SPI3_Send16(uint16_t data)
{
    /* 等待发送缓冲区空 */
    while ((SPI3->SR & SPI_SR_TXE) == 0) {}

    /* 写入数据寄存器 */
    SPI3->DR = data;

    /* 等待接收缓冲区非空 */
    while ((SPI3->SR & SPI_SR_RXNE) == 0) {}

    /* 读取 DR 清除 RXNE */
    (void)SPI3->DR;

    /* 等待 SPI 不忙 */
    while (SPI3->SR & SPI_SR_BSY) {}
}

/* ================================================================
   接口函数实现
   ================================================================ */

/**
  * @brief  初始化 SPI1 和 SPI3 用于 DAC7512 通信
  */
void DAC7512_Init(void)
{
    SPI1_Init_HW();
    SPI3_Init_HW();

    /* 上电后两路 DAC 输出置 0 */
    DAC7512_SetZeroAll();
}

/**
  * @brief  向指定 DAC 通道写入 12-bit 数据
  * @param  ch: DAC_CH_SMALL_AUGER 或 DAC_CH_BIG_AUGER
  * @param  value: 12-bit DAC 值 (0x000~0xFFF)
  */
void DAC7512_Write(dac_channel_t ch, uint16_t value)
{
    uint16_t frame;

    /* 限制为 12-bit */
    if (value > 0x0FFF) {
        value = 0x0FFF;
    }

    /* DAC7512 帧格式: D15-D14=00(写入并更新), D13-D12=00(正常模式), D11-D0=data */
    frame = DAC7512_CMD_NORMAL | value;

    if (ch == DAC_CH_SMALL_AUGER) {
        DAC1_CS_LOW();
        SPI1_Send16(frame);
        DAC1_CS_HIGH();
    } else {
        DAC2_CS_LOW();
        SPI3_Send16(frame);
        DAC2_CS_HIGH();
    }
}

/**
  * @brief  设置指定 DAC 通道输出为 0
  */
void DAC7512_SetZero(dac_channel_t ch)
{
    DAC7512_Write(ch, 0);
}

/**
  * @brief  两路 DAC 同时置 0
  */
void DAC7512_SetZeroAll(void)
{
    DAC7512_Write(DAC_CH_SMALL_AUGER, 0);
    DAC7512_Write(DAC_CH_BIG_AUGER, 0);
}
