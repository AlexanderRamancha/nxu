# H1 — Historical NXU tree (`AlexanderRamancha/nxu`)

**Context.** Privacy-first ARM64 OS, public tree, Linux-shaped layout
(`arch/` `kernel/` `drivers/`). Goal stated as explicit authority and isolation.

**Reasoning.** Bring up a machine on QEMU virt: UART, exceptions, DTB, GICv3,
CPU topology, PSCI, generic timer, logging. Treat “done” as those pieces existing.

**Proposals.** Interrupt manager + backend; capability-based IRQ (not built);
three CPU states OFFLINE/STARTING/ONLINE; boot_info {affinity, DTB, flags}.

**Experiments.** QEMU virt, GICv3, PSCI `CPU_ON`, timer PPI fire, FDT header
checks, wrap-safe physical membership. Makefile `-smp 2` vs claims of 4-CPU
validation (measurement hygiene failure).

**Decisions (historical, not current authority).** Hardcoded QEMU RAM/GIC/UART/
load; GIC redistributor walk as topology; `cpu_start(1)`; function-pointer IRQ
handlers; logging subsystem in the TCB; experiments in kernel main.

**Rejected / unfinished then.** Capabilities, userspace, panic subsystem,
persistent diagnostics, MMU.

**Mistakes.** DTB pointer ≠ 0 as “boot validation”; DTB validated then unused
while addresses stayed hardcoded; CPU1-only secondary stack; secondary path
without MMU/timer/context; UART before ownership; no MMU while claiming
privacy-by-construction.

**Evolution.** Properties worth keeping: DAIF mask, VBAR before unmask, wrap-safe
ranges, fail-closed WFI, PSCI can start a secondary on virt, timer PPI can fire.
The tree is evidence only. Genesis does not migrate it.
