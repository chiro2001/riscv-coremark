CROSS_COMPILE := riscv32-unknown-elf
CC := $(CROSS_COMPILE)-gcc
LD := $(CC)
DUMP := $(CROSS_COMPILE)-objdump
COPY := $(CROSS_COMPILE)-objcopy

ITERATIONS ?= 1
FREQ ?= 55

BASE_DIR := $(abspath .)
BUILD_DIR := $(BASE_DIR)/../../overlay

PORT_DIR := $(BASE_DIR)
CORE_SRCS := $(PORT_DIR)/core_list_join.c $(PORT_DIR)/core_main.c $(PORT_DIR)/core_matrix.c $(PORT_DIR)/core_state.c $(PORT_DIR)/core_util.c
PORT_SRCS := $(PORT_DIR)/core_portme.c $(PORT_DIR)/syscalls.c $(PORT_DIR)/crt.S $(PORT_DIR)/sifive-uart.c $(PORT_DIR)/printf.c
# CONFIG_FLAGS := -DCOREMARK_ITERATIONS=150
# CONFIG_FLAGS := -DCOREMARK_ITERATIONS=1500
CONFIG_FLAGS := -DCOREMARK_ITERATIONS=$(ITERATIONS) -DDEFINE_CPU_FREQ_MHZ=$(FREQ)
PORT_CFLAGS := -O3 -mcmodel=medany -static -std=gnu99 -fno-common -nostdlib -nostartfiles -lm -lgcc -T $(PORT_DIR)/link.ld -g -I$(PORT_DIR) -g
FLAGS_STR := "$(PORT_CFLAGS) $(XCFLAGS) $(XLFLAGS) $(LFLAGS_END)"
CFLAGS += $(PORT_CFLAGS) $(CONFIG_FLAGS)
CFLAGS += -DFLAGS_STR=\"$(FLAGS_STR)\"

EXE_PREFIX := $(BUILD_DIR)/coremark.perf
FILE_EXE := $(EXE_PREFIX).riscv
FILE_BIN := $(EXE_PREFIX).bin
FILE_DUMP := $(EXE_PREFIX).asm

.PHONY: default
default: $(FILE_BIN) $(FILE_DUMP)

mkbuildir:
	-mkdir -p $(BUILD_DIR)

$(FILE_EXE): mkbuildir
	$(CC) $(CORE_SRCS) $(PORT_SRCS) $(CFLAGS) -o $(FILE_EXE)

$(FILE_BIN): $(FILE_EXE)
	$(COPY) -O binary $^ $@

$(FILE_DUMP): $(FILE_EXE)
	$(DUMP) -d -S $(FILE_EXE) > $(FILE_DUMP)
.PHONY: clean
clean:
	-rm -rf $(FILE_EXE) $(FILE_BIN) $(FILE_DUMP)

.PHONY: force
force:
	$(MAKE) -C $(BASE_DIR) clean
	$(MAKE) -C $(BASE_DIR) default