# STM32F103VET6 Makefile
# Target: Cortex-M3, 72MHz, 512KB Flash, 64KB SRAM
# Supports dual-target build: Bootloader + Application

######################################
# common variables
######################################
DEBUG = 1
OPT = -Og
BUILD_DIR = build
BOOT_BUILD_DIR = build_boot

PREFIX = arm-none-eabi-
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
LD = $(PREFIX)gcc
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S

MCU = -mcpu=cortex-m3 -mthumb

######################################
# C flags common
######################################
C_DEFS = -DSTM32F103xE
CFLAGS = $(C_DEFS) $(OPT) -Wall -fdata-sections -ffunction-sections

ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
endif

CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"

######################################
# Bootloader build
######################################
BOOT_TARGET = bootloader
BOOT_C_SOURCES = \
Bootloader/Src/main.c \
Bootloader/Src/flash.c \
Bootloader/Src/iap_bootloader.c \
Core/Src/system_clock.c \
Core/Src/gpio_ctrl.c \
Core/Src/usart_ctrl.c \
Core/Src/stm32f1xx_it.c \
Core/Src/syscalls.c \
Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/system_stm32f1xx.c

BOOT_ASM_SOURCES = \
Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/gcc/startup_stm32f103xe.s

BOOT_C_INCLUDES = \
-IBootloader/Inc \
-ICore/Inc \
-IDrivers/CMSIS/Core/Include \
-IDrivers/CMSIS/Device/ST/STM32F1xx/Include

BOOT_CFLAGS = $(MCU) $(CFLAGS) $(BOOT_C_INCLUDES)
BOOT_ASFLAGS = $(MCU) $(C_DEFS) $(BOOT_C_INCLUDES) $(OPT)
BOOT_LDSCRIPT = Bootloader/LinkerScript/STM32F103VETx_BOOT.ld
BOOT_LDFLAGS = $(MCU) -nostdlib -T$(BOOT_LDSCRIPT) \
-lgcc \
-Wl,-Map=$(BOOT_BUILD_DIR)/$(BOOT_TARGET).map,--cref -Wl,--gc-sections

# Use path-based object names to avoid collision with App
BOOT_OBJECTS = $(addprefix $(BOOT_BUILD_DIR)/,$(BOOT_C_SOURCES:.c=.o))
BOOT_OBJECTS += $(addprefix $(BOOT_BUILD_DIR)/,$(BOOT_ASM_SOURCES:.s=.o))

######################################
# Application build
######################################
APP_TARGET = STM32F103VET6_Demo
APP_C_SOURCES = \
Core/Src/main.c \
Core/Src/system_clock.c \
Core/Src/gpio_ctrl.c \
Core/Src/pwm_ctrl.c \
Core/Src/dac7512.c \
Core/Src/usart_ctrl.c \
Core/Src/protocol_sac.c \
Core/Src/dispenser.c \
Core/Src/led_indicator.c \
Core/Src/watchdog.c \
Core/Src/ads1256.c \
Core/Src/params_storage.c \
Core/Src/iap_app.c \
Core/Src/stm32f1xx_it.c \
Core/Src/syscalls.c \
Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/system_stm32f1xx.c

APP_ASM_SOURCES = \
Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/gcc/startup_stm32f103xe.s

APP_C_INCLUDES = \
-ICore/Inc \
-IDrivers/CMSIS/Core/Include \
-IDrivers/CMSIS/Device/ST/STM32F1xx/Include

APP_CFLAGS = $(MCU) $(CFLAGS) $(APP_C_INCLUDES)
APP_ASFLAGS = $(MCU) $(C_DEFS) $(APP_C_INCLUDES) $(OPT)
APP_LDSCRIPT = LinkerScript/STM32F103VETx_APP.ld
APP_LDFLAGS = $(MCU) -nostdlib -T$(APP_LDSCRIPT) \
-lgcc \
-Wl,-Map=$(BUILD_DIR)/$(APP_TARGET).map,--cref -Wl,--gc-sections

APP_OBJECTS = $(addprefix $(BUILD_DIR)/,$(APP_C_SOURCES:.c=.o))
APP_OBJECTS += $(addprefix $(BUILD_DIR)/,$(APP_ASM_SOURCES:.s=.o))

