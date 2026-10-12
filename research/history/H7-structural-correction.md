# H7 — Structural correction (S0–S14 frozen)

**Context.** Tree had `foundation/exec/`, a single `build/`, generic MAP.md as primary maps, and no FVP artifact path.

**Reasoning.** Genesis four-layer split requires exception and context as distinct foundation directories; QEMU and FVP must be independently identifiable artifacts with identical architecture.

**Proposals.** None of Room/Wall/Edge/Fabric/Pulse. No stage redesign.

**Experiments.** `make qemu` S14 freeze. `make fvp` builds, NOT EXECUTED (no FVP binary).

**Decisions.** `foundation/exception/` + `foundation/context/`. `build-qemu/` + `build-fvp/`. Source maps in files. `include/nxu/exec.h` removed; type remains `nxu_cpu_exec_context`.

**Rejected.** Linux `kernel/arch/drivers`. Copying historical NXU.

**Mistakes.** S11 wait loop without volatile caused a false “no S14 freeze” while the IRQ had fired.

**Evolution.** Structure matches the accepted tree. Stages unchanged.
