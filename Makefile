CC := aarch64-linux-gnu-gcc
LD := aarch64-linux-gnu-ld
OBJCOPY := aarch64-linux-gnu-objcopy
OBJDUMP := aarch64-linux-gnu-objdump
SIZE := aarch64-linux-gnu-size
GDB := gdb-multiarch

BUILD_DIR := build

ASM_SOURCES := \
	arch/arm64/boot/start.S \
	arch/arm64/exception/entry.S

C_SOURCES := \
	arch/arm64/exception/exceptions.c \
	kernel/init/main.c \
	kernel/mmio.c \
	kernel/timer.c \
	drivers/uart/pl011.c \
	arch/arm64/gic/gic.c \
	kernel/interrupt/interrupt_manager.c \
	arch/arm64/gic/gic_interrupt.c \
	kernel/interrupt/interrupt_backend.c \
	arch/arm64/psci/psci.c \
	kernel/cpu/cpu.c \
	arch/arm64/platform/platform.c \
	kernel/init/interrupt_test.c

OBJS := $(patsubst %.S,$(BUILD_DIR)/%.o,$(ASM_SOURCES)) \
	$(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES))

LDSCRIPT := arch/arm64/boot/link.ld

INCLUDES := \
	-I. \
	-Iarch/arm64 \
	-Iarch/arm64/include \
	-Ikernel/include \
	-Idrivers

CFLAGS_COMMON := \
	-Wall \
	-Wextra \
	-ffreestanding \
	-nostdlib \
	-mgeneral-regs-only \
	-mcmodel=tiny \
	-mstrict-align \
	-fno-pic \
	-fno-pie \
	-fno-stack-protector \
	$(INCLUDES)

CFLAGS_RELEASE := $(CFLAGS_COMMON) -O2
CFLAGS_DEBUG := $(CFLAGS_COMMON) -O0 -g3 -DDEBUG -DNXU_RUN_INTERRUPT_TEST

CFLAGS ?= $(CFLAGS_RELEASE)
LDFLAGS := -nostdlib

.PHONY: all release debug clean qemu qemu-debug disasm symbols size help

all: release

release: CFLAGS := $(CFLAGS_RELEASE)
release: $(BUILD_DIR)/nxu.bin

debug: CFLAGS := $(CFLAGS_DEBUG)
debug: $(BUILD_DIR)/nxu.bin

$(BUILD_DIR)/nxu.bin: $(BUILD_DIR)/nxu.elf
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/nxu.elf: $(OBJS) $(LDSCRIPT)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -T $(LDSCRIPT) -o $@ $(OBJS)

$(BUILD_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

qemu: $(BUILD_DIR)/nxu.bin
	qemu-system-aarch64 -M virt,gic-version=3 -cpu cortex-a57 -smp 2 -m 512M \
		-nographic -serial mon:stdio -kernel $(BUILD_DIR)/nxu.bin

qemu-debug: debug
	@echo "QEMU waiting for GDB on port 1234"
	qemu-system-aarch64 -M virt,gic-version=3 -cpu cortex-a57 -smp 2 -m 512M \
		-nographic -serial mon:stdio -kernel $(BUILD_DIR)/nxu.bin \
		-S -gdb tcp::1234

disasm: $(BUILD_DIR)/nxu.elf
	$(OBJDUMP) -d -S $< > $(BUILD_DIR)/disasm.txt

symbols: $(BUILD_DIR)/nxu.elf
	$(OBJDUMP) -t $< | sort > $(BUILD_DIR)/symbols.txt

size: $(BUILD_DIR)/nxu.elf
	$(SIZE) $<

clean:
	rm -rf $(BUILD_DIR)

help:
	@echo "make"
	@echo "make release"
	@echo "make debug"
	@echo "make qemu"
	@echo "make qemu-debug"
	@echo "make clean"