# 中药机终端通信协议 V2.2 对比验证报告

**验证日期**: 2026-05-13  
**验证对象**: `Core/Src/protocol_sac.c` + `Core/Src/protocol_sac.h`  
**参考文档**: `Docs/中药机终端通信协议V2.2.doc`

---

## 一、总体结论

| 项目 | 结论 |
|------|------|
| 帧格式与校验 | ✅ 完全正确 |
| 命令码映射 | ✅ 基本正确（2条命令缺失） |
| 数据格式/字节序 | ✅ 正确 |
| IO状态字节 | ❌ **bit位定义错误，需修复** |
| 拼写/命名 | ⚠️ 2处拼写错误 |

**结论**: 协议栈整体实现与 V2.2 文档高度一致，但存在 **1处严重错误**（IO状态字节bit位错位）和 **2条缺失命令**，需要修复。

---

## 二、帧格式验证

### 2.1 帧结构

| 字节 | 协议要求 | 代码实现 | 状态 |
|------|----------|----------|------|
| 帧头0 | 0x55 | `COMPROTOCOLHEAD1 = 0x55` | ✅ |
| 帧头1 | 0xBB | `COMPROTOCOLHEAD2 = 0xBB` | ✅ |
| 类型 | 1字节消息类型 | `tx_buf[2] = type` | ✅ |
| REV1 | 板卡ID | `s_board_id` | ✅ |
| REV2 | 机器ID | `s_machine_id` | ✅ |
| LEN_HI | 长度高字节 | 含子类型编码 | ✅ |
| LEN_LO | 长度低字节 | 含子类型编码 | ✅ |
| DATA | 0~N字节 | 正确填充 | ✅ |
| CRC | 校验字 | 正确计算 | ✅ |

### 2.2 长度字段编码

协议要求：**高3bit = 子类型，低13bit = 数据长度**

```c
/* 代码实现 (protocol_sac.c:74) */
len_field = ((subtype & 0x07) << 13) | (len & 0x1FFF);
```

✅ **正确**：子类型编码到 bit15:13，长度限制在 bit12:0。

### 2.3 CRC校验

协议要求：**除消息头和校验字本身外的异或值**

```c
/* 发送端 (protocol_sac.c:84) */
tx_buf[pos] = calc_crc(&tx_buf[2], pos - 2);

/* 接收端 (protocol_sac.c:317-358) */
s_rx_crc ^= ch;  /* 累加 TYPE/REV1/REV2/LEN_HI/LEN_LO/DATA */
if (ch == s_rx_crc)  /* 与 CRC 字节比较 */
```

✅ **正确**：CRC 计算范围排除帧头（tx_buf[0]/tx_buf[1]）和 CRC 字节本身。

### 2.4 子类型定义

| 子类型值 | 协议定义 | 代码枚举 | 状态 |
|----------|----------|----------|------|
| 0 | 未使用 | `NOUSE` | ✅ |
| 1 | 成功但不携带数据 | `SUC_NODATA` | ✅ |
| 2 | 成功且携带数据 | `SUC_HAVEDATA` | ✅ |
| 3 | 失败，携带原因值 | `FAILED` | ✅ |
| 4 | 暂时保留 | `RESVED` | ✅ |

### 2.5 原因值定义

| 原因值 | 协议定义 | 代码枚举 | 状态 |
|--------|----------|----------|------|
| 0x00 | 成功 | `CAUSE_SUCC` | ✅ |
| 0x01 | 校验失败 | `CAUSE_CKS_ERR` | ✅ |
| 0x02 | 参数非法 | `CAUSE_ARG_ILLEGAL` | ✅ |
| 0x03 | 操作失败 | `CAUSE_OPS_FAIL` | ✅ |
| 0x04 | 配置不存在 | `CAUSE_CFG_NOT_EXIST` | ✅ |
| 0x05 | 内部错误 | `CAUSE_INTR_ERR` | ✅ |
| 0x06 | 状态不匹配 | `CAUSE_STATE_MISMATCH` | ✅ |
| 0x07 | 无有效数据 | `CAUSE_NO_VALID_DATA` | ✅ |
| 0xFF | 未说明 | `CAUSE_UNSPECIFIED` | ✅ |

---

## 三、命令码完整对比

### 3.1 请求命令码

