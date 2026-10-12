# NXU Genesis v2 — Closure

**Baseline:** Genesis v2 (`genesis.md` / `genesis v2.md`)
**This document:** closure delta. Binding after `ACCEPT GENESIS V2`.
**Canonical execution abstraction:** `nxu_cpu_exec_context`
**Not renamed** to Atom or any other metaphorical type name.
**Status:** ACCEPTED with Genesis v2 (`ACCEPT GENESIS V2`, 2026-09-22).

S0 may begin. Do not implement past the current accepted stage without a new instruction.

History of this pass: `research/history/H4-genesis-v2-closure.md`.
History is evidence, never architectural authority.

---

## A. Closure audit

Only four classes of finding. No new OS features.

### A.1 Contradictions

| ID | Collision | Resolution (exact change) |
|---|---|---|
| X1 | Inv. 4 “unknown physical memory is inaccessible” vs bounded DTB probe before the map exists | Inv. 4 applies **after space armed**. Bootstrap window is the documented exception: image footprint + bounded DTB probe. |
| X2 | Inv. 12 “exactly one `running` context per online CPU” vs secondaries going `waiting` after online | One **bound** `nxu_cpu_exec_context` per online CPU. Its life is `running` or `waiting`. |
| X3 | Inv. 23 “incoming page tables are never used” vs MMU-on takeover of firmware tables | Entry with MMU on (**SCTLR.M=1**) **rejects**. No takeover. |
| X4 | A11 stay-at-EL2 vs time “typically PPI 30” | Timer INTID follows ARM generic-timer PPI for `control_el`: EL1 physical **30**, EL2 physical **26**. Not a board address. If facts omit it, use that ISA default. If facts contradict ISA, reject. |
| X5 | “Present set closed” at topology-live / S8 vs S10 GIC count mismatch | Present set is **provisional** until controller corroboration. Topology freeze is after S10. |
| X6 | Facts-phase “controller accepted” vs no MMIO access before ownership | Candidate **addresses** at facts. **IIDR/TYPER observation** only after the MMIO region is owned and mapped. Grade may upgrade then. |
| X7 | Chronicle listed as shared mutable object vs SMP without a lock protocol vs “no implicit shared mutable kernel objects” | Chronicle is **per-CPU**. Facts, ownership map, and page tables become **read-only** before `CPU_ON`, except each CPU’s own topology life-state. |
| X8 | Inv. 3 “no use before ownership” vs DTB probe | Probe is a bounded untrusted read in the bootstrap window, not ownership. Wording in C1/C8. |

### A.2 Unnecessary complexity

| ID | Item | Action |
|---|---|---|
| U1 | Firmware MMU-on takeover | Removed (X3). Fail closed is simpler and matches inv. 23. |
| U2 | IPI “bring-up poke” as an architectural purpose | Removed. `CPU_ON` starts the secondary. S13 remains: SGI as TCB signal, for later TLB maintenance, proven by a targeted IPI test. |
| U3 | Requiring a compiled-in board RAM map as a second source on every boot | Not required. That would re-introduce hardcoded QEMU RAM as mandatory. One RAM source + image ⊂ RAM is allowed (grade `single`, chronicled). Two sources that disagree reject. |
| U4 | Distributed abort IPI if boot CPU fails after secondaries are online | Not in foundation. Boot CPU halts. Secondaries remain `waiting`. No recovery plane. |
| U5 | Fifth top-level layer for FDT | Not added. FDT decode is a facts-path decoder inside foundation facts. No policy. |
| U6 | Calling `nxu_cpu_exec_context` an “atom” or renaming it | Dropped as a type name. The type is `nxu_cpu_exec_context`. |

### A.3 Ambiguous rules (now exact)

