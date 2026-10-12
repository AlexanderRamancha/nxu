# H10 — Room boundaries, and the FVP compare that stayed in the past

History, not authority. The kernel source and the UART captures are the evidence. H9 remains the record of the Secure group assignment. This file does not replace it.

## Question

After both instruments reached an EL1 kernel, CNTP PPI 30, and a self-IPI, the next measured property was a separation boundary: two subjects, one active, private pages, a kernel-copied value or a rejection, a contained fault, and a deadline that preempts a subject that does not leave. QEMU completed that sequence. FVP halted on the first entry.

The question was not "does the room code compile." The question was why a compare write that is in the future on QEMU is still behind `CNTPCT_EL0` on FVP, and what entry rule is actually enforced once the deadline can be armed again.

## What was assumed

A fresh `CNTFRQ_EL0` read divided by ten is a safe slice length. Disabling `CNTP_CTL_EL0`, writing `CNTP_TVAL_EL0`, and enabling the timer clears `ISTATUS`. The kernel may enter either room whenever it wants. A pulse is only a copy, not permission to run the destination.

The first assumption is the one FVP contradicted.

## Hardware

`CNTP_TVAL_EL0` is a signed 32-bit view. A write sets `CNTP_CVAL_EL0` to `CNTPCT_EL0` plus that value. A write of zero sets the compare to now, so the timer condition is already met. `ISTATUS` is that condition. It is not cleared by writing `CNTP_CTL_EL0` alone.

`CNTFRQ_EL0` is writable only at the highest implemented exception level. FVP Base enters at EL3. QEMU virt enters at EL1 and does not take the drop. `foundation/facts/facts.c` already replaces a zero frequency read with `nxu_board.timer_freq` when the board is present. The chronicle value `0x5f5e100` on FVP is therefore not, by itself, proof that EL1 read 100 MHz. It is the accepted fact: the register, or the board candidate when the register is zero.

FVP's counter is configured at 100 MHz (`bp.refcounter.base_frequency=100000000`). QEMU's board candidate is 62.5 MHz. The slice uses the accepted fact divided by ten. That is 10,000,000 ticks on FVP and 6,250,000 ticks on QEMU. Both fit in a positive signed 32-bit TVAL.

## FACT — FVP halt before the re-arm change

`probe/evidence/fvp-uart.log` from the image that wrote `nxu_cntfrq()/10` into `CNTP_TVAL_EL0` after forcing `CNTP_CTL_EL0` to 0. The kernel line and both ownership lines had already printed. Then:

```
  cntp-cval 00000000002dbcb6
  tnow 00000000002dc04e
  cntp-ctl 0000000000000005
[NXU] Halted
  room.c run 204 code 8000000d
```

`CNTP_CTL_EL0` value 5 is ENABLE and ISTATUS. The compare `0x2dbcb6` is `0x398` ticks (920 ticks) behind `CNTPCT_EL0` `0x2dc04e`. A slice of frequency/10 would have put the compare about 10,000,000 ticks ahead. The observed gap is the distance from a compare of "now" to the later read, not a 10 ms deadline that failed to clear.

The same boot's chronicle, still on that halt, records the earlier kernel deadline:

```
rec 00000b01 0000000005f5e100 0000000000000000
rec 00000b03 0000000000030d38 000000000027619b
rec 0002100b 0000000000276237 0000000000030d38
```

S11 programmed `nxu_facts.timer_freq/100` (1,000,000 ticks), not a live `CNTFRQ` read. That deadline was delivered: INTID `0x1e` is in the chronicle, and the console printed `CNTP deadline observed`. The room path was the first writer to use `nxu_cntfrq()` after that.

## INFERENCE

`nxu_cntfrq()` returned a value below 10, the unsigned division produced 0, and the TVAL write set the compare to the counter. The 920-tick lag is the time from that write to the failing read. This fits the facts-layer fallback, the TVAL architecture, and the measured gap. It was not printed as its own register on the failing path: the failure log shows the compare, the counter, and the control register, not `CNTFRQ_EL0`. The inference stays an inference.

Other explanations that were considered and do not fit the numbers:

