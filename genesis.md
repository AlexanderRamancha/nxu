# NXU Genesis Architecture V2

**Status:** ACCEPTED (`ACCEPT GENESIS V2`, 2026-09-22).
**Closure delta is binding:** `GENESIS_V2_CLOSURE.md` (C1–C18).
**Canonical execution abstraction:** `nxu_cpu_exec_context` (not renamed).
**Implementation gate:** S0 may begin. Historical NXU (`AlexanderRamancha/nxu`) remains untouched evidence.

---

## 1. What NXU is

NXU is a **machine-control and isolation substrate**.

It is not a Linux replacement. It is not a Unix. It is not a collection of conventional OS features with different names.

**Job of the foundation:** take exclusive, observable control of a machine from an untrusted environment; prove discovered facts are consistent; establish NXU’s own execution environment; enforce with hardware what hardware can enforce; fail closed otherwise.

**Job of research:** test whether further isolation properties (spatial, temporal, information-flow, fault isolation) are real, measurable, and not already provided by the foundation.

**Minimal TCB** means minimal authority and trusted mechanisms. It does not mean an incomplete machine-control system.

### Core rule: question every assumption

For every mechanism:

1. What problem are we solving?
2. What assumption normally solves it?
3. Can we remove that assumption?
4. Can hardware enforce the property?
5. What is the simplest strong mechanism?
6. How can we experimentally prove or disprove it?

A conventional, popular, or language-driven answer is not an answer.

### Working assumptions of V2 (must be accepted or rejected with the document)

| ID | Assumption | Why it is here |
|---|---|---|
| A1 | Structural boot contract is sufficient foundation boot-trust. Cryptographic boot is research. | Key custody is an authority model, not a register write. |
| A2 | DTB is a candidate fact source, not hardware truth. | Platforms actually pass one. That does not make it trusted. |
| A3 | Shared-memory SMP with explicit per-CPU state is foundation. Multikernel is research. | Typical AArch64 presents coherent shared RAM. That is a hardware fact to observe, not Linux SMP copied. |
| A4 | PSCI `CPU_ON` is the bring-up adapter when facts say so. `CPU_ON` success ≠ online. | It is the firmware interface that exists. It is not trusted firmware. |
| A5 | Stage-1 MMU only in foundation. Stage-2 is research. | Stage-2 requires staying at EL2 and a Room model that is not yet evidenced. |
| A6 | Wrap-safe physical membership is mandatory. | Validated by historical NXU. Independent of Linux. |
| A7 | Signal route is exactly one CPU. 1-of-N is forbidden. | 1-of-N is ambient authority. |
| A8 | No function-pointer dispatch in the TCB signal path. | Callers must not inject TCB control flow. |
| A9 | ARM generic timer + PPI is foundation time. | Observed `CNTPCT`/`CNTFRQ` plus a bound PPI. Not a UNIX tick. |
| A10 | First implementation language is freestanding C + assembler. Architecture is not C. | Historical toolchain evidence. Language does not define isolation. |
| A11 | If entered at EL2, NXU stays at EL2. If entered at EL1, stay at EL1. Mixed ELs reject. | Dropping EL2→EL1 is a Linux assumption, not a hardware requirement. |
| A12 | All *present* CPUs must reach *online* or the boot fails closed. | Revisit on heterogeneous/offline-cluster evidence (Q5). |

---

## 2. Architecture map

### 2.1 Four-layer map

