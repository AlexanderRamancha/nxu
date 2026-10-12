# TCB allow-list: foundation/, silicon/, image/, board/ (data).
# research/ must never appear in TCB_SRC.
#
# Include model (project-wide, not per file):
#   -Iinclude   for <nxu/...>
#   -I.         for <silicon/...>
# Header changes rebuild objects via -MMD -MP.

ifeq ($(ZIG),)
ifneq ($(wildcard probe/host/zig/zig),)
ZIG := probe/host/zig/zig
endif
endif

ifeq ($(CROSS),)
ifeq ($(ZIG),)
ifneq ($(shell command -v aarch64-none-elf-gcc 2>/dev/null),)
CROSS := aarch64-none-elf-
else ifneq ($(shell command -v aarch64-linux-gnu-gcc 2>/dev/null),)
CROSS := aarch64-linux-gnu-
endif
endif
endif

CC      := $(if $(ZIG),$(ZIG) cc,$(CROSS)gcc)
OBJCOPY := $(if $(ZIG),$(ZIG) objcopy,$(CROSS)objcopy)

QEMU_BUILD  := build-qemu
FVP_BUILD   := build-fvp
DEBUG_BUILD := build-debug

INCLUDES := -Iinclude -I.

TARGET_FLAGS :=
VECTOR_FLAGS :=
ifneq ($(ZIG),)
TARGET_FLAGS := -target aarch64-freestanding-none
VECTOR_FLAGS := -fno-vectorize -fno-slp-vectorize -mno-implicit-float
else
VECTOR_FLAGS := -fno-tree-vectorize
endif

CFLAGS := $(TARGET_FLAGS) \
	-ffreestanding -fno-builtin -fno-stack-protector \
	-fno-asynchronous-unwind-tables -fno-unwind-tables -fno-ident \
	-fno-pic -fno-pie -mgeneral-regs-only \
	-mstrict-align \
	-Wall -Werror -Os -MMD -MP \
	$(VECTOR_FLAGS) \
	$(INCLUDES)

LDFLAGS := $(TARGET_FLAGS) -nostdlib -static -fno-pie \
	-T image/nxu.ld -Wl,--build-id=none

# Same foundation + silicon + image for every board.
TCB_CORE := \
	silicon/aarch64/entry/start.S \
	silicon/aarch64/exception/vectors.S \
	silicon/aarch64/uart/pl011.c \
	silicon/aarch64/gicv3/gicv3.c \
	silicon/aarch64/psci/psci.c \
	silicon/aarch64/timer/timer.c \
	silicon/aarch64/translate/mmu.c \
	image/image.c \
	foundation/intake/s0.c \
	foundation/intake/intake.c \
	foundation/observe/chronicle.c \
	foundation/observe/log.c \
	foundation/observe/bbox.c \
	foundation/observe/console.c \
	foundation/observe/halt.c \
	foundation/observe/fail.c \
	foundation/observe/sink.c \
	foundation/observe/freeze.c \
	foundation/enter.c \
	foundation/contract/contract.c \
	foundation/facts/fdt.c \
	foundation/facts/facts.c \
	foundation/space/ownership.c \
	foundation/space/mmu_policy.c \
	foundation/exception/exception.c \
	foundation/context/context.c \
	foundation/cpu/cpu.c \
	foundation/cpu/smp.c \
	foundation/signal/signal.c \
	foundation/time/time.c \
	foundation/room/room.c \
	foundation/room/room_entry.S

QEMU_SRC  := $(TCB_CORE) board/qemu-virt/facts.c
FVP_SRC   := $(TCB_CORE) board/fvp-base/facts.c
DEBUG_SRC := $(QEMU_SRC)

QEMU_OBJ  := $(patsubst %,$(QEMU_BUILD)/%.o,$(QEMU_SRC))
FVP_OBJ   := $(patsubst %,$(FVP_BUILD)/%.o,$(FVP_SRC))
DEBUG_OBJ := $(patsubst %,$(DEBUG_BUILD)/%.o,$(DEBUG_SRC))

QEMU_DEP  := $(QEMU_OBJ:.o=.d)
FVP_DEP   := $(FVP_OBJ:.o=.d)
DEBUG_DEP := $(DEBUG_OBJ:.o=.d)

.PHONY: all clean qemu debug fvp fvp-run check

all: $(QEMU_BUILD)/nxu.img

$(QEMU_BUILD)/nxu.elf: $(QEMU_OBJ) image/nxu.ld
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(QEMU_OBJ)

$(QEMU_BUILD)/nxu.img: $(QEMU_BUILD)/nxu.elf
	$(OBJCOPY) -O binary $< $@

$(FVP_BUILD)/nxu.elf: $(FVP_OBJ) image/nxu.ld
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(FVP_OBJ)

$(FVP_BUILD)/nxu.img: $(FVP_BUILD)/nxu.fvp.elf
	$(OBJCOPY) -O binary $< $@

# DRAM-linked ELF for FVP -a (p_vaddr 0x80000000). QEMU keeps VA 0.
$(FVP_BUILD)/nxu.fvp.elf: $(FVP_OBJ) probe/fvp/nxu-load.ld
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -nostdlib -static -fno-pie -T probe/fvp/nxu-load.ld -Wl,--build-id=none -o $@ $(FVP_OBJ)

$(DEBUG_BUILD)/nxu.elf: $(DEBUG_OBJ) image/nxu.ld
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -DNXU_DEBUG=1 $(LDFLAGS) -o $@ $(DEBUG_OBJ)

$(DEBUG_BUILD)/nxu.img: $(DEBUG_BUILD)/nxu.elf
	$(OBJCOPY) -O binary $< $@

$(QEMU_BUILD)/%.o: %
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(FVP_BUILD)/%.o: %
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(DEBUG_BUILD)/%.o: %
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -DNXU_DEBUG=1 -c $< -o $@

check:
	sh probe/check.sh

qemu: $(QEMU_BUILD)/nxu.img
	sh probe/check.sh
	sh probe/qemu/run.sh

debug: $(DEBUG_BUILD)/nxu.img
	sh probe/check.sh
	NXU_IMG=$(DEBUG_BUILD)/nxu.img NXU_LOG=probe/evidence/qemu-debug.log sh probe/qemu/run.sh

fvp: $(FVP_BUILD)/nxu.img $(FVP_BUILD)/nxu.fvp.elf
	sh probe/check.sh
	sh probe/fvp/run.sh

fvp-run: $(FVP_BUILD)/nxu.img $(FVP_BUILD)/nxu.fvp.elf
	sh probe/check.sh
	NXU_FVP_INTERACTIVE=1 sh probe/fvp/run.sh

clean:
	rm -rf $(QEMU_BUILD) $(FVP_BUILD) $(DEBUG_BUILD) build

-include $(QEMU_DEP)
-include $(FVP_DEP)
-include $(DEBUG_DEP)
