# H8 — S14 visibility, console, Black Box

**Context.** Downloaded tree still failed QEMU capture (`no S14 freeze record`) after a volatile-only edit. Makefile had no header dependencies. Console was a hex dump of the chronicle at freeze/halt.

**Reasoning.** Same-CPU IRQ stores must be compiler-visible. Header edits must rebuild waiters. Capture should observe a live freeze sentence, not a late hex dump that UART timeout can clip. UART remains an optional sink after ownership.

**Proposals.** None of Room/Wall/Edge/Fabric/Pulse. No stage redesign.

**Experiments.** `make qemu` and `make debug` require `[NXU][S14]`. Negative header must not print it.

**Decisions.** `volatile` wait flags; DSB after IRQ store; compiler barrier in S11/S13 waits; `-MMD -MP`; live `[NXU][TAG]` console; Black Box as bounded preserved evidence, not a logging framework.

**Rejected.** Weakening S14 capture. UART-first architecture. Historical logging policy/storage/subsystem split as TCB.

**Mistakes.** Treating a hex dump at halt as the only freeze evidence.

**Evolution.** Observe path is chronicle + Black Box + optional console. Stages unchanged.