```
                    untrusted
 ┌──────────────────────────────────────────────────────┐
 │ firmware EL3, bootloader, DTB, PSCI, residue, MMIO   │
 │ claims, incoming SP, incoming page tables            │
 └──────────────────────────┬───────────────────────────┘
                            │ observe / decode / never inherit authority
                            ▼
 ┌──────────── BOARD ────────────┐   candidate facts only
 │ qemu-virt / fvp / soc data    │   no behavior
 └────────────┬──────────────────┘
              │
              ▼
 ┌──────────── SILICON ──────────┐   operations only
 │ AArch64, GICv3, PSCI, timer,  │   no policy
 │ UART ops, MMU programming     │
 └────────────┬──────────────────┘
              │
              ▼
 ┌──────────── FOUNDATION ───────┐   architecture, TCB
 │ contract → facts → space/MMU  │
 │ exception → cpu/SMP           │
 │ nxu_cpu_exec_context          │
 │ signals → time → chronicle    │
 │ fail closed                   │
 └────────────┬──────────────────┘
              │ promote only with evidence
              ▼
 ┌──────────── RESEARCH ─────────┐   not TCB, not imported
 │ Room Wall Edge Fabric Pulse   │
 │ Rushby properties             │
 │ boot-trust / execution models │
 └───────────────────────────────┘
```

QEMU, FVP, and real SoCs differ in **board facts** and which **silicon adapters** are present. They do not differ in foundation state machines, invariants, or execution semantics.

### 2.2 Ownership-phase map

NXU is sequenced as ownership transitions, not `init()` calls. No phase uses a resource it does not yet own.

```
entry residue
  → intake (observe, mask DAIF, install NXU stack)
  → contract (accept / reject)
  → facts (normalize corroborated claims)
  → space armed (NXU page tables, MMU on)
  → topology live (present set closed, per-CPU storage exists)
  → exec context bound (one running context on boot CPU)
  → signals bound (TCB-owned only)
  → time armed (per-CPU deadline)
  → secondaries online (or fail closed)
  → foundation freeze
```

| Phase | CPU | Memory | Signals | Time |
|---|---|---|---|---|
| Entry | firmware residue | unknown | unknown | unknown |
| Intake | NXU, masked | image footprint only | masked, unbound | unread |
| Contract | NXU | image + bounded probe | masked | unread |
| Facts accepted | NXU | normalized map | masked | frequency readable |
| Space armed | NXU | NXU tables | masked | readable |
| Topology live | NXU | per-CPU stacks owned | masked | unarmed |
| Signals bound | NXU | accepted MMIO mapped | TCB-owned only | deadline bindable |
| Secondaries online | NXU on each online CPU | same | per-CPU redistributor owned | per-CPU deadline live |
| Freeze | NXU | frozen | frozen | frozen |

### 2.3 Object map

```
fact ────────────── accepted platform datum + source mask + corroboration grade
region ──────────── physical range + owner class
cpu ─────────────── topology node + lifecycle
nxu_cpu_exec_context  execution atom (not a thread)
signal ──────────── interrupt identity + exclusive owner + single-CPU route
deadline ────────── per-CPU timer event
chronicle ───────── append-only measurement of transitions
```

No process, thread, task, file, socket, driver object, or capability object exists in the foundation.

### 2.4 CPU lifecycle map

```
present ──► initialized ──► available ──► starting ──► online
                │               │              │
                └──────► failed ◄──────────────┘
```

`present ≠ initialized ≠ available ≠ online`

- **present** — corroborated existence. Boot CPU is present by MPIDR observation.
- **initialized** — per-CPU storage, stack, exec context, redistributor identity exist.
- **available** — initialized and not failed; legal to start.
- **starting** — `CPU_ON` issued; not yet executing NXU entry.
- **online** — executing NXU-owned code on its stack, in its context, published with a release barrier.
- **failed** — init, start, or handshake did not complete.

Logical id is assigned after the present set is closed. Affinity 0 is not CPU0 until matched. `CPU_ON` success is not online.

Boot CPU owns discovery, initialization of **all** present CPUs, start of each available secondary, and the timeout. It never hard-codes “start CPU 1”.

Secondaries occupy. They do not discover, parse DTB, or rebuild facts.

### 2.5 `nxu_cpu_exec_context` map

