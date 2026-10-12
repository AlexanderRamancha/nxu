# silicon/

ARM64 hardware mechanisms. No policy. Addresses arrive as parameters.

```
aarch64/entry/       DAIF mask, NXU stack, residue capture
aarch64/exception/   VBAR slots; IRQ save/restore
aarch64/translate/   page tables, MMU on
aarch64/gicv3/       distributor / redistributor / ICC
aarch64/psci/        CPU_ON adapter
aarch64/timer/       CNTPCT / CNTP / CNTHP
aarch64/uart/        PL011 putc
```

Silicon must not read `board/`.