######################################
# default target: build both
######################################
all: bootloader app

######################################
# Bootloader rules
######################################
bootloader: $(BOOT_BUILD_DIR)/$(BOOT_TARGET).elf $(BOOT_BUILD_DIR)/$(BOOT_TARGET).hex $(BOOT_BUILD_DIR)/$(BOOT_TARGET).bin

$(BOOT_BUILD_DIR)/%.o: %.c Makefile | $(BOOT_BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) -c $(BOOT_CFLAGS) $< -o $@

$(BOOT_BUILD_DIR)/%.o: %.s Makefile | $(BOOT_BUILD_DIR)
	@mkdir -p $(dir $@)
	$(AS) -c $(BOOT_ASFLAGS) $< -o $@

$(BOOT_BUILD_DIR)/$(BOOT_TARGET).elf: $(BOOT_OBJECTS) Makefile
	$(LD) $(BOOT_OBJECTS) $(MCU) $(BOOT_LDFLAGS) -o $@
	$(SZ) $@

$(BOOT_BUILD_DIR)/$(BOOT_TARGET).hex: $(BOOT_BUILD_DIR)/$(BOOT_TARGET).elf | $(BOOT_BUILD_DIR)
	$(HEX) $< $@

$(BOOT_BUILD_DIR)/$(BOOT_TARGET).bin: $(BOOT_BUILD_DIR)/$(BOOT_TARGET).elf | $(BOOT_BUILD_DIR)
	$(BIN) $< $@

######################################
# Application rules
######################################
app: $(BUILD_DIR)/$(APP_TARGET).elf $(BUILD_DIR)/$(APP_TARGET).hex $(BUILD_DIR)/$(APP_TARGET).bin

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) -c $(APP_CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(AS) -c $(APP_ASFLAGS) $< -o $@

$(BUILD_DIR)/$(APP_TARGET).elf: $(APP_OBJECTS) Makefile
	$(LD) $(APP_OBJECTS) $(MCU) $(APP_LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/$(APP_TARGET).hex: $(BUILD_DIR)/$(APP_TARGET).elf | $(BUILD_DIR)
	$(HEX) $< $@

$(BUILD_DIR)/$(APP_TARGET).bin: $(BUILD_DIR)/$(APP_TARGET).elf | $(BUILD_DIR)
	$(BIN) $< $@

######################################
# directories
######################################
$(BUILD_DIR):
	mkdir -p $@

$(BOOT_BUILD_DIR):
	mkdir -p $@

######################################
# clean
######################################
clean:
	rm -rf $(BUILD_DIR) $(BOOT_BUILD_DIR)

######################################
# flash targets (openocd)
######################################
flash-boot: $(BOOT_BUILD_DIR)/$(BOOT_TARGET).elf
	openocd -f interface/stlink.cfg -f target/stm32f1x.cfg -c "program $< verify reset exit"

flash-app: $(BUILD_DIR)/$(APP_TARGET).elf
	openocd -f interface/stlink.cfg -f target/stm32f1x.cfg -c "program $< verify reset exit"

flash-all: $(BOOT_BUILD_DIR)/$(BOOT_TARGET).elf $(BUILD_DIR)/$(APP_TARGET).elf
	openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
	-c "program $(BOOT_BUILD_DIR)/$(BOOT_TARGET).elf verify" \
	-c "program $(BUILD_DIR)/$(APP_TARGET).elf verify reset exit"

######################################
# merge hex files for production
######################################
merge-hex: $(BOOT_BUILD_DIR)/$(BOOT_TARGET).hex $(BUILD_DIR)/$(APP_TARGET).hex
	cat $(BOOT_BUILD_DIR)/$(BOOT_TARGET).hex $(BUILD_DIR)/$(APP_TARGET).hex > $(BUILD_DIR)/full_firmware.hex
	@echo "Merged HEX created: $(BUILD_DIR)/full_firmware.hex"

debug:
	openocd -f interface/stlink.cfg -f target/stm32f1x.cfg

######################################
# dependencies
######################################
-include $(wildcard $(BUILD_DIR)/**/*.d)
-include $(wildcard $(BOOT_BUILD_DIR)/**/*.d)

.PHONY: all clean bootloader app flash-boot flash-app flash-all merge-hex debug
