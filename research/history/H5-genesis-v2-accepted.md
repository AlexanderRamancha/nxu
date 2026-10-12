# H5 — Genesis v2 accepted (2026-09-22)

**Context.** Explicit command `genesis v2 accepted`. Create `nxu/` as the new
tree. Move `genesis.md`, `genesis v2.md`, and `research/` to that project root.

**Reasoning.** Historical `AlexanderRamancha/nxu` stays untouched. New tree
follows Foundation / Silicon / Board / Research. S0 is the first implementation
stage.

**Proposals.** None new. Closure C1–C18 remain binding.

**Experiments.** S0 image: PC-relative entry, NXU stack, DAIF mask, chronicle,
halt. Incoming SP unused. Halt injectable.

**Decisions.** Genesis v2 + closure accepted. `nxu/` is the project root.
`nxu_cpu_exec_context` is the canonical execution abstraction (not renamed).

**Rejected.** Migrating the historical tree. Board addresses in architectural
code. Implementing past S0 in this step’s scope except as S0 requires.

**Mistakes.** —

**Evolution.** Implementation starts at S0 in `nxu/`.
