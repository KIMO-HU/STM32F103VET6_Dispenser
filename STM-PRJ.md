# STM32F103VET6 嵌入式软件开发项目文档

> **任务来源**: 在 Visual Studio Code 中，使用 KimiCode 和 STM32CubeMX 制作一个 STM32 MCU（STM32F103VET6）的软件
> **创建时间**: 2026-05-13
> **项目路径**: `/Users/kimohu/Documents/WORKSPACE/STM32F103VET6_Project/`

---

## 一、任务目标

搭建完整的 STM32F103VET6 开发环境，包括：
1. 安装和配置交叉编译工具链（ARM GCC）
2. 安装调试工具（OpenOCD + GDB）
3. 配置 Visual Studio Code 作为 IDE
4. 创建基于 CMSIS 的基础项目框架
5. 实现 LED 闪烁等基础功能示例
6. 提供 STM32CubeMX 的整合方案

---

## 二、硬件平台参数

| 参数 | 规格 |
|------|------|
| MCU 型号 | STM32F103VET6 |
| 内核 | ARM Cortex-M3 |
| 主频 | 72 MHz |
| Flash | 512 KB |
| SRAM | 64 KB |
| 封装 | LQFP-100 |
| 调试接口 | SWD (Serial Wire Debug) |

---

## 三、开发环境搭建记录

### 3.1 操作系统环境
- **系统**: macOS (Apple Silicon)
- **包管理器**: Homebrew (已安装)
- **Shell**: zsh

### 3.2 已安装工具清单

| 工具 | 安装方式 | 版本 | 用途 |
|------|----------|------|------|
| Visual Studio Code | 从 Downloads 复制到 /Applications | 最新版 | 代码编辑和调试 |
| arm-none-eabi-gcc | `brew install arm-none-eabi-gcc` | 16.1.0 | ARM 交叉编译器 |
| arm-none-eabi-gdb | `brew install arm-none-eabi-gdb` | 17.2 | 调试器 |
| arm-none-eabi-binutils | Homebrew 依赖自动安装 | 2.46.0 | 链接器、objcopy 等 |
| OpenOCD | `brew install openocd` | 0.12.0 | 片上调试器服务器 |

### 3.3 VS Code 扩展
- **C/C++** (ms-vscode.cpptools) - IntelliSense 和调试支持

### 3.4 安装过程说明

#### ARM GCC 工具链
最初尝试通过 `brew install --cask gcc-arm-embedded` 安装完整版（含 newlib），但因下载文件较大（约 140MB）多次超时。最终采用 formula 版本：

```bash
brew install arm-none-eabi-gcc
brew install arm-none-eabi-gdb
```

formula 版本不含完整 newlib 标准库，因此项目中通过以下方式适配：
- 创建了最小化的 `Core/Inc/stdint.h` 和 `Core/Inc/stddef.h`
- 使用 `-nostdlib` 编译选项
- 在 `syscalls.c` 中提供了 `__libc_init_array` 空实现
- 显式链接 `libgcc.a` 处理编译器内部函数

#### OpenOCD
```bash
brew install openocd
```

---

## 四、项目目录结构

```
STM32F103VET6_Project/
├── Core/                          # 用户核心代码
│   ├── Inc/                       # 头文件目录
│   │   ├── stdint.h              # 最小化 stdint.h（适配无 newlib 环境）
│   │   ├── stddef.h              # 最小化 stddef.h
│   │   └── stm32f1xx_it.h        # 中断服务程序声明
│   └── Src/                       # 源文件目录
│       ├── main.c                # 主程序（时钟配置 + LED 闪烁）
│       ├── stm32f1xx_it.c        # 中断服务程序实现
│       └── syscalls.c            # 系统调用桩函数（newlib 替代）
│
├── Drivers/                       # 驱动库
│   └── CMSIS/                    # ARM CMSIS 核心库
│       ├── Core/Include/         # CMSIS-Core 头文件（core_cm3.h 等）
│       └── Device/ST/STM32F1xx/  # STM32F1 设备支持包
│           ├── Include/          # 设备头文件（stm32f103xe.h 等）
│           └── Source/Templates/ # 启动文件和系统初始化
│               ├── gcc/
│               │   └── startup_stm32f103xe.s
│               └── system_stm32f1xx.c
│
├── LinkerScript/                  # 链接器脚本
│   └── STM32F103VETx_FLASH.ld    # 针对 512KB Flash / 64KB RAM
│
├── .vscode/                       # VS Code 工作区配置
│   ├── c_cpp_properties.json     # C/C++ IntelliSense 配置
│   ├── tasks.json                # 构建/烧录/调试任务
│   ├── launch.json               # GDB 调试启动配置
│   └── settings.json             # 编辑器设置
│
├── build/                         # 编译输出目录（自动生成）
│   ├── STM32F103VET6_Demo.elf    # ELF 可执行文件
│   ├── STM32F103VET6_Demo.hex    # Intel HEX 格式
│   ├── STM32F103VET6_Demo.bin    # 原始二进制格式
│   └── *.o, *.d, *.map           # 中间文件和映射文件
│
├── Makefile                       # Make 构建脚本
├── README.md                      # 项目使用说明
├── STM32CubeMX_SETUP.md          # STM32CubeMX 安装和使用指南
└── STM-PRJ.md                     # 本文件
```