- A stale s11 compare would sit near `0x276xxx`, hundreds of thousands of ticks behind `0x2dc04e`, not 920 ticks behind.
- A trapped MSR would have entered the EL2 halt vector and would not have returned to print the three machine lines.
- Secure group state was already fixed. S11's PPI 30 had fired on this same boot.

## DECISION

Arm a room from the accepted frequency, `nxu_facts.timer_freq`, never from a second `CNTFRQ` read. Write `CNTP_CVAL_EL0` to counter plus that slice, then write `CNTP_TVAL_EL0` to the same slice, then set `CNTP_CTL_EL0` to ENABLE with IMASK clear. Refuse to enter EL0 if the compare is not strictly ahead of the counter or if ISTATUS is set. Clear the timer PPI's pending bit in the redistributor so the kernel deadline cannot be delivered as the room's first interrupt.

At EL3, before the drop, if `CNTFRQ_EL0` reads as zero, publish `nxu_board.timer_freq` (offset 260, `0x104`) into the register. EL1 cannot do that write. QEMU never executes this path.

The two writes and the pending clear shipped together. This record does not claim which one of them was sufficient by itself.

## IMPLEMENTATION

`foundation/room/room.c` arms the slice. `silicon/aarch64/entry/start.S` publishes a zero `CNTFRQ_EL0` at EL3. `nxu_gicr_clear_pending` writes `GICR_ICPENDR0` in the SGI frame.

Pages are four frames immediately after the image, and only if that range sits inside `nxu_facts.mem[0]`. During a room, that room's code page is user execute and its data page is user read/write. The other room and the kernel stay EL1-only. The kernel text leaf has UXN set. The same stage-1 root is updated. There is no second translation regime.

Entry is ERET to EL0t with IRQ unmasked and the other DAIF bits masked. `x0` is the room's data page. Lower-EL synchronous exceptions and IRQs are not the current-EL fatal handlers. SVC `#0` returns to the kernel. SVC `#1` is a pulse and returns to EL0. A data abort returns to the kernel as a contained fault. A lower-EL IRQ must be INTID 30: it is acknowledged, the timer is masked, and the INTID is deactivated. Any other INTID halts.

Room 0 is the root and may be entered by the kernel. A delivered pulse from room 0 names room 1 and grants one entry. A rejected pulse does not grant entry. The grant is consumed by the entry. After that, room 1 is refused again until another pulse names it.

The only accepted pulse is source room 0, destination 1, payload `0x5055`, and a pointer inside room 0's data page. The kernel copies eight bytes to room 1's data page. Room 1's own pulse is rejected even when the pointer is inside room 1's page.

## TEST

2026-09-26. `make qemu` (virt, GICv3, cortex-a57, smp 4, 128M) and `make fvp` (Base RevC, SMP 1, load `0x80000000`, 10 s probe timeout). Image size reported by `probe/check.sh`: 37024 bytes. FVP's process status 124 is the probe stopping the model after the kernel has reached its halt loop. The probe fails the run if the UART contains `[NXU] Halted` or lacks the application line.

QEMU console, `probe/evidence/qemu.log`:

```
[NXU] CPU 0 is now online.
[NXU] CPU 1 is now online.
[NXU] CPU 2 is now online.
[NXU] CPU 3 is now online.
[NXU] CPU set online
[NXU] IPI observed
[NXU] IPI observed
[NXU] NXU kernel execution is established.
[NXU] The spatial boundary is established.
[NXU] Room 0 now owns its execution region.
[NXU] Room 1 now owns its execution region.
[NXU] Room 0 had its fault contained.
[NXU] Room 1 was refused entry.
[NXU] The pulse was rejected.
[NXU] Room 1 was refused entry.
[NXU] The pulse from Room 0 to Room 1 was delivered.
[NXU] The NXU runtime has entered Room 1.
[NXU] Room 1 returned by its own request.
[NXU] Room 1 was refused entry.
[NXU] The pulse from Room 0 to Room 1 was delivered.
[NXU] The pulse was rejected.
[NXU] Room 1 was refused entry.
[NXU] The pulse from Room 0 to Room 1 was delivered.
[NXU] Room 1 was preempted at its deadline.
[NXU] The first NXU application has completed.
```

