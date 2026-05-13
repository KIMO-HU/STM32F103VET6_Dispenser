/**
  ******************************************************************************
  * @file    protocol_sac.c
  * @brief   中药机终端通信协议解析与响应
  *          协议格式: [0x55][0xBB][TYPE][REV1][REV2][LEN_HI][LEN_LO][DATA...][CRC]
  *          校验: 除帧头和校验字节外的异或值
  ******************************************************************************
  */

#include <stddef.h>
#include "protocol_sac.h"
#include "usart_ctrl.h"
#include "dispenser.h"
#include "gpio_ctrl.h"
#include "main.h"

/* 接收状态机 */
typedef enum {
    ST_HEAD1 = 0,
    ST_HEAD2,
    ST_TYPE,
    ST_REV1,
    ST_REV2,
    ST_LEN_HI,
    ST_LEN_LO,
    ST_DATA,
    ST_CRC
} rx_state_t;

static rx_state_t s_rx_state = ST_HEAD1;
static uint8_t  s_rx_buf[64];
static uint16_t s_rx_len = 0;
static uint16_t s_rx_cnt = 0;
static uint8_t  s_rx_crc = 0;

/* 协议私有变量 */
static uint8_t s_board_id = 1;
static uint8_t s_machine_id = 1;

/**
  * @brief  计算异或校验
  */
static uint8_t calc_crc(uint8_t *data, uint16_t len)
{
    uint8_t crc = 0;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= data[i];
    }
    return crc;
}

/**
  * @brief  发送协议帧
  * @param  type: 消息类型
  * @param  subtype: 子类型 (编码到长度高字节的 bit15:13)
  * @param  data: 消息内容
  * @param  len: 消息内容长度 (低13位有效)
  */
void Protocol_SendResponse(uint8_t type, enum Message_Sub_Type subtype,
                           uint8_t *data, uint16_t len)
{
    uint8_t tx_buf[64];
    uint16_t pos = 0;
    uint16_t len_field;

    tx_buf[pos++] = COMPROTOCOLHEAD1;
    tx_buf[pos++] = COMPROTOCOLHEAD2;
    tx_buf[pos++] = type;
    tx_buf[pos++] = s_board_id;     /* REV1: 板卡ID */
    tx_buf[pos++] = s_machine_id;   /* REV2: 机器ID */

    /* 长度字段: 高3bit = 子类型, 低13bit = 长度 */
    len_field = ((subtype & 0x07) << 13) | (len & 0x1FFF);
    tx_buf[pos++] = (len_field >> 8) & 0xFF;
    tx_buf[pos++] = len_field & 0xFF;

    /* 消息内容 */
    for (uint16_t i = 0; i < len; i++) {
        tx_buf[pos++] = data[i];
    }

    /* 校验 (除帧头和校验字节本身) */
    tx_buf[pos] = calc_crc(&tx_buf[2], pos - 2);
    pos++;

    UART_PutBuf(UART_IDX_UART4, tx_buf, pos);
}

/**
  * @brief  处理具体命令
  */
