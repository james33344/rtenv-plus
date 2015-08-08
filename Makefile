TARGET = main
TARGET_STMF4 = main
.DEFAULT_GOAL = all

APP_DIR = 

include app/build.mk

CROSS_COMPILE ?= arm-none-eabi-
CC := $(CROSS_COMPILE)gcc
CFLAGS = -fno-common -ffreestanding -O0 \
         -gdwarf-2 -g3 -Wall -Werror \
         -mcpu=cortex-m3 -mthumb \
         -Wl,-Tmain.ld -nostartfiles \
         -DUSER_NAME=\"$(USER)\"	\
		 -std=c11

CFLAGS_429 = $(CFLAGS) -DSTM32F4

ARCH = CM3
VENDOR = ST
PLAT = STM32F10x

LIBDIR = .
CMSIS_LIB=$(LIBDIR)/libraries/CMSIS/$(ARCH)
STM32_LIB=$(LIBDIR)/libraries/STM32F10x_StdPeriph_Driver

CMSIS_429_LIB=$(LIBDIR)/libraries/CMSIS_429
STM32_429_LIB=$(LIBDIR)/libraries/STM32F4xx_StdPeriph_Driver

CMSIS_PLAT_SRC = $(CMSIS_LIB)/DeviceSupport/$(VENDOR)/$(PLAT)


OUTDIR = build
OUTDIR_429 = build_stmf4
SRCDIR = src \
         $(CMSIS_LIB)/CoreSupport \
         $(STM32_LIB)/src \
         $(CMSIS_PLAT_SRC) \
		 $(APP_DIR)
INCDIR = include \
         $(CMSIS_LIB)/CoreSupport \
         $(STM32_LIB)/inc \
         $(CMSIS_PLAT_SRC) \
		 $(APP_DIR)
INCLUDES = $(addprefix -I,$(INCDIR))

SRCDIR_429 = src_429 \
         $(CMSIS_429_LIB)/ST/STM32F4xx/Source/Templates \
         $(STM32_429_LIB)/src 
INCDIR_429 = include \
         $(CMSIS_429_LIB)/ST/STM32F4xx/Include \
         $(CMSIS_429_LIB)/Include \
         $(STM32_429_LIB)/inc

INCLUDES_429 = $(addprefix -I,$(INCDIR_429))
DATDIR = data
TOOLDIR = tool

SRC = $(wildcard $(addsuffix /*.c,$(SRCDIR))) \
      $(wildcard $(addsuffix /*.s,$(SRCDIR))) \
      $(CMSIS_PLAT_SRC)/startup/gcc_ride7/startup_stm32f10x_md.s
OBJ := $(addprefix $(OUTDIR)/,$(patsubst %.s,%.o,$(SRC:.c=.o)))
DEP = $(OBJ:.o=.o.d)

SRC_429 = $(wildcard $(addsuffix /*.c,$(SRCDIR_429))) \
      $(wildcard $(addsuffix /*.s,$(SRCDIR_429))) \
      $(CMSIS_429_LIB)/ST/STM32F4xx/Source/Templates/gcc_ride7/startup_stm32f4xx.s
OBJ_429 := $(addprefix $(OUTDIR_429)/,$(patsubst %.s,%.o,$(SRC_429:.c=.o)))
DEP_429 = $(OBJ_429:.o=.o.d)
DAT =

MAKDIR = mk
MAK = $(wildcard $(MAKDIR)/*.mk)

include $(MAK)

all: $(OUTDIR)/$(TARGET).bin $(OUTDIR)/$(TARGET).lst

$(OUTDIR)/$(TARGET).bin: $(OUTDIR)/$(TARGET).elf
	@echo "    OBJCOPY "$@
	@$(CROSS_COMPILE)objcopy -Obinary $< $@

$(OUTDIR)/$(TARGET).lst: $(OUTDIR)/$(TARGET).elf
	@echo "    LIST    "$@
	@$(CROSS_COMPILE)objdump -S $< > $@

$(OUTDIR)/$(TARGET).elf: $(OBJ) $(DAT)
	@echo "    LD      "$@
	@echo "    MAP     "$(OUTDIR)/$(TARGET).map
	@$(CROSS_COMPILE)gcc $(CFLAGS) -Wl,-Map=$(OUTDIR)/$(TARGET).map -o $@ $^

$(OUTDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "    CC      "$@
	@$(CROSS_COMPILE)gcc $(CFLAGS) -MMD -MF $@.d -o $@ -c $(INCLUDES) $<

$(OUTDIR)/%.o: %.s
	@mkdir -p $(dir $@)
	@echo "    CC      "$@
	@$(CROSS_COMPILE)gcc $(CFLAGS) -MMD -MF $@.d -o $@ -c $(INCLUDES) $<


stmf4: $(OUTDIR_429)/$(TARGET_STMF4).bin $(OUTDIR_429)/$(TARGET_STMF4).lst

$(OUTDIR_429)/$(TARGET_STMF4).bin: $(OUTDIR_429)/$(TARGET_STMF4).elf
	@echo "    OBJCOPY "$@
	@$(CROSS_COMPILE)objcopy -Obinary $< $@

$(OUTDIR_429)/$(TARGET_STMF4).lst: $(OUTDIR_429)/$(TARGET_STMF4).elf
	@echo "    LIST    "$@
	@$(CROSS_COMPILE)objdump -S $< > $@

$(OUTDIR_429)/$(TARGET_STMF4).elf: $(OBJ_429) $(DAT)
	@echo "    LD      "$@
	@echo "    MAP     "$(OUTDIR_429)/$(TARGET_STMF4).map
	@$(CROSS_COMPILE)gcc $(CFLAGS_429) -Wl,-Map=$(OUTDIR_429)/$(TARGET_STMF4).map -o $@ $^

$(OUTDIR_429)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "    CC      "$@
	@$(CROSS_COMPILE)gcc $(CFLAGS_429) -MMD -MF $@.d -o $@ -c $(INCLUDES_429) $<

$(OUTDIR_429)/%.o: %.s
	@mkdir -p $(dir $@)
	@echo "    CC      "$@
	@$(CROSS_COMPILE)gcc $(CFLAGS_429) -MMD -MF $@.d -o $@ -c $(INCLUDES_429) $<


flash:
	st-flash write $(OUTDIR_429)/main.bin 0x8000000
runstm: 
	@echo " Debuggin..."
	@$(CROSS_COMPILE)gdb $(OUTDIR_429)/main.bin \
		-ex 'target remote :3333' \
		-ex 'monitor reset halt' \
		-ex 'load' \
		-ex 'monitor arm semihosting enable' \
		-ex 'continue'

runstmdbg:
	@echo " Debuggin..."
	@$(CROSS_COMPILE)gdb $(OUTDIR_429)/main.elf \
		-ex 'target remote :3333' \
		-ex 'monitor reset halt' \
		-ex 'load' \
		-ex 'monitor arm semihosting enable' 
	@echo " Start to dgb!!"

openocd:
	openocd -f board/stm32f4discovery.cfg

clean:
	rm -rf $(OUTDIR)
	rm -rf $(OUTDIR_429)

-include $(DEP)
-include $(DEP_429)
