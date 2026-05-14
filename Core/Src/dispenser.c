/**
  ******************************************************************************
  * @file    dispenser.c
  * @brief   中药机出药控制逻辑
  * @note    基于裸机轮询架构，替代原有 RT-Thread 多线程实现
  ******************************************************************************
  */

#include "dispenser.h"
#include "gpio_ctrl.h"
#include "pwm_ctrl.h"
#include "dac7512.h"
#include "system_clock.h"
#include "protocol_sac.h"
#include "ads1256.h"
#include "params_storage.h"

/* 全局变量 */
disp_params_t g_disp_params = {
    .target_weight     = 0,
    .current_weight    = 0,
    .high_speed        = 0x0FFF,     /* 与旧版一致: 高速默认值 */
    .mid_speed         = 0x07FF,     /* 与旧版一致: 中速默认值 */
    .low_speed         = 0x04FF,     /* 与旧版一致: 低速默认值 */
    .speed_percent     = 50,
    .door_time         = 5,
    .timeout           = 255,
    .auger_check_time  = 3000,
    .auger_weight_diff = 1,
    .low_weight_thresh = 0,
    .cali_high_weight  = 500,
    .cali_zero_weight  = 0,
    .stir_speed        = 2,
    .board_id          = 1,
    .machine_id        = 1,
};

disp_status_t g_disp_status = DISP_IDLE;

volatile uint8_t g_dispense_cmd    = 0;
volatile uint8_t g_emergency_stop  = 0;
volatile uint8_t g_door_open_cmd   = 0;
volatile uint8_t g_vibrate_cmd     = 0;

/* 内部状态 */
static uint32_t s_dispense_start_time = 0;
static uint32_t s_last_auger_check    = 0;
static uint16_t s_last_weight         = 0;
static uint8_t  s_door_is_open        = 0;
static uint32_t s_door_open_time      = 0;
static uint16_t s_complete_confirm_cnt = 0;   /* 完成确认计数器 (与旧版 MEDCINE_STEP4_TIMES 对应) */
static uint8_t  s_timeout_need_resume = 0;    /* 超时后是否需要续药 */

/**
  * @brief  初始化 dispenser
  */
void DISPENSER_Init(void)
{
    g_disp_status = DISP_IDLE;
    g_dispense_cmd = 0;
    g_emergency_stop = 0;

    /* 初始化 ADS1256 称重模块 */
    ADS1256_Init();

    /* 所有电机停止 */
    MOTOR_DC1_Set(0, 0);
    MOTOR_DC2_Set(0, 0);
    MOTOR_DC3_Set(0, 0);
    MOTOR_DC4_Set(0, 0);
    AUGER_S_Set(0, 0, 1);  /* 小绞龙停止+刹车 */
    AUGER_B_Set(0, 0, 1);  /* 大绞龙停止+刹车 */

    /* 尝试从 Flash 加载参数，失败则保留默认值 */
    if (PARAMS_Load() != 0) {
        /* 首次运行或 Flash 无效，保存默认参数 */
        PARAMS_Save();
    }
}

/**
  * @brief  主处理循环 (需在 main loop 中周期性调用)
  */