| ID | Ambiguity | Exact rule |
|---|---|---|
| M1 | DTB overlap with image | DTB range must **not** overlap the image footprint. Overlap rejects. |
| M2 | DTB `totalsize` cap | Contract constant in the image/header. Independent of board RAM size. Value is an implementation choice; architecture only forbids using platform RAM size as the cap. |
| M3 | “Recorded weakness” still boots? | **Load-bearing** facts: RAM (with image ⊂ RAM), boot MPIDR in present set, `control_el`, timer frequency (observed), enable-method if present count > 1, interrupt controller identity after mapped probe. Missing load-bearing → halt. Optional (UART sink) may be absent or `single`. |
| M4 | CNTFRQ / MPIDR grade | `observed-only` is sufficient. They are not DTB claims. |
| M5 | Silicon and board addresses | Silicon operations take physical addresses **as parameters**. Silicon does not read `board/`. |
| M6 | `nxu_cpu_exec_context.translate` in foundation | Always equal to the single TCB translation regime. Divergence is research. |
| M7 | Chronicle overflow | Per-CPU ring for non-failure records. A **last-failure slot** on that CPU is never overwritten by non-failure records. Overflow never fail-opens. |
| M8 | Cache at entry | MMU must be off (X3). I-cache state is observed, not required. NXU makes I/D coherent when it enables its own MMU. |
| M9 | Halt after partial SMP | No abort protocol (U4). |
| M10 | `available` vs A12 | `available` is a **bring-up gate**. At foundation freeze, no CPU may remain `available` or `starting`: each present CPU is `online` or the boot has already halted. `failed` during bring-up is halt under A12. |

### A.4 Assumptions requiring experiments

See §F. These remain unresolved: A3, A11/EL2 cost, A12/heterogeneous CPUs, RAM identity vs high window, Room-name promotion, boot-key custody, secondary cache/TLB, GICv3 security model, language choice. They are **not** silently decided here.

---

## B. Exact changes from Genesis v2

Apply these. Do not apply anything else.

| ID | Change |
|---|---|
| C1 | Inv. 4 scoped to **after space armed**. Bootstrap exception: image footprint + bounded DTB probe. |
| C2 | Inv. 12: one **bound** `nxu_cpu_exec_context` per online CPU; life is `running` or `waiting`. |
| C3 | Entry **SCTLR.M=1** rejects. No firmware table takeover. Intake still *records* SCTLR so the reject is observable. |
| C4 | Timer PPI from ISA given `control_el` (EL1 physical 30, EL2 physical 26). Facts may not contradict ISA. Not a QEMU constant. |
| C5 | Present set provisional until interrupt-controller corroboration. Freeze after S10. |
| C6 | Controller MMIO: candidate address at facts; access only after region owned+mapped. |
| C7 | Chronicle is per-CPU. Shared enumerated objects after `CPU_ON` starts: page tables (RO), fact table (RO), ownership map (RO), topology slots with **exclusive writers** (boot CPU: `starting`; target CPU: `online`/`failed` of itself). |
| C8 | DTB must not overlap image. |
| C9 | DTB probe bound is a contract constant, not board RAM. |
| C10 | Load-bearing vs optional facts (M3). UART is optional. |
| C11 | Silicon receives addresses as parameters; never reads `board/`. |
| C12 | FDT decoder lives on the facts path. Not a fifth layer. No policy. |
| C13 | No architectural “bring-up poke” IPI. S13 still required as SGI proof / TLB path. |
| C14 | Foundation `nxu_cpu_exec_context.translate` = TCB regime only. |
| C15 | Chronicle overflow: per-CPU ring + immortal last-failure slot. |
| C16 | Type name remains `nxu_cpu_exec_context`. Informal “atom” / Atom dropped. |
| C17 | `research/` TCB firewall (§E). History has no authority. |
| C18 | A12 + `available`: M10. Four-state model unchanged; freeze policy restated. |

S0–S14 order is unchanged. S8 still builds a present set; S10 may halt on mismatch (C5). No Room / Wall / Edge / Fabric / Pulse implementation. No scheduler, filesystem, POSIX, driver framework, or verified-boot hierarchy.

---

## C. Final invariants

Genesis v2 invariants, with C1–C18 applied. After the establishing phase, each is true or NXU has halted.

