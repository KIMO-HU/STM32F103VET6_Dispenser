# STM32F103VET6 基础项目

## 项目概述

这是一个基于 **STM32F103VET6** (Cortex-M3, 72MHz, 512KB Flash, 64KB RAM) 的基础嵌入式项目，使用纯寄存器操作开发，不依赖 HAL/LL 库或标准外设库 (SPL)。

## 开发环境

- **IDE**: Visual Studio Code
- **编译器**: ARM GCC (arm-none-eabi-gcc)
- **调试器**: OpenOCD + ST-Link V2
- **构建系统**: GNU Make

## 项目结构

```
STM32F103VET6_Project/
├── Core/
│   ├── Inc/                    # 头文件
│   │   └── stm32f1xx_it.h      # 中断服务程序声明
│   └── Src/                    # 源代码
│       ├── main.c              # 主程序 (LED闪烁 + 时钟配置)
│       ├── stm32f1xx_it.c      # 中断服务程序
│       └── syscalls.c          # Newlib 系统调用
├── Drivers/
│   └── CMSIS/                  # CMSIS 核心库
│       ├── Core/Include/       # CMSIS-Core 头文件
│       └── Device/ST/STM32F1xx/# STM32F1 设备支持包
├── LinkerScript/               # 链接器脚本
├── build/                      # 编译输出目录 (自动生成)
├── .vscode/                    # VS Code 配置文件
├── Makefile                    # Make 构建脚本
└── README.md                   # 本文件
```

## 功能说明

### 当前实现
- **系统时钟**: 外部晶振 8MHz -> PLL 倍频 x9 -> **SYSCLK = 72MHz**
- **LED 闪烁**: PC13 引脚，周期 1秒 (亮 500ms / 灭 500ms)
- **SysTick**: 1ms 定时中断，提供 `delay_ms()` 延时函数

### 时钟树配置
| 总线 | 频率 |
|------|------|
| SYSCLK (系统时钟) | 72 MHz |
| AHB (HCLK) | 72 MHz |
| APB1 | 36 MHz (最大) |
| APB2 | 72 MHz |
| ADCCLK | 36 MHz |

## 快速开始

### 1. 编译项目

在 VS Code 中按 `Cmd+Shift+B` 选择 **"Build STM32 Project"**，或在终端执行：

```bash
cd /Users/kimohu/Documents/WORKSPACE/STM32F103VET6_Project
make all
```

### 2. 烧录程序

连接 ST-Link 调试器到开发板，然后在 VS Code 中按 `Cmd+Shift+P` -> **"Tasks: Run Task"** -> **"Flash to STM32"**，或在终端执行：

```bash
make flash
```

### 3. 调试程序

在 VS Code 中按 `F5` 启动调试会话：
- 会自动启动 OpenOCD GDB Server
- 使用 `arm-none-eabi-gdb` 连接 localhost:3333
- 支持断点、单步、变量查看等功能

## VS Code 任务说明

| 任务 | 快捷键/命令 | 说明 |
|------|------------|------|
| Build | `Cmd+Shift+B` | 编译整个项目 |
| Clean | 任务面板 | 清理编译产物 |
| Flash | 任务面板 | 通过 OpenOCD + ST-Link 烧录 |
| Debug Server | 任务面板 | 启动 OpenOCD GDB Server |
| Debug | `F5` | 启动 GDB 调试会话 |

## 硬件连接

| 功能 | 引脚 | 说明 |
|------|------|------|
| LED | PC13 | 板载 LED (低电平点亮) |
| SWDIO | PA13 | ST-Link 调试数据线 |
| SWCLK | PA14 | ST-Link 调试时钟线 |
| NRST | NRST | 复位线 |
| HSE | OSC_IN/OUT | 8MHz 外部晶振 |

## 使用 STM32CubeMX

如果你想使用 STM32CubeMX 生成初始化代码：

1. 打开 STM32CubeMX
2. 选择 **STM32F103VET6**
3. 配置引脚和时钟
4. 生成代码时选择 **Makefile** 工具链
5. 将生成的代码合并到此项目中

## 常见问题

### Q: 编译报错 "arm-none-eabi-gcc: command not found"
A: ARM GCC 未安装或不在 PATH 中。请确保已通过 Homebrew 安装：
```bash
brew install --cask gcc-arm-embedded
```

### Q: OpenOCD 连接失败
A: 检查 ST-Link 驱动和连接：
```bash
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg
```
如果出现 "LIBUSB_ERROR_ACCESS"，可能需要安装 ST-Link 驱动。

### Q: 如何修改 LED 引脚？
A: 在 `main.c` 的 `GPIO_Init()` 函数中修改对应的 GPIO 端口和引脚号。

## 许可证

本项目基于 MIT 许可证开源。