void DISPENSER_Process(void)
{
    /* 急停处理 (最高优先级) */
    if (g_emergency_stop) {
        DISPENSER_EmergencyStop();
        g_emergency_stop = 0;
        return;
    }

    /* 手动开门 */
    if (g_door_open_cmd) {
        g_door_open_cmd = 0;
        DISPENSER_DoorOpen();
        return;
    }

    /* 手动振动 */
    if (g_vibrate_cmd) {
        g_vibrate_cmd = 0;
        DISPENSER_Vibrate();
        return;
    }

    /* 一键清空 */
    if (g_dispense_cmd == 0xFF) {
        g_dispense_cmd = 0;
        DISPENSER_Clear();
        return;
    }

    /* 正常出药流程 */
    if (g_dispense_cmd && (g_disp_status == DISP_IDLE)) {
        g_dispense_cmd = 0;
        DISPENSER_Start(g_disp_params.target_weight);
    }

    /* 出药状态机 */
    if (g_disp_status != DISP_IDLE) {
        uint32_t elapsed = sysTickCount - s_dispense_start_time;

        /* 超时检测 -- 与旧版一致: 超时后回到 IDLE */
        if (elapsed > (uint32_t)g_disp_params.timeout * 1000) {
            DISPENSER_Stop();
            /* 续药逻辑: 如果未达到目标重量, 标记需要续药 */
            uint16_t diff = (g_disp_params.target_weight > g_disp_params.current_weight)
                            ? (g_disp_params.target_weight - g_disp_params.current_weight)
                            : 0;
            if (diff > g_disp_params.auger_weight_diff) {
                /* 未达到目标重量, 标记需要续药 */
                s_timeout_need_resume = 1;
            } else {
                s_timeout_need_resume = 0;
            }
            g_disp_status = DISP_IDLE;
            return;
        }

        /* 读取当前重量 */
        g_disp_params.current_weight = DISPENSER_GetWeight();

        /* 出药完成检测 -- 与旧版一致: 重量>=目标值持续10秒才确认完成 */
        if (g_disp_params.current_weight >= g_disp_params.target_weight) {
            s_complete_confirm_cnt++;
            /* 旧版逻辑: MEDCINE_TIME>=10000ms 且持续10次(约10秒) */
            if (s_complete_confirm_cnt >= 10) {
                DISPENSER_Stop();
                g_disp_status = DISP_IDLE;
                s_complete_confirm_cnt = 0;
                return;
            }
        } else {
            s_complete_confirm_cnt = 0;  /* 重量回落, 重置计数器 */
        }

        /* 速度切换逻辑 -- 与旧版一致: 基于已出重量百分比 */
        /* 旧版: <50%高速, 50%-75%中速, 75%-100%低速 */
        uint16_t dispensed = g_disp_params.current_weight;
        uint16_t target = g_disp_params.target_weight;
        if (dispensed >= (target * 3 / 4)) {
            /* 已出重量 >= 75% 目标值 -> 低速 (对应旧版 MEDCINE_STEP=1) */
            if (g_disp_status != DISP_LOW_SPEED) {
                g_disp_status = DISP_LOW_SPEED;
                /* 小绞龙低速运行, 大绞龙停止 */
                AUGER_S_Set(1, 0, 0);
                DAC7512_Write(DAC_CH_SMALL_AUGER, g_disp_params.low_speed);
                AUGER_B_Set(0, 0, 1);
                DAC7512_SetZero(DAC_CH_BIG_AUGER);
            }
        } else if (dispensed >= (target / 2)) {
            /* 已出重量 >= 50% 目标值 -> 中速 (对应旧版 MEDCINE_STEP=2) */
            if (g_disp_status != DISP_MID_SPEED) {
                g_disp_status = DISP_MID_SPEED;
                /* 大小绞龙中速运行 */
                AUGER_S_Set(1, 0, 0);
                DAC7512_Write(DAC_CH_SMALL_AUGER, g_disp_params.mid_speed);
                AUGER_B_Set(1, 0, 0);
                DAC7512_Write(DAC_CH_BIG_AUGER, g_disp_params.mid_speed);
            }
        } else {
            /* 已出重量 < 50% -> 高速 (对应旧版 MEDCINE_STEP=3) */
            if (g_disp_status != DISP_HIGH_SPEED) {
                g_disp_status = DISP_HIGH_SPEED;
                /* 大小绞龙高速运行 */
                AUGER_S_Set(1, 0, 0);
                DAC7512_Write(DAC_CH_SMALL_AUGER, g_disp_params.high_speed);
                AUGER_B_Set(1, 0, 0);
                DAC7512_Write(DAC_CH_BIG_AUGER, g_disp_params.high_speed);
            }
        }

        /* 防悬空电机周期性动作 -- 与旧版一致: 约5ms周期脉冲 */
        if ((sysTickCount % 5) < 3) {
            MOTOR_DC1_Set(1, 0);
            MOTOR_DC2_Set(1, 0);
        } else {
            MOTOR_DC1_Set(0, 0);
            MOTOR_DC2_Set(0, 0);
        }
    }

    /* 料斗门自动关闭 */
    if (s_door_is_open) {
        uint32_t door_elapsed = sysTickCount - s_door_open_time;
        /* 先按时间关闭 */
        if (door_elapsed > (uint32_t)g_disp_params.door_time * 1000) {
            MOTOR_DC3_Set(0, 0);  /* 关门 */
            /* 如有门到位传感器, 等待确认关到位 (最多额外等500ms) */
            if (SENSOR_ReadDoor()) {
                /* 门到位传感器有效 (有料=1/无料=1, 根据实际硬件确认) */
                /* 当前仅依赖时间, 如需硬件确认可在此添加: */
                /* if (door_elapsed > g_disp_params.door_time * 1000 + 500) s_door_is_open = 0; */
            }
            s_door_is_open = 0;
        }
    }

    /* 续药逻辑: 超时后如果药未出完, 自动重新启动 */
    if (s_timeout_need_resume && (g_disp_status == DISP_IDLE)) {
        s_timeout_need_resume = 0;
        uint16_t remaining = (g_disp_params.target_weight > g_disp_params.current_weight)
                             ? (g_disp_params.target_weight - g_disp_params.current_weight)
                             : 0;
        if (remaining > g_disp_params.auger_weight_diff) {
            /* 自动重新启动出药, 继续出剩余的药量 */
            DISPENSER_Start(remaining);
        }
    }
}

