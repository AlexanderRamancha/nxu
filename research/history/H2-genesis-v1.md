# H2 — Genesis v1 (proposed, 2026-09-22)

**Context.** Restart from first principles. Historical tree untouched.
Instruction: no implementation until accepted. Sixteen-section architecture.

**Reasoning.** NXU is a machine-control substrate, not a Unix. Untrusted intake.
Hardware enforcement. Platform-independent facts. Isolation vocabulary as
research. Rushby properties as claims, not slogans.

**Proposals.** Ownership-phase model; corroboration of facts; four CPU states
plus failed; `nxu_cpu_exec_context`; signals instead of irqchip; chronicle
instead of logging; stay at entry EL; S0–S14; research names Room/Wall/Edge/
Fabric/Pulse; repo split foundation/silicon/board/research/probe.

**Experiments.** None (design-only).

**Decisions (proposed, not accepted).** 22 invariants; A1–A10 working
assumptions; structural boot contract in foundation; crypto boot in research.

**Rejected.** Linux-shaped repo; hardcoded QEMU as architecture; handler
pointers; 1-of-N; scheduler/filesystem/POSIX in foundation; vendor verified
boot as silent default.

**Mistakes / gaps carried forward.** Informal name “execution atom”; timer
“typically PPI 30” vs EL2; MMU-on entry takeover vs never using incoming
tables; shared chronicle after SMP; present-set closed before GIC corroboration.

**Evolution.** Baseline for v2’s four-layer and rejected-assumption lists.
