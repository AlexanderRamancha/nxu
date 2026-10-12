# H4 — Genesis v2 closure (proposed, 2026-09-22)

**Context.** Final gate before implementation. Canonical execution abstraction
remains `nxu_cpu_exec_context` (not renamed). Report contradictions, extra
complexity, ambiguous rules, experiments. Create `research/history/`. TCB
firewall for `research/`. Produce `GENESIS_V2_CLOSURE.md`. Do not implement S0.

**Reasoning.** Close internal contradictions without adding OS features. Prefer
hardware enforcement and fail-closed over takeover of firmware state.

**Proposals.** Scoped invariants (unknown-memory, bound context life-state);
MMU-on entry rejects; ISA timer PPI by control EL; provisional present set;
per-CPU chronicle; load-bearing vs optional facts; research allow-list firewall.

**Experiments.** Listed in the closure document; none executed.

**Decisions (proposed, not accepted until `ACCEPT GENESIS V2`).** Exact changes
C1–C18 in `GENESIS_V2_CLOSURE.md`. Genesis v2 text remains the baseline; closure
is the delta.

**Rejected.** Rename to Atom; S0 implementation; silent research-in-TCB;
distributed abort IPI; MMU-on firmware-table takeover; required compiled-in
QEMU RAM as second source; bring-up-poke IPI as architecture.

**Mistakes this pass is meant to stop.** Shipping v2 with contradictory
invariants (running vs waiting; unknown inaccessible vs DTB probe; PPI 30 vs
EL2; shared mutable chronicle).

**Evolution.** Implementation may begin only on the explicit command
`ACCEPT GENESIS V2`.
