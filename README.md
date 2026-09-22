# NXU

**Not eXactly Unix.**

A privacy-first kernel for a mobile machine.

—

This repository is the beginning.

The first public snapshot. Early ARM64 bring-up. It is not the kernel we are building now, and it is not the architecture we hold ourselves to.

NXU is still in original development. The work continues off this tree.

If you are reading the code here, you are looking at how it started — not at where it is.

—

**Why.** Unix asked who you are, then what you may touch. NXU asks what physically exists, and what is already wired. No root. No `/dev/uart`. No chmod later. Privacy is not a setting.

**How.** Hardware first. Then an experiment that could fail. QEMU is the laboratory. ARM is the law. The phone is the destination.

**Where we are.** Foundation. Not a product. Laboratory demos include untrusted boot, default-deny occupant maps, MMU-denied peer RAM, and a timer path that does not map the GIC into the occupant. Not shown: a phone OS, formal proof, silicon, interrupt isolation, an occupant that cannot reprogram the MMU.

**This tree.** Clone it for the origin story. Expect retired names and claims we took back. The living design is not published here yet.

—

Alexander Ramancha

Users should own their systems.  
Systems should not own their users.

Build commands for *this* workspace live in [BUILD.md](BUILD.md), so the GitHub page can stay a statement instead of a makefile.
