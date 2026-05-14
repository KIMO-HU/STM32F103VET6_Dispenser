/**
  ******************************************************************************
  * @file    params_storage.c
  * @brief   运行参数内部 Flash 持久化存储实现
  *          双备份机制: 主区(0x0807F000) + 备份区(0x0807F800)
  *          每页 2KB，仅使用前 64 字节存储参数包。
  ******************************************************************************
  */

#include "params_storage.h"
#include "dispenser.h"
#include "ads1256.h"
#include "stm32f103xe.h"
#include "watchdog.h"

/* Flash 解锁序列 */
#define FLASH_PRG_KEY1  0x45670123U
#define FLASH_PRG_KEY2  0xCDEF89ABU

/* 参数结构版本 */
#define PARAMS_VERSION  0x0100

/* ==================================================================
   内部 Flash 操作 (寄存器级)
   ================================================================== */

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

static void flash_clear_flags(void)
{
    FLASH->SR = FLASH_SR_EOP | FLASH_SR_WRPRTERR | FLASH_SR_PGERR;
}

static int flash_erase_page(uint32_t addr)
{
    flash_wait_busy();
    flash_clear_flags();
    FLASH->CR |= FLASH_CR_PER;
    FLASH->AR = addr;
    FLASH->CR |= FLASH_CR_STRT;
    /* Flash擦除约需20~40ms，喂狗防止看门狗复位 */
    IWDG_Feed();
    flash_wait_busy();
    IWDG_Feed();
    FLASH->CR &= ~FLASH_CR_PER;

    if (FLASH->SR & (FLASH_SR_PGERR | FLASH_SR_WRPRTERR)) {
        FLASH->SR = FLASH_SR_EOP | FLASH_SR_WRPRTERR | FLASH_SR_PGERR;
        return -1;
    }
    return 0;
}

static int flash_program_word(uint32_t addr, uint32_t data)
{
    flash_wait_busy();
    flash_clear_flags();
    FLASH->CR |= FLASH_CR_PG;
    *(__IO uint16_t *)addr = (uint16_t)(data & 0xFFFF);
    flash_wait_busy();
    *(__IO uint16_t *)(addr + 2) = (uint16_t)((data >> 16) & 0xFFFF);
    flash_wait_busy();
    FLASH->CR &= ~FLASH_CR_PG;

    if (FLASH->SR & (FLASH_SR_PGERR | FLASH_SR_WRPRTERR)) {
        FLASH->SR = FLASH_SR_EOP | FLASH_SR_WRPRTERR | FLASH_SR_PGERR;
        return -1;
    }
    return 0;
}

/* ==================================================================
   校验和
   ================================================================== */

static uint16_t params_checksum(const params_pack_t *p)
{
    const uint8_t *data = (const uint8_t *)p;
    uint16_t crc = 0xFFFF;
    /* 跳过 magic 和 checksum 本身 */
    for (uint16_t i = 8; i < sizeof(params_pack_t); i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 1) crc = (crc >> 1) ^ 0xA001;
            else crc >>= 1;
        }
    }
    return crc;
}

/* ==================================================================
   序列化 / 反序列化
   ================================================================== */

static void params_pack(params_pack_t *p)
{
    uint8_t *ptr = (uint8_t *)p;
    for (uint16_t i = 0; i < sizeof(params_pack_t); i++) ptr[i] = 0;
    p->magic          = PARAMS_MAGIC;
    p->version        = PARAMS_VERSION;
    p->target_weight  = g_disp_params.target_weight;
    p->current_weight = g_disp_params.current_weight;
    p->high_speed     = g_disp_params.high_speed;
    p->mid_speed      = g_disp_params.mid_speed;
    p->low_speed      = g_disp_params.low_speed;
    p->speed_percent  = g_disp_params.speed_percent;
    p->door_time      = g_disp_params.door_time;
    p->timeout        = g_disp_params.timeout;
    p->auger_check_time  = g_disp_params.auger_check_time;
    p->auger_weight_diff = g_disp_params.auger_weight_diff;
    p->low_weight_thresh = g_disp_params.low_weight_thresh;
    p->cali_high_weight  = g_disp_params.cali_high_weight;
    p->cali_zero_weight  = g_disp_params.cali_zero_weight;
    p->stir_speed     = g_disp_params.stir_speed;
    p->board_id       = g_disp_params.board_id;
    p->machine_id     = g_disp_params.machine_id;
    p->adc_zero       = ADS1256_GetZeroCalibration();
    p->adc_full       = ADS1256_GetFullCalibration();
    p->weight_full    = ADS1256_GetFullWeight();
    p->checksum       = params_checksum(p);
}