| 命令码 | 协议定义 | 代码定义 | 响应码 | 实现状态 | 备注 |
|--------|----------|----------|--------|----------|------|
| 0x01 | 当前IO状态查询 | `SYS_IO_REQ` | 0x81 | ✅ 已实现 | |
| 0x04 | 单次出药指令 | `MEDCINE_START_REQ` | 0x84 | ✅ 已实现 | 拼写：MEDCIEN |
| 0x06 | 当前重量查询 | `SAMP_WEIGHT_REQ` | 0x86 | ✅ 已实现 | |
| 0x07 | 速度切换百分比设置 | `SAMP_PERCENT_CFG_REQ` | 0x87 | ✅ 已实现 | |
| 0x08 | *(协议未定义)* | `SYS_REBOOT_REQ` | 0x88 | ⚠️ 扩展功能 | V2.2文档无此命令 |
| 0x09 | 药量设置 | `CFG_DATA_MEDCINE_SET` | 0x89 | ✅ 已实现 | |
| 0x0F | 高速速度设置 | `CFG_SPEED_MODE_H_SET` | 0x8F | ✅ 已实现 | |
| 0x10 | 出药状态查询 | `CFG_OUT_STATUS_REQ` | 0x90 | ✅ 已实现 | |
| 0x18 | 开门时间设置 | `CFG_DOOR_TIME_REQ` | 0x98 | ✅ 已实现 | |
| 0x19 | 手动开门 | `CFG_DOOR_OPEN_REQ` | 0x99 | ✅ 已实现 | |
| 0x1A | 手动振动 | `CFG_VIBRATE_REQ` | 0x9A | ✅ 已实现 | |
| 0x1B | 出药超时时间 | `CFG_TIMEOUT_REQ` | 0x9B | ✅ 已实现 | |
| 0x1C | 高重量标定 | *(无)* | 0x9C | ✅ 已实现 | 拼写：HIGN→HIGH |
| 0x1D | 零重量标定 | `CFG_WEIGHT_LOW_REQ` | 0x9D | ✅ 已实现 | |
| 0x1E | 绞龙确认时间 | `CFG_AUGER_TIME_REQ` | 0x9E | ✅ 已实现 | |
| 0x1F | 绞龙差值 | `CFG_AUGER_WEIGHT_REQ` | 0x9F | ✅ 已实现 | |
| 0x20 | 中速速度设置 | `CFG_SPEED_MODE_M_SET` | 0xA0 | ✅ 已实现 | |
| 0x21 | 低速速度设置 | `CFG_SPEED_MODE_L_SET` | 0xA1 | ✅ 已实现 | |
| 0x22 | 花草型低速阈值 | `CFG_LOW_WEIGHT_REQ` | 0xA2 | ✅ 已实现 | |
| 0x23 | *(保留)* | `CTRL_ABN_DETECT_STOP_REQ` | 0xA3 | ⚠️ 未处理 | 返回 FAILED |
| 0x24 | *(保留)* | `CTRL_ACC_ZERO_CORR_REQ` | 0xA4 | ⚠️ 未处理 | 返回 FAILED |
| 0x33 | 告警查询 | `ALARM_REQ` | 0xB3 | ⚠️ 未处理 | 返回 FAILED |
| 0x5D | 心跳 | `SYS_HB_REQ` | 0xDD | ✅ 已实现 | |
| 0x60 | 搅拌速度 | `STIR_SPEED_REQ` | 0xE0 | ✅ 已实现 | |
| 0x61 | 急停 | `SYS_EMERGENCY_STOP_REQ` | 0xE1 | ✅ 已实现 | |
| 0x62 | 一键清空 | `SYS_CLEAR_REQ` | 0xE2 | ✅ 已实现 | |
| **0x63** | **模拟量采集** | ❌ **未定义** | **0xE3** | ❌ **缺失** | 协议有定义 |
| **0x64** | **停止模拟量采集** | ❌ **未定义** | **0xE4** | ❌ **缺失** | 协议有定义 |
| 0x70~0x78 | IAP升级 | 已定义 | 0xF0~0xF8 | ✅ 已实现 | |

### 3.2 缺失命令（需补充）

```
0x63 模拟量采集请求          → 响应 0xE3
0x64 停止模拟量采集请求       → 响应 0xE4
```