```
nxu_cpu_exec_context
  identity    cpu_id, context_id, generation
  life        inert | armed | running | waiting | faulted
  gpr         x0–x30
  sp pc pstate el
  translate   TTBR/TCR/MAIR this context is entitled to
  exception   ESR, FAR when captured
  signal      DAIF snapshot; active INTID if in signal path
  stack       base, limit (guard page unmapped once MMU is on)
  owner       foundation: TCB; later, if promoted: Room id
```

An online CPU has exactly one `running` context. Foundation may create, bind, enter, save/restore on exception, idle, halt. It does not schedule. FP/SIMD is absent until TCB code uses it.

### 2.6 Boot flow map

```
PC, CurrentEL, DAIF, SCTLR, TTBR, MPIDR, x0–x3     OBSERVE
        │
        ▼
install NXU stack; mask DAIF; do not trust incoming SP / tables
        │
        ▼
image header from PC-relative footprint
        │
        ▼
bounded DTB probe (optional) + board candidate facts
        │
        ▼
validate ── corroborate ── normalize
        │
        ├─ contradiction / unsafe ──► chronicle + halt
        │
        ▼
physical ownership map
        │
        ▼
NXU page tables; MMU on; default deny; never WX
        │
        ▼
vectors → exec context → topology → GICv3 adapter
        → timer signal → secondary bring-up with deadline
        │
        ▼
foundation freeze (research may begin, not import)
```

---

## 3. Trust boundaries

```
hardware  ── observed through registers. Not trusted to be correctly described.
firmware / bootloader / DTB / PSCI / board data / incoming SP / incoming tables
          ── claims. Zero inherited authority.
NXU TCB   ── foundation + the silicon operations it invokes + accepted facts.
research  ── untrusted relative to TCB. Cannot be imported.
probe     ── host-side. Never on the machine’s TCB path.
```

| Boundary | May cross | Must not cross |
|---|---|---|
| Firmware → NXU | register residue, DTB pointer, PSCI calling convention | authority, memory ownership, interrupt ownership, stack, page tables |
| Board → NXU | candidate addresses and topology | behavior, skip-validation, alternate SMP/exception paths |
| DTB → NXU | structured claims after bounded probe | MMIO use, CPU_ON, mapping, before corroboration |
| Silicon → Foundation | operation results (read IIDR, program TCR, `CPU_ON` return) | policy, “this board is QEMU”, skip fail-closed |
| Foundation → Research | measurements, freeze snapshot | TCB control flow, page-table writes, signal binding |
| CPU A → CPU B | enumerated shared objects only (facts, tables, topology, chronicle) | implicit shared mutable “kernel objects” |

**TCB contents:** intake, contract, fact normalization, page-table construction, MMU programming, exception entry/exit, signal ack/EOI/bind, CPU bring-up, exec-context save/restore, chronicle write, halt.

UART, if mapped, is a chronicle **sink**. Mapping it is TCB. Printing is not architecture. Halt does not require UART.

---

## 4. Foundation invariants

After the phase that establishes each invariant, it is true or NXU has halted.

1. DAIF is masked until vectors and TCB signal bind are complete.
2. A broken invariant is halt, not continue.
3. No use before ownership.
4. Unknown physical memory is inaccessible.
5. Physical membership is wrap-safe. No overflowing `address + size`.
6. Image ⊂ accepted RAM.
7. After space armed: MMU on, WX never, default deny.
8. VBAR is NXU’s before any unmask.
9. `present ≠ initialized ≠ available ≠ online`.
10. Boot MPIDR is in the present set.
11. All online CPUs are at the same control EL.
12. Exactly one `running` exec context per online CPU.
13. A running context’s stack is exclusive and bounded; SP outside `[base, limit)` is fault → halt.
14. A signal has zero or one owner; enabled implies one.
15. Signal route is a single CPU.
16. Accepted facts do not contradict observations or each other.
17. Secondaries do not discover; they occupy.
18. `CPU_ON` success ≠ online.
19. Chronicle is append-only, fixed-capacity, never a reason to fail open.
20. No ambient authority on the TCB path (no handler tables, no 1-of-N, no global current-task).
21. Architectural code contains no board addresses and no board-named behavior.
22. Research is not linked into the TCB.
23. Incoming SP and incoming page tables are never used as NXU’s environment.
24. Every accepted fact carries a source mask and a corroboration grade: `observed-only` | `single` | `corroborated`.
25. Owner classes of regions are one of: `unknown`, `firmware`, `image`, `nxu`, `mmio-accepted`, `reserved-do-not-touch`. `unknown` is never mapped.

