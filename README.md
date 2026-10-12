# NXU — Not eXactly Unix

NXU is a small ARM64 separation substrate under active development. It is not a Unix clone, not a microkernel with a process table, and not a capability-based operating system. The name is an architectural statement: NXU is investigating a different composition of authority, isolation, and execution boundaries than the inode/UID/chmod model that Unix established and refined.

Unix (and the research that followed it) contributed hardware protection rings, address-space isolation, access-control lists, and decades of measured security practice. NXU does not claim to have invented isolation, separation kernels, or hardware-enforced boundaries, nor does it claim to be the first system of its kind. It is exploring one particular composition: explicit, bounded ownership relationships; a fixed topology; droppable, kernel-mediated communication; and hardware page-table enforcement, with a developing model for temporal preemption. Formal proof has not been performed. The measurements recorded so far demonstrate only the behaviors that were tested on the captured tree.

## What NXU is and what it is trying to change

NXU is a from-scratch ARM64 bare-metal kernel. Its current implementation is a staged foundation (S0–S14) followed by a two-Room experiment. The design objective is a privacy-oriented mobile execution substrate in which:

- Authority is not ambient. A subject does not start with a global namespace of resources.
- Ownership is explicit and bounded. Each Room is given a small set of pages; everything else is unmapped or privileged.
- Topology is fixed for the experiment. There are two Rooms and one Pulse edge (Room 0 → Room 1).
- Communication is bounded and droppable. A Pulse is a kernel-mediated copy of a single value, or it is rejected. There is no persistent capability or open channel.
- Spatial boundaries are hardware-enforced by stage-1 page tables (user-accessible pages for the active Room only).
- Temporal boundaries are exercised by a timer deadline that can reclaim a Room that does not leave.

What is implemented today is the two-Room experiment on the boot CPU, with the spatial and temporal tests listed in the evidence section. What remains a design objective is a more general separation-kernel model (multiple subjects, richer resource relationships, DMA, secondary-CPU Rooms, real SoC).

## Why the architecture evolved

The public record is in `research/history/`. The following is a reading of that record; it does not rewrite earlier decisions as if they were steps toward the current design.

**H1 (historical tree).** The earlier public tree was Linux-shaped (`arch/`, `kernel/`, `drivers/`). It brought up UART, exceptions, DTB parsing, GICv3, PSCI, and the generic timer on QEMU virt. A capability-based IRQ was proposed and not built. Addresses were largely hardcoded. The tree is preserved as evidence only; Genesis does not migrate it. That earlier structure remains in the Git history of this repository (see `archive/main-before-recovered-baseline-2026-10-12`).

**Capability exploration and reversal.** Early notes considered capability tokens or capability tables as a way to express explicit authority. That direction was not adopted as the foundation. The reasoning recorded in the Genesis documents is that research names must not enter the foundation merely because they sound unique, and that a property must be added by mechanisms the kernel already owns (memory, MMU, execution context, signals, time) rather than by a new token vocabulary. The replacement direction is software-defined policy: the kernel decides which pages a Room may touch and which Pulse values are permitted, and the MMU and exception paths enforce the decision. No capability table or unforgeable token is present in the current source as the foundational authority mechanism.

**Genesis v1 → v2 (H2–H5, 2026-09-22).** The project moved to a four-layer split (Foundation / Silicon / Board / Research). Research is not linked. S0–S14 became the implementation sequence. EL normalization to EL1, fact validation before use, and fail-closed paths were adopted. Rushby-style separation properties were noted as claims to be measured, not as slogans.

**Rooms and Pulse (H10–H11, 2026-09-26).** After an EL1 kernel with CNTP and a self-IPI was measured on both QEMU and FVP, the next experiment was two subjects with private pages, a kernel-copied Pulse or a rejection, contained faults, and a deadline. H10 records that FVP halted on the first entry until the timer was re-armed from facts. H11 records the exercised violations (foreign load/store/fetch, malformed Pulse, one owed entry, foreign IRQ, deadline).

**Topology (H12, 2026-09-27).** FVP secondaries are released via the power controller; QEMU uses HVC PSCI. Online is not defined as “CPU_ON returned success.” The captured runs show 4 CPUs on QEMU and 8 CPUs on FVP.

The current architecture is one outcome of those reversals and measurements. It is not presented as the only possible outcome.

## Architecture and authority model

```
Hardware
   │
   ▼
silicon/aarch64     register operations (addresses as parameters)
   │
   ▼
board/*             candidate facts only (QEMU virt, FVP Base, SoC placeholder)
   │
   ▼
foundation/enter.c  S0 → S14, then nxu_rooms_run(), then halt
   │
   ├── intake/ contract/ facts/ space/ exception/
   ├── cpu/ signal/ time/ room/ observe/
```

Steady-state execution is EL1. QEMU enters at EL1; FVP enters at EL3 and drops. The kernel installs its own stack and vectors before the drop on the FVP path. Translation is a single stage-1 identity map. The active Room receives user-accessible mappings for its code and data pages; all other Room pages are privileged. A Pulse is accepted only from Room 0, only when the destination is 1 and the payload is the constant `0x5055`, and only when the pointer lies inside Room 0’s data page. The kernel copies the value; the sender does not write the receiver’s page.

## Rooms, Pulse, and the Rushby-inspired separation model

