/**
  ******************************************************************************
  * @file    led_indicator.c
  * @brief   料位LED指示灯控制
  *          根据料位传感器状态发送对应颜色到外部LED控制器
  *          协议帧格式兼容旧版 software_V1.0 (protocolSAC.c)
  *
  *  帧格式 (21字节单点 / 114字节LED条):
  *    [0xDD][0x55][0xEE]      - 帧头
  *    [0x00][0x00]            - 源地址
  *    [0x00][0x01]            - 设备地址
  *    [0x00]                  - 端口号
  *    [0x99]                  - 指令类型: 显示
  *    [0x01]                  - 灯串编号
  *    [0x00][0x00]            - 保留
  *    [LEN_HI][LEN_LO]        - 数据长度 (3=单点, 0x60=32点LED条)
  *    [0x00][0x20]            - 扩展字段
  *    [R,G,B] x N             - RGB颜色数据
  *    [0xAA][0xBB]            - 帧尾
  *
  *  料位状态 → LED颜色映射 (与旧版 applicationSAC.c 保持一致):
  *    全空 (000)              → RED        红色 (紧急缺料)
  *    仅低位 (001)            → 33%BLUE    33%蓝 + 66%黑
  *    低位+中位 (011)         → 66%BLUE    66%蓝 + 33%黑
  *    全满 (111)              → WATHETBLUE 浅蓝色 (料满)
  *    低位+高位 (101)         → 66%BLUE    66%蓝 + 33%黑
  *    仅中位 (010)            → 33%BLUE    33%蓝 + 66%黑
  *    仅高位 (100)            → 33%BLUE    33%蓝 + 66%黑
  ******************************************************************************
  */

#include "led_indicator.h"
#include "gpio_ctrl.h"
#include "usart_ctrl.h"
#include "dispenser.h"

/* LED控制器帧常量 */
#define LED_FRAME_HEAD1     0xDD
#define LED_FRAME_HEAD2     0x55
#define LED_FRAME_HEAD3     0xEE
#define LED_FRAME_TAIL1     0xAA
#define LED_FRAME_TAIL2     0xBB
#define LED_CMD_DISPLAY     0x99

/* RUO模式亮度系数 (旧版实际使用弱化亮度) */
#define LED_RUO_R           0x00
#define LED_RUO_G           0x10
#define LED_RUO_B           0x10

/* 全亮度颜色 */
#define LED_RED_R           0xFF
#define LED_RED_G           0x00
#define LED_RED_B           0x00

#define LED_BLUE_R          0x00
#define LED_BLUE_G          0x00
#define LED_BLUE_B          0xFF

#define LED_GREEN_R         0x00
#define LED_GREEN_G         0xFF
#define LED_GREEN_B         0x00

#define LED_BLACK_R         0x00
#define LED_BLACK_G         0x00
#define LED_BLACK_B         0x00

/* 帧缓冲区 */
static uint8_t s_led_frame[114];

/**
  * @brief  填充21字节单点帧
  */
static void led_build_single_frame(uint8_t r, uint8_t g, uint8_t b)
{
    s_led_frame[0]  = LED_FRAME_HEAD1;
    s_led_frame[1]  = LED_FRAME_HEAD2;
    s_led_frame[2]  = LED_FRAME_HEAD3;
    s_led_frame[3]  = 0x00;     /* 源地址高 */
    s_led_frame[4]  = 0x00;     /* 源地址低 */
    s_led_frame[5]  = 0x00;     /* 设备地址高 */
    s_led_frame[6]  = 0x01;     /* 设备地址低 */
    s_led_frame[7]  = 0x00;     /* 端口号 */
    s_led_frame[8]  = LED_CMD_DISPLAY;
    s_led_frame[9]  = 0x01;     /* 灯串编号 */
    s_led_frame[10] = 0x00;     /* 保留 */
    s_led_frame[11] = 0x00;     /* 保留 */
    s_led_frame[12] = 0x00;     /* 数据长度高 */
    s_led_frame[13] = 0x03;     /* 数据长度低 = 3字节 */
    s_led_frame[14] = 0x00;     /* 扩展 */
    s_led_frame[15] = 0x20;     /* 扩展 */
    s_led_frame[16] = r;        /* R */
    s_led_frame[17] = g;        /* G */
    s_led_frame[18] = b;        /* B */
    s_led_frame[19] = LED_FRAME_TAIL1;
    s_led_frame[20] = LED_FRAME_TAIL2;
}

/**
  * @brief  填充114字节LED条帧 (32个RGB点)
  * @param  blue_cnt: 蓝色(或暗青色)LED数量 (0~32)
  * @param  dim: 1=使用RUO弱化亮度, 0=全亮度
  */