---

## 5. Foundation / Silicon / Board / Research boundaries

### 5.1 Import rules

| From \ Into | Foundation | Silicon | Board | Research |
|---|---|---|---|---|
| Foundation | — | may call operations | may read candidate facts | must not import |
| Silicon | operations only | — | must not read board | must not import research |
| Board | candidate facts only | must not call silicon | — | no |
| Research | **forbidden** | **forbidden** | **forbidden** | — |

Silicon has no policy. Board has no behavior. Research has no TCB presence.

Untrusted blob **decoders** (FDT) emit candidate facts. They do not decide. They are not a device model. They live on the facts path as decoders, not as architectural policy.

### 5.2 Foundation (solid, not merely small)

Foundation is complete when NXU owns CPU, memory, translation, exceptions, signals, time, and an observable fail-closed execution environment on all present CPUs — with hardware enforcement where hardware can enforce.

It is incomplete if any of those are missing. It is bloated if it contains isolation *names*, drivers, filesystems, schedulers, or crypto boot without evidence.

#### Boot contract

| # | Problem | Usual assumption | Remove? | Hardware? | Mechanism | Proof |
|---|---|---|---|---|---|---|
| 1 | Entry environment may be a lie | Linux ARM64 boot protocol is the contract | Yes as trust. Accept as one *intake shape* only | Observe EL, DAIF, SCTLR, MPIDR, PC | Structural contract: observe, validate, corroborate, reject | Bad header / mixed EL / unmasked IRQ that cannot be masked → halt |
| 2 | DTB/memory/topology are claims | Trust DTB, then parse as truth | Yes | Cannot trust a blob | Bounded probe + wrap-safe validate + corroboration with board + observation | Disagreeing RAM/GIC/CPU sources → halt |
| 3 | Crypto boot | UEFI/AVB/TBBR | Not in foundation (A1) | ROM/fuses maybe | Research comparison of authority models | See §6 |

**Intake observes:** PC, CurrentEL, DAIF, SCTLR, TTBR if MMU on, MPIDR, x0–x3.

**NXU installs its own stack from the image.** Incoming SP is discarded.

**Pre-MMU window:** touch only the image footprint (PC + header). DTB probe allowed only if aligned, magic matches, `totalsize` ≤ a compile-time cap (not a platform RAM size), and overlap rules hold. After a memory map exists, image ⊂ RAM or reject. Then the window is discarded.

**Stay at entry EL (A11).** Do not drop EL2→EL1.

**Cryptographic verification is not foundation.** Fail closed is structural.

#### Facts

| # | Problem | Usual assumption | Remove? | Mechanism |
|---|---|---|---|---|
| 4 | Platforms differ in addresses | `#define` QEMU virt in “platform” code | Yes | Normalized fact table only |
| 5 | One lying source | Single source of truth (DTB *or* hardcoded board) | Partially: single source is a recorded weakness, not a pass with full corroboration | Corroborate independent sources; mismatch rejects |

Fact kinds: CPU, memory, controller, time, optional console sink, bring-up method.

Board may supply candidates. Observation may supply MPIDR, IIDR, CNTFRQ, PC. DTB may supply candidates. Foundation accepts a fact only when well-formed, non-contradictory, and agreed where two sources speak.

#### Memory ownership + MMU

