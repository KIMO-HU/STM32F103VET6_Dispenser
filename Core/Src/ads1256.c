/**
  ******************************************************************************
  * @file    ads1256.c
  * @brief   ADS1256 24位 ADC 驱动 (软件 SPI / GPIO 模拟)
  *          引脚: CS=PB12, SCLK=PB13, DIN=PB15, DOUT=PB14, DRDY=PD10
  *          时序: CPOL=0, CPHA=0, SCLK 最大 5MHz (72MHz 系统下 GPIO 翻转足够)
  ******************************************************************************
  */

#include "ads1256.h"
#include "stm32f103xe.h"
#include "system_clock.h"

/* ==================================================================
   标定参数 (RAM 存储，后续可扩展为 Flash/W25Q128 保存)
   ================================================================== */
static int32_t  s_adc_zero     = 0;         /* 零重标定 ADC 值 */
static int32_t  s_adc_full     = 0x7FFFFF;  /* 满量程标定 ADC 值 */
static uint16_t s_weight_full  = 5000;      /* 满量程对应重量 (克) */
static uint8_t  s_is_calibrated = 0;

/* ==================================================================
   软件滤波: 滑动平均 (窗口大小 = ADS1256_FILTER_WINDOW)
   ================================================================== */
#define ADS1256_FILTER_WINDOW   8
static int32_t s_filter_buf[ADS1256_FILTER_WINDOW];
static uint8_t s_filter_idx = 0;
static uint8_t s_filter_cnt = 0;

/* ==================================================================
   内部宏: GPIO 快速操作
   ================================================================== */
#define CS_LOW()        (ADS1256_CS_PORT->BSRR = (ADS1256_CS_PIN << 16))
#define CS_HIGH()       (ADS1256_CS_PORT->BSRR = ADS1256_CS_PIN)
#define SCLK_LOW()      (ADS1256_SCLK_PORT->BSRR = (ADS1256_SCLK_PIN << 16))
#define SCLK_HIGH()     (ADS1256_SCLK_PORT->BSRR = ADS1256_SCLK_PIN)
#define DIN_LOW()       (ADS1256_DIN_PORT->BSRR = (ADS1256_DIN_PIN << 16))
#define DIN_HIGH()      (ADS1256_DIN_PORT->BSRR = ADS1256_DIN_PIN)
#define DOUT_IS_HIGH()  ((ADS1256_DOUT_PORT->IDR & ADS1256_DOUT_PIN) != 0)
#define DRDY_IS_LOW()   ((ADS1256_DRDY_PORT->IDR & ADS1256_DRDY_PIN) == 0)

/**
  * @brief  微秒级延时 (简单空循环，72MHz 下约 7-8 个时钟周期/us)
  */
static inline void ads_delay_us(uint32_t us)
{
    volatile uint32_t n;
    while (us--) {
        n = 8;
        while (n--);
    }
}

/**
  * @brief  纳秒级延时 (72MHz 下，一个空循环约 50-100ns)
  */
static inline void ads_delay_ns(void)
{
    volatile uint32_t n = 2;
    while (n--);
}

/* ==================================================================
   GPIO 初始化
   ================================================================== */

/**
  * @brief  初始化 ADS1256 使用的 GPIO
  */
static void ADS1256_GPIO_Init(void)
{
    /* 使能时钟 */
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPDEN;

    /* PB12 (CS), PB13 (SCLK), PB15 (DIN): 推挽输出 50MHz */
    GPIOB->CRH &= ~((0x0F << ((12 - 8) * 4)) |
                    (0x0F << ((13 - 8) * 4)) |
                    (0x0F << ((15 - 8) * 4)));
    GPIOB->CRH |= ((0x03 << ((12 - 8) * 4)) |   /* MODE=11, CNF=00 */
                   (0x03 << ((13 - 8) * 4)) |
                   (0x03 << ((15 - 8) * 4)));

    /* PB14 (DOUT): 浮空输入 */
    GPIOB->CRH &= ~(0x0F << ((14 - 8) * 4));
    GPIOB->CRH |= (0x04 << ((14 - 8) * 4));     /* MODE=00, CNF=01 */

    /* PD10 (DRDY): 浮空输入 */
    GPIOD->CRH &= ~(0x0F << ((10 - 8) * 4));
    GPIOD->CRH |= (0x04 << ((10 - 8) * 4));     /* MODE=00, CNF=01 */

    /* 初始状态 */
    CS_HIGH();
    SCLK_LOW();
    DIN_HIGH();
}

