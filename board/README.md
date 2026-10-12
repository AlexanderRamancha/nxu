# board/

Candidate facts only. No behavior. No alternate architecture.

```
qemu-virt/    QEMU virt RAM/GIC/UART/CPU/PSCI candidates
fvp-base/     FVP Base candidates
soc/          real hardware placeholder (present=0)
```

QEMU, FVP, and a real SoC share foundation and silicon. They differ only here and in probe launchers.
