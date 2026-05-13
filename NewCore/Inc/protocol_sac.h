/**
  ******************************************************************************
  * @file    protocol_sac.h
  * @brief   中药机终端通信协议定义 (基于 中药机终端通信协议V2.2)
  ******************************************************************************
  */

#ifndef __PROTOCOL_SAC_H__
#define __PROTOCOL_SAC_H__

#include <stdint.h>

/* 帧头定义 */
#define COMPROTOCOLHEAD1    0x55
#define COMPROTOCOLHEAD2    0xBB

/* 消息子类型 */
enum Message_Sub_Type {
    NOUSE = 0,              /* 未使用 */
    SUC_NODATA,             /* 成功但不携带数据 */
    SUC_HAVEDATA,           /* 成功且携带数据 */
    FAILED,                 /* 失败，携带失败原因值，消息长度1字节 */
    RESVED,                 /* 暂时保留 */
};

/* 原因值 */
typedef enum {
    CAUSE_SUCC = 0,            /* 成功 */
    CAUSE_CKS_ERR,             /* 校验失败 */
    CAUSE_ARG_ILLEGAL,         /* 参数非法 */
    CAUSE_OPS_FAIL,            /* 操作失败 */
    CAUSE_CFG_NOT_EXIST,       /* 配置参数不存在 */
    CAUSE_INTR_ERR,            /* 内部错误 */
    CAUSE_STATE_MISMATCH,      /* 状态不匹配 */
    CAUSE_NO_VALID_DATA,       /* 没有有效数据 */
    CAUSE_UNSPECIFIED = 0xFF   /* 未说明 */
} cause_e;

/* ========== 请求消息类型 ========== */
#define SYS_IO_REQ                  0x01    /* 当前IO状态查询 */
#define MEDCINE_START_REQ           0x04    /* 单次出药指令 */
#define SAMP_WEIGHT_REQ             0x06    /* 当前重量查询 */
#define SAMP_PERCENT_CFG_REQ        0x07    /* 速度切换百分比设置 */
#define SYS_REBOOT_REQ              0x08    /* 系统重启 */
#define CFG_DATA_MEDCINE_SET        0x09    /* 药量设置 */
#define CFG_SPEED_MODE_H_SET        0x0F    /* 高速速度设置 */
#define CFG_OUT_STATUS_REQ          0x10    /* 系统当前出药状态查询 */
#define CFG_DOOR_TIME_REQ           0x18    /* 开门时间设置 */
#define CFG_DOOR_OPEN_REQ           0x19    /* 手动开门指令 */
#define CFG_VIBRATE_REQ             0x1A    /* 手动振动指令 */
#define CFG_TIMEOUT_REQ             0x1B    /* 出药超时时间设置 */
#define CFG_WEIGHT_HIGN_REQ         0x1C    /* 重量标定高重量设置 */
#define CFG_WEIGHT_LOW_REQ          0x1D    /* 重量标定零重量设置 */
#define CFG_AUGER_TIME_REQ          0x1E    /* 绞龙重量确认时间间隔设置 */
#define CFG_AUGER_WEIGHT_REQ        0x1F    /* 绞龙重量前后差值设置 */
#define CFG_SPEED_MODE_M_SET        0x20    /* 中速速度设置 */
#define CFG_SPEED_MODE_L_SET        0x21    /* 低速速度设置 */
#define CFG_LOW_WEIGHT_REQ          0x22    /* 花草型低于此重量进入低速 */
#define CTRL_ABN_DETECT_STOP_REQ    0x23    /* 采集异常检测停止 */
#define CTRL_ACC_ZERO_CORR_REQ      0x24    /* 加速度采集零点校正 */
#define ALARM_REQ                   0x33    /* 告警查询 */
#define SYS_HB_REQ                  0x5D    /* 心跳查询 */
#define STIR_SPEED_REQ              0x60    /* 搅拌速度设置 */
#define SYS_EMERGENCY_STOP_REQ      0x61    /* 急停 */
#define SYS_CLEAR_REQ               0x62    /* 一键清空 */

