# NXU Kernel Makefile (reliable version)

CC      := aarch64-linux-gnu-gcc
LD      := aarch64-linux-gnu-ld
OBJCOPY := aarch64-linux-gnu-objcopy

BUILD_DIR := build

# Explicit list of sources (no auto-detect problems)
ASM_SOURCES := \
	arch/arm64/boot/start.S \
	arch/arm64/exception/entry.S

C_SOURCES := \
	arch/arm64/exception/exceptions.c \
	kernel/init/main.c \
	drivers/uart/pl011.c

OBJS := $(patsubst %.S,$(BUILD_DIR)/%.o,$(ASM_SOURCES)) \
        $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES))

CFLAGS := -Wall -O2 -ffreestanding -nostdlib \
          -mgeneral-regs-only -mcmodel=tiny -mstrict-align \
          -fno-pic -fno-pie -fno-stack-protector \
          -I. \
          -Iarch/arm64/include \
          -Ikernel/include \
          -Idrivers

LDFLAGS := -nostdlib

all: $(BUILD_DIR)/nxu.bin

$(BUILD_DIR)/nxu.bin: $(BUILD_DIR)/nxu.elf
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/nxu.elf: $(OBJS) arch/arm64/boot/link.ld
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -T arch/arm64/boot/link.ld -o $@ $(OBJS)

# Assembly
$(BUILD_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	@echo "  AS  $<"
	$(CC) $(CFLAGS) -c $< -o $@

# C
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "  CC  $<"
	$(CC) $(CFLAGS) -c $< -o $@

qemu: $(BUILD_DIR)/nxu.bin
	qemu-system-aarch64 \
		-M virt \
		-cpu cortex-a57 \
		-smp 1 \
		-m 512M \
		-nographic \
		-serial mon:stdio \
		-kernel $(BUILD_DIR)/nxu.bin

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all qemu clean