void Protocol_ProcessCommand(uint8_t cmd_type, uint8_t *data, uint16_t len)
{
    uint8_t resp_data[16];
    uint16_t resp_len = 0;
    uint8_t resp_type;
    enum Message_Sub_Type resp_subtype = SUC_NODATA;

    switch (cmd_type) {
        /* ---------------- 查询类 ---------------- */
        case SYS_IO_REQ: {  /* 0x01: 当前IO状态查询 */
            resp_type = SYS_IO_RSP;
            resp_data[0] = DISPENSER_GetStatusByte();
            resp_len = 1;
            resp_subtype = SUC_HAVEDATA;
            break;
        }

        case SAMP_WEIGHT_REQ: {  /* 0x06: 当前重量查询 */
            resp_type = SAMP_WEIGHT_RSP;
            uint16_t w = DISPENSER_GetWeight();
            resp_data[0] = w & 0xFF;
            resp_data[1] = (w >> 8) & 0xFF;
            resp_len = 2;
            resp_subtype = SUC_HAVEDATA;
            break;
        }

        case CFG_OUT_STATUS_REQ: {  /* 0x10: 出药状态查询 */
            resp_type = CFG_OUT_STATUS_RSP;
            resp_data[0] = (uint8_t)g_disp_status;
            resp_len = 1;
            resp_subtype = SUC_HAVEDATA;
            break;
        }

        case IAP_VERSION_INQUIRE_REQ: {  /* 0x78: 读取版本 */
            resp_type = IAP_VERSION_INQUIRE_RSP;
            resp_data[0] = FW_VERSION_MAJOR;
            resp_data[1] = FW_VERSION_MINOR;
            resp_data[2] = FW_VERSION_PATCH;
            resp_len = 3;
            resp_subtype = SUC_HAVEDATA;
            break;
        }

        /* ---------------- 设置类 ---------------- */
        case MEDCINE_START_REQ: {  /* 0x04: 单次出药 */
            resp_type = MEDCIEN_START_RSP;
            if (len >= 2) {
                uint16_t weight = ((uint16_t)data[0] << 8) | data[1];
                g_disp_params.target_weight = weight;
                g_dispense_cmd = 1;
                resp_subtype = SUC_NODATA;
            } else {
                resp_subtype = FAILED;
                resp_data[0] = CAUSE_ARG_ILLEGAL;
                resp_len = 1;
            }
            break;
        }

        case CFG_DATA_MEDCINE_SET:      /* 0x09: 药量设置 */
        case CFG_SPEED_MODE_H_SET:      /* 0x0F: 高速设置 */
        case CFG_SPEED_MODE_M_SET:      /* 0x20: 中速设置 */
        case CFG_SPEED_MODE_L_SET:      /* 0x21: 低速设置 */
        case SAMP_PERCENT_CFG_REQ:      /* 0x07: 速度百分比 */
        case CFG_DOOR_TIME_REQ:         /* 0x18: 开门时间 */
        case CFG_TIMEOUT_REQ:           /* 0x1B: 超时时间 */
        case CFG_AUGER_TIME_REQ:        /* 0x1E: 绞龙时间 */
        case CFG_AUGER_WEIGHT_REQ:      /* 0x1F: 绞龙差值 */
        case CFG_LOW_WEIGHT_REQ:        /* 0x22: 低速阈值 */
        case CFG_WEIGHT_HIGN_REQ:       /* 0x1C: 标定高重 */
        case CFG_WEIGHT_LOW_REQ:        /* 0x1D: 标定零重 */
        case STIR_SPEED_REQ: {          /* 0x60: 搅拌速度 */
            DISPENSER_SetParam(cmd_type, data, len);
            /* 根据具体命令返回对应响应 */
            if (cmd_type == CFG_DATA_MEDCINE_SET) resp_type = CFG_DATA_MEDCINE_SET_RSP;
            else if (cmd_type == CFG_SPEED_MODE_H_SET) resp_type = CFG_SPEED_MODE_H_SET_RSP;
            else if (cmd_type == CFG_SPEED_MODE_M_SET) resp_type = CFG_SPEED_MODE_M_SET_RSP;
            else if (cmd_type == CFG_SPEED_MODE_L_SET) resp_type = CFG_SPEED_MODE_L_SET_RSP;
            else if (cmd_type == SAMP_PERCENT_CFG_REQ) resp_type = SAMP_PERCENT_CFG_RSP;
            else if (cmd_type == CFG_DOOR_TIME_REQ) resp_type = CFG_DOOR_TIME_RSP;
            else if (cmd_type == CFG_TIMEOUT_REQ) resp_type = CFG_TIMEOUT_RSP;
            else if (cmd_type == CFG_AUGER_TIME_REQ) resp_type = CFG_AUGER_TIME_RSP;
            else if (cmd_type == CFG_AUGER_WEIGHT_REQ) resp_type = CFG_AUGER_WEIGHT_RSP;
            else if (cmd_type == CFG_LOW_WEIGHT_REQ) resp_type = CFG_LOW_WEIGHT_RSP;
            else if (cmd_type == CFG_WEIGHT_HIGN_REQ) resp_type = CFG_WEIGHT_HIGN_RSP;
            else if (cmd_type == CFG_WEIGHT_LOW_REQ) resp_type = CFG_WEIGHT_LOW_RSP;
            else if (cmd_type == STIR_SPEED_REQ) resp_type = STIR_SPEED_RSP;
            else resp_type = cmd_type + 0x80;
            resp_subtype = SUC_NODATA;
            break;
        }

        /* ---------------- 动作类 ---------------- */
        case CFG_DOOR_OPEN_REQ: {  /* 0x19: 手动开门 */
            resp_type = CFG_DOOR_OPEN_RSP;
            g_door_open_cmd = 1;
            resp_subtype = SUC_NODATA;
            break;
        }

        case CFG_VIBRATE_REQ: {  /* 0x1A: 手动振动 */
            resp_type = CFG_VIBRATE_RSP;
            g_vibrate_cmd = 1;
            resp_subtype = SUC_NODATA;
            break;
        }

        case SYS_EMERGENCY_STOP_REQ: {  /* 0x61: 急停 */
            resp_type = SYS_EMERGENCY_STOP_RSP;
            g_emergency_stop = 1;
            resp_subtype = SUC_NODATA;
            break;
        }

        case SYS_CLEAR_REQ: {  /* 0x62: 一键清空 */
            resp_type = SYS_CLEAR_RSP;
            g_dispense_cmd = 0xFF;
            resp_subtype = SUC_NODATA;
            break;
        }

        case SYS_REBOOT_REQ: {  /* 0x08: 系统重启 */
            resp_type = SYS_REBOOT_RSP;
            resp_subtype = SUC_NODATA;
            Protocol_SendResponse(resp_type, resp_subtype, resp_data, resp_len);
            delay_ms(100);
            NVIC_SystemReset();
            break;
        }

        /* ---------------- 心跳 ---------------- */
        case SYS_HB_REQ: {  /* 0x5D: 心跳 */
            resp_type = SYS_HB_RSP;
            resp_subtype = SUC_NODATA;
            break;
        }

        /* ---------------- 未实现/保留 ---------------- */
        default: {
            resp_type = cmd_type + 0x80;
            resp_subtype = FAILED;
            resp_data[0] = CAUSE_OPS_FAIL;
            resp_len = 1;
            break;
        }
    }

    Protocol_SendResponse(resp_type, resp_subtype, resp_data, resp_len);
}

