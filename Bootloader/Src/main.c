/**
  ******************************************************************************
  * @file    main.c
  * @brief   Bootloader 主程序
  *          上电流程:
  *          1. 检查 IAP 标志 (Flash 0x08007FF0)
  *          2. 若标志 == 0x5A5A5A5A，直接进入 IAP 升级模式
  *          3. 否则等待 2 秒，检测是否收到 IAP 进入指令 (0x70/0x71)
  *          4. 若收到，进入 IAP 模式
  *          5. 若未收到且 App 有效，跳转到 App (0x08008000)
  ******************************************************************************
  */

#include <stdint.h>
#include "stm32f103xe.h"
#include "system_clock.h"
#include "gpio_ctrl.h"
#include "usart_ctrl.h"
#include "iap_bootloader.h"

/* 超时等待时间 (ms) */
#define BOOT_WAIT_TIMEOUT_MS    2000

/* LED 指示 */
#define LED_BLINK_FAST_MS       50
#define LED_BLINK_SLOW_MS       500

extern void UART4_IRQHandler(void);

/**
  * @brief  主函数
  */
int main(void)
{
    /* 1. 基础初始化 */
    SystemClock_Config();
    SysTick_Init();
    GPIO_Init_All();
    UART4_Init(115200);
    __enable_irq();

    /* 2. 检查 IAP 强制标志 */
    uint32_t iap_flag = IAP_ReadFlag();

    if (iap_flag == IAP_MAGIC_ENTER) {
        /* 强制进入 IAP 模式 */
        IAP_ClearFlag();
        IAP_EnterMode();
        IAP_Run();
        while (1);
    }

    /* 3. 未强制升级，等待上位机指令 */
    uint32_t start_tick = sysTickCount;
    uint8_t enter_iap = 0;

    /* 快速闪烁 LED 表示 Bootloader 运行中 */
    while ((sysTickCount - start_tick) < BOOT_WAIT_TIMEOUT_MS) {
        LED_Set(0, ((sysTickCount / LED_BLINK_FAST_MS) % 2));

        /* 检查是否收到 IAP 进入指令 */
        uint8_t ch;
        if (UART_GetChar(UART_IDX_UART4, &ch)) {
            /* 简单检测: 收到 0x55 0xBB 0x70 或 0x71 序列 */
            static uint8_t s_detect_state = 0;
            switch (s_detect_state) {
                case 0:
                    if (ch == 0x55) s_detect_state = 1;
                    break;
                case 1:
                    if (ch == 0xBB) s_detect_state = 2;
                    else s_detect_state = (ch == 0x55) ? 1 : 0;
                    break;
                case 2:
                    if (ch == 0x70 || ch == 0x71) {
                        enter_iap = 1;
                    }
                    s_detect_state = 0;
                    break;
            }
        }

        if (enter_iap) {
            break;
        }
    }

    LED_Set(0, 0);

    if (enter_iap) {
        /* 清空接收缓冲区，进入 IAP 协议处理 */
        UART_FlushRx(UART_IDX_UART4);
        IAP_EnterMode();
        IAP_Run();
        while (1);
    }

    /* 4. 检查 App 是否有效 */
    if (IAP_CheckAppValid(APP_START_ADDR)) {
        /* 设置向量表偏移并跳转 */
        IAP_JumpToApp(APP_START_ADDR);
    }

    /* 5. App 无效，停留在 Bootloader 慢闪指示 */
    IAP_EnterMode();
    while (1) {
        LED_Set(0, ((sysTickCount / LED_BLINK_SLOW_MS) % 2));
        IAP_Run();  /* 持续等待升级指令 */
    }
}
