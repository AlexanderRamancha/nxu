# NXU Kernel

**NXU: Not X Unix**

A clean-slate, from-scratch ARM64 kernel built as the foundation for a security-first mobile operating system.

### Vision
NXU is designed from first principles to create a fundamentally more secure mobile platform.  
No Linux heritage. No ambient authority. No persistent privileges.  

Permissions are strictly ephemeral — valid only for the lifetime of a task or session — and enforced directly by the kernel.

### Core Principles
- Built entirely from scratch with security as the central invariant
- Native ARM64 target for modern mobile hardware
- Minimal and auditable codebase
- Open-core development model

### Current Status (March 2026)
- Bare-metal AArch64 kernel successfully boots on QEMU `-M virt`
- Reliable PL011 UART console output from C code
- Proper assembly entry point with stack setup and .bss zeroing
- Clean, reproducible build system

We have reached the first major milestone: a working kernel that executes C code and produces visible output.

### Quick Start

# Clone the repository
git clone https://github.com/AlexanderRamancha/nxu.git
cd nxu

# Build and run in QEMU
make clean
make qemu

**Expected output:**

NXU kernel initialized

**Requirements:**

aarch64-linux-gnu-gcc and binutils-aarch64-linux-gnu
QEMU (qemu-system-aarch64)

**Why Contribute to NXU?**
Modern mobile security still rests on decades-old kernel foundations.
NXU offers a chance to build something cleaner, more auditable, and deliberately secure from the ground up.
If you are interested in low-level systems programming, ARM64 architecture, or building a more secure foundation for mobile devices, you are welcome here.

**How to Contribute**
See CONTRIBUTING.md for details on:

Development workflow
Coding style
Submitting patches

We especially welcome students and developers who want hands-on experience with bare-metal ARM64 kernel development.
Documentation

Architecture Overview
Boot Process

**License**
Licensed under the MIT License (open-core model).
See LICENSE for full details.

Security should not be added as a layer.
It should be the foundation.
— The NXU Project
We are building the kernel that mobile devices deserve.

**Join us.**
