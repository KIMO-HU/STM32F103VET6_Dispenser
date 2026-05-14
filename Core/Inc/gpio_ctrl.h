/**
  ******************************************************************************
  * @file    gpio_ctrl.h
  * @brief   GPIO 控制头文件 - 中药机硬件引脚定义
  ******************************************************************************
  */

#ifndef __GPIO_CTRL_H__
#define __GPIO_CTRL_H__

#include "stm32f103xe.h"
#include <stdint.h>

/* ========================================================================
   UART 索引定义
   ======================================================================== */
#define UART_IDX_USART3     0
#define UART_IDX_UART4      1

/* ========================================================================
   RS485 方向控制引脚
   ======================================================================== */
/* UART4 RS485 (中药机主通信接口) */
#define RS485_UART4_RE_PIN      GPIO_Pin_3      /* PD3: /RE */
#define RS485_UART4_DE_PIN      GPIO_Pin_4      /* PD4: DE  */
#define RS485_UART4_GPIO        GPIOD

/* USART3 RS485 */
#define RS485_USART3_RE_PIN     GPIO_Pin_15     /* PE15: /RE */
#define RS485_USART3_DE_PIN     GPIO_Pin_14     /* PE14: DE  */
#define RS485_USART3_GPIO       GPIOE

/* ========================================================================
   传感器输入引脚 (基于现有软件代码分析)
   ======================================================================== */
/* 料位传感器 */
#define SENSOR_LOW_PIN          GPIO_Pin_0      /* PD0: 低位传感器 */
#define SENSOR_MID_PIN          GPIO_Pin_6      /* PD6: 中位传感器 */
#define SENSOR_HIGH_PIN         GPIO_Pin_12     /* PD12: 高位传感器 */
#define SENSOR_GPIO             GPIOD

/* 入药/门状态传感器 */
#define MATERIAL_INPUT_PIN      GPIO_Pin_3      /* PC3: 入药传感器 */
#define MATERIAL_DOOR_PIN       GPIO_Pin_4      /* PC4: 门状态传感器 */
#define MATERIAL_GPIO           GPIOC

/* 料斗舱门旋转到位 / 入药舱门旋转到位 */
#define DOOR_ROTATE_OK_PIN      GPIO_Pin_12     /* PC12 */
#define INPUT_ROTATE_OK_PIN     GPIO_Pin_13     /* PC13 */
#define ROTATE_GPIO             GPIOC

/* ========================================================================
   电机控制输出引脚 (DRV8870 H桥驱动)
   ======================================================================== */
/* DC1: 发料防悬空电机1 */
#define DC1_IN1_PIN             GPIO_Pin_8      /* PA8 */
#define DC1_IN2_PIN             GPIO_Pin_9      /* PA9 */
#define DC1_GPIO                GPIOA

/* DC2: 料仓防悬空电机2 */
#define DC2_IN1_PIN             GPIO_Pin_6      /* PC6 */
#define DC2_IN2_PIN             GPIO_Pin_7      /* PC7 */
#define DC2_GPIO                GPIOC

/* DC3: 称重料斗开门/关门 */
#define DC3_IN1_PIN             GPIO_Pin_0      /* PB0 */
#define DC3_IN2_PIN             GPIO_Pin_1      /* PB1 */
#define DC3_GPIO                GPIOB

/* DC4: 入药电机 -- 与旧版一致 */
#define DC4_IN1_PIN             GPIO_Pin_8      /* PD8 */
#define DC4_IN2_PIN             GPIO_Pin_9      /* PD9 */
#define DC4_GPIO                GPIOD

/* ========================================================================
   绞龙驱动控制引脚 (BLDC 驱动器接口)
   ======================================================================== */
/* 小绞龙 (Small Auger) -- 与旧版软件保持一致 */
#define S_EN_PIN                GPIO_Pin_10     /* PA10: 使能 (旧版定义) */
#define S_FR_PIN                GPIO_Pin_1      /* PD1: 正反转 */
#define S_BRK_PIN               GPIO_Pin_5      /* PD5: 刹车 */
#define S_SV_PIN                GPIO_Pin_0      /* PA0: 调速 (PWM/TIM5_CH1) */

/* 大绞龙 (Big Auger) -- 与旧版软件保持一致 */
#define B_EN_PIN                GPIO_Pin_3      /* PA3: 使能 */
#define B_FR_PIN                GPIO_Pin_2      /* PD2: 正反转 (旧版定义) */
#define B_BRK_PIN               GPIO_Pin_6      /* PB6: 刹车 */
#define B_SV_PIN                GPIO_Pin_1      /* PA1: 调速 (PWM/TIM2_CH2) */

/* ========================================================================
   LED 指示引脚
   ======================================================================== */
#define LED1_PIN                GPIO_Pin_10     /* PE10 */
#define LED2_PIN                GPIO_Pin_11     /* PE11 */
#define LED_GPIO                GPIOE

/* ========================================================================
   DAC7512 模拟输出引脚 (SPI1 + SPI3)
   ======================================================================== */
/* DAC1 (小绞龙) - SPI1 */
#define DAC1_CS_PIN             GPIO_Pin_4      /* PA4: SYNC/CS */
#define DAC1_SCLK_PIN           GPIO_Pin_5      /* PA5: SCLK */
#define DAC1_MOSI_PIN           GPIO_Pin_7      /* PA7: DIN */
#define DAC1_GPIO               GPIOA

/* DAC2 (大绞龙) - SPI3 */
#define DAC2_CS_PIN             GPIO_Pin_15     /* PA15: SYNC/CS */
#define DAC2_SCLK_PIN           GPIO_Pin_3      /* PB3: SCLK */
#define DAC2_MOSI_PIN           GPIO_Pin_5      /* PB5: DIN */
#define DAC2_CS_GPIO            GPIOA
#define DAC2_SPI_GPIO           GPIOB

/* ========================================================================
   拨码开关输入 (Board ID 等)
   ======================================================================== */
/* BSIN1-16 用于设置板卡地址等，此处简化处理 */

/* ========================================================================
   函数声明
   ======================================================================== */
void GPIO_Init_All(void);
void RS485_SetDirection(uint8_t uart_idx, uint8_t tx_mode);

uint8_t SENSOR_ReadLow(void);
uint8_t SENSOR_ReadMid(void);
uint8_t SENSOR_ReadHigh(void);
uint8_t SENSOR_ReadInput(void);
uint8_t SENSOR_ReadDoor(void);

void MOTOR_DC1_Set(uint8_t in1, uint8_t in2);
void MOTOR_DC2_Set(uint8_t in1, uint8_t in2);
void MOTOR_DC3_Set(uint8_t in1, uint8_t in2);
void MOTOR_DC4_Set(uint8_t in1, uint8_t in2);

void AUGER_S_Set(uint8_t en, uint8_t fr, uint8_t brk);
void AUGER_B_Set(uint8_t en, uint8_t fr, uint8_t brk);

void LED_Set(uint8_t led_idx, uint8_t state);

#endif /* __GPIO_CTRL_H__ */