FVP console, `probe/evidence/fvp-uart.log`, same image sources, one CPU because the probe passes no DTB:

```
[NXU] CPU 0 is now online.
[NXU] CPU set online
[NXU] IPI observed
[NXU] NXU kernel execution is established.
[NXU] The spatial boundary is established.
[NXU] Room 0 now owns its execution region.
[NXU] Room 1 now owns its execution region.
[NXU] Room 0 had its fault contained.
[NXU] Room 1 was refused entry.
[NXU] The pulse was rejected.
[NXU] Room 1 was refused entry.
[NXU] The pulse from Room 0 to Room 1 was delivered.
[NXU] The NXU runtime has entered Room 1.
[NXU] Room 1 returned by its own request.
[NXU] Room 1 was refused entry.
[NXU] The pulse from Room 0 to Room 1 was delivered.
[NXU] The pulse was rejected.
[NXU] Room 1 was refused entry.
[NXU] The pulse from Room 0 to Room 1 was delivered.
[NXU] Room 1 was preempted at its deadline.
[NXU] The first NXU application has completed.
```

Neither capture contains `[NXU] Halted`. A corrupted ARM64 header magic still does not reach the application line (`probe/cases/neg-header.sh`).

## MEASUREMENT — what each line is

| Line | What was true when it printed |
|---|---|
| Spatial boundary | Kernel leaf has UXN and no EL0 access bit. The active data leaf has the EL0 access bit. |
| Room owns its region | That room's two pages exist inside RAM and were mapped for EL1 before either room ran. |
| Fault contained | Room 0 loaded a kernel address and returned. The kernel did not halt. |
| Refused entry | Room 1 had no unconsumed pulse grant. It was not entered. |
| Pulse rejected | First, room 0's pointer was outside its data page. Later, room 1 executed a pulse. `rejected` increased. The grant did not. |
| Pulse delivered | Room 0 named room 1 with payload `0x5055`. Eight bytes were copied. One entry was granted. |
| Runtime entered Room 1 | The first grant was consumed and EL0 entry of room 1 began. |
| Returned by its own request | Room 1 executed SVC `#0` before the deadline. The copied payload matched `0x5055`. |
| Preempted at its deadline | Room 1 branched to itself. The lower-EL IRQ was INTID 30. The kernel resumed. |

Three deliveries are required because each non-root entry consumes one grant: the reader, the rejected reply, and the spinning body.

## What was rejected

A UNIX scheduler, a process, and a syscall table. The sequence is the separation kernel entering a subject or refusing to. The subject leaves by its own SVC or by the deadline.

Disabling the timer before every re-arm. That was the sequence present at the halt. The correction does not disable the timer first.

A second page-table root per room. Leaf permissions on the one EL1 root are the boundary that was checked.

Naming the pulse properties "UDP". No definition of UDP exists in this tree, in Genesis v2, or in the closure. A network stack was not added.

Wall, Edge, and Fabric. Those names are still absent from the TCB.

## UNKNOWN

Whether EL1 `CNTFRQ_EL0` on FVP was actually zero. The EL3 publish covers that case. The successful runs did not print the raw register.

Whether writing CVAL, writing TVAL, or clearing the pending bit was the necessary part of the re-arm. They were applied together. The before/after UART is the measurement of the combination.

Cache, TLB, and speculation interference between the two rooms. The page permissions were checked. Residual timing interference was not measured.

Ordering of more than one pulse into the same slot. A later delivery overwrites the eight-byte slot. There is no queue.

Any room other than the single edge from 0 to 1. A different destination or payload is a rejection.

FVP with more than one CPU. The probe starts one core and supplies no DTB, so the present set is the boot CPU only. QEMU smp=4 is the SMP measurement.

A verified application image that is not instructions copied out of the kernel. The bodies that ran are the blobs in `foundation/room/room_entry.S`.

UDP. The word is not defined by the current architecture. Nothing in the kernel claims a UDP property.

## Consequence

The kernel line is the start of the boundary measurement, not the end of the boot. Both instruments now pass the boundary, the refusal, the delivery, the contained fault, the deadline, and the application line, or the probe fails.
