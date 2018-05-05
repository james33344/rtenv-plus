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

ARCH ?= CM3
VENDOR ?= ST
PLAT ?= stm32f10x
PLAT_DIR := platform

TOP ?= $(shell pwd -L)
LIBDIR = .
CMSIS_LIB = $(LIBDIR)/libraries/CMSIS/$(ARCH)
STM32_LIB = $(LIBDIR)/libraries/$(PLAT)_StdPeriph_Driver
CMSIS_PLAT_SRC = $(CMSIS_LIB)/DeviceSupport/$(VENDOR)/$(PLAT)
CMSIS_PLAT_INC = $(CMSIS_PLAT_SRC)/Include


OUTDIR = build
SRCDIR = src \
         $(CMSIS_LIB)/CoreSupport \
         $(STM32_LIB)/src \
         $(CMSIS_PLAT_SRC) \
         $(PLAT_DIR)/$(PLAT) \
         $(APP_DIR)
INCDIR = include \
         $(CMSIS_LIB)/CoreSupport \
         $(STM32_LIB)/inc \
         $(CMSIS_PLAT_INC) \
         $(PLAT_DIR)/$(PLAT) \
         $(APP_DIR)
INCLUDES = $(addprefix -I, $(INCDIR))

DATDIR = data
TOOLDIR = tool

SRC = $(wildcard $(addsuffix /*.c,$(SRCDIR))) \
      $(wildcard $(addsuffix /*.s,$(SRCDIR))) \
      $(CMSIS_PLAT_SRC)/startup/gcc_ride7/startup_$(PLAT).s
OBJ := $(addprefix $(OUTDIR)/,$(patsubst %.s,%.o,$(SRC:.c=.o)))
DEP = $(OBJ:.o=.o.d)

DAT =

MAKDIR = mk
MAK = $(wildcard $(MAKDIR)/*.mk)

PLAT_LIST := $(subst $(TOP)/$(PLAT_DIR)/,,$(wildcard $(TOP)/$(PLAT_DIR)/*))

include $(MAK)
-include $(DEP)