---

## 五、软件架构设计

### 5.1 时钟配置方案

| 时钟源 | 配置 | 输出频率 |
|--------|------|----------|
| HSE (外部晶振) | 8 MHz | - |
| PLL | HSE × 9 | 72 MHz |
| SYSCLK | PLL | **72 MHz** |
| AHB (HCLK) | SYSCLK / 1 | **72 MHz** |
| APB1 | HCLK / 2 | **36 MHz** |
| APB2 | HCLK / 1 | **72 MHz** |
| SysTick | HCLK / 1 | 1 ms 中断 |

### 5.2 启动流程

1. **复位后**: 从 `0x0800_0000` 开始执行
2. **启动文件** (`startup_stm32f103xe.s`):
   - 调用 `SystemInit()`（设置向量表）
   - 将 `.data` 段从 Flash 复制到 RAM
   - 清零 `.bss` 段
   - 调用 `__libc_init_array()`（C++ 构造函数初始化，空实现）
   - 跳转到 `main()`
3. **main() 函数**:
   - 调用 `SystemClock_Config()` → 配置 72MHz 时钟
   - 调用 `SysTick_Init()` → 启动 1ms 定时中断
   - 调用 `GPIO_Init()` → 配置 PC13 为推挽输出
   - 进入主循环: LED 闪烁（500ms 亮 / 500ms 灭）

### 5.3 中断向量表

| 中断/异常 | 处理函数 | 说明 |
|-----------|----------|------|
| Reset | Reset_Handler | 复位入口 |
| NMI | NMI_Handler | 不可屏蔽中断 |
| HardFault | HardFault_Handler | 硬错误 |
| MemManage | MemManage_Handler | 内存管理错误 |
| BusFault | BusFault_Handler | 总线错误 |
| UsageFault | UsageFault_Handler | 用法错误 |
| SVC | SVC_Handler | 超级调用 |
| DebugMon | DebugMon_Handler | 调试监视器 |
| PendSV | PendSV_Handler | 可挂起超级调用 |
| SysTick | SysTick_Handler | 系统滴答定时器（1ms 计数） |

---

## 六、构建系统说明

### 6.1 Makefile 关键配置

```makefile
# 目标芯片
MCU = -mcpu=cortex-m3 -mthumb

# 不使用标准库（因 formula 版 GCC 不含 newlib）
LDFLAGS = -nostdlib -lgcc

# 链接脚本
LDSCRIPT = LinkerScript/STM32F103VETx_FLASH.ld

# 宏定义
C_DEFS = -DSTM32F103xE
```

### 6.2 构建命令

| 命令 | 作用 |
|------|------|
| `make all` | 编译全部，生成 elf/hex/bin |
| `make clean` | 清理编译产物 |
| `make flash` | 通过 OpenOCD + ST-Link 烧录 |
| `make debug` | 启动 OpenOCD GDB Server |

### 6.3 编译产物信息

```
   text    data     bss     dec     hex filename
    884       4    1540    2428     97c build/STM32F103VET6_Demo.elf
```

- **text**: 884 字节（代码 + 只读数据）
- **data**: 4 字节（已初始化全局变量）
- **bss**: 1540 字节（未初始化全局变量 + 堆栈预留）

---

## 七、VS Code 任务配置

### 7.1 构建任务 (`Cmd+Shift+B`)

```json
{
    "label": "Build STM32 Project",
    "type": "shell",
    "command": "make",
    "args": ["all", "-j$(sysctl -n hw.ncpu)"],
    "group": { "kind": "build", "isDefault": true }
}
```

### 7.2 烧录任务

```json
{
    "label": "Flash to STM32",
    "type": "shell",
    "command": "make",
    "args": ["flash"]
}
```

### 7.3 调试配置 (`F5`)