| # | Problem | Usual assumption | Remove? | Hardware? | Mechanism |
|---|---|---|---|---|---|
| 6 | What may be touched | Firmware memory map is correct | Yes | MMU after takeover | Physical ownership map, then stage-1 translation |
| 7 | Spatial isolation | Software convention / language | No, do not remove the *need* | **Yes, MMU** | Default deny, never WX, unknown unmapped |
| 8 | Early access | Identity-map all RAM | Yes | — | Bootstrap window = image only |
| 9 | Range checks | `addr + size` | Yes that form | — | Wrap-safe membership (A6) |

Owner classes as invariant 25. After space armed, MMU stays on. No demand paging, swap, overcommit, or userspace layout.

#### Exceptions

| # | Problem | Usual assumption | Mechanism |
|---|---|---|---|
| 10 | Unexpected control transfer | Panic pretty-printer, then continue if possible | Vector table is NXU’s. Entry saves into the running exec context. Unexpected class → halt |

#### CPU topology / SMP

| # | Problem | Usual assumption | Remove? | Mechanism |
|---|---|---|---|---|
| 11 | Several CPUs | Linux `cpu_possible/present/online` + `smp_ops` + `cpu_start(1)` | Yes those | Four-state model + failed. Boot CPU iterates the available set |
| 12 | Bring-up | Trust PSCI; treat return as online | Yes the trust | Adapter + timeout + self-publish online |
| 13 | Secondary stack | `__secondary_stack_cpu1` | Yes | Per-CPU stack created at initialize, for every present CPU |
| 14 | Shared kernel | Either Linux one-kernel or Barrelfish multikernel by default | Do not default to either ideology | Shared enumerated objects; everything else CPU-local. Multikernel remains research (A3) |

Secondary sequence: mask → own SP → VBAR → enable already-built tables → bind redistributor matching MPIDR → arm timer → publish online → idle context.

#### `nxu_cpu_exec_context`

| # | Problem | Usual assumption | Remove? | Mechanism |
|---|---|---|---|---|
| 15 | Need a restorable execution state | Thread / process / task / seL4 TCB | Yes those names and policies | The atom: save/restore/halt/idle only |

Not a scheduler entity. Not a POSIX thread. Room-binding, if ever, is a promotion, not a field we treat as real today.

#### Signals

| # | Problem | Usual assumption | Remove? | Hardware? | Mechanism |
|---|---|---|---|---|---|
| 16 | Hardware events | Linux IRQ domain/chip, `request_irq`, 1-of-N | Yes | GICv3 can route to one CPU | Signal = identity + exclusive owner + single-CPU route |
| 17 | Dispatch | Function pointer table | Yes (A8) | — | Compiled-in TCB paths by INTID class |
| 18 | GICv3 is the architecture | irqchip | Yes | GICv3 is *an* adapter | Architecture is signals. Silicon programs GICv3 |

Foundation binds only: per-CPU timer PPI, and SGI-as-IPI after SMP is online. Unbound pending INTID → chronicle, EOI, halt. Spurious 1020–1023 counted, not dispatched.

#### Time

| # | Problem | Usual assumption | Remove? | Mechanism |
|---|---|---|---|---|
| 19 | Ordering and timeouts | HZ tick, jiffies, `timer_list` | Yes | Observed counter + per-CPU one-shot deadline as a TCB signal |
| 20 | Secondary hang | Wait forever / hope | Yes | Deadline; expected present CPUs not online → halt (A12) |

Not wall clock. Not POSIX timers. Not a demo UART tick.

#### Chronicle and fail-closed

| # | Problem | Usual assumption | Remove? | Mechanism |
|---|---|---|---|---|
| 21 | Need to know what NXU did | printk, logging framework, earlycon as architecture | Yes those | Append-only in-memory chronicle; optional UART sink after facts accept it |
| 22 | Bad state | Degraded boot | Yes | Halt: DAIF masked, no CPU_ON, WFI. UART optional |