/* IAP/升级相关 */
#define IAP_INTEGRAL_START_REQ      0x70
#define IAP_SINGAL_UNIT_START_REQ   0x71
#define IAP_SINGAL_PACKEG_START_REQ 0x72
#define IAP_SINGAL_PACKEG_REQ       0x73
#define IAP_SIGNAL_PACKEG_END_REQ   0x74
#define IAP_SINGAL_UNIT_END_REQ     0x75
#define IAP_INTEGRAL_END_REQ        0x76
#define IAP_STATUS_INQUIRE_REQ      0x77
#define IAP_VERSION_INQUIRE_REQ     0x78

/* ========== 响应消息类型 ========== */
#define SYS_IO_RSP                  0x81
#define SYS_ID_RSP                  0x82
#define SYS_TEMP_RSP                0x83
#define MEDCIEN_START_RSP           0x84
#define ACC_RPT_STOP_RSP            0x85
#define SAMP_WEIGHT_RSP             0x86
#define SAMP_PERCENT_CFG_RSP        0x87
#define SYS_REBOOT_RSP              0x88
#define CFG_DATA_MEDCINE_SET_RSP    0x89
#define CFG_SPEED_MODE_H_SET_RSP    0x8F
#define CFG_OUT_STATUS_RSP          0x90
#define ALARM_SWITCH_SET_RSP        0x91
#define ALARM_SWITCH_RSP            0x92
#define ALARM_DELAY_SET_RSP         0x93
#define ALARM_DELAY_RSP             0x94
#define RELAY_SWITCH_RSP            0x95
#define MONITOR_MODE_SET_RSP        0x96
#define MONITOR_MODE_RSP            0x97
#define CFG_DOOR_TIME_RSP           0x98
#define CFG_DOOR_OPEN_RSP           0x99
#define CFG_VIBRATE_RSP             0x9A
#define CFG_TIMEOUT_RSP             0x9B
#define CFG_WEIGHT_HIGN_RSP         0x9C
#define CFG_WEIGHT_LOW_RSP          0x9D
#define CFG_AUGER_TIME_RSP          0x9E
#define CFG_AUGER_WEIGHT_RSP        0x9F
#define CFG_SPEED_MODE_M_SET_RSP    0xA0
#define CFG_SPEED_MODE_L_SET_RSP    0xA1
#define CFG_LOW_WEIGHT_RSP          0xA2
#define CTRL_ABN_DETECT_STOP_RSP    0xA3
#define CTRL_ACC_ZERO_CORR_RSP      0xA4
#define ALARM_RSP                   0xB3
#define SYS_HB_RSP                  0xDD
#define STIR_SPEED_RSP              0xE0
#define SYS_EMERGENCY_STOP_RSP      0xE1
#define SYS_CLEAR_RSP               0xE2

/* IAP 响应 */
#define IAP_INTEGRAL_START_RSP      0xF0
#define IAP_SINGAL_UNIT_START_RSP   0xF1
#define IAP_SINGAL_PACKEG_START_RSP 0xF2
#define IAP_SINGAL_PACKEG_RSP       0xF3
#define IAP_SIGNAL_PACKEG_END_RSP   0xF4
#define IAP_SINGAL_UNIT_END_RSP     0xF5
#define IAP_INTEGRAL_END_RSP        0xF6
#define IAP_STATUS_INQUIRE_RSP      0xF7
#define IAP_VERSION_INQUIRE_RSP     0xF8
#define IAP_ReplyforPackage_RSP     0xF9
#define IAP_ReplyforPackS_RSP       0xFA

/* 协议处理函数 */
uint8_t Protocol_ParseFrame(uint8_t *data, uint16_t len);
void Protocol_SendResponse(uint8_t type, enum Message_Sub_Type subtype,
                           uint8_t *data, uint16_t len);
void Protocol_ProcessCommand(uint8_t cmd_type, uint8_t *data, uint16_t len);

#endif /* __PROTOCOL_SAC_H__ */
