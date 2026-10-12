# FOUNDATION_EVIDENCE

Do not claim execution that did not happen.

## Platforms

| Platform | Role | Executed |
|---|---|---|
| QEMU `virt,gic-version=3` cortex-a57 | validation instrument | yes |
| ARM FVP Base | validation instrument | yes — application line, no halt. Earlier timer halt is H9 and H10 |
| Real ARM64 hardware | architectural target | **no** |

QEMU addresses are board facts (`board/qemu-virt/facts.c`). They are not architecture.

## Build

- Toolchain: `probe/host/zig/zig cc -target aarch64-freestanding-none`
- Include roots: `-Iinclude -I.`
- Header deps: `-MMD -MP`
- Images: `build-qemu/nxu.img`, `build-fvp/nxu.img`, `build-debug/nxu.img`
- Static checks: `probe/check.sh`

## QEMU virt, smp=4, 128M

Log: `probe/evidence/qemu-virt.log`.

Live console includes `[NXU] NXU kernel execution is established.` and `[NXU] The first NXU application has completed.`
Capture requires both. It does not accept a hex dump substitute.

## Negative tests

| Test | Method | Result |
|---|---|---|
| Bad image magic | `probe/cases/neg-header.sh` | no `[nxu] note boot kernel` |

## FVP

Model present:

`/home/alwaysalex/fvp/Base_RevC_AEMvA_pkg/models/Linux64_GCC-9.3/FVP_Base_RevC-2xAEMvA`

2026-09-26, `make fvp`, SMP 1, load `0x80000000`, return code 124 (`timeout` SIGTERM after NXU had already halted).

Unmodified image, byte-identical to `probe/evidence/fvp-uart-repro.log` and to the capture preserved in `probe/evidence/prior-2026-09-26/fvp-uart.log`:

- Boot, DTB absent, RAM, translation, vectors, one CPU, GIC wake all printed.
- `[NXU][TIME] Timer configured @ 0x1a`
- `t0=0x184b76`, `tnow=0x3133f5a`, `tval=0xfd144a6c`, `istatus=1`
- Halt `0x8000000d` (`NXU_FAIL_TIMER`) with arguments `(0x1a, 2)`
- No `[nxu] note boot kernel`

Same image, one added failure-path read (`nxu_timer_observe_delivery`), log `probe/evidence/fvp-uart.log`:

| Register | Value | Reading |
|---|---|---|
| GICR_WAKER | `0` | ProcessorSleep and ChildrenAsleep clear |
| GICD_CTLR | `0x12` | readback of the value NXU wrote |
| GICR_IGROUPR0 | `0` | group write not visible |
| GICR_ISENABLER0 | `0` | enable write not visible |
| GICR_ISPENDR0 | `0` | no pending INTID in this view |
| GICR_IGRPMODR0 | `0` | |
| DAIF | `0x340` | IRQ unmasked, FIQ masked |
| HCR_EL2 | `0x08000000` | TGE set |
| ISR_EL1 | `0` | PE IRQ and FIQ inputs idle |
| ICC_IGRPEN1_EL1 | `1` | Non-secure Group 1 enabled |
| ICC_HPPIR1_EL1 | `1023` | no Group 1 INTID |
| ICC_HPPIR0_EL1 | `1023` | no Group 0 INTID in this view |
| CNTHP ISTATUS | `1` | timer condition true |

`t0` and `tnow` each moved by one count relative to the unmodified run. The halt did not.

`ICC_IGRPEN1_EL3` read back `0x3` after a write of `7` in `silicon/aarch64/entry/start.S`. In the current AArch64 register map, bits `[63:2]` are RES0, bit `[1]` is EnableGrp1S, and bit `[0]` is EnableGrp1NS. The readback is that write with bit 2 discarded. It is not a dropped EL2 enable.

**OPEN.** The enable of INTID 26 performed by `gicr_frame_enable` is not visible from Non-secure EL2, and the PE is not signaled. A Secure-EL3 readback of the same redistributor registers, taken before the drop, is the measurement that separates "the write did not land" from "Non-secure cannot see it".

The logging change did not clear this halt. The failure line names `time.c` `nxu_s11_time` and code `0x8000000d` with arguments `(0x1a, 2)`. `ISENABLER0` still reads 0. `ISTATUS` is still 1.

## Real hardware

**NOT EXECUTED.**
