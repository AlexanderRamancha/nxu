# NXU kernel Makefile

CC      = aarch64-linux-gnu-gcc
LD      = aarch64-linux-gnu-ld
OBJCOPY = aarch64-linux-gnu-objcopy

CFLAGS  = -Wall -O2 -ffreestanding -nostdlib \
          -mgeneral-regs-only -mcmodel=tiny -mstrict-align \
          -fno-pic -fno-pie -fno-stack-protector \
          -I. -Iboot/arm64 -Ikernel/include

LDFLAGS = -nostdlib

OBJS    = boot/arm64/start.o kernel/src/init/main.o

all: nxu.bin

nxu.elf: $(OBJS) boot/arm64/link.ld
	$(LD) $(LDFLAGS) -T boot/arm64/link.ld -o $@ $(OBJS)

nxu.bin: nxu.elf
	$(OBJCOPY) -O binary $< $@

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) nxu.elf nxu.bin

# ... your existing variables and rules ...

qemu: nxu.bin
	# Use cortex-a57 for better reliability in some Debian/QEMU setups
	qemu-system-aarch64 \
	  -M virt \
	  -cpu cortex-a57 \
	  -smp 1 \
	  -m 512M \
	  -nographic \
	  -serial mon:stdio \
	  -kernel nxu.bin

.PHONY: all clean qemu