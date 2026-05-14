/**
  ******************************************************************************
  * @file    iap_bootloader.c
  * @brief   Bootloader IAP 协议处理与 Flash 编程
  ******************************************************************************
  */

#include <stddef.h>
#include "iap_bootloader.h"
#include "flash.h"
#include "usart_ctrl.h"
#include "system_clock.h"

/* 协议定义 (与 App 一致) */
#define COMPROTOCOLHEAD1        0x55
#define COMPROTOCOLHEAD2        0xBB

#define IAP_INTEGRAL_START_REQ      0x70
#define IAP_SINGAL_UNIT_START_REQ   0x71
#define IAP_SINGAL_PACKEG_START_REQ 0x72
#define IAP_SINGAL_PACKEG_REQ       0x73
#define IAP_SIGNAL_PACKEG_END_REQ   0x74
#define IAP_SINGAL_UNIT_END_REQ     0x75
#define IAP_INTEGRAL_END_REQ        0x76
#define IAP_STATUS_INQUIRE_REQ      0x77
#define IAP_VERSION_INQUIRE_REQ     0x78

#define IAP_INTEGRAL_START_RSP      0xF0
#define IAP_SINGAL_UNIT_START_RSP   0xF1
#define IAP_SINGAL_PACKEG_START_RSP 0xF2
#define IAP_SINGAL_PACKEG_RSP       0xF3
#define IAP_SIGNAL_PACKEG_END_RSP   0xF4
#define IAP_SINGAL_UNIT_END_RSP     0xF5
#define IAP_INTEGRAL_END_RSP        0xF6
#define IAP_STATUS_INQUIRE_RSP      0xF7
#define IAP_VERSION_INQUIRE_RSP     0xF8

#define BOOTLOADER_VERSION_MAJOR    1
#define BOOTLOADER_VERSION_MINOR    0

iap_ctx_t g_iap_ctx = {0};

/* 本地缓冲区 */
static uint8_t s_tx_buf[64];

/* 计算协议异或校验 */
static uint8_t calc_bcc(uint8_t *data, uint16_t len)
{
    uint8_t crc = 0;
    for (uint16_t i = 0; i < len; i++) crc ^= data[i];
    return crc;
}

/**
  * @brief  Bootloader 发送响应帧
  */
static void iap_send_response(uint8_t type, uint8_t subtype, uint8_t *data, uint16_t len)
{
    uint16_t pos = 0;
    uint16_t len_field = ((subtype & 0x07) << 13) | (len & 0x1FFF);

    s_tx_buf[pos++] = COMPROTOCOLHEAD1;
    s_tx_buf[pos++] = COMPROTOCOLHEAD2;
    s_tx_buf[pos++] = type;
    s_tx_buf[pos++] = 0x01;     /* REV1: board ID */
    s_tx_buf[pos++] = 0x01;     /* REV2: machine ID */
    s_tx_buf[pos++] = (len_field >> 8) & 0xFF;
    s_tx_buf[pos++] = len_field & 0xFF;

    for (uint16_t i = 0; i < len; i++) {
        s_tx_buf[pos++] = data[i];
    }
    s_tx_buf[pos] = calc_bcc(&s_tx_buf[2], pos - 2);
    pos++;

    UART_PutBuf(UART_IDX_UART4, s_tx_buf, pos);
}

/**
  * @brief  擦除 App 区域的所有页
  */
static flash_status_t iap_erase_app_area(uint32_t size)
{
    flash_status_t status;
    uint32_t pages = (size + FLASH_PAGE_SIZE - 1) / FLASH_PAGE_SIZE;
    if (pages == 0) pages = APP_MAX_SIZE / FLASH_PAGE_SIZE;
    if (pages > (APP_MAX_SIZE / FLASH_PAGE_SIZE)) pages = APP_MAX_SIZE / FLASH_PAGE_SIZE;

    FLASH_Unlock();
    FLASH_ClearFlags();

    for (uint32_t i = 0; i < pages; i++) {
        uint32_t addr = APP_START_ADDR + i * FLASH_PAGE_SIZE;
        status = FLASH_ErasePage(addr);
        if (status != FLASH_OK) {
            FLASH_Lock();
            return status;
        }
    }
    return FLASH_OK;
}

/**
  * @brief  写入数据到 Flash (App 区域)
  * @note   addr 是相对于 APP_START_ADDR 的偏移
  */
