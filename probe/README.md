# probe/

Host-side execution and validation. Not TCB.

```
check.sh          architectural source and image-header checks
qemu/run.sh       QEMU virt; requires the kernel-established line
fvp/run.sh        FVP if present, else NOT EXECUTED
cases/            negative cases
host/             local zig and qemu binaries
evidence/         captured serial logs
```