/* ==================================================================
   软件 SPI 底层
   ================================================================== */

/**
  * @brief  软件 SPI 发送 8 bit
  */
static void ADS1256_Send8Bit(uint8_t data)
{
    ads_delay_ns();
    for (uint8_t i = 0; i < 8; i++) {
        if (data & 0x80) {
            DIN_HIGH();
        } else {
            DIN_LOW();
        }
        SCLK_HIGH();
        ads_delay_ns();
        data <<= 1;
        SCLK_LOW();
        ads_delay_ns();
    }
}

/**
  * @brief  软件 SPI 接收 8 bit
  */
static uint8_t ADS1256_Recv8Bit(void)
{
    uint8_t read = 0;
    ads_delay_ns();
    for (uint8_t i = 0; i < 8; i++) {
        SCLK_HIGH();
        ads_delay_ns();
        read <<= 1;
        SCLK_LOW();
        if (DOUT_IS_HIGH()) {
            read |= 0x01;
        }
        ads_delay_ns();
    }
    return read;
}

/* ==================================================================
   寄存器/命令接口
   ================================================================== */

/**
  * @brief  向 ADS1256 写寄存器
  */
void ADS1256_WriteReg(uint8_t reg_id, uint8_t value)
{
    CS_LOW();
    ADS1256_Send8Bit(CMD_WREG | reg_id);
    ADS1256_Send8Bit(0x00);         /* 写 1 个寄存器 */
    ADS1256_Send8Bit(value);
    CS_HIGH();
}

/**
  * @brief  从 ADS1256 读寄存器
  */
uint8_t ADS1256_ReadReg(uint8_t reg_id)
{
    uint8_t value;
    CS_LOW();
    ADS1256_Send8Bit(CMD_RREG | reg_id);
    ADS1256_Send8Bit(0x00);         /* 读 1 个寄存器 */
    ads_delay_us(1);
    value = ADS1256_Recv8Bit();
    CS_HIGH();
    return value;
}

/**
  * @brief  发送单字节命令
  */
void ADS1256_WriteCmd(uint8_t cmd)
{
    CS_LOW();
    ADS1256_Send8Bit(cmd);
    CS_HIGH();
}

/* ==================================================================
   ADC 配置与读取
   ================================================================== */

/**
  * @brief  配置 ADS1256 增益与数据率
  * @param  gain:  PGA_1 ~ PGA_64
  * @param  drate: 数据率 (如 DRATE_10)
  */
static void ADS1256_CfgADC(uint8_t gain, uint8_t drate)
{
    uint8_t buf[4];

    /* 复位 */
    ADS1256_WriteCmd(CMD_RESET);
    delay_ms(10);

    /* 状态寄存器: MSB first, ACAL=1(自校准使能), BUFEN=0 */
    buf[0] = (0 << 3) | (1 << 2) | (0 << 1);
    /* MUX: AIN0 + AINCOM */
    buf[1] = CH_AIN0;
    /* ADCON: CLKOUT=OFF, SDCS=OFF, PGA=gain */
    buf[2] = (0 << 5) | (0 << 3) | (gain << 0);
    /* DRATE */
    buf[3] = drate;

    CS_LOW();
    ADS1256_Send8Bit(CMD_WREG | REG_STATUS);
    ADS1256_Send8Bit(0x03);         /* 连续写 4 个寄存器 */
    ADS1256_Send8Bit(buf[0]);
    ADS1256_Send8Bit(buf[1]);
    ADS1256_Send8Bit(buf[2]);
    ADS1256_Send8Bit(buf[3]);
    CS_HIGH();

    delay_ms(5);
}

/**
  * @brief  读取原始 24位 ADC 值 (有符号补码)
  * @retval 32位有符号整数
  */
