# NXU Archive Manifest

This manifest records recovered historical snapshots. It does not invent missing Git metadata.

## Snapshots

| File | SHA-256 | Known date | Git metadata | Notes |
|---|---|---|---|---|
| `archive/snapshots/nxu-2026-09-27.tar.gz` | `9142c03e471446e8d95403a9062608ff59be656270d7f02e7fdba6bdfddd65fd` | Evidence logs dated 2026-09-27; archive attached 2026-10-12 | None (no `.git` in archive) | Authoritative recovered baseline. Contains Room/Pulse, H9–H12, 2026-09-27 QEMU and FVP captures. |
| `archive/snapshots/nxu-2026-09-23-artifacts.zip` | `2c8c03d8845521acf0089bda55a8bbbc91cb1942abdf2227c0348f61cf056774` | File dates 2026-09-22–23 | None | Pre-Room snapshot. Includes `build-*` outputs. No `room.c`, no H12. |
| `archive/snapshots/nxu-2026-09-23-workspace.zip` | `8ad856baf7db9387da2a8239bf01bfe9bb98886d341520fd067eb7d528a79864` | File dates 2026-09-22–23 | None | Pre-Room snapshot. Different inventory from the artifacts zip (additional README files, different H9/H10 filenames). No `room.c`, no H12. |

## Comparison note

The two ZIP files have different checksums and different file inventories. They are not duplicates. Both are pre-Room. The tar is a later baseline that supersedes them for the current working tree but does not replace them as historical evidence.

## Git refs (verified 2026-10-12)

| Ref | SHA | Relationship |
|---|---|---|
| `main` | `fb02f5f105337cf7bfff41c7f837f6d7c9403abf` | Linux-shaped tree tip |
| `archive/main-before-recovered-baseline-2026-10-12` | `fb02f5f105337cf7bfff41c7f837f6d7c9403abf` | Points at same commit as main |
| `nxu/irq-authority-boundary` | `f786d3be6224a5f11143b6aa019e39fb59b2b82b` | Open draft PR #2 |
| `nxu/latest-baseline-review-2026-10-12` | `4e13d899a1db03274117ab477ecd0ada6fa5bbb3` | Four-commit recovered baseline |
| `nxu/archive-publication-2026-10-12` | (this branch) | Created from candidate; adds archival material only |

## What is not recoverable

- Original commit SHA of the 2026-09-27 tar (no `.git` metadata).
- Full conversation transcripts (only research notes H1–H12 and summaries exist).
- Pre-push local trees that were never archived.
