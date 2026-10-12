# Foundation status

S0–S14 frozen. Genesis v2 + closure unchanged.

| Stage | Status | Evidence |
|---|---|---|
| S0–S14 | implemented | QEMU smp=4 and FVP SMP=1 print the kernel sentence |
| Rooms and one pulse edge | exercised | both consoles: cross-room load and store, forbidden fetch, code write, UART load, timer write, malformed pulse, one owed entry, foreign interrupt, deadline. H11 |
| CPU topology and SMP | exercised | QEMU 4 CPUs, one cluster. FVP 8 CPUs, two clusters. Cross-CPU IPI including FVP CPU 4 to CPU 0. H12 |
| UDP | not defined | no mechanism and no success line |
| QEMU | EXECUTED | `make qemu` requires the kernel sentence and the application sentence, and rejects `[NXU] Halted` |
| FVP | EXECUTED | same two sentences, no halt line. Return 124 is the probe stopping the model |
| Real SoC | NOT EXECUTED | `board/soc/` facts present=0 |

S14 wait-loop failure, historical: IRQ 30 was delivered; `timer_fired` was not compiler-visible and headers did not rebuild waiters. Volatile field, pointer-to-volatile wait, DSB after IRQ store, `-MMD -MP`. The live capture looks for the kernel sentence and the application sentence.
