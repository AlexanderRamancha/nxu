# H11 — The room boundary was attacked from both sides

History, not authority. H10 recorded that both instruments completed one room sequence. This file records the attacks that sequence did not perform, and the results after they were added.

## Question

A function that maps a page is not a separation property. After H10, the exercised violations were: room 0 loading one kernel address, one pulse with a pointer outside its page, room 1 sending a pulse, and one spinning room hitting its deadline.

These were not exercised:

- a store into the other room
- a load of the other room
- a fetch of a page the room must not execute
- a store into the room's own code
- a load of a device the kernel uses
- a write of the physical timer from EL0
- a malformed destination or payload
- two pulses outstanding at once
- an interrupt that is not the kernel deadline

The question was whether the kernel stays intact and the other room's memory stays intact when those happen.

## ASSUMPTION

Stage-1 AP bits and UXN are enough to stop an EL0 load, store, or fetch. `CNTKCTL_EL1` reset state already traps EL0 timer access, but that had not been written by NXU, so it was an accident of reset rather than a property NXU established. A GIC interrupt is not a pulse. One outstanding entry is the right bound for a single slot: a second authorized pulse may replace the payload, and it must not create a queue of entries.

## DECISION

Keep one slot and one owed entry. Do not add a queue. Write `CNTKCTL_EL1` to 0 before any room runs, and require the EL0 physical-timer and physical-counter enables to read back clear. On a non-deadline interrupt during a room, end that room, deactivate the interrupt, and continue the kernel. Plant a canary in each data page and require it to survive every attack.

## IMPLEMENTATION

`foundation/room/room.c` and `foundation/room/room_entry.S`. The attack bodies are the same shape as the earlier blobs: a few instructions copied into the room's code page. The kernel passes the forbidden address in a register. The room does not choose the kernel's page tables.

Each entry arms a fresh compare of the accepted frequency divided by ten. A fault or a voluntary return must happen before that compare. The spinning body must cross it. A room that tries to write `CNTP_TVAL_EL0` must fault, and the compare read back afterwards must be the value armed at entry.

## TEST

2026-09-26. `make qemu` (virt, GICv3, cortex-a57, four CPUs) and `make fvp` (Base RevC, one CPU, no DTB, load `0x80000000`). Image size reported by `probe/check.sh`: 41120 bytes. FVP process status 124 is the probe stopping the model. Neither capture contains `[NXU] Halted`. Both end with `[NXU] The first NXU application has completed.`

The two consoles match on the room sequence. QEMU additionally names CPU 0, CPU 1, CPU 2, and CPU 3, and shows two IPIs. FVP names CPU 0 and shows one IPI. Captures: `probe/evidence/qemu.log`, `probe/evidence/fvp-uart.log`.

## MEASUREMENT

Both consoles showed, in this order, for the attacks:

- Room 0 contained on a kernel load, a load of room 1, a store into room 1, a fetch of its own data, a fetch of room 1's code, a store into its own code, and a load of the UART.
- Room 1 refused before any pulse.
- A pulse with destination 2 rejected. A pulse with a payload other than `0x5055` rejected. A pulse whose pointer was a kernel address rejected. Room 1's slot stayed empty. Room 1 still refused.
- Two authorized pulses, then one entry of room 1. Room 1 read the payload back. A second entry was refused. The extra words the sender stored beside the payload were not copied. The canary beside the slot was unchanged.
- Room 1 contained on a store into room 0, a kernel load, a fetch of room 0's code, a store into its own code, and a write of `CNTP_TVAL_EL0`. The armed compare was unchanged after that write.
- Room 1's own pulse rejected, and it was refused again.
- Room 1 presented with a self-interrupt that was not CNTP. The kernel reclaimed execution before the deadline. Both canaries unchanged.
- Room 1 reached its own deadline. The kernel reclaimed execution. Room 0's canary still unchanged.

The checks that are not printed, and that halt if they fail, are the canary compares, the unchanged instruction word, the unchanged compare, the empty slot after rejection, the single owed entry, and the early-versus-deadline counter check.

## FAILURE

None on these two runs. The previous hole was the absence of the attacks, not a failed attack.

## REMAINING UNKNOWN

Cache, TLB, and speculation can still couple the rooms. These runs do not measure that.

No DMA master exists in this kernel. The UART test shows EL0 cannot load the device page. It does not show what a DMA engine that ignores the page tables would do.

Rooms are entered on the boot CPU. The other CPUs, when present, were online before the rooms ran and were not given an entry. There is no measurement of a secondary CPU attempting a room entry, because nothing on a secondary CPU calls the entry.

UDP remains undefined. No UDP line was added.

The boot sequencer still uses its historical stage-numbered entry points. New room code does not add any.
