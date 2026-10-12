# Building NXU

Instructions derived from the recovered `Makefile` and `README.md`. No fresh cross-build, QEMU, or FVP run was performed in the environment that prepared this file.

## Prerequisites

- AArch64 freestanding toolchain (`aarch64-none-elf-gcc` or `aarch64-linux-gnu-gcc`), or Zig (`probe/host/zig/zig` if present).
- QEMU with `virt` machine and GICv3 for `make qemu`.
- ARM FVP Base RevC binary for `make fvp` / `make fvp-run` (not redistributed here).

## Targets

| Command | Effect |
|---|---|
| `make` | Builds `build-qemu/nxu.img` |
| `make check` | Runs `probe/check.sh` source-rule checks |
| `make qemu` | Builds and runs QEMU virt (requires the kernel sentence and application sentence) |
| `make debug` | Debug build with extra notes |
| `make fvp` | Builds `build-fvp/nxu.img` and `nxu.fvp.elf` |
| `make fvp-run` | Runs FVP if the binary is present; otherwise reports NOT EXECUTED |
| `make clean` | Removes build directories |

## Linker scripts

- `image/nxu.ld` — QEMU VA 0
- `probe/fvp/nxu-load.ld` — FVP load at `0x80000000`

## Historical evidence

Successful QEMU and FVP captures dated 2026-09-27 are in `probe/evidence/`. They are not proof that the current branch still passes those runs.
