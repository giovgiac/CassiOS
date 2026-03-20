# CassiOS

A microkernel operating system targeting i386 (32-bit x86), written in C++ and assembly. Boots via GRUB using the Multiboot specification.

## Technical Details

- Toolchain: `g++` (with `-m32`), `as` (with `--32`), `ld` (with `-melf_i386`)
- Two-tier test framework: `make test` runs kernel unit tests then userspace integration tests in QEMU
- Modular standard library under `libs/` with `std::` namespace -- modules from types to service clients, plus `StringView`, `Box<T>`, `Rc<T>` (see `docs/plans/2026-03-19-std-library-design.md`)
- Custom fixed-width types defined in `libs/types/include/std/types.hpp`: `u8`, `u16`, `u32`, `u64`, `i8`, `i16`, `i32`, `i64`, `f32`, `f64`, `usize`, `isize`
- Code formatted with `.clang-format` -- run clang-format or use editor format-on-save
- Headers use `#ifndef` include guards (not `#pragma once`)
- Assembly uses AT&T syntax
- Copy/move constructors and assignment operators are explicitly deleted on hardware-bound and singleton classes
- When issues or bugs are encountered, start by reproducing the issue yourself, determine the root cause and only then attempt to fix. Once applied, verify and prove that the issue is no longer reproducible through testing
- Manual testing: `make run` boots with all services in QEMU
- For headless/automated testing, use the QEMU monitor with `screendump` to capture VGA screenshots (see `docs/DEBUGGING.md` for the full test script and techniques)

## Architecture

CassiOS is a microkernel. The kernel contains only: GDT/TSS, interrupt subsystem (IDT, exceptions, IRQ dispatch), memory management (physical, heap, paging), process management, scheduler, PIT timer, IPC, ELF loader, and serial. Everything else runs as userspace services.

- **Boot flow**: GRUB loads the kernel via Multiboot (`kernel/src/core/loader.s`), which sets up a 2MiB stack, calls `ctors()` for global constructors, then calls `start()` in `kernel.cpp`. `start()` initializes GDT, interrupts, memory, PIT, scheduler, loads userspace ELF modules, and enters an idle loop (`hlt`).
- **Interrupt dispatch**: assembly stubs in `kernel/src/hardware/stub.s` bridge IRQs to `IrqManager::handleIrq()`. IRQs are either handled by in-kernel handlers (PIT) or forwarded to registered userspace processes via IPC.
- **IPC**: synchronous message passing (send/receive/reply) plus fire-and-forget notify. Messages are 24 bytes (type + 5 args) with optional bulk data buffers for large transfers. The kernel copies messages between address spaces. `receive()` priority: IRQ notifications > notify queue > send queue.
- **Userspace services**: each is a separate ELF binary loaded as a GRUB multiboot module. Services register with the nameserver and communicate via IPC. Drivers live under `userspace/drivers/`, core services under `userspace/core/` (ns, shell, vfs), and user programs under `userspace/apps/`. Service client libraries under `libs/` (e.g., `std::vga::Vga`, `std::vfs::Vfs`) auto-resolve PIDs from the nameserver on construction.
- **User program execution**: the shell `exec` command reads an ELF from the FAT32 filesystem, passes it to the `Exec` syscall which creates a new process, then blocks via `WaitPid` until the child exits. Programs under `userspace/apps/` are compiled to `disk/bin/` and included in the FAT32 image.
- **FAT32 filesystem**: the VFS service parses FAT32 on-disk structures, reads FAT entries on demand via an LRU sector cache, supports long filenames (LFN), case-sensitive matching. The ATA driver provides full 512-byte sector transfers via IPC data buffers. The build system creates a 32 MiB FAT32 disk image with pre-seeded files using `mkfs.fat` and `mtools`.
- **Singletons** for kernel managers: `InterruptManager`, `IrqManager`, `PitTimer`, `COM1`, etc. (private constructors, static `instance`, accessed via getter)
- **Serial**: `Serial` is a general class taking `PortType` values; `COM1` is a singleton wrapping it for COM1 ports. Used by the test framework for output.

## Pull Request Policy

Never merge a pull request without explicit user approval. After opening a PR, stop and wait for the user to review and test it. Only merge when the user gives the go-ahead.

## Coding Standards

1. Keep it simple - NEVER over-engineer, ALWAYS simplify, NO unnecessary defensive programming. No extra features - focus on simplicity
2. Be concise. Keep README minimal.
3. NEVER use emojis.
4. Use proper C++ classes, not C-style free function APIs. Follow existing patterns.
5. Hardware-bound classes use singletons (private constructor, static instance, public getter). General-purpose classes take configuration via constructor and are wrapped by singletons for specific instances (e.g., Serial + COM1).
6. I/O port addresses go in the `PortType` enum in `std/io.hpp` -- no magic numbers. Do not add raw `u16` constructors to `Port`.
7. Functions called from assembly (`ctors`, `start`) must be declared `extern "C"`.
8. Tests are part of the implementation, not separate work items. Everything testable must be tested within the same issue. Only skip tests when something genuinely can't be tested or the effort is disproportionate -- but make every effort not to skip. Never create separate issues for testing.
9. Userspace services must have proper structure: `include/`, `src/`, `tests/`. Split logic into classes -- `main.cpp` should only contain the entry point and receive loop. For complex services like the shell, split further (e.g., `src/commands/`).
10. Use the kernel heap (`KernelHeap`, `operator new`) for dynamic data structures rather than defaulting to fixed-size arrays. Fixed arrays are appropriate only when the size is hardware-dictated (IDT, IRQ tables, page directories).

## Working Documentation

All documents for planning, working with and executing this project are in the docs/ directory.
Check @docs/WORKFLOW.md for instructions on how to use the project tooling, such as GitHub Issues and Git.
ALWAYS CHECK docs/DEBUGGING.md for the methodology to follow when fixing bugs and issues.