/**
  * @brief  解析接收到的协议帧
  * @param  data: 缓冲区
  * @param  len: 长度
  * @retval 0: 未完整, 1: 成功解析
  */
uint8_t Protocol_ParseFrame(uint8_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        uint8_t ch = data[i];

        switch (s_rx_state) {
            case ST_HEAD1:
                if (ch == COMPROTOCOLHEAD1) {
                    s_rx_state = ST_HEAD2;
                }
                break;

            case ST_HEAD2:
                if (ch == COMPROTOCOLHEAD2) {
                    s_rx_state = ST_TYPE;
                    s_rx_crc = 0;
                } else {
                    s_rx_state = ST_HEAD1;
                }
                break;

            case ST_TYPE:
                s_rx_buf[0] = ch;
                s_rx_crc ^= ch;
                s_rx_state = ST_REV1;
                break;

            case ST_REV1:
                s_rx_buf[1] = ch;
                s_rx_crc ^= ch;
                s_rx_state = ST_REV2;
                break;

            case ST_REV2:
                s_rx_buf[2] = ch;
                s_rx_crc ^= ch;
                s_rx_state = ST_LEN_HI;
                break;

            case ST_LEN_HI:
                s_rx_buf[3] = ch;
                s_rx_crc ^= ch;
                s_rx_state = ST_LEN_LO;
                break;

            case ST_LEN_LO:
                s_rx_buf[4] = ch;
                s_rx_crc ^= ch;
                s_rx_len = (((uint16_t)s_rx_buf[3] << 8) | s_rx_buf[4]) & 0x1FFF;
                s_rx_cnt = 0;
                if (s_rx_len > 0) {
                    if (s_rx_len > 48) {
                        s_rx_state = ST_HEAD1;  /* 长度异常 */
                    } else {
                        s_rx_state = ST_DATA;
                    }
                } else {
                    s_rx_state = ST_CRC;
                }
                break;

            case ST_DATA:
                s_rx_buf[5 + s_rx_cnt] = ch;
                s_rx_crc ^= ch;
                s_rx_cnt++;
                if (s_rx_cnt >= s_rx_len) {
                    s_rx_state = ST_CRC;
                }
                break;

            case ST_CRC:
                if (ch == s_rx_crc) {
                    /* CRC 正确，处理命令 */
                    uint8_t cmd_type = s_rx_buf[0];
                    uint8_t *cmd_data = (s_rx_len > 0) ? &s_rx_buf[5] : NULL;
                    Protocol_ProcessCommand(cmd_type, cmd_data, s_rx_len);
                }
                s_rx_state = ST_HEAD1;
                break;

            default:
                s_rx_state = ST_HEAD1;
                break;
        }
    }
    return 0;
}