---

## 四、数据格式验证

### 4.1 0x04 单次出药

**协议描述**: "单次出药指令设置"（未明确数据格式）

**代码实现**:
```c
if (len >= 2) {
    uint16_t weight = ((uint16_t)data[0] << 8) | data[1];  /* 大端序 */
    g_disp_params.target_weight = weight;
}
```

✅ **正确**：接收大端序2字节重量值，与旧版软件一致。

### 4.2 0x06 重量查询响应

**协议描述**: "响应低8位在前，高8位在后"

**代码实现**:
```c
resp_data[0] = w & 0xFF;        /* 低8位 */
resp_data[1] = (w >> 8) & 0xFF; /* 高8位 */
```

✅ **正确**：小端序发送，符合协议"低8位在前"的要求。

### 4.3 0x09 药量设置

**协议描述**: "16位无符号整型，单位克，<=500克"

**代码实现**:
```c
g_disp_params.target_weight = ((uint16_t)data[0] << 8) | data[1];
```

⚠️ **注意**：代码未做 `<=500` 的限制校验。如果上位机发送超过500的值，代码会接受。

### 4.4 0x0F/0x20/0x21 速度设置

**协议描述**: "16位无符号整型，高位在前低位在后，颗粒型最小0x0333"

**代码实现**: 大端序接收，存储到 `g_disp_params.high_speed/mid_speed/low_speed`

✅ **正确**：大端序处理正确。

### 4.5 0x1E 绞龙确认时间

**协议描述**: "16位无符号整型，高位在前低位在后，默认3000秒"

⚠️ **注意**：协议写"3000秒"，但实际应为毫秒（3秒更合理）。代码以毫秒使用，与旧版软件一致。

### 4.6 0x10 出药状态查询响应

**协议描述**: "0出药已完成，1低速出药中，2中速出药中，3高速出药中，0xFF出药超时"

**代码实现**:
```c
/* disp_status_t 枚举 */
DISP_IDLE = 0, DISP_LOW_SPEED = 1, DISP_MID_SPEED = 2,
DISP_HIGH_SPEED = 3, DISP_TIMEOUT = 0xFF
```

✅ **正确**：状态值与协议完全对应。

---

## 五、❌ 严重问题：IO状态字节 (0x01响应)

### 5.1 协议定义

协议 V2.2 原文：
> "返回一个字节低4位为输入传感器。最高位为门状态，0为开门1为关门。最低位缺料传感器，倒数第二位为中位传感器，倒数第三位为高位传感器。有料为0，无料为1。倒数第四位为关门到位传感器(此传感器未安装)。"

| bit | 协议定义 | 有效值 |
|-----|----------|--------|
| bit0 | 缺料传感器 | 有料=0, 无料=1 |
| bit1 | 中位传感器 | 有料=0, 无料=1 |
| bit2 | 高位传感器 | 有料=0, 无料=1 |
| bit3 | 关门到位传感器 | 未安装 |
| bit4~6 | 保留 | - |
| bit7 | 门状态 | 0=开门, 1=关门 |

### 5.2 代码实现（错误）

```c
uint8_t DISPENSER_GetStatusByte(void)
{
    uint8_t status = 0;

    if (!s_door_is_open) status |= 0x80;    /* bit7: 门状态 ✅ */

    if (SENSOR_ReadHigh()) status |= 0x08;  /* bit3: 高位无料 ❌ */
    if (SENSOR_ReadMid())  status |= 0x04;  /* bit2: 中位无料 ❌ */
    if (SENSOR_ReadLow())  status |= 0x02;  /* bit1: 低位无料 ❌ */
    if (SENSOR_ReadLow())  status |= 0x01;  /* bit0: 缺料/低位 ✅ */
    return status;
}
```

### 5.3 错误分析

| bit | 协议要求 | 代码实际 | 问题 |
|-----|----------|----------|------|
| bit0 | 缺料传感器 | 低位传感器 | ⚠️ 如果缺料=低位，则等价 |
| bit1 | **中位传感器** | **低位传感器（重复）** | ❌ **严重：bit1 与 bit0 相同** |
| bit2 | **高位传感器** | **中位传感器** | ❌ **错位** |
| bit3 | **关门到位** | **高位传感器** | ❌ **错位** |
| bit7 | 门状态 | 门状态 | ✅ 正确 |

