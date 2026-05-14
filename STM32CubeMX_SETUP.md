# STM32CubeMX 安装与使用指南

## 1. 下载 STM32CubeMX

STM32CubeMX 是 ST 官方提供的图形化配置工具，用于生成初始化代码。

### 下载地址
1. 访问 ST 官网: https://www.st.com/en/development-tools/stm32cubemx.html
2. 点击 **"Get Software"** 按钮
3. 注册/登录 ST 账号（免费）
4. 下载 macOS 版本的安装包

### 国内镜像（可选）
如果官网下载慢，可以尝试 ST 中文社区或镜像站。

## 2. 安装 STM32CubeMX

```bash
# 下载完成后，运行安装包
open ~/Downloads/SetupSTM32CubeMX-*.exe

# 或使用命令行安装（如果支持）
java -jar ~/Downloads/SetupSTM32CubeMX-*.exe
```

> **注意**: STM32CubeMX 是基于 Java 的应用程序，系统需要安装 JRE 8 或更高版本。

## 3. 使用 STM32CubeMX 创建项目

### 步骤 1: 新建工程
1. 打开 STM32CubeMX
2. 点击 **"File" → "New Project"**
3. 在搜索框输入 **"STM32F103VET6"**
4. 选择芯片，点击 **"Start Project"**

### 步骤 2: 配置时钟
1. 点击 **"Clock Configuration"** 标签页
2. 设置:
   - **HSE**: Crystal/Ceramic Resonator (8MHz)
   - **PLL Source**: HSE
   - **PLL Mul**: x9
   - **SYSCLK**: 72 MHz
   - **AHB Prescaler**: /1
   - **APB1 Prescaler**: /2
   - **APB2 Prescaler**: /1
3. 按 **Enter** 键自动解决时钟冲突

### 步骤 3: 配置 GPIO
1. 点击 **"Pinout & Configuration"** 标签页
2. 找到 **PC13** 引脚（通常在芯片封装的右下角附近）
3. 点击 PC13，选择 **"GPIO_Output"**
4. 在左侧 **"GPIO"** 配置中:
   - **GPIO mode**: Output Push Pull
   - **GPIO Pull-up/Pull-down**: No pull-up and no pull-down
   - **Maximum output speed**: Low
   - **User Label**: LED

### 步骤 4: 配置调试接口
1. 在左侧找到 **"SYS"**
2. **Debug**: Serial Wire (启用 SWD 调试接口)

### 步骤 5: 生成代码
1. 点击 **"Project" → "Generate Code"**
2. 配置项目:
   - **Project Name**: STM32F103VET6_CubeMX
   - **Project Location**: /Users/kimohu/Documents/WORKSPACE
   - **Toolchain / IDE**: **Makefile**
3. 在 **"Code Generator"** 标签页:
   - 勾选 **"Copy only the necessary library files"**
   - 勾选 **"Generate peripheral initialization as a pair of '.c/.h' files per peripheral"**
4. 点击 **"GENERATE CODE"**

## 4. 将 CubeMX 代码与现有项目整合

### 方法一: 直接替换（推荐新手）
1. 删除当前项目的 `DemoCore/Src` 和 `DemoCore/Inc` 目录下的所有文件
2. 复制 CubeMX 生成的 `Src/` 和 `Inc/` 目录内容到对应位置
3. 修改 Makefile 添加 HAL 库路径和源文件

### 方法二: 手动合并（推荐有经验的开发者）
1. 对比 CubeMX 生成的 `main.c` 和当前项目的 `main.c`
2. 将 CubeMX 生成的初始化代码（`MX_GPIO_Init()`, `SystemClock_Config()` 等）合并到当前项目
3. 保留当前项目的应用逻辑代码

## 5. 添加 HAL 库到 Makefile

如果切换到 HAL 库版本，需要在 Makefile 中添加:

```makefile
# HAL 库路径
C_SOURCES += \
Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal.c \
Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_cortex.c \
Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio.c \
Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc.c \
Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc_ex.c

C_INCLUDES += \
-IDrivers/STM32F1xx_HAL_Driver/Inc \
-IDrivers/STM32F1xx_HAL_Driver/Inc/Legacy

C_DEFS += -DUSE_HAL_DRIVER
```

## 6. 常用快捷键

| 快捷键 | 功能 |
|--------|------|
| `Ctrl+S` | 保存配置 |
| `Alt+K` | 自动排列引脚 |
| `Alt+Enter` | 引脚配置详情 |
| `Ctrl+Shift+G` | 生成代码 |

## 7. 注意事项

1. **不要直接修改 CubeMX 生成的代码中的 USER CODE 区域外的部分**，否则重新生成时会被覆盖
2. 每次修改配置后，点击 **"Generate Code"** 重新生成
3. 如果需要添加自定义代码，请放在 `/* USER CODE BEGIN xxx */` 和 `/* USER CODE END xxx */` 之间

## 8. 替代方案

如果不希望使用 STM32CubeMX，可以直接在当前项目基础上继续开发：
- 本项目使用纯寄存器操作，不依赖 HAL 库
- 代码更精简，执行效率更高
- 适合学习和理解底层原理
