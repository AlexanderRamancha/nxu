# H12 — FVP was one CPU because the launch and the topology were different facts

History, not authority. The UART captures are the evidence.

## Question

QEMU reported four CPUs online. FVP reported one. The model is a two-cluster machine that can expose eight cores. The question was where the other CPUs disappeared, and whether that disappearance was the platform, the discovery, or the boot.

## FACT — the launch

`probe/fvp/run.sh` set `cluster0.NUM_CORES` from `NXU_SMP`, default 1, and `cluster1.NUM_CORES=0`. `pctl.startup=0.0.0.0` released only the boot core. The kernel never saw a second cluster because the model was not asked to provide one.

The model parameters, read from `FVP_Base_RevC-2xAEMvA --list-params`:

- `cluster0.NUM_CORES` default 4, range 1..4
- `cluster1.NUM_CORES` default 4, range 0..4
- `pctl.Affinity-shifted=1`, so the core number is Aff1, not Aff0
- `pctl.CPU-affinities=0.0.0.0, 0.0.1.0, 0.0.2.0, 0.0.3.0, 0.1.0.0, 0.1.1.0, 0.1.2.0, 0.1.3.0`

## FACT — what the GIC reported once both clusters were exposed

With four cores in each cluster and only the boot core released, the kernel still accepted one CPU, because a missing DTB took the single-boot-MPIDR path. The redistributors were a separate observation. All eight reported:

| Cluster (Aff2) | Aff1 | Aff0 |
|---|---|---|
| 0 | 0, 1, 2, 3 | 0 |
| 1 | 0, 1, 2, 3 | 0 |

The boot processor was Aff3.0 Aff2.0 Aff1.0 Aff0.0. The old board list `0, 1, 2, 3, 0x100, 0x101, 0x102, 0x103` is the QEMU shape, not this FVP shape. QEMU's own list stays `0, 1, 2, 3`, and its DTB agrees.

## ASSUMPTION that failed

A missing DTB means the machine has one CPU. It meant NXU had not been given a topology source. The board candidate list is that source when no DTB exists. The redistributor count is not the topology. The affinity in each redistributor is the check that a candidate CPU has an interrupt controller.

Another failed assumption: an SMC PSCI call on FVP reaches firmware. NXU is EL3 on FVP, so the SMC has nowhere to go. QEMU enters at EL1 and its firmware answers HVC. Those are different owners of the release, not different CPU architectures.

## DECISION

The probe exposes both clusters at the model's per-cluster maximum, four and four, unless a run asks for fewer. Only affinity `0.0.0.0` is powered at reset. Every exposed core's reset address is the NXU image.

Without a DTB, the board candidate list is the present set. The boot MPIDR must be in it. Each candidate is then matched to a redistributor by affinity. A candidate with no redistributor still fails the existing count check. A redistributor with no candidate is logged and is not turned into a CPU.

A secondary is released by writing its MPIDR to the FVP power-on register at `0x1C100000+4`, after a gate records the logical id. The secondary resets at EL3, finds its gate, and uses the same EL3-to-EL1 drop as the boot CPU. QEMU still uses HVC and never reads the gate. A secondary is online only after it has reached EL1, installed the vectors, and enabled its own redistributor.

Clusters in the narrative are Aff2 groups, because that is the field that separated the two FVP clusters and that stayed 0 for every QEMU CPU.

## FAILURE

The first release of CPU 1 restarted a full boot instead of the secondary path. The gate record was 24 bytes and the reset search stepped 32. The secondary read an empty slot, ignored the gate, and executed the boot CPU path beside the real boot CPU. The boot CPU then timed out in `nxu_s12_smp` (`0x8000000f`, CPU 1, MPIDR `0x100`). The second boot halted in the timer wait because the deadline had already fired. Aligning the gate to 32 bytes made the search and the writer agree.

## TEST

2026-09-27.

QEMU virt, GICv3, cortex-a57, smp 4: four present, four accepted, four online, all Aff2 0. CPU 0 sent an IPI to CPU 0, 1, 2, and 3. CPU 1 sent one back to CPU 0. The room sequence still completed. No halt.

FVP Base RevC, clusters 4+4, startup `0.0.0.0`: eight present, eight accepted, eight redistributors matched, eight online. Cluster 0 has four. Cluster 1 has four. CPU 0 sent an IPI to every CPU. CPU 1 replied to CPU 0. CPU 4, the first CPU with Aff2 1, replied to CPU 0. Then the same room sequence completed. No halt. FVP status 124 is the probe stopping the model after the application line.

Present, initialized, and online are separate lines. A CPU is named online only after its own EL1 and vector lines.

## REMAINING UNKNOWN

Rooms are still entered by the boot CPU. The pending-bit clear uses that CPU's redistributor. Secondaries do not enter a room. There is no migration and no scheduler.

No redistributor was left without a CPU, and no CPU was left without a redistributor, on these two runs. A smaller FVP launch than the board list would fail the association check. That is intentional.

UDP remains undefined.