static void led_build_strip_frame(uint8_t blue_cnt, uint8_t dim)
{
    uint8_t i;
    uint8_t r_on = dim ? LED_RUO_R : LED_BLUE_R;
    uint8_t g_on = dim ? LED_RUO_G : (dim ? LED_RUO_G : LED_GREEN_G);
    uint8_t b_on = dim ? LED_RUO_B : LED_BLUE_B;

    /* 帧头 */
    s_led_frame[0]  = LED_FRAME_HEAD1;
    s_led_frame[1]  = LED_FRAME_HEAD2;
    s_led_frame[2]  = LED_FRAME_HEAD3;
    s_led_frame[3]  = 0x00;
    s_led_frame[4]  = 0x00;
    s_led_frame[5]  = 0x00;
    s_led_frame[6]  = 0x01;
    s_led_frame[7]  = 0x00;
    s_led_frame[8]  = LED_CMD_DISPLAY;
    s_led_frame[9]  = 0x01;
    s_led_frame[10] = 0x00;
    s_led_frame[11] = 0x00;
    s_led_frame[12] = 0x00;     /* 数据长度高 */
    s_led_frame[13] = 0x60;     /* 数据长度低 = 96 = 32*3 */
    s_led_frame[14] = 0x00;
    s_led_frame[15] = 0x01;

    /* 前 (32-blue_cnt) 个LED = 黑色 */
    for (i = 0; i < (32 - blue_cnt); i++) {
        s_led_frame[16 + i*3 + 0] = LED_BLACK_R;
        s_led_frame[16 + i*3 + 1] = LED_BLACK_G;
        s_led_frame[16 + i*3 + 2] = LED_BLACK_B;
    }
    /* 后 blue_cnt 个LED = 蓝色/暗青色 */
    for (; i < 32; i++) {
        s_led_frame[16 + i*3 + 0] = r_on;
        s_led_frame[16 + i*3 + 1] = g_on;
        s_led_frame[16 + i*3 + 2] = b_on;
    }

    s_led_frame[112] = LED_FRAME_TAIL1;
    s_led_frame[113] = LED_FRAME_TAIL2;
}

/**
  * @brief  发送LED帧 (通过USART3)
  * @param  len: 帧长度 (21 或 114)
  */
static void led_send_frame(uint16_t len)
{
    UART_PutBuf(UART_IDX_USART3, s_led_frame, len);
}

void LED_Indicator_Init(void)
{
    /* USART3 已在 main.c 中初始化，此处无需额外操作 */
}

void LED_Indicator_SendColor(uint8_t r, uint8_t g, uint8_t b, uint8_t dim)
{
    if (dim) {
        r = (r == 0) ? 0 : LED_RUO_R;
        g = (g == 0) ? 0 : LED_RUO_G;
        b = (b == 0) ? 0 : LED_RUO_B;
    }
    led_build_single_frame(r, g, b);
    led_send_frame(21);
}

void LED_Indicator_Update(void)
{
    static uint8_t s_last_sensor = 0xFF;
    static uint8_t s_first_run = 1;

    /* 读取当前料位状态
     * 传感器有料=1, 无料=0
     * bit0=低位, bit1=中位, bit2=高位
     */
    uint8_t low  = SENSOR_ReadLow()  ? 1 : 0;
    uint8_t mid  = SENSOR_ReadMid()  ? 1 : 0;
    uint8_t high = SENSOR_ReadHigh() ? 1 : 0;
    uint8_t sensor = (low << 0) | (mid << 1) | (high << 2);

    /* 首次运行或状态变化时才发送 */
    if (!s_first_run && (sensor == s_last_sensor)) {
        return;
    }
    s_first_run = 0;
    s_last_sensor = sensor;

    /* 根据料位状态发送对应LED颜色
     * 映射关系与旧版 applicationSAC.c 保持一致
     */
    switch (sensor & 0x07) {

        case 0x00:  /* 全空: 低位=0, 中位=0, 高位=0 → RED 红色 */
            led_build_single_frame(LED_RED_R, LED_RED_G, LED_RED_B);
            led_send_frame(21);
            break;

        case 0x01:  /* 仅低位有料 → 33%蓝色 (约10/32个LED亮) */
            led_build_strip_frame(10, 1);  /* RUO弱化亮度 */
            led_send_frame(114);
            break;

        case 0x03:  /* 低位+中位有料 → 66%蓝色 (约22/32个LED亮) */
            led_build_strip_frame(22, 1);
            led_send_frame(114);
            break;

        case 0x07:  /* 全满: 低位=1, 中位=1, 高位=1 → WATHETBLUE 浅蓝色 */
            led_build_single_frame(LED_RUO_R, LED_RUO_G, LED_RUO_B);
            led_send_frame(21);
            break;

        case 0x05:  /* 低位+高位有料 (中位无) → 66%蓝色 */
            led_build_strip_frame(22, 1);
            led_send_frame(114);
            break;

        case 0x02:  /* 仅中位有料 → 33%蓝色 */
            led_build_strip_frame(10, 1);
            led_send_frame(114);
            break;

        case 0x04:  /* 仅高位有料 → 33%蓝色 */
            led_build_strip_frame(10, 1);
            led_send_frame(114);
            break;

        case 0x06:  /* 中位+高位有料 (低位无) → 33%蓝色 */
            led_build_strip_frame(10, 1);
            led_send_frame(114);
            break;

        default:
            break;
    }
}
