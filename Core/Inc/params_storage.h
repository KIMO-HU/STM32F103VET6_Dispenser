/**
  ******************************************************************************
  * @file    params_storage.h
  * @brief   运行参数内部 Flash 持久化存储
  *          旧版软件使用 0x0803F800 内部 Flash 存参数，本方案沿用类似思路，
  *          地址迁移到 App Flash 区域末尾 (0x0807F000/0x0807F800)，
  *          采用双备份 + magic + checksum 机制确保可靠性。
  ******************************************************************************
  */

#ifndef __PARAMS_STORAGE_H__
#define __PARAMS_STORAGE_H__

#include <stdint.h>

/* Flash 参数区 (内部 Flash 最后两页，每页 2KB) */
#define PARAMS_ADDR_MAIN     0x0807F000
#define PARAMS_ADDR_BACKUP   0x0807F800
#define PARAMS_MAGIC         0x5A5AA5A5

/* 参数包结构 (与 dispenser.h 中的 disp_params_t 对应) */
typedef struct {
    uint32_t magic;              /* PARAMS_MAGIC */
    uint16_t version;            /* 结构版本，用于后续兼容升级 */
    uint16_t checksum;           /* 校验和 */

    /* --- 与 disp_params_t 一一对应 --- */
    uint16_t target_weight;
    uint16_t current_weight;
    uint16_t high_speed;
    uint16_t mid_speed;
    uint16_t low_speed;
    uint8_t  speed_percent;
    uint8_t  door_time;
    uint8_t  timeout;
    uint16_t auger_check_time;
    uint16_t auger_weight_diff;
    uint16_t low_weight_thresh;
    uint16_t cali_high_weight;
    uint16_t cali_zero_weight;
    uint8_t  stir_speed;
    uint8_t  board_id;
    uint8_t  machine_id;

    /* --- ADS1256 标定参数 --- */
    int32_t  adc_zero;
    int32_t  adc_full;
    uint16_t weight_full;
    uint8_t  pad[2];             /* 对齐到 4 字节倍数 */
} params_pack_t;

/* 函数声明 */
int  PARAMS_Load(void);
void PARAMS_Save(void);
void PARAMS_Erase(void);

#endif /* __PARAMS_STORAGE_H__ */