/**
  * @brief  开始出药
  */
void DISPENSER_Start(uint16_t weight)
{
    if (g_disp_status != DISP_IDLE) return;

    g_disp_params.target_weight = weight;
    g_disp_status = DISP_HIGH_SPEED;
    s_dispense_start_time = sysTickCount;
    s_last_auger_check = sysTickCount;
    s_last_weight = 0;
    s_complete_confirm_cnt = 0;  /* 重置完成确认计数器 */

    /* 打开门 */
    MOTOR_DC3_Set(1, 0);
    s_door_is_open = 1;
    s_door_open_time = sysTickCount;

    /* 启动大绞龙高速, 设置 DAC 输出 */
    AUGER_B_Set(1, 0, 0);
    DAC7512_Write(DAC_CH_BIG_AUGER, g_disp_params.high_speed);

    /* 启动小绞龙, 设置 DAC 输出 */
    AUGER_S_Set(1, 0, 0);
    DAC7512_Write(DAC_CH_SMALL_AUGER, g_disp_params.high_speed);
}

/**
  * @brief  停止出药
  */
void DISPENSER_Stop(void)
{
    MOTOR_DC1_Set(0, 0);
    MOTOR_DC2_Set(0, 0);
    MOTOR_DC3_Set(0, 0);
    MOTOR_DC4_Set(0, 0);
    AUGER_S_Set(0, 0, 1);
    AUGER_B_Set(0, 0, 1);

    /* 关闭 DAC 输出 (模拟电压置 0) */
    DAC7512_SetZeroAll();

    if (g_disp_status != DISP_TIMEOUT) {
        g_disp_status = DISP_IDLE;
    }
}

/**
  * @brief  急停
  */
void DISPENSER_EmergencyStop(void)
{
    DISPENSER_Stop();
    g_disp_status = DISP_IDLE;
    g_dispense_cmd = 0;
}

/**
  * @brief  手动开门
  */
void DISPENSER_DoorOpen(void)
{
    MOTOR_DC3_Set(1, 0);
    s_door_is_open = 1;
    s_door_open_time = sysTickCount;
}

/**
  * @brief  手动振动 (启动防悬空电机)
  */
void DISPENSER_Vibrate(void)
{
    MOTOR_DC1_Set(1, 0);
    MOTOR_DC2_Set(1, 0);
    delay_ms(500);
    MOTOR_DC1_Set(0, 0);
    MOTOR_DC2_Set(0, 0);
}

/**
  * @brief  一键清空 (快速倒药)
  */
void DISPENSER_Clear(void)
{
    DISPENSER_Stop();
    /* 打开门 */
    MOTOR_DC3_Set(1, 0);
    s_door_is_open = 1;
    s_door_open_time = sysTickCount;
    /* 启动绞龙反转排空 */
    AUGER_B_Set(1, 1, 0);
    delay_ms(3000);
    DISPENSER_Stop();
}

/**
  * @brief  获取状态字节
  * @retval bit7: 门状态(1=关,0=开), bit3: 高料位, bit2: 中料位, bit1: 低料位, bit0: 缺料
  */