static flash_status_t iap_write_flash(uint32_t offset, uint8_t *data, uint16_t len)
{
    flash_status_t status = FLASH_OK;
    uint32_t addr = APP_START_ADDR + offset;

    if (addr < APP_START_ADDR || (addr + len) > (APP_START_ADDR + APP_MAX_SIZE)) {
        return FLASH_ERROR;
    }

    /* 按半字写入 */
    for (uint16_t i = 0; i < len; i += 2) {
        uint16_t hw = data[i];
        if ((i + 1) < len) {
            hw |= ((uint16_t)data[i + 1] << 8);
        } else {
            hw |= 0xFF00;  /* 奇数字节，高字节补 0xFF */
        }
        status = FLASH_ProgramHalfWord(addr + i, hw);
        if (status != FLASH_OK) break;
    }

    return status;
}

/* ==========================================================================
   标志操作
   ========================================================================== */

void IAP_SetFlag(uint32_t magic)
{
    FLASH_Unlock();
    FLASH_ClearFlags();
    FLASH_ErasePage(IAP_FLAG_ADDR & ~(FLASH_PAGE_SIZE - 1));
    FLASH_ProgramWord(IAP_FLAG_ADDR, magic);
    FLASH_Lock();
}

uint32_t IAP_ReadFlag(void)
{
    return *(volatile uint32_t *)IAP_FLAG_ADDR;
}

void IAP_ClearFlag(void)
{
    IAP_SetFlag(IAP_MAGIC_NONE);
}

/* ==========================================================================
   App 跳转与校验
   ========================================================================== */

int IAP_CheckAppValid(uint32_t addr)
{
    /* 检查栈顶地址是否在 RAM 范围内 (0x20000000 - 0x20010000) */
    uint32_t sp = *(volatile uint32_t *)addr;
    if (sp < 0x20000000 || sp > 0x20010000) {
        return 0;
    }
    /* 检查复位向量是否在 Flash 范围内 */
    uint32_t pc = *(volatile uint32_t *)(addr + 4);
    if (pc < APP_START_ADDR || pc > (APP_START_ADDR + APP_MAX_SIZE)) {
        return 0;
    }
    return 1;
}

void IAP_JumpToApp(uint32_t addr)
{
    if (!IAP_CheckAppValid(addr)) {
        return;
    }

    /* 关闭中断 */
    __disable_irq();

    /* 重置外设 (简化处理) */
    RCC->APB1RSTR = 0xFFFFFFFF;
    RCC->APB1RSTR = 0;
    RCC->APB2RSTR = 0xFFFFFFFF;
    RCC->APB2RSTR = 0;

    /* 设置向量表偏移 */
    SCB->VTOR = addr;

    /* 获取栈顶和复位向量 */
    uint32_t sp = *(volatile uint32_t *)addr;
    uint32_t pc = *(volatile uint32_t *)(addr + 4);

    /* 设置主栈指针 */
    __set_MSP(sp);

    /* 跳转到 App 复位向量 */
    __asm volatile (
        "BX %0" :: "r"(pc)
    );

    while (1);
}

/* ==========================================================================
   IAP 协议处理
   ========================================================================== */

void IAP_EnterMode(void)
{
    g_iap_ctx.state = IAP_ST_READY;
    g_iap_ctx.total_size = 0;
    g_iap_ctx.rx_packets = 0;
    g_iap_ctx.rx_bytes = 0;
    g_iap_ctx.error_code = 0;
}

