/**
  ******************************************************************************
  * @file    dispenser.h
  * @brief   中药机出药控制逻辑头文件
  ******************************************************************************
  */

#ifndef __DISPENSER_H__
#define __DISPENSER_H__

#include <stdint.h>

/* 出药状态 */
typedef enum {
    DISP_IDLE = 0,          /* 空闲/出药完成 */
    DISP_LOW_SPEED,         /* 低速出药中 */
    DISP_MID_SPEED,         /* 中速出药中 */
    DISP_HIGH_SPEED,        /* 高速出药中 */
    DISP_TIMEOUT = 0xFF     /* 出药超时 */
} disp_status_t;

/* 设备运行参数 */
typedef struct {
    uint16_t target_weight;         /* 目标药量 (克) */
    uint16_t current_weight;        /* 当前重量 (克) */
    uint16_t high_speed;            /* 高速速度值 (0x0000~0x0FFF) */
    uint16_t mid_speed;             /* 中速速度值 */
    uint16_t low_speed;             /* 低速速度值 */
    uint8_t  speed_percent;         /* 速度切换百分比 */
    uint8_t  door_time;             /* 开门时间 (秒) */
    uint8_t  timeout;               /* 出药超时时间 (秒) */
    uint16_t auger_check_time;      /* 绞龙重量确认时间间隔 (ms, 默认3000) */
    uint16_t auger_weight_diff;     /* 绞龙重量前后差值 (克, 默认1) */
    uint16_t low_weight_thresh;     /* 花草型低速阈值 */
    uint16_t cali_high_weight;      /* 标定高重量 */
    uint16_t cali_zero_weight;      /* 标定零重量 (去皮) */
    uint8_t  stir_speed;            /* 搅拌速度 1-3 */
    uint8_t  board_id;              /* 板卡ID */
    uint8_t  machine_id;            /* 中药机ID */
} disp_params_t;

/* 外部变量 */
extern disp_params_t g_disp_params;
extern disp_status_t g_disp_status;
extern volatile uint8_t g_dispense_cmd;     /* 出药指令标志 */
extern volatile uint8_t g_emergency_stop;   /* 急停标志 */
extern volatile uint8_t g_door_open_cmd;    /* 手动开门指令 */
extern volatile uint8_t g_vibrate_cmd;      /* 手动振动指令 */

/* 函数声明 */
void DISPENSER_Init(void);
void DISPENSER_Process(void);
void DISPENSER_Start(uint16_t weight);
void DISPENSER_Stop(void);
void DISPENSER_EmergencyStop(void);
void DISPENSER_DoorOpen(void);
void DISPENSER_Vibrate(void);
void DISPENSER_Clear(void);

uint8_t DISPENSER_GetStatusByte(void);
uint16_t DISPENSER_GetWeight(void);
void DISPENSER_SetParam(uint8_t param_type, uint8_t *data, uint16_t len);

#endif /* __DISPENSER_H__ */
