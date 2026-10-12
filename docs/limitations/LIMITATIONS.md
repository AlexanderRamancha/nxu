# NXU Limitations

Source-grounded gaps. Not a claim that the project is incomplete in every respect; a record of what the recovered tree does not establish.

| Item | Status | Evidence |
|---|---|---|
| Rooms on secondary CPUs | Not implemented | `nxu_rooms_run()` called once from `foundation/enter.c`; `nxu_secondary()` in `foundation/cpu/smp.c` loops on WFI + IPI echo |
| Secure-interrupt coverage | Partial | `silicon/aarch64/entry/start.S` assigns INTID 0 and 30 to Non-secure Group 1 on the FVP EL3 path only |
| Real SoC | Not implemented | `board/soc/facts.c` has `present = 0`; boot fails closed |
| DMA isolation | Not implemented | No DMA ownership or bus-master isolation; CPU page tests only |
| Speculation / TLB / cache coupling | Not measured | Noted in `FOUNDATION_MAP.md`; no experiment |
| Formal Rushby verification | Not verified | Room/Pulse is an experiment, not a formal proof |
| UDP / Fabric / Edge / Wall | Not implemented | Research vocabulary only; not in the TCB |
| Fresh QEMU / FVP runs of this branch | Not run | 2026-09-27 logs are historical evidence only |

Status labels: implemented, source-verified, historically observed, experimental, partial, not implemented, not measured, not verified.
