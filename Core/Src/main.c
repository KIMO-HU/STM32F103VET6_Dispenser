/**
  ******************************************************************************
  * @file    main.c
  * @brief   中药机终端主程序 (裸机架构)
  *          基于 STM32F103VET6, 72MHz, 配合 KZB-ZYFYJ-HCX-V1.4 硬件
  ******************************************************************************
  */

#include "main.h"
#include "led_indicator.h"
#include "watchdog.h"

/**
  * @brief  主函数
  * @note   sysTickCount 定义在 system_clock.c 中
  */
int main(void)
{
    /* 0. 设置向量表偏移到 App 区域 */
    SCB->VTOR = 0x08008000;

    /* 1. 配置系统时钟 72MHz */
    SystemClock_Config();

    /* 2. 初始化 SysTick 1ms 中断 */
    SysTick_Init();

    /* 2.5 初始化独立看门狗 (超时~2秒) */
    IWDG_Init();

    /* 3. 初始化所有 GPIO (传感器、电机、RS485方向、LED) */
    GPIO_Init_All();

    /* 3.5 初始化 DAC7512 双路模拟输出 (SPI1=小绞龙, SPI3=大绞龙) */
    DAC7512_Init();

    /* 4. 初始化 UART4 (RS485, 115200) - 中药机主通信 */
    UART4_Init(115200);

    /* 5. 初始化 USART3 (RS485, 115200) - 备用/扩展通信 */
    USART3_Init(115200);

    /* 6. 初始化 dispenser 控制逻辑 */
    DISPENSER_Init();

    /* 6.5 初始化料位LED指示器 */
    LED_Indicator_Init();

    /* 7. 使能全局中断 */
    __enable_irq();

    /* 启动指示: LED 闪烁 3 次 */
    for (int i = 0; i < 3; i++) {
        LED_Set(0, 1);
        delay_ms(100);
        LED_Set(0, 0);
        delay_ms(100);
    }

    /* 主循环 */
    while (1)
    {
        /* ---- 通信处理 ---- */
        uint8_t ch;
        if (UART_GetChar(UART_IDX_UART4, &ch)) {
            Protocol_ParseFrame(&ch, 1);
        }

        /* ---- 出药控制状态机 ---- */
        DISPENSER_Process();

        /* ---- 料位LED指示器更新 ---- */
        LED_Indicator_Update();

        /* ---- LED 心跳 (每 1 秒闪一次) ---- */
        if ((sysTickCount % 1000) < 50) {
            LED_Set(1, 1);
        } else {
            LED_Set(1, 0);
        }

        /* ---- 喂看门狗 ---- */
        IWDG_Feed();
    }
}

/**
  * @brief  错误处理函数
  */
void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
        LED_Set(0, 1);
        delay_ms(100);
        LED_Set(0, 0);
        delay_ms(100);
    }
}