### 5.4 修复方案

```c
uint8_t DISPENSER_GetStatusByte(void)
{
    uint8_t status = 0;

    /* bit7: 门状态，0=开门, 1=关门 */
    if (!s_door_is_open) status |= 0x80;

    /* bit2: 高位传感器，有料=0, 无料=1 */
    if (SENSOR_ReadHigh()) status |= 0x04;

    /* bit1: 中位传感器，有料=0, 无料=1 */
    if (SENSOR_ReadMid())  status |= 0x02;

    /* bit0: 缺料传感器(低位)，有料=0, 无料=1 */
    if (SENSOR_ReadLow())  status |= 0x01;

    /* bit3: 关门到位传感器，未安装，保持0 */
    /* bit4~6: 保留 */

    return status;
}
```

---

## 六、⚠️ 拼写错误

### 6.1 `MEDCIEN_START_RSP` → `MEDICINE_START_RSP`

**影响文件**:
- `Core/Inc/protocol_sac.h:82`
- `Core/Src/protocol_sac.c:140`

### 6.2 `CFG_WEIGHT_HIGN_REQ/RSP` → `CFG_WEIGHT_HIGH_REQ/RSP`

**影响文件**:
- `Core/Inc/protocol_sac.h:52` (HIGN_REQ)
- `Core/Inc/protocol_sac.h:101` (HIGN_RSP)
- `Core/Src/protocol_sac.c:164` (HIGN_REQ)
- `Core/Src/protocol_sac.c:179` (HIGN_RSP)
- `Core/Src/dispenser.c:373` (HIGN_REQ)

---

## 七、其他注意事项

### 7.1 0x08 系统重启命令

协议 V2.2 文档中 **未定义** 0x08 命令。代码中实现了系统重启功能（发送响应后执行 `NVIC_SystemReset()`）。

- 如果上位机使用此命令：✅ 正常工作
- 协议兼容性：⚠️ 属于扩展功能，不影响标准命令

**建议**: 保留此功能，但建议在文档中注明为扩展命令。

### 7.2 0x23/0x24/0x33 命令

代码中定义了这些命令的宏，但在 `Protocol_ProcessCommand()` 中未处理，会进入 `default` 分支返回 `CAUSE_OPS_FAIL`。

- 协议 V2.2 中未定义 0x23/0x24
- 0x33 告警查询在协议中有定义，但未实现

**建议**: 如果不需要这些功能，可以保留当前行为（返回失败）；如果需要，需补充实现。

### 7.3 0x63/0x64 模拟量采集

协议 V2.2 中定义了这两个命令，但旧版 RT-Thread 代码中也未实现。如果硬件不需要模拟量采集功能，可以忽略。

**建议**: 如需完整兼容协议，应补充实现或注明不支持。

---

## 八、修复清单

### 必须修复（影响功能正确性）

| 优先级 | 问题 | 文件 | 行号 |
|--------|------|------|------|
| 🔴 P0 | IO状态字节 bit1/bit2/bit3 错位 | `Core/Src/dispenser.c` | 309-313 |

### 强烈建议修复（影响代码质量）

| 优先级 | 问题 | 文件 | 行号 |
|--------|------|------|------|
| 🟡 P1 | 拼写 `MEDCIEN_START_RSP` → `MEDICINE_START_RSP` | `protocol_sac.h/c` | 多行 |
| 🟡 P1 | 拼写 `CFG_WEIGHT_HIGN_*` → `CFG_WEIGHT_HIGH_*` | `protocol_sac.h/c`, `dispenser.c` | 多行 |

### 可选修复（功能扩展）

| 优先级 | 问题 | 说明 |
|--------|------|------|
| 🟢 P2 | 实现 0x63/0x64 模拟量采集 | 协议有定义，旧代码也未实现 |
| 🟢 P2 | 0x09 药量限制 <=500g | 协议有说明，代码未校验 |
| 🟢 P2 | 实现 0x33 告警查询 | 协议有定义 |

---

## 九、验证签名

```
验证人: Kimi Code CLI
验证范围: protocol_sac.c/h 完整指令集
参考文档: 中药机终端通信协议V2.2
结论: 整体实现正确，1处严重错误需修复，2处拼写错误建议修复
```