static void params_unpack(const params_pack_t *p)
{
    g_disp_params.target_weight  = p->target_weight;
    g_disp_params.current_weight = p->current_weight;
    g_disp_params.high_speed     = p->high_speed;
    g_disp_params.mid_speed      = p->mid_speed;
    g_disp_params.low_speed      = p->low_speed;
    g_disp_params.speed_percent  = p->speed_percent;
    g_disp_params.door_time      = p->door_time;
    g_disp_params.timeout        = p->timeout;
    g_disp_params.auger_check_time  = p->auger_check_time;
    g_disp_params.auger_weight_diff = p->auger_weight_diff;
    g_disp_params.low_weight_thresh = p->low_weight_thresh;
    g_disp_params.cali_high_weight  = p->cali_high_weight;
    g_disp_params.cali_zero_weight  = p->cali_zero_weight;
    g_disp_params.stir_speed     = p->stir_speed;
    g_disp_params.board_id       = p->board_id;
    g_disp_params.machine_id     = p->machine_id;
    ADS1256_SetZeroCalibration(p->adc_zero);
    ADS1256_SetFullScaleCalibration(p->adc_full, p->weight_full);
}

/* ==================================================================
   写入指定地址
   ================================================================== */

static int params_write_to(uint32_t addr, const params_pack_t *p)
{
    int rc;
    const uint32_t *src = (const uint32_t *)p;
    uint16_t words = (sizeof(params_pack_t) + 3) / 4;

    rc = flash_erase_page(addr);
    if (rc != 0) return -1;

    for (uint16_t i = 0; i < words; i++) {
        rc = flash_program_word(addr + i * 4, src[i]);
        if (rc != 0) return -1;
    }
    return 0;
}

/* ==================================================================
   从指定地址读取并校验
   ================================================================== */

static int params_read_from(uint32_t addr, params_pack_t *p)
{
    const params_pack_t *flash = (const params_pack_t *)addr;

    /* 读取 */
    const uint8_t *src = (const uint8_t *)flash;
    uint8_t *dst = (uint8_t *)p;
    for (uint16_t i = 0; i < sizeof(params_pack_t); i++) dst[i] = src[i];

    /* 检查 magic */
    if (p->magic != PARAMS_MAGIC) return -1;

    /* 检查版本 */
    if (p->version != PARAMS_VERSION) return -1;

    /* 检查校验和 */
    uint16_t crc = params_checksum(p);
    if (crc != p->checksum) return -1;

    return 0;
}

/* ==================================================================
   公共接口
   ================================================================== */

/**
  * @brief  从 Flash 加载参数到 RAM
  * @retval 0: 成功, -1: 无有效参数
  */
int PARAMS_Load(void)
{
    params_pack_t pack;

    /* 尝试主区 */
    if (params_read_from(PARAMS_ADDR_MAIN, &pack) == 0) {
        params_unpack(&pack);
        return 0;
    }

    /* 主区损坏，尝试备份区 */
    if (params_read_from(PARAMS_ADDR_BACKUP, &pack) == 0) {
        params_unpack(&pack);
        /* 将备份恢复回主区 */
        flash_unlock();
        params_write_to(PARAMS_ADDR_MAIN, &pack);
        flash_lock();
        return 0;
    }

    return -1;
}

/**
  * @brief  将当前 RAM 参数保存到 Flash (双备份)
  */
void PARAMS_Save(void)
{
    params_pack_t pack;
    params_pack(&pack);

    __disable_irq();
    flash_unlock();

    /* 先写主区 */
    if (params_write_to(PARAMS_ADDR_MAIN, &pack) == 0) {
        /* 主区成功后再写备份区 */
        params_write_to(PARAMS_ADDR_BACKUP, &pack);
    } else {
        /* 主区失败，尝试备份区 */
        params_write_to(PARAMS_ADDR_BACKUP, &pack);
    }

    flash_lock();
    __enable_irq();
}

/**
  * @brief  擦除参数区 (恢复出厂设置)
  */
void PARAMS_Erase(void)
{
    __disable_irq();
    flash_unlock();
    flash_erase_page(PARAMS_ADDR_MAIN);
    flash_erase_page(PARAMS_ADDR_BACKUP);
    flash_lock();
    __enable_irq();
}
