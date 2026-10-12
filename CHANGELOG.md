# NXU Changelog

Evidence-based chronology. Dates are taken from research notes, Git commits, or evidence-log timestamps. Entries without a recoverable date are marked undated.

## 2026-09-06 — Git history begins (main)
Linux-shaped tree (`arch/`, `kernel/`, `drivers/`). Commits cover platform description, DTB validation, interrupt subsystem, logging, timer, CPU topology. Tip of this era is `fb02f5f105337cf7bfff41c7f837f6d7c9403abf` (2026-09-22 README rewrite).

## 2026-09-11 — IRQ authority branch
`nxu/irq-authority-boundary` (`f786d3be6224a5f11143b6aa019e39fb59b2b82b`). Open draft PR #2: “IRQ: remove ambient authority from interrupt subsystem.”

## 2026-09-22 — Genesis v1/v2 accepted
H2–H5. Four-layer split (Foundation / Silicon / Board / Research). Capability-based IRQ proposed in H1 was not adopted as the foundation. Research is not linked.

## 2026-09-22–23 — Pre-Room snapshots
Two ZIP archives (see `archive/manifests/MANIFEST.md`). Neither contains `room.c` or H12. Different inventories; both preserved.

## 2026-09-26 — Rooms, timer, GIC security
H9: FVP Secure-group assignment for INTID 0 and 30. H10: Room timer re-arm from facts. H11: Separation suite exercised.

## 2026-09-27 — Topology and captures
H12: FVP 8-CPU (two clusters) and QEMU 4-CPU. Evidence logs in `probe/evidence/` dated 2026-09-27. Authoritative tar SHA-256 `9142c03e471446e8d95403a9062608ff59be656270d7f02e7fdba6bdfddd65fd`.

## 2026-10-12 — Recovered baseline published for review
Candidate branch `nxu/latest-baseline-review-2026-10-12` at `4e13d899a1db03274117ab477ecd0ada6fa5bbb3` (four commits on top of `fb02f5f`). This archival branch adds snapshots, manifest, limitations, and build notes. No license selected.
