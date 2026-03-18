# CassiOS

A microkernel operating system targeting i386 (32-bit x86), written in C++ and assembly. Boots via GRUB using the Multiboot specification. Built from scratch with no standard library -- every component from the GDT to the IPC layer is hand-written.

## Features

- **Microkernel architecture** -- kernel provides IPC, scheduling, memory management, and interrupt routing; everything else runs as userspace services
- **Synchronous IPC** -- message passing (send/receive/reply) and fire-and-forget notify between processes
- **Userspace drivers** -- PS/2 keyboard, PS/2 mouse, VGA terminal, ATA PIO disk, all as separate services
- **Userspace services** -- nameserver for service discovery, FAT32 filesystem with long filename support, interactive shell with 16 commands
- **Higher-half kernel** mapped at 0xC0000000+ with per-process address spaces
- **Preemptive round-robin scheduler** driven by PIT timer (~1000 Hz)
- **ELF32 loader** for statically linked userspace binaries via GRUB multiboot modules
- **Syscall interface** via `int 0x80` (IPC, memory, process control, system info)
- **Physical memory manager**, kernel heap with `new`/`delete`, paging
- **Persistent FAT32 filesystem** -- 32 MiB disk image, on-demand FAT via LRU sector cache, directories, LFN, case-sensitive
- **Two-tier test framework** -- 118 kernel unit tests + 88 userspace integration tests, run headlessly via QEMU

## Prerequisites

- `g++`, `as`, `ld` (GNU toolchain with 32-bit support)
- `qemu-system-i386` (for running)
- `dosfstools` and `mtools` (for FAT32 disk image creation)
- `grub-mkrescue` (optional, for building the ISO)

## Build

```sh
make kernel    # compile and link the kernel binary
make run       # build and launch in QEMU with all services
make test      # build and run all tests (kernel + userspace)
make iso       # build a bootable ISO
make clean     # remove build artifacts
```
