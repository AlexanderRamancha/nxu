# H6 — S1–S14 foundation implementation (2026-09-22)

**Context.** Genesis v2 + closure accepted. S0 already present. Instruction:
implement remaining foundation stages in order, real ARM64 first, QEMU/FVP as
validation instruments.

**Reasoning.** Do not redesign Genesis. Do not copy the historical tree. Reuse
validated properties (DAIF mask, VBAR before unmask, wrap-safe ranges, PSCI
bring-up, timer PPI, WFI halt) without historical architecture (hardcoded
QEMU addresses, `cpu_start(1)`, handler pointers, UART-as-time).

**Proposals.** None that amend Genesis. `nxu_cpu_exec_context` kept as the type
name (C16).

**Experiments.**

- QEMU virt GICv3, smp=4: freeze, 4 online, targeted SGI, timer PPI IRQ.
- QEMU virt smp=2: present/online/freeze = 2 (N from facts).
- Negative: corrupted ARM64 magic → no freeze.
- `BOARD=fvp-base` image builds. FVP not executed (no binary).
- Real hardware not executed.

**Decisions.**

- DTB decoder records `method` on the node and applies it at END_NODE so
  property order cannot drop PSCI (facts-path, no policy).
- Ownership table: RAM=nxu, image=image, DTB=reserved, GICD/GICR/UART=mmio.
  S6 maps image + mmio only.
- Timer deadline is a TCB-bound PPI, proven by IRQ, not UART output.
- Wait flags observed from IRQ context are `volatile` (compiler hoist).
- Present set frozen after GIC redistributor corroboration (C5).

**Rejected.** Mapping all RAM. Board addresses in foundation/silicon. Function
pointer IRQ dispatch. 1-of-N SGI. ITS/LPI. Room/Wall/Edge/Fabric/Pulse.
Scheduler, filesystem, POSIX, capabilities. FVP-specific workarounds (FVP
not run). Treating QEMU success as real-hardware validation.

**Mistakes.** First S11 wait loop allowed the compiler to hoist `timer_fired`.
IRQ had already been delivered (chronicle `IRQ 30`). Fixed; not an
architectural change.

**Evolution.** Foundation freeze exists as a machine-control substrate on QEMU
virt. FVP and real hardware remain open validation.