void IAP_ProcessPacket(uint8_t cmd, uint8_t *data, uint16_t len)
{
    uint8_t resp_data[16];
    uint16_t resp_len = 0;
    uint8_t resp_type = 0;
    uint8_t resp_subtype = 1;   /* SUC_NODATA */

    switch (cmd) {
        /* 全体/单元机进入升级模式 */
        case IAP_INTEGRAL_START_REQ:
        case IAP_SINGAL_UNIT_START_REQ: {
            IAP_EnterMode();
            IAP_ClearFlag();  /* 清除标志 */
            if (cmd == IAP_INTEGRAL_START_REQ) resp_type = IAP_INTEGRAL_START_RSP;
            else resp_type = IAP_SINGAL_UNIT_START_RSP;
            break;
        }

        /* 单包数据准备帧 */
        case IAP_SINGAL_PACKEG_START_REQ: {
            if (len >= 8) {
                g_iap_ctx.total_size    = ((uint32_t)data[0] << 24)
                                        | ((uint32_t)data[1] << 16)
                                        | ((uint32_t)data[2] << 8)
                                        | data[3];
                g_iap_ctx.packet_size   = ((uint16_t)data[4] << 8) | data[5];
                g_iap_ctx.fw_crc        = ((uint16_t)data[6] << 8) | data[7];
                g_iap_ctx.total_packets = (g_iap_ctx.total_size + g_iap_ctx.packet_size - 1)
                                        / g_iap_ctx.packet_size;
                g_iap_ctx.rx_packets = 0;
                g_iap_ctx.rx_bytes = 0;

                if (g_iap_ctx.total_size > 0 && g_iap_ctx.total_size <= APP_MAX_SIZE) {
                    flash_status_t st = iap_erase_app_area(g_iap_ctx.total_size);
                    if (st == FLASH_OK) {
                        g_iap_ctx.state = IAP_ST_RECEIVING;
                        g_iap_ctx.error_code = 0;
                        resp_subtype = 1;  /* SUC_NODATA */
                    } else {
                        g_iap_ctx.state = IAP_ST_ERROR;
                        g_iap_ctx.error_code = 0x03; /* 擦除失败 */
                        resp_subtype = 3;  /* FAILED */
                        resp_data[0] = 0x03;
                        resp_len = 1;
                    }
                } else {
                    g_iap_ctx.state = IAP_ST_ERROR;
                    g_iap_ctx.error_code = 0x02; /* 大小非法 */
                    resp_subtype = 3;
                    resp_data[0] = 0x02;
                    resp_len = 1;
                }
            } else {
                g_iap_ctx.state = IAP_ST_ERROR;
                g_iap_ctx.error_code = 0x01; /* 参数错误 */
                resp_subtype = 3;
                resp_data[0] = 0x01;
                resp_len = 1;
            }
            resp_type = IAP_SINGAL_PACKEG_START_RSP;
            break;
        }

        /* 单包数据帧 */
        case IAP_SINGAL_PACKEG_REQ: {
            if (g_iap_ctx.state != IAP_ST_RECEIVING) {
                resp_subtype = 3; /* FAILED */
                resp_data[0] = 0x04; /* 状态不匹配 */
                resp_len = 1;
            } else if (len >= 4) {
                uint32_t offset = ((uint32_t)data[0] << 24)
                                | ((uint32_t)data[1] << 16)
                                | ((uint32_t)data[2] << 8)
                                | data[3];
                uint8_t *payload = &data[4];
                uint16_t payload_len = len - 4;

                if (payload_len > 0) {
                    flash_status_t st = iap_write_flash(offset, payload, payload_len);
                    if (st == FLASH_OK) {
                        g_iap_ctx.rx_bytes += payload_len;
                        g_iap_ctx.rx_packets++;
                        g_iap_ctx.last_addr = APP_START_ADDR + offset + payload_len;
                        resp_subtype = 1; /* SUC_NODATA */
                    } else {
                        g_iap_ctx.state = IAP_ST_ERROR;
                        g_iap_ctx.error_code = 0x05; /* 写入失败 */
                        resp_subtype = 3;
                        resp_data[0] = 0x05;
                        resp_len = 1;
                    }
                }
            } else {
                resp_subtype = 3;
                resp_data[0] = 0x01;
                resp_len = 1;
            }
            resp_type = IAP_SINGAL_PACKEG_RSP;
            break;
        }

        /* 单包数据结束帧 */
        case IAP_SIGNAL_PACKEG_END_REQ: {
            if (g_iap_ctx.state == IAP_ST_RECEIVING) {
                g_iap_ctx.state = IAP_ST_VERIFY;
                /* 简单校验: 比较接收字节数 */
                if (g_iap_ctx.rx_bytes >= g_iap_ctx.total_size) {
                    IAP_SetFlag(IAP_MAGIC_OK);
                    g_iap_ctx.state = IAP_ST_COMPLETE;
                    resp_subtype = 1; /* SUC_NODATA */
                } else {
                    g_iap_ctx.state = IAP_ST_ERROR;
                    g_iap_ctx.error_code = 0x06; /* 数据不完整 */
                    resp_subtype = 3;
                    resp_data[0] = 0x06;
                    resp_len = 1;
                }
            } else {
                resp_subtype = 3;
                resp_data[0] = 0x04;
                resp_len = 1;
            }
            resp_type = IAP_SIGNAL_PACKEG_END_RSP;
            break;
        }

        /* 单元机/全体结束升级 */
        case IAP_SINGAL_UNIT_END_REQ:
        case IAP_INTEGRAL_END_REQ: {
            if (g_iap_ctx.state == IAP_ST_COMPLETE) {
                resp_subtype = 1;
            } else {
                resp_subtype = 3;
                resp_data[0] = g_iap_ctx.error_code;
                resp_len = 1;
            }
            if (cmd == IAP_SINGAL_UNIT_END_REQ) resp_type = IAP_SINGAL_UNIT_END_RSP;
            else resp_type = IAP_INTEGRAL_END_RSP;

            /* 发送响应 */
            iap_send_response(resp_type, resp_subtype, resp_data, resp_len);

            /* 若升级成功，延时后跳转到 App */
            if (g_iap_ctx.state == IAP_ST_COMPLETE) {
                delay_ms(100);
                IAP_JumpToApp(APP_START_ADDR);
            }
            return;  /* 已手动发送响应，直接返回 */
        }

        /* 升级状态查询 */
        case IAP_STATUS_INQUIRE_REQ: {
            resp_type = IAP_STATUS_INQUIRE_RSP;
            resp_data[0] = (uint8_t)g_iap_ctx.state;
            resp_data[1] = (uint8_t)(g_iap_ctx.rx_packets >> 8);
            resp_data[2] = (uint8_t)(g_iap_ctx.rx_packets & 0xFF);
            resp_data[3] = (uint8_t)(g_iap_ctx.rx_bytes >> 24);
            resp_data[4] = (uint8_t)(g_iap_ctx.rx_bytes >> 16);
            resp_data[5] = (uint8_t)(g_iap_ctx.rx_bytes >> 8);
            resp_data[6] = (uint8_t)(g_iap_ctx.rx_bytes & 0xFF);
            resp_len = 7;
            resp_subtype = 2; /* SUC_HAVEDATA */
            break;
        }

        /* 版本查询 */
        case IAP_VERSION_INQUIRE_REQ: {
            resp_type = IAP_VERSION_INQUIRE_RSP;
            resp_data[0] = BOOTLOADER_VERSION_MAJOR;
            resp_data[1] = BOOTLOADER_VERSION_MINOR;
            resp_len = 2;
            resp_subtype = 2;
            break;
        }

        default: {
            resp_type = cmd + 0x80;
            resp_subtype = 3; /* FAILED */
            resp_data[0] = 0xFF;
            resp_len = 1;
            break;
        }
    }

    iap_send_response(resp_type, resp_subtype, resp_data, resp_len);
}

