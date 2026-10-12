# foundation/

Genesis policy and sequencing. This is the TCB.

```
enter.c              S0–S14 call order
intake/              S0 image footprint, S1 residue
contract/            S2 image header
facts/               S3 DTB probe, S4 corroboration
space/               S5 ownership, S6 MMU policy
exception/           S7 VBAR and C exception/IRQ
context/             nxu_cpu_exec_context bind
cpu/                 S8 present, S9 init, S12 occupy
signal/              S10 GIC bind, S13 IPI
time/                S11 one-shot timer
observe/             chronicle, Black Box, console, fail, freeze, halt
```

Foundation decides. It calls silicon with addresses as parameters. It does not contain QEMU or FVP addresses.
