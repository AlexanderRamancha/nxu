# Observe

CURRENT STATE: IMPLEMENTED for the kernel event path. PARTIALLY IMPLEMENTED for presentation. Host sound is NOT IMPLEMENTED.

## Files

| File | Role |
|---|---|
| `include/nxu/log.h` | Event ids, record, `nxu_log` |
| `foundation/observe/log.c` | Record, chronicle slot, handoff to console |
| `foundation/observe/console.c` | One text line per record. Boot identity on the boot event |
| `foundation/observe/sink.c` | Bound PL011 bytes. Silent if unbound |
| `foundation/observe/chronicle.c` | Per-CPU ring and the immortal failure slot |
| `foundation/observe/fail.c` | Failure record, black-box dump, halt |
| `foundation/observe/bbox.c` | Bounded dump. Not a second logger |
| `silicon/aarch64/uart/pl011.c` | Register writes |

## Flow

```
subsystem
  → nxu_log
      → chronicle ring (event, v0, v1)
      → nxu_console_record
          → nxu_sink_write
              → PL011
```

A missing sink returns. It does not halt and it does not change EL, MMU, GIC, timer, or SMP.

The failure slot keeps `fail_func`, `fail_file`, and `fail_line` from the `nxu_fail` call site. Those strings are the compiler's `__func__` and `__FILE__`.

## Events that exist

`boot`, `dtb`, `memory`, `mmu`, `exception`, `fault`, `cpu`, `gic`, `gic-init`, `timer`, `timer-fire`, `cpu-online`, `cpu-set`, `ipi`, `kernel`, `machine`, `halt`.

`machine` is only the timer-delivery read that already ran when the deadline was set and `timer_fired` stayed clear. It is not a new subsystem.

## Error path

`nxu_fail` stores the code and the call site, emits `[nxu] fail fail halt`, dumps the black box, then halts. The line carries the code, both values, the file base name, the function, and the line.

## Not implemented

Host-side vintage sound. The kernel line is the sync point. Playback must stay on the development host.