1. DAIF is masked until vectors and TCB signal bind are complete.
2. A broken invariant is halt, not continue.
3. No use before ownership, except the bootstrap window (image + bounded DTB probe) which is untrusted read, not ownership.
4. After space armed, unknown physical memory is inaccessible.
5. Physical membership is wrap-safe. No overflowing `address + size`.
6. Image ⊂ accepted RAM. DTB range does not overlap the image.
7. After space armed: MMU on, WX never, default deny.
8. VBAR is NXU’s before any unmask.
9. `present ≠ initialized ≠ available ≠ online`. Present is provisional until controller corroboration.
10. Boot MPIDR is in the present set.
11. All online CPUs are at the same `control_el`. Entry MMU-on rejects. Mixed ELs reject.
12. Each online CPU has exactly one bound `nxu_cpu_exec_context`, life `running` or `waiting`.
13. That context’s stack is exclusive and bounded; SP outside `[base, limit)` is fault → halt.
14. A signal has zero or one owner; enabled implies one.
15. Signal route is a single CPU.
16. Accepted facts do not contradict observations or each other. Load-bearing facts follow M3.
17. Secondaries do not discover; they occupy.
18. `CPU_ON` success ≠ online.
19. Chronicle is per-CPU, append-only in the ring sense, with an immortal last-failure slot; never a reason to fail open.
20. No ambient authority on the TCB path (no handler tables, no 1-of-N, no global current-task).
21. Architectural code contains no board addresses and no board-named behavior. Silicon takes addresses as parameters.
22. Research is not linked into the TCB.
23. Incoming SP is never used. Incoming page tables are never used. MMU-on entry rejects.
24. Every accepted fact carries a source mask and grade: `observed-only` | `single` | `corroborated`.
25. Region owners: `unknown` | `firmware` | `image` | `nxu` | `mmio-accepted` | `reserved-do-not-touch`. `unknown` is never mapped.
26. After space armed, TCB translation is the only regime. All bound `nxu_cpu_exec_context.translate` fields equal it.
27. After `CPU_ON` of the first secondary is issued, fact table, ownership map, and page tables are read-only. Topology life-state fields have exclusive writers.
28. At freeze, no present CPU remains `available` or `starting`.

---

## D. Accepted assumptions

Working assumptions of Genesis v2, as closed here. Accepting Genesis v2 means accepting these, or amending them explicitly.

| ID | Assumption | Closure note |
|---|---|---|
| A1 | Structural boot contract is foundation boot-trust. Crypto boot is research. | Unchanged. |
| A2 | DTB is a candidate fact source, not hardware truth. | Unchanged. Decoder is facts-path, no policy. |
| A3 | Shared-memory SMP with explicit per-CPU state is foundation. Multikernel is research. | Unchanged. Sharing enumerated and frozen (C7). Still needs R16. |
| A4 | PSCI `CPU_ON` is the bring-up adapter when facts say so. Success ≠ online. | Unchanged. |
| A5 | Stage-1 only in foundation. Stage-2 is research. | Unchanged. At EL2 this is EL2’s own stage-1 (`TTBR0_EL2`), not stage-2. |
| A6 | Wrap-safe physical membership is mandatory. | Unchanged. |
| A7 | Signal route is exactly one CPU. 1-of-N forbidden. | Unchanged. |
| A8 | No function-pointer dispatch in the TCB signal path. | Unchanged. |
| A9 | ARM generic timer + PPI is foundation time. | Closed by C4: PPI is ISA(control_el), not “typically 30”. |
| A10 | First implementation language is freestanding C + assembler. Architecture is not C. | Unchanged. Not isolation. |
| A11 | Stay at entry EL. Mixed ELs reject. | Unchanged. MMU-on entry rejects (C3). EL2 cost remains R10. |
| A12 | All present CPUs must reach online or halt. | Closed by M10: `available` is a gate, not a freeze state. Heterogeneous offline clusters remain R11. |

---

## E. Research boundaries

### E.1 What research is

`research/` holds hypotheses, alternatives, experiments, measurements, evidence, and history.

It is **not** architecture. It is **not** TCB. Promotion requires an explicit Genesis amendment after a measurement that could have failed.

Room / Wall / Edge / Fabric / Pulse remain research names. They are not foundation mechanisms. They are promoted only if they add a property **not** already provided by memory + MMU + `nxu_cpu_exec_context` + signals + time.

Rushby spatial separation, temporal separation, information-flow control, and fault isolation are **claims to measure**. They are not claimed by this closure.

