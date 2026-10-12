# H9 — Non-secure EL1 cannot assign a GICv3 interrupt group

History, not authority. The kernel behavior is the source and the UART captures.

## Question

After NXU normalized to EL1, FVP still halted in the timer wait. QEMU did not. Was the timer wrong, the PPI number wrong, or was the interrupt not in a group EL1 is allowed to enable?

## What was assumed

The redistributor enable register was treated as an EL1 operation. `gicr_frame_enable` wrote `GICR_IGROUPR0` to all ones and set one bit in `GICR_ISENABLER0`, then returned. The timer wait treated a missing `timer_fired` as a timer failure.

## Hardware model

GICv3 with two security states (`GICD_CTLR.DS` clear, the FVP Base default) splits each interrupt into Secure Group 0, Secure Group 1, or Non-secure Group 1. The pair is `GICR_IGROUPR0` and `GICR_IGRPMODR0` for SGIs and PPIs. Non-secure Group 1 is IGROUPR=1 and IGRPMODR=0. Those group registers are Secure. A Non-secure read is zero for a Secure interrupt, and a Non-secure write does not change it.

At Non-secure EL1, a Non-secure Group 1 interrupt is an IRQ. A Secure interrupt is an FIQ, or it is not forwarded at all if the Non-secure side cannot enable it.

CNTP is the EL1 physical timer. The GICv3 recommendation for its INTID is 30. That number is not a QEMU address. The level that signals it is still a GIC group fact.

NXU drops out of EL3 into Non-secure EL1 and does not keep a Secure monitor. After that drop, the group bits are outside what the kernel can write.

## Observation before the group write

FVP, EL1, PPI 30, `make fvp` on 2026-09-26:

| Register | Value | Reading |
|---|---|---|
| `CNTP_CTL_EL0` | `5` | ENABLE set, IMASK clear, ISTATUS set |
| `CNTP_CVAL_EL0` | `0x27908d` | sample plus 1,000,000 ticks |
| `CNTPCT_EL0` | past that compare | deadline reached |
| `GICR_WAKER` | `0` | redistributor awake |
| `GICR_IGROUPR0` | `0` | group write not visible |
| `GICR_ISENABLER0` | `0` | enable write not visible |
| `GICR_ISPENDR0` | `0` | no pending INTID in this view |
| `ICC_IGRPEN1_EL1` | `1` | Non-secure Group 1 enabled at the CPU interface |
| `ICC_HPPIR1_EL1` | `1023` | nothing to acknowledge |
| `ISR_EL1` | `0` | the PE was not signaled |
| Halt | `0x8000000d` `(0x1e, 2)` | `time.c` `nxu_s11_time` |

FACT: the timer condition was true. FACT: the Non-secure redistributor view did not show the interrupt enabled or pending. INFERENCE: the writes in `gicr_frame_enable` did not change Secure group state.

An earlier run that reached only `GICv3 detected` and then went silent was an EL2 trap. `ICC_SRE_EL2` had been written as 7, which does not set Enable. EL1 ICC accesses were taken to EL2, and `VBAR_EL2` was the halt vector. Setting Enable (`0xf`) let execution continue to the timer halt above. That was a transition requirement, not the group bug.

## Decision

While still at EL3, and only when `nxu_board` names a redistributor, walk frames until `GICR_TYPER` affinity matches MPIDR. Wake that frame. Set IGROUPR bits for INTID 30 and INTID 0. Clear the same IGRPMODR bits. Then enter Non-secure EL1.

INTID 30 is CNTP. INTID 0 is the IPI SGI already used by the kernel. No other interrupt is moved. The address comes from the board fact, not from a literal in the instruction stream. QEMU never takes this path; it does not enter at EL3.

Rejected: changing the PPI to 26 to match the old EL2 path. Rejected: copying a distributor init sequence. Rejected: setting `GICD_CTLR.DS` and deleting the second security state. NXU does not yet have a Secure partition, but burning DS was a larger choice than assigning two interrupts.

## Measurement after the decision

FVP Base, one CPU, same model and load address:

```
[NXU] EL1 translation established
[NXU] CNTP established
  Interrupt source PPI 30
[NXU] CNTP deadline observed
[NXU] CPU set online
[NXU] NXU kernel execution established
```

No halt line. The self-IPI in the one-CPU path returned, because the kernel event is after that wait.

QEMU virt, four CPUs, same source, EL1 entry so the EL3 group walk does not run:

```
[NXU] CNTP deadline observed
[NXU] CPU set online
[NXU] IPI observed
[NXU] NXU kernel execution established
```

## What this does not show

Two security states still exist on FVP. Only INTID 0 and INTID 30 were assigned to Non-secure Group 1. A later interrupt that stays Secure will look like the old timer halt. `GICR_IGROUPR0` from EL1 remains the wrong place to assign a group.

Rooms, Pulses, and a Rushby claim are not established by this result. The kernel timer and the self-IPI now cross the GIC on both instruments. That is interrupt delivery for the foundation, not a partition.

## Remaining uncertainty

Whether every FVP redistributor resets IGRPMODR to 0. The clear of those two bits is the defensive half of the encoding. The successful deadline is evidence the encoding was accepted for INTID 30 on this model, not a survey of reset values.
