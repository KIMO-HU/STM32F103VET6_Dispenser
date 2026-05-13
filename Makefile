# STM32F103VET6 Makefile
# Target: Cortex-M3, 72MHz, 512KB Flash, 64KB SRAM

######################################
# target
######################################
TARGET = STM32F103VET6_Demo

######################################
# building variables
######################################
DEBUG = 1
OPT = -Og

######################################
# paths
######################################
BUILD_DIR = build

######################################
# source
######################################
C_SOURCES = \
NewCore/Src/main.c \
NewCore/Src/system_clock.c \
NewCore/Src/gpio_ctrl.c \
NewCore/Src/usart_ctrl.c \
NewCore/Src/protocol_sac.c \
NewCore/Src/dispenser.c \
NewCore/Src/stm32f1xx_it.c \
NewCore/Src/syscalls.c \
Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/system_stm32f1xx.c

ASM_SOURCES = \
Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/gcc/startup_stm32f103xe.s

######################################
# C includes
######################################
C_INCLUDES = \
-INewCore/Inc \
-IDrivers/CMSIS/Core/Include \
-IDrivers/CMSIS/Device/ST/STM32F1xx/Include

######################################
# macros for gcc
######################################
AS_DEFS =

C_DEFS = \
-DSTM32F103xE

######################################
# AS flags
######################################
ASFLAGS = $(C_DEFS) $(C_INCLUDES) $(OPT)

######################################
# C flags
######################################
CFLAGS = $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections

ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
endif

# Generate dependency information
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"

######################################
# LDFLAGS
######################################
LDSCRIPT = LinkerScript/STM32F103VETx_FLASH.ld

# 使用 -nostdlib 因为我们没有完整的 newlib，只链接 libgcc.a 处理编译器内部函数
LDFLAGS = $(MCU) -nostdlib -T$(LDSCRIPT) \
-lgcc \
-Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections

######################################
# Toolchain
######################################
PREFIX = arm-none-eabi-
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
AR = $(PREFIX)ar
LD = $(PREFIX)gcc
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S

######################################
# MCU flags
######################################
MCU = -mcpu=cortex-m3 -mthumb

# default action: build all
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

######################################
# build the application
######################################
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	$(CC) -c $(MCU) $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(AS) -c $(MCU) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	$(LD) $(OBJECTS) $(MCU) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf | $(BUILD_DIR)
	$(HEX) $< $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf | $(BUILD_DIR)
	$(BIN) $< $@

$(BUILD_DIR):
	mkdir -p $@

######################################
# clean up
######################################
clean:
	rm -rf $(BUILD_DIR)

######################################
# openocd flash
######################################
flash: $(BUILD_DIR)/$(TARGET).elf
	openocd -f interface/stlink.cfg -f target/stm32f1x.cfg -c "program $< verify reset exit"

######################################
# openocd debug server
######################################
debug:
	openocd -f interface/stlink.cfg -f target/stm32f1x.cfg

######################################
# dependencies
######################################
-include $(wildcard $(BUILD_DIR)/*.d)

.PHONY: all clean flash debug
