# CassiOS

A hobby operating system targeting i386 (32-bit x86), written in C++ and assembly. Boots via GRUB using the Multiboot specification. Built from scratch with no standard library -- every component from the GDT to the filesystem is hand-written.

## Features

- **Higher-half kernel** mapped at 0xC0000000+ with GDT, IDT, and interrupt-driven I/O
- **Userspace execution** -- ring 3 init process loaded from an ELF binary via GRUB multiboot module
- **Preemptive round-robin scheduler** driven by PIT timer (~100 Hz), context switching via saved ESP
- **Per-process address spaces** with separate page directories and shared kernel mappings
- **ELF32 loader** for statically linked executables
- **Syscall interface** via `int 0x80` (write, read, sleep, uptime)
- **PS/2 keyboard driver** with full scancode translation, arrow keys, and extended key support
- **VGA text-mode terminal** with cursor control and color attributes
- **Physical memory manager** with bitmap allocator
- **Kernel heap** with `new`/`delete` support
- **Paging** with direct-mapped kernel memory and per-process user pages
- **ATA PIO block device driver** for IDE disk access
- **In-memory hierarchical filesystem** supporting files, directories, and path resolution
- **Interactive shell** with line editing and 22 built-in commands
- **In-kernel test framework** with 190 automated tests, run headlessly via serial output and QEMU exit codes

## Prerequisites

- `g++`, `as`, `ld` (GNU toolchain with 32-bit support)
- `qemu-system-i386` (for running)
- `grub-mkrescue` (optional, for building the ISO)

## Build

```sh
make kernel    # compile and link the kernel binary
make run       # build and launch in QEMU
make test      # build and run in-kernel tests
make iso       # build a bootable ISO
make clean     # remove build artifacts
```
