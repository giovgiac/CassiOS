# CassiOS

A hobby operating system targeting i386 (32-bit x86), written in C++ and assembly. Boots via GRUB using the Multiboot specification.

## Prerequisites

- `g++`, `as`, `ld` (GNU toolchain with 32-bit support)
- `grub-mkrescue` (for building the ISO)
- [VirtualBox](https://www.virtualbox.org/) (for running, expects a VM named "CassiOS")

## Build

```sh
make cassio.bin      # compile and link the kernel binary
make cassio.iso      # build a bootable ISO
make run             # launch in VirtualBox
make clean           # remove build artifacts
```