uint8_t DISPENSER_GetStatusByte(void)
{
    uint8_t status = 0;

    /* 门状态: 0=开门, 1=关门 (协议规定) */
    if (!s_door_is_open) status |= 0x80;

    /* 料位传感器: 有料=0, 无料=1 (协议规定)
     * 协议定义: bit0=缺料, bit1=中位, bit2=高位, bit3=关门到位(未安装)
     */
    if (SENSOR_ReadHigh()) status |= 0x04;  /* bit2: 高位无料 */
    if (SENSOR_ReadMid())  status |= 0x02;  /* bit1: 中位无料 */
    if (SENSOR_ReadLow())  status |= 0x01;  /* bit0: 缺料/低位无料 */
    /* bit3: 关门到位传感器，未安装，保持0 */

    return status;
}

/**
  * @brief  获取当前重量 (通过 ADS1256 读取)
  */
uint16_t DISPENSER_GetWeight(void)
{
    return ADS1256_ReadWeight();
}

/**
  * @brief  设置参数
  */
void DISPENSER_SetParam(uint8_t param_type, uint8_t *data, uint16_t len)
{
    if (len < 1) return;

    switch (param_type) {
        case CFG_DATA_MEDCINE_SET:   /* 药量 */
            if (len >= 2) g_disp_params.target_weight = ((uint16_t)data[0] << 8) | data[1];
            break;
        case CFG_SPEED_MODE_H_SET:   /* 高速 */
            if (len >= 2) g_disp_params.high_speed = ((uint16_t)data[0] << 8) | data[1];
            break;
        case CFG_SPEED_MODE_M_SET:   /* 中速 */
            if (len >= 2) g_disp_params.mid_speed = ((uint16_t)data[0] << 8) | data[1];
            break;
        case CFG_SPEED_MODE_L_SET:   /* 低速 */
            if (len >= 2) g_disp_params.low_speed = ((uint16_t)data[0] << 8) | data[1];
            break;
        case SAMP_PERCENT_CFG_REQ:   /* 速度百分比 */
            g_disp_params.speed_percent = data[0];
            break;
        case CFG_DOOR_TIME_REQ:      /* 开门时间 */
            g_disp_params.door_time = data[0];
            break;
        case CFG_TIMEOUT_REQ:        /* 超时时间 */
            g_disp_params.timeout = data[0];
            break;
        case CFG_AUGER_TIME_REQ:     /* 绞龙确认时间 */
            if (len >= 2) g_disp_params.auger_check_time = ((uint16_t)data[0] << 8) | data[1];
            break;
        case CFG_AUGER_WEIGHT_REQ:   /* 绞龙差值 */
            if (len >= 2) g_disp_params.auger_weight_diff = ((uint16_t)data[0] << 8) | data[1];
            break;
        case CFG_LOW_WEIGHT_REQ:     /* 花草型低速阈值 */
            if (len >= 2) g_disp_params.low_weight_thresh = ((uint16_t)data[0] << 8) | data[1];
            break;
        case CFG_WEIGHT_LOW_REQ: {   /* 标定零重量 (0x1D) */
            if (len >= 2) {
                g_disp_params.cali_zero_weight = ((uint16_t)data[0] << 8) | data[1];
            }
            /* 记录当前 ADC 原始值为零点 */
            int32_t zero_adc = ADS1256_ReadRaw();
            ADS1256_SetZeroCalibration(zero_adc);
            break;
        }
        case CFG_WEIGHT_HIGH_REQ: {  /* 标定高重量 (0x1C) */
            if (len >= 2) {
                g_disp_params.cali_high_weight = ((uint16_t)data[0] << 8) | data[1];
            }
            /* 记录当前 ADC 原始值为满量程点 */
            int32_t full_adc = ADS1256_ReadRaw();
            ADS1256_SetFullScaleCalibration(full_adc, g_disp_params.cali_high_weight);
            break;
        }
        case STIR_SPEED_REQ:         /* 搅拌速度 */
            g_disp_params.stir_speed = data[0];
            break;
        default:
            break;
    }

    /* 参数修改后自动保存到 Flash */
    PARAMS_Save();
}