### 5.3 Silicon

Hardware knowledge, not architectural policy.

Allowed: AArch64 register read/write, exception entry assembler, MMU programming ops, GICv3 distributor/redistributor/ICC ops, PSCI calling-convention ops, generic timer ops, PL011 ops.

Forbidden: addresses of QEMU/FVP/SoC, “if qemu”, CPU count constants, `cpu_start(1)`, skip-validation, handler policy, ownership decisions, EL drop policy.

Silicon answers “how is this register accessed?” Foundation answers “may it be accessed, by whom, and what happens if it disagrees?”

### 5.4 Board

Platform facts only.

Allowed: candidate RAM, MMIO, CPU affinities, controller bases, optional UART base, enable-method claims.

Forbidden: functions that implement SMP, exceptions, signals, MMU policy, or fail-open. Forbidden: a different architecture per board.

QEMU virt RAM `0x40000000`, GICD `0x08000000`, UART `0x09000000`, load `0x40080000` are **board facts** if used at all. They are not NXU architecture. Historical NXU put them in architectural C. That is a documented failure.

### 5.5 Research

Hypotheses, alternatives, experiments, measurements, evidence.

Cannot be imported by foundation, silicon, or board.

**Room / Wall / Edge / Fabric / Pulse are research names.** They are not subsystems. They are not implemented because the names are unique. Promotion requires a property **not already provided by memory + MMU + exec context + signals + time**, plus a hardware lever, fail-closed behavior, and a measurement that can fail.

| Name | Hypothesis | Promotion bar |
|---|---|---|
| Room | Rushby regime: private machine except declared Edges | Not just “an address space” (MMU already is). Must add isolation of *execution* and *faults* beyond one TCB context |
| Wall | Hardware barrier between Rooms | Must be a checkable configuration of MMU + signal non-delivery + time non-sharing, not a comment |
| Edge | Only legal information flow | Must be enforceable (no other path), typed, exclusive, measured |
| Fabric | Graph of Rooms and Edges | Only if the graph is an artifact NXU can verify, not a drawing |
| Pulse | Temporal isolation / interference bounds | Only if a measurement shows non-interference or a bound; Cortex-A cache/Spectre may falsify the claim |

Rushby properties — spatial separation, temporal separation, information-flow control, fault isolation — are **claims to measure**. NXU must not claim them without evidence.

---

## 6. Research questions

### 6.1 Isolation (measure, do not claim)

| ID | Question |
|---|---|
| R1 | After S14, can two exec contexts with disjoint maps + disjoint signals + disjoint deadlines still interfere (cache, contention, timers, speculation)? If yes, Room is not implied by the foundation. |
| R2 | Does “Room” add anything beyond a second exec context with its own translation and signal bindings? If no, reject the name. |
| R3 | Can a non-TCB signal be delivered with zero TCB policy beyond ack/EOI/context switch? If no, Rooms cannot own devices. |
| R4 | Pulse: periodic vs one-shot. What is the actual interference bound on Cortex-A? If unmeasurable, do not claim temporal isolation. |
| R5 | Fault in a non-TCB context: can the TCB contain it without halt? Foundation currently halt-on-TCB-fault is correct. Room-fault-isolation is a different claim. |

### 6.2 Boot trust (compare before choosing keys)

| ID | Question |
|---|---|
| R6 | User-sovereign verified boot vs DICE vs measured-only vs structural-only. **Who holds the root of authority?** Vendor-held keys conflict with “users own the system.” |
| R7 | Rollback: RPMB / fuses / offline counter — which exists on the target class of boards, and what fails closed on missing storage? |
| R8 | Is a Linux 64-byte ARM64 Image header an intake convenience or a hidden Linux dependency? Compare a native NXU header. |
| R9 | Board facts compiled-in vs a second untrusted blob. Corroboration vs update. |

### 6.3 Execution and SMP