int32_t ADS1256_ReadRaw(void)
{
    uint32_t read;
    int32_t result;

    /* 等待 DRDY 变低 (转换完成) */
    uint32_t timeout = 100000;
    while (!DRDY_IS_LOW() && timeout--);

    CS_LOW();
    ADS1256_Send8Bit(CMD_RDATA);
    ads_delay_us(1);

    read = ((uint32_t)ADS1256_Recv8Bit()) << 16;
    read |= ((uint32_t)ADS1256_Recv8Bit()) << 8;
    read |= ADS1256_Recv8Bit();

    CS_HIGH();

    /* 24位补码转 32位有符号整数 */
    if (read & 0x800000) {
        result = (int32_t)(read | 0xFF000000);
    } else {
        result = (int32_t)read;
    }

    return result;
}

/* ==================================================================
   标定接口
   ================================================================== */

/**
  * @brief  设置零重标定值
  */
void ADS1256_SetZeroCalibration(int32_t adc_zero)
{
    s_adc_zero = adc_zero;
    s_is_calibrated = 1;
}

/**
  * @brief  设置满量程标定值
  */
void ADS1256_SetFullScaleCalibration(int32_t adc_full, uint16_t weight_full)
{
    s_adc_full = adc_full;
    s_weight_full = weight_full;
}

int32_t ADS1256_GetZeroCalibration(void)
{
    return s_adc_zero;
}

int32_t ADS1256_GetFullCalibration(void)
{
    return s_adc_full;
}

uint16_t ADS1256_GetFullWeight(void)
{
    return s_weight_full;
}

uint8_t ADS1256_IsCalibrated(void)
{
    return s_is_calibrated;
}

/* ==================================================================
   重量读取
   ================================================================== */

/**
  * @brief  软件滤波: 滑动平均
  * @param  raw: 当前原始ADC值
  * @retval 滤波后的ADC值
  */
static int32_t ads1256_filter(int32_t raw)
{
    s_filter_buf[s_filter_idx] = raw;
    s_filter_idx = (s_filter_idx + 1) % ADS1256_FILTER_WINDOW;
    if (s_filter_cnt < ADS1256_FILTER_WINDOW) {
        s_filter_cnt++;
    }

    int64_t sum = 0;
    for (uint8_t i = 0; i < s_filter_cnt; i++) {
        sum += s_filter_buf[i];
    }
    return (int32_t)(sum / s_filter_cnt);
}

/**
  * @brief  读取当前重量 (克)
  * @note   必须先进行零重标定 (0x1D) 才能获取准确值
  *         已增加滑动平均软件滤波 (窗口=8)
  */
uint16_t ADS1256_ReadWeight(void)
{
    int32_t raw = ADS1256_ReadRaw();

    /* 软件滤波 */
    raw = ads1256_filter(raw);

    if (!s_is_calibrated) {
        /* 未标定: 返回原始值的高 16 位作为演示 */
        return (uint16_t)((raw >> 8) & 0xFFFF);
    }

    /* 计算重量: weight = (raw - zero) * full_weight / (full - zero) */
    int32_t diff = raw - s_adc_zero;
    int32_t adc_range = s_adc_full - s_adc_zero;

    if (adc_range == 0) {
        return 0;
    }

    /* 使用 64 位中间值防止溢出 */
    int64_t weight64 = (int64_t)diff * (int64_t)s_weight_full;
    int64_t result = weight64 / adc_range;

    if (result < 0) result = 0;
    if (result > 65535) result = 65535;

    return (uint16_t)result;
}

/* ==================================================================
   初始化
   ================================================================== */

/**
  * @brief  初始化 ADS1256
  * @note   配置为 PGA=64, 10SPS, AIN0+AINCOM
  */
void ADS1256_Init(void)
{
    ADS1256_GPIO_Init();

    delay_ms(50);   /* 上电稳定时间 */

    ADS1256_CfgADC(PGA_64, DRATE_10);

    /* 默认标定参数 (用户需通过 0x1C/0x1D 指令标定) */
    s_adc_zero = 0;
    s_adc_full = 0x7FFFFF / 64;   /* 约 24bit 正满量程 / PGA */
    s_weight_full = 5000;
    s_is_calibrated = 0;
}