Boot security beyond the structural contract (measured boot, user-controlled verified boot, DICE, rollback/recovery) remains research. Compare authority/key models before choosing one.

### E.2 TCB firewall (research cannot silently become foundation)

1. TCB image sources are an **explicit allow-list**: `foundation/`, `silicon/`, `board/` (data), `image/`. `research/` is absent.
2. Foundation, silicon, and board must not include `research/` headers or call `research/` symbols.
3. A probe that links research is a **different artifact** from the TCB image. Never one binary with a flag that “turns research on.”
4. `NXU_RUN_*` and any experiment hook live only in probe/research images.
5. Promotion is a Genesis amendment plus a change of the allow-list. Not a `#define`.
6. `research/history/` is not compiled and has **no architectural authority**. Past proposals filed there are not current architecture.

If a patch makes research reachable from the TCB path, the patch is rejected.

### E.3 History

`research/history/` records substantial design work:

context → reasoning → proposals → experiments → decisions → rejected ideas → mistakes → architectural evolution.

Not a transcript. Not authority.

---

## F. Required experiments

None of these are done. None are excuses to implement S0.

| ID | Experiment | Why it is required |
|---|---|---|
| R1 | Two `nxu_cpu_exec_context` with disjoint maps, signals, deadlines: remaining interference | If interference remains, foundation does not imply Room |
| R2 | Does “Room” add a property beyond a second context + map + signals? | If no, reject the name |
| R3 | Non-TCB signal delivery with zero TCB policy beyond ack/EOI/context switch | Device ownership by non-TCB |
| R4 | Pulse interference bound on Cortex-A | Do not claim temporal isolation if unmeasurable |
| R5 | Contain a non-TCB fault without halt | Fault isolation is not foundation behavior |
| R6 | User-sovereign verified boot vs DICE vs measured-only; **who holds the root of authority** | A1 must not be replaced by vendor lock-in |
| R7 | Rollback storage present/absent fail-closed | Platform-specific |
| R8 | Linux Image header vs native NXU header as intake | Hidden Linux dependency |
| R9 | Compiled-in board facts vs second blob | Corroboration vs update |
| R10 | EL2 vs EL1 cost; stage-2 value | A11/A5 |
| R11 | A12 on heterogeneous / offline-cluster descriptions | Freeze policy may need amendment |
| R12 | Spin-table only if a board without PSCI exists | Do not invent it |
| R13 | Secondary MMU-on cache/TLB completeness | Do not copy Linux |
| R14 | Identity-map NXU RAM vs high virtual TCB window | S6 layout |
| R15 | Firmware MMIO remaining after takeover | PSCI/GIC only vs none |
| R16 | Shared-memory SMP vs multikernel | Before changing A3 |
| R17 | seL4 untyped+caps vs Rushby static partitions | After S14, do not pre-select |
| R18 | When `nxu_cpu_exec_context` becomes schedulable without becoming a UNIX thread | Pulse |
| R19 | GICv3 two-security vs single-security; ITS/LPI TCB cost | Adapter scope |
| R20 | Bounded C subset vs Rust | A10 is vehicle, not isolation |
| R21 | Chronicle freeze-format reconstructs a boot on paper | S14 evidence |
| R22 | Two fact sources (different addresses, same semantics) at each stage | Platform independence |

---

## G. Unresolved questions

Not closed. Not to be papered over by convention.

- A3 vs measured value of per-CPU kernel instances (R16)
- A11/A5: stay-at-EL2 worth vs cost (R10)
- A12 vs big.LITTLE / firmware-offline CPUs (R11)
- Whether a native image header should replace the Linux 64-byte shape (R8)
- Whether board facts should exist at all if DTB+observation suffice (R9, U3)
- Whether identity map of NXU-owned RAM is enough spatial machinery for later isolation (R14)
- Whether any of Room / Wall / Edge / Fabric / Pulse survive R1–R5
- Key custody for any future verified boot (R6)
- Language of the TCB (R20)

---

## H. Gate

Genesis v2 + this closure is the proposed baseline.

**STOP.**

Do not implement S0.
Do not write implementation code.

Wait for the explicit command:

`ACCEPT GENESIS V2`