| ID | Question |
|---|---|
| R10 | Stay-at-EL2 cost vs EL1; value of stage-2 for later isolation (Q1/A11). |
| R11 | Must all present CPUs come online (A12) on heterogeneous/offline-cluster descriptions? |
| R12 | Spin-table: reject until a board without PSCI exists. |
| R13 | Cache/TLB completeness for secondary MMU-on. Measure; do not copy Linux. |
| R14 | Identity-map NXU RAM vs a high virtual TCB window. |
| R15 | How much firmware MMIO must remain mapped after takeover? |
| R16 | Shared-memory SMP vs multikernel: measure false sharing and bring-up complexity before changing A3. |
| R17 | seL4-style untyped+capabilities vs Rushby static partitions. Compare **after** S14. Do not pre-select. |
| R18 | When, if ever, does exec context become schedulable without becoming a UNIX thread? |
| R19 | GICv3 two-security vs single-security: observe and adapt, or require one? ITS/LPI TCB cost? |
| R20 | Bounded C subset vs Rust. Language must not become the isolation story. |
| R21 | Chronicle freeze-format at S14 sufficient to reconstruct a boot on paper without a debugger. |

### 6.4 Method

```
hypothesis → mechanism → isolated implementation → experiment → measurement → evidence
                                                                        │
                                                            promote | modify | reject
```

No security property is claimed without a measurement that could have failed.

---

## 7. Implementation stages

Do not start until `NXU GENESIS V2 ACCEPTED`.

Each stage: implement, run on **two fact sources** (QEMU plus FVP, or two QEMU machines with different addresses), chronicle, halt on failure. Do not sneak in the next stage. Do not implement Room/Wall/Edge/Fabric/Pulse.

| Stage | Delivers | Done when |
|---|---|---|
| S0 | Image, PC-relative entry, NXU stack, DAIF mask, chronicle, halt | Incoming SP unused. Halt injectable |
| S1 | Intake: EL, MPIDR, SCTLR, PC, x0 | Two boards differ in values, not in record shape |
| S2 | Contract + image header | Bad header halts |
| S3 | Bounded DTB probe + wrap-safe validate | Bad magic / oversized / overlap halt |
| S4 | Memory facts + corroboration + image ⊂ RAM | Disagreeing sources halt |
| S5 | Physical ownership map | Access policy is table-driven |
| S6 | MMU on, WX never, default deny | Write-to-RX and unmapped both halt |
| S7 | Vectors save into exec context | Sync fault records ESR/FAR in the context |
| S8 | CPU present set + MPIDR match | Missing boot affinity halts |
| S9 | Per-CPU init for **all** present | No `cpu1`-only symbols |
| S10 | GICv3 adapter; corroborate CPU count | rdist vs CPU present mismatch halts |
| S11 | Time + timer signal TCB-bound | Deadline fires. No UART-tick policy |
| S12 | Secondary bring-up with timeout | N present → N online or halt. N from facts, not Makefile |
| S13 | IPI SGI | Wrong target does not execute |
| S14 | Foundation freeze | Invariants vs chronicle. Research only in research |

S0–S14 contain no capabilities, Rooms, scheduler, filesystem, networking, POSIX, driver framework, or verified-boot hierarchy.

---

## 8. Explicitly rejected assumptions

These are not “later.” They are out.

### Conventional OS shape

- NXU is a Linux replacement, UNIX clone, or POSIX OS.
- Foundation should contain a filesystem, network stack, driver framework, UI, libc, ELF loader, syscalls, page cache, swap, overcommit.
- Directory shape `arch/` + `kernel/` + `drivers/` as the architecture.
- Migrating or forking the historical NXU tree.

### Trust and boot

