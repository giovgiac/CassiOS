# CassiOS

A hobby operating system targeting i386 (32-bit x86), written in C++ and assembly. Boots via GRUB using the Multiboot specification.

## Prerequisites

- `g++`, `as`, `ld` (GNU toolchain with 32-bit support)
- `qemu-system-i386` (for running)
- `grub-mkrescue` (optional, for building the ISO)

## Build

```sh
make kernel    # compile and link the kernel binary
make run       # build and launch in QEMU
make iso       # build a bootable ISO
make clean     # remove build artifacts
```
