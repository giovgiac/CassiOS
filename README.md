# CassiOS

A hobby operating system targeting i386 (32-bit x86), written in C++ and assembly. Boots via GRUB using the Multiboot specification. Built from scratch with no standard library -- every component from the GDT to the filesystem is hand-written.

## Features

- **Protected mode kernel** with GDT, IDT, and interrupt-driven I/O
- **PS/2 keyboard driver** with full scancode translation, arrow keys, and extended key support
- **VGA text-mode terminal** with cursor control and color attributes
- **Physical memory manager** with bitmap allocator
- **Kernel heap** with `new`/`delete` support
- **Paging** with identity-mapped virtual memory
- **In-memory hierarchical filesystem** supporting files, directories, and path resolution
- **Interactive shell** with line editing (insert, delete, arrow keys) and a growing set of built-in commands:
  - `help`, `clear`, `mem`, `reboot`, `shutdown`
  - `ls`, `cd`, `pwd`, `mkdir`, `rmdir`, `touch`, `rm`
- **In-kernel test framework** with 120+ automated tests, run headlessly via serial output and QEMU exit codes

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
