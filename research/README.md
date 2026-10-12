# research/

Not TCB. Not foundation. Not silicon. Not board.

This directory is hypotheses, experiments, measurements, evidence, and history.
It has **no architectural authority**. Promotion into foundation requires an explicit
amendment of Genesis, not a compile flag, not an include, not a quiet link.

## TCB firewall (non-negotiable)

1. Foundation, silicon, board, and the TCB image allow-list must not include
   `research/` sources, headers, or objects.
2. A probe image that links research is not the TCB image. Two artifacts, never one.
3. `NXU_RUN_*` and kin live in probe/research images only.
4. History under `research/history/` is evidence of evolution. It must not be cited
   as a reason to add a foundation mechanism.
5. Wall, Edge, and Fabric are names only. Rooms and pulses are in the current
   kernel because a later implementation instruction required them and H10
   records the measurement. This directory is still not authority for either.

If a change would make research reachable from the TCB path, the change is rejected.
