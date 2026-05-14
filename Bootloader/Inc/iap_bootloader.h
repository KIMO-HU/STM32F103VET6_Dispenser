/**
  ******************************************************************************
  * @file    iap_bootloader.h
  * @brief   Bootloader IAP 协议处理头文件
  ******************************************************************************
  */

#ifndef __IAP_BOOTLOADER_H__
#define __IAP_BOOTLOADER_H__

#include <stdint.h>

/* Flash 分区定义 */
#define BOOTLOADER_START_ADDR   0x08000000
#define BOOTLOADER_SIZE         0x00008000      /* 32KB */
#define APP_START_ADDR          0x08008000
#define APP_MAX_SIZE            0x00078000      /* 480KB */
#define FLASH_PAGE_SIZE         2048            /* 2KB/页 */

/* IAP 标志 (存放在 Bootloader 最后一页倒数第二个字) */
#define IAP_FLAG_ADDR           0x08007FF0
#define IAP_MAGIC_ENTER         0x5A5A5A5A
#define IAP_MAGIC_OK            0xA5A5A5A5
#define IAP_MAGIC_NONE          0xFFFFFFFF

/* 升级状态 */
typedef enum {
    IAP_ST_IDLE = 0,
    IAP_ST_READY,           /* 收到 0x70/0x71，等待准备帧 */
    IAP_ST_RECEIVING,       /* 收到 0x72，正在接收数据 */
    IAP_ST_VERIFY,          /* 收到 0x74，校验中 */
    IAP_ST_COMPLETE,        /* 升级完成 */
    IAP_ST_ERROR,
} iap_state_t;

/* IAP 上下文 */
typedef struct {
    iap_state_t state;
    uint32_t    total_size;     /* 固件总大小 */
    uint16_t    packet_size;    /* 每包大小 */
    uint16_t    total_packets;  /* 总包数 */
    uint16_t    rx_packets;     /* 已接收包数 */
    uint32_t    rx_bytes;       /* 已接收字节数 */
    uint16_t    fw_crc;         /* 固件 CRC */
    uint32_t    last_addr;      /* 最后写入地址 */
    uint8_t     error_code;     /* 错误码 */
} iap_ctx_t;

extern iap_ctx_t g_iap_ctx;

/* 主流程 */
void IAP_Run(void);
void IAP_EnterMode(void);
void IAP_ProcessPacket(uint8_t cmd, uint8_t *data, uint16_t len);
void IAP_JumpToApp(uint32_t addr);

/* 标志操作 */
void IAP_SetFlag(uint32_t magic);
uint32_t IAP_ReadFlag(void);
void IAP_ClearFlag(void);

/* App 有效性检查 */
int IAP_CheckAppValid(uint32_t addr);

#endif /* __IAP_BOOTLOADER_H__ */