The Room/Pulse experiment is inspired by separation-kernel ideas (including Rushby’s work on partitioning and information flow). It is an analytical influence and a measurement target, not a claim of formal verification.

What the current source implements:

- Two Rooms, one active at a time.
- Private code and data pages; foreign and kernel accesses fault and are contained.
- One Pulse edge with a fixed payload; malformed Pulses are rejected.
- One outstanding entry is owed; a second Pulse refreshes the payload rather than creating a second entry.
- A timer deadline reclaims a Room that does not exit.
- A foreign IRQ reclaims the active Room.

What is not implemented or not measured:

- Rooms on secondary CPUs (secondaries WFI after online).
- DMA isolation.
- Speculation or TLB interference between Rooms.
- UDP, Fabric, Edge, Wall (research vocabulary only; not in the TCB).
- Formal proof of spatial, temporal, or information-flow isolation.

Status legend used below:

- **IMPLEMENTED AND TESTED** — present in source and observed in the 2026-09-27 captures.
- **SOURCE-VERIFIED** — present in source; not re-run in this review.
- **EXPERIMENTAL** — exercised on the captured tree; scope is the two-Room suite.
- **IN PROGRESS** — separation model is developing; not complete.
- **NOT IMPLEMENTED** — no mechanism in the tree.
- **NOT MEASURED** — no evidence in the captures or research notes.

## What the current evidence demonstrates

The captures in `probe/evidence/qemu.log` and `probe/evidence/fvp-uart.log` are dated 2026-09-27. They are historical evidence of the tree that produced them, not fresh test results.

| Observation | Status | Evidence |
|---|---|---|
| QEMU virt / GICv3 / Cortex-A57, 4 CPUs online (Aff2=0) | IMPLEMENTED AND TESTED | `probe/evidence/qemu.log` 2026-09-27 |
| FVP Base RevC, 8 CPUs online across two affinity clusters | IMPLEMENTED AND TESTED | `probe/evidence/fvp-uart.log` 2026-09-27 |
| Room/Pulse suite (contained faults, Pulse accept/reject, foreign IRQ, deadline) completes without `[NXU] Halted` | EXPERIMENTAL | both logs; H11 |
| EL1 steady state; FVP EL3→EL1 drop | IMPLEMENTED AND TESTED | logs + `silicon/aarch64/entry/start.S` |
| Secure interrupt assignment for INTID 0 and 30 on FVP path | SOURCE-VERIFIED | `start.S` group assignment; H9 |
| Rooms execute only on the boot CPU | SOURCE-VERIFIED | `foundation/enter.c` calls `nxu_rooms_run()` once; `foundation/cpu/smp.c` secondaries WFI |
| Real SoC | NOT IMPLEMENTED | `board/soc/facts.c` `present = 0` |
| DMA isolation | NOT IMPLEMENTED | no DMA ownership in source |
| Speculation / TLB coupling | NOT MEASURED | noted in `FOUNDATION_MAP.md` |
| UDP / Fabric / Edge / Wall | NOT IMPLEMENTED | research vocabulary only |

## What remains incomplete

- Secondary-CPU Room entry.
- Broader Secure-interrupt coverage beyond INTID 0 and 30.
- DMA isolation.
- Measurement of speculation and TLB interference.
- Real-SoC fact file and execution.
- Any general claim of spatial, temporal, information-flow, fault, DMA, or microarchitectural isolation beyond the tested two-Room suite.
- Formal verification.

## Hardware/platform support and reproducibility

| Platform | Status in archive | How to run |
|---|---|---|
| QEMU virt (GICv3, Cortex-A57, smp=4) | EXECUTED in 2026-09-27 capture | `make qemu` |
| FVP Base RevC (4+4, load 0x80000000) | EXECUTED in 2026-09-27 capture | `make fvp` / `make fvp-run` (requires local FVP binary) |
| Real SoC | NOT EXECUTED | `board/soc/` placeholder |

Linker scripts: `image/nxu.ld` (QEMU VA 0) and `probe/fvp/nxu-load.ld` (FVP 0x80000000). The FVP launcher sets RVBAR and uses `--data` to place the image in DRAM, avoiding the BL1 region at 0x0.

## Historical research and dated evidence

| Note | Date | Subject |
|---|---|---|
| H1 | (historical) | Earlier public tree; capability IRQ proposed, not built |
| H2–H5 | 2026-09-22 | Genesis v1/v2; four-layer split accepted |
| H6–H8 | 2026-09-22 | S1–S14 foundation; structural correction; S14 observe |
| H9 | 2026-09-26 | GIC security-group assignment on FVP |
| H10 | 2026-09-26 | Rooms and timer re-arm |
| H11 | 2026-09-26 | Separation suite exercised |
| H12 | 2026-09-27 | CPU topology and SMP |

Full notes are in `research/history/`. Evidence logs are in `probe/evidence/`.

## Roadmap and contribution guidance

The immediate engineering gaps visible in the current tree are secondary-CPU Rooms, broader interrupt-group coverage, and a real-SoC fact file. Research directions that are explicitly not implemented include DMA isolation, measurement of microarchitectural interference, and any Fabric/Edge/Wall/UDP mechanism.

Contributions that add research vocabulary to the foundation without a measured property are out of scope for the current tree. Changes should preserve the fail-closed paths, the two-source fact rule, and the distinction between research notes and linked code.

No license is selected in this document.