/**
  * @brief  Bootloader 主循环: 解析协议帧并处理 IAP 命令
  */
void IAP_Run(void)
{
    /* 简单的状态机解析 */
    typedef enum {
        ST_HEAD1 = 0, ST_HEAD2, ST_TYPE, ST_REV1, ST_REV2,
        ST_LEN_HI, ST_LEN_LO, ST_DATA, ST_CRC
    } rx_state_t;

    static rx_state_t state = ST_HEAD1;
    static uint8_t  rx_buf[64];
    static uint16_t rx_len = 0;
    static uint16_t rx_cnt = 0;
    static uint8_t  rx_crc = 0;

    uint8_t ch;

    while (1) {
        while (UART_GetChar(UART_IDX_UART4, &ch)) {
            switch (state) {
                case ST_HEAD1:
                    if (ch == COMPROTOCOLHEAD1) state = ST_HEAD2;
                    break;

                case ST_HEAD2:
                    state = (ch == COMPROTOCOLHEAD2) ? ST_TYPE : ST_HEAD1;
                    if (state == ST_TYPE) rx_crc = 0;
                    break;

                case ST_TYPE:
                    rx_buf[0] = ch; rx_crc ^= ch; state = ST_REV1; break;
                case ST_REV1:
                    rx_buf[1] = ch; rx_crc ^= ch; state = ST_REV2; break;
                case ST_REV2:
                    rx_buf[2] = ch; rx_crc ^= ch; state = ST_LEN_HI; break;
                case ST_LEN_HI:
                    rx_buf[3] = ch; rx_crc ^= ch; state = ST_LEN_LO; break;
                case ST_LEN_LO:
                    rx_buf[4] = ch; rx_crc ^= ch;
                    rx_len = (((uint16_t)rx_buf[3] << 8) | rx_buf[4]) & 0x1FFF;
                    rx_cnt = 0;
                    state = (rx_len > 0 && rx_len <= 48) ? ST_DATA : ((rx_len == 0) ? ST_CRC : ST_HEAD1);
                    break;

                case ST_DATA:
                    rx_buf[5 + rx_cnt] = ch; rx_crc ^= ch; rx_cnt++;
                    if (rx_cnt >= rx_len) state = ST_CRC;
                    break;

                case ST_CRC:
                    if (ch == rx_crc) {
                        IAP_ProcessPacket(rx_buf[0], (rx_len > 0) ? &rx_buf[5] : NULL, rx_len);
                    }
                    state = ST_HEAD1;
                    break;

                default:
                    state = ST_HEAD1;
                    break;
            }
        }

        /* 如果升级完成且收到结束命令，准备跳转 */
        if (g_iap_ctx.state == IAP_ST_COMPLETE) {
            /* 持续处理，等待 0x75/0x76 或新的 0x70 */
        }
    }
}
