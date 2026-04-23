#
# Copyright 2021, Breakaway Consulting Pty. Ltd.
#
# SPDX-License-Identifier: BSD-2-Clause
#
ifeq ($(strip $(BUILD_DIR)),)
$(error BUILD_DIR must be specified)
endif

ifeq ($(strip $(MICROKIT_SDK)),)
$(error MICROKIT_SDK must be specified)
endif

ifeq ($(strip $(MICROKIT_BOARD)),)
$(error MICROKIT_BOARD must be specified)
endif

ifeq ($(strip $(MICROKIT_CONFIG)),)
$(error MICROKIT_CONFIG must be specified)
endif

BOARD_DIR := $(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)

ARCH := ${shell grep 'CONFIG_SEL4_ARCH  ' $(BOARD_DIR)/include/kernel/gen_config.h | cut -d' ' -f4}

ifeq ($(ARCH),aarch64)
  TARGET_TRIPLE := aarch64-none-elf
  CFLAGS_ARCH := -mstrict-align
else ifeq ($(ARCH),riscv64)
  TARGET_TRIPLE := riscv64-unknown-elf
  CFLAGS_ARCH := -march=rv64imafdc_zicsr_zifencei -mabi=lp64d
else
$(error Unsupported ARCH)
endif

ifeq ($(strip $(LLVM)),True)
  CC := clang -target $(TARGET_TRIPLE)
  AS := clang -target $(TARGET_TRIPLE)
  LD := ld.lld
else
  CC := $(TARGET_TRIPLE)-gcc
  LD := $(TARGET_TRIPLE)-ld
  AS := $(TARGET_TRIPLE)-as
endif

MICROKIT_TOOL ?= $(MICROKIT_SDK)/bin/microkit

SERVER_CORE0_OBJS := server_core0.o
SERVER_CORE1_OBJS := server_core1.o
CLIENT_CORE0_OBJS := client_core0.o
CLIENT_CORE1_OBJS := client_core1.o

IMAGES := server_core0.elf server_core1.elf client_core0.elf client_core1.elf
CFLAGS := -nostdlib -ffreestanding -g -O3 -Wall  -Wno-unused-function -Werror -I$(BOARD_DIR)/include $(CFLAGS_ARCH)
LDFLAGS := -L$(BOARD_DIR)/lib
LIBS := -lmicrokit -Tmicrokit.ld

IMAGE_FILE = $(BUILD_DIR)/loader.img
REPORT_FILE = $(BUILD_DIR)/report.txt

all: $(IMAGE_FILE)

$(BUILD_DIR)/%.o: %.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile
	$(AS) -g -mcpu=$(CPU) $< -o $@

$(BUILD_DIR)/server_core0.elf: $(addprefix $(BUILD_DIR)/, $(SERVER_CORE0_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(BUILD_DIR)/server_core1.elf: $(addprefix $(BUILD_DIR)/, $(SERVER_CORE1_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(BUILD_DIR)/client_core0.elf: $(addprefix $(BUILD_DIR)/, $(CLIENT_CORE0_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(BUILD_DIR)/client_core1.elf: $(addprefix $(BUILD_DIR)/, $(CLIENT_CORE1_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(IMAGE_FILE) $(REPORT_FILE): $(addprefix $(BUILD_DIR)/, $(IMAGES)) passive_server.system
	$(MICROKIT_TOOL) passive_server.system --search-path $(BUILD_DIR) --board $(MICROKIT_BOARD) \
  --config $(MICROKIT_CONFIG) -o $(IMAGE_FILE) -r $(REPORT_FILE) --capdl-json $(BUILD_DIR)/capdl_spec.json
