# NXU

**A privacy-first mobile operating system built from first principles.**

NXU (Not eXactly Unix) is an experimental ARM64 operating system that rethinks mobile OS design with privacy, minimalism, and explicit authority as foundational constraints — not features added later.

---

## Philosophy

NXU begins with one question:

> What problem are we actually solving?

Legacy complexity is rejected unless it can be justified. Every subsystem must earn its place.

**Design Principles**

- **Privacy by construction** — Data exposure is minimized by architecture, not policy
- **Explicit authority** — Capability-based security; no ambient ambient rights
- **Minimal trusted computing base** — Reduce what must be trusted
- **First-principles design** — Re-examine assumptions instead of inheriting them
- **Simplicity over tradition** — Prefer clear mechanisms to legacy compatibility
- **Inspectable systems** — Design decisions should be understandable

---

## Privacy & Security Approach

Most systems treat privacy as an application-layer concern.  
NXU treats it as a systems problem.

- Authority is granted through unforgeable capabilities, not global permissions
- Components receive only the rights they need to perform a specific task
- Isolation boundaries are enforced by the kernel, not by convention
- User data paths are designed to be narrow and intentional

Security is not a checklist. It is a consequence of limiting power by default.

---

## Current Status

Foundation stage.

**Done**

* ARM64 boot
* UART console
* Exception vectors + basic handling
* Early physical memory map
* Platform hardware description
* DTB validation
* GICv3 interrupt subsystem
* Interrupt manager + backend
* CPU topology + SMP startup
* PSCI CPU startup
* ARM Generic Timer
* Structured logging
* Kernel startup integration
* 4-CPU QEMU validation

**Next**

* Console subsystem
* Panic and fatal-error handling
* Capability primitives
* Capability-based interrupt access
* Additional SMP validation
* Persistent diagnostic storage
* Userspace/runtime architecture


---

## Build

```bash
make          # Build
make clean
make qemu     # Run in QEMU

```
---

## Structure

- `arch/arm64/` — Architecture code (boot, exceptions)
- `drivers/` — Hardware drivers
- `kernel/` — Core kernel

## Contributors

Background doesn’t matter.

Whether you’re a student, researcher, professional, or self-taught explorer — if you’re curious about how systems work and willing to reason carefully, you’re welcome.

Useful contributions include:

- Kernel and architecture code
- Drivers
- Testing and experimentation
- Documentation of design decisions
- Thoughtful critique

No prior kernel experience is required.  
Clear thinking and respect for complexity are.

---
## Founder

**Alexander Ramancha**

Computer enthusiast exploring how systems work at a fundamental level. Currently learning operating system design, ARM64, and low-level programming by building NXU in public. More curious than experienced — focused on understanding deeply and improving step by step.

***Reality defines the constraints. Engineering designs the solution.***

---

**Users should own their systems.**
**Systems should not own their users.**

— NXU (Not eXactly Unix)