- **调试器**: `arm-none-eabi-gdb`
- **GDB Server**: OpenOCD (localhost:3333)
- **接口配置**: `interface/stlink.cfg` + `target/stm32f1x.cfg`

---

## 八、STM32CubeMX 整合方案

### 8.1 设计思路

当前项目采用 **纯寄存器操作** 方式，不依赖 HAL 库。这种方式的优势：
- 代码精简，执行效率高
- 不依赖 STM32CubeMX 代码生成
- 便于理解底层硬件原理

STM32CubeMX 的使用方式：
1. 使用 CubeMX 进行引脚和时钟的图形化配置
2. 生成 Makefile 类型的代码
3. 将生成的初始化代码手动合并到当前项目
4. 或直接用 CubeMX 生成的项目替换本项目的 Core 目录

### 8.2 CubeMX 配置要点

1. **芯片选择**: STM32F103VET6
2. **时钟配置**: HSE 8MHz → PLL×9 → SYSCLK 72MHz
3. **调试接口**: Serial Wire (SWD)
4. **代码生成**: 选择 Makefile 工具链

详细步骤见 **`STM32CubeMX_SETUP.md`**

---

## 九、烧录与调试

### 9.1 硬件连接

```
ST-Link V2          STM32F103VET6 开发板
-----------          ------------------
SWDIO  ------------>  PA13 (SWDIO)
SWCLK  ------------>  PA14 (SWCLK)
GND    ------------>  GND
3.3V   ------------>  3.3V (可选，若开发板自供电可不接)
```

### 9.2 OpenOCD 烧录命令

```bash
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
  -c "program build/STM32F103VET6_Demo.elf verify reset exit"
```

### 9.3 OpenOCD GDB Server

```bash
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg
```

然后在另一个终端：
```bash
arm-none-eabi-gdb build/STM32F103VET6_Demo.elf
(gdb) target remote localhost:3333
(gdb) load
(gdb) continue
```

---

## 十、遇到的问题与解决方案

### 问题 1: ARM GCC 安装困难
**现象**: `brew install --cask gcc-arm-embedded` 下载 140MB 的 pkg 文件多次超时
**解决**: 改用 formula 版本 `brew install arm-none-eabi-gcc`，并适配无 newlib 环境

### 问题 2: 缺少标准头文件
**现象**: 编译报错 `stdint.h: No such file or directory`
**解决**: 创建最小化的 `Core/Inc/stdint.h` 和 `Core/Inc/stddef.h`

### 问题 3: 链接器找不到 libc.a
**现象**: `cannot find libc.a: Invalid argument`
**解决**: 
1. Makefile 中使用 `-nostdlib` 选项
2. 修改链接器脚本，移除 `/DISCARD/` 中对 libc/libm/libgcc 的引用

### 问题 4: undefined reference to `__libc_init_array`
**现象**: 启动文件引用了 newlib 的初始化函数
**解决**: 在 `syscalls.c` 中提供空实现 `void __libc_init_array(void) {}`

### 问题 5: CMSIS 头文件宏名称差异
**现象**: `RCC_CFGR_PLLSRC_HSE` 未定义
**解决**: 查阅 `stm32f103xe.h`，使用正确的宏 `RCC_CFGR_PLLSRC`

---

## 十一、后续优化建议

1. **安装完整版 ARM GCC**: 如需 printf/malloc 等标准库，建议安装 `gcc-arm-embedded` cask 版
2. **添加 UART 驱动**: 实现串口输出，便于调试打印
3. **添加 FreeRTOS**: 如需多任务调度，可集成 RTOS
4. **使用 STM32CubeMX 生成 HAL 版本**: 对于复杂项目，HAL 库可提高开发效率
5. **配置 Git 版本控制**: 初始化仓库，管理代码版本

---

## 十二、参考资源

- [STM32F103xE 参考手册 (RM0008)](https://www.st.com/resource/en/reference_manual/rm0008-stm32f101xx-stm32f102xx-stm32f103xx-stm32f105xx-and-stm32f107xx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- [STM32F103VET6 数据手册](https://www.st.com/resource/en/datasheet/stm32f103ve.pdf)
- [ARM CMSIS 文档](https://arm-software.github.io/CMSIS_5/Core/html/index.html)
- [OpenOCD 文档](http://openocd.org/doc/html/Debug-Adapter-Configuration.html)
- [STM32CubeMX 下载页面](https://www.st.com/en/development-tools/stm32cubemx.html)

---

*本文档由 KimiCode 自动生成，记录了从环境搭建到项目创建的完整过程。*