- Firmware, DTB, PSCI, bootloader, incoming SP, incoming page tables, or MMIO maps are trusted.
- Linux ARM64 boot protocol is the trust contract (intake shape only).
- Drop EL2 to EL1 because Linux does.
- DTB is source of truth.
- Hardcoded QEMU/FVP/SoC addresses in architectural code.
- “Bring up UART first” as an architectural requirement (chronicle does not depend on it; halt does not depend on it).
- Vendor verified boot / UEFI Secure Boot / Android AVB / TF-A TBBR as the NXU authority model without a key-custody experiment.
- Crypto boot as a substitute for the structural contract.

### CPU and execution

- `present == online`.
- GIC redistributor walk **is** topology (it is a corroboration source).
- `CPU_ON` success **is** online.
- Hardcoded `cpu_start(1)` and a single secondary stack symbol.
- Linux `cpu_possible/present/online` masks as the model (the words are similar; the state machine is not).
- `nxu_cpu_exec_context` is a thread, process, task, or seL4 TCB.
- A scheduler, timeslice, or runqueue in the foundation.
- Multikernel as silent default, or Linux SMP as silent default. Foundation is explicit per-CPU state plus enumerated sharing.

### Memory and interrupts

- Identity-map all RAM “for convenience.”
- WX mappings, default-allow tables, demand paging as foundation.
- Overflowing `address + size` membership.
- Linux IRQ domain/chip, `request_irq`, handler function pointers.
- 1-of-N targeting.
- GICv3 (or any controller) **is** the interrupt architecture.
- ITS/LPI in foundation.
- HZ tick / jiffies / demo `uart_puts("tick")` as time architecture.

### Authority, language, research names

- Ambient authority: global current-task, caller-supplied TCB handlers, unbound enabled signals.
- Language (C types, Rust borrow checker) defines isolation or safety semantics.
- Minimal TCB means skip MMU, SMP completeness, timeouts, or fail-closed.
- Room / Wall / Edge / Fabric / Pulse belong in the foundation because they sound architectural.
- Claiming Rushby properties without a measurement that could fail.
- seL4 object names (CSpace, untyped, endpoint) copied into foundation before a comparison experiment.
- Experiments (`NXU_RUN_INTERRUPT_TEST` and kin) in the TCB boot path.
- A logging framework (policy + storage + subsystems) as architecture.

### Historical failures that must not be repeated

| Evidence | Verdict |
|---|---|
| DAIF mask at entry; VBAR before unmask; PSCI can start a secondary on QEMU virt GICv3; redistributor walk counts CPUs; timer PPI can fire; wrap-safe FDT header and physical membership; WFI halt | **Keep the property** |
| Boot validation = DTB pointer ≠ 0 | Reject |
| RAM/GIC/UART/load hardcoded as architecture | Reject |
| DTB validated then unused while addresses stayed hardcoded | Reject |
| Topology from GIC only; three states OFFLINE/STARTING/ONLINE | Reject |
| `cpu_start(1)`, `__secondary_stack_cpu1_top` | Reject |
| Secondary: GIC init + online + unmask; no MMU, timer, or context | Reject |
| Interrupt manager + backend + handler pointers | Reject |
| Timer demo tick as architecture | Reject |
| Logging subsystem in the TCB | Reject |
| Experiments in kernel main | Reject |
| Makefile `-smp 2` vs “4-CPU validation” | Measurement hygiene failure |
| No MMU while claiming privacy-by-construction | Reject |
| UART before memory ownership | Reject |

---

## Review gate

V2 is **PROPOSED**.

Acceptance is the explicit statement:

`NXU GENESIS V2 ACCEPTED`

That statement means agreement on:

- the four-layer split and import rules
- the 25 invariants
- the CPU four-state model
- `nxu_cpu_exec_context` as execution atom, not a thread
- signals as exclusive, single-CPU, no function-pointer dispatch
- structural boot contract in foundation; cryptographic boot in research
- Room / Wall / Edge / Fabric / Pulse as research until evidenced
- A1–A12 as working assumptions, or an explicit amendment of any of them
- the rejected-assumption list
- S0–S14 as the only implementation order after acceptance

No implementation code until then.
