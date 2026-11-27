# CassiOS

A microkernel operating system targeting i386 (32-bit x86), written in Rust and assembly. Boots via GRUB using the Multiboot specification.

## Technical Details

- Toolchain: Rust (see below), `as` (with `--32`), `ld` (with `-melf_i386`)
- Three-tier test framework: `make test` runs Rust host unit tests, then kernel unit tests and userspace integration tests in QEMU
- Assembly uses AT&T syntax
- When issues or bugs are encountered, start by reproducing the issue yourself, determine the root cause and only then attempt to fix. Once applied, verify and prove that the issue is no longer reproducible through testing
- Manual testing: `make run` boots with all services in QEMU
- For headless/automated testing, use the QEMU monitor with `screendump` to capture screenshots (see `docs/DEBUGGING.md` for the full test script and techniques)

## Architecture

CassiOS is a microkernel. The kernel contains only: GDT/TSS, interrupt subsystem (IDT, exceptions, IRQ dispatch), memory management (physical, heap, paging), process management, scheduler, PIT timer, IPC, ELF loader, and serial. Everything else runs as userspace services.

- **Boot flow**: GRUB loads the kernel via Multiboot (`kernel/src/core/loader.s`), which maps the kernel into the higher half, sets up a 2MiB stack and calls `start()` in `kernel/src/core/kernel.rs`. `start()` initializes COM1, the GDT, interrupts, memory, the PIT and the kernel task, loads the userspace ELF modules, and becomes the idle loop (`hlt`).
- **Interrupt dispatch**: assembly stubs in `kernel/src/hardware/stub.s` bridge exceptions and IRQs to `handleException` and `handleInterrupt` in Rust (`kernel/src/hardware/`). IRQ 0 drives the PIT; other IRQs are forwarded to registered userspace processes via IPC.
- **IPC**: synchronous message passing (send/receive/reply) plus fire-and-forget notify. Messages are 24 bytes (type + 5 args) with optional bulk data buffers for large transfers. The kernel copies messages between address spaces. `receive()` priority: IRQ notifications > notify queue > send queue.
- **Userspace services**: each is a separate ELF binary loaded as a GRUB multiboot module. Services register with the nameserver and communicate via IPC. Drivers live under `userspace/drivers/`, core services under `userspace/core/` (ns, shell, vfs), and user programs under `userspace/apps/`. Service clients in `libs/cassio` (e.g. `cassio::vfs::Vfs`) look up the service's PID in the nameserver when they connect.
- **User program execution**: the shell `exec` command reads an ELF from the FAT32 filesystem, passes it to the `Exec` syscall which creates a new process, then blocks via `WaitPid` until the child exits. Programs under `userspace/apps/` are compiled to `disk/bin/` and included in the FAT32 image.
- **FAT32 filesystem**: the VFS service parses FAT32 on-disk structures, reads FAT entries on demand via an LRU sector cache, supports long filenames (LFN), case-sensitive matching. The ATA driver provides full 512-byte sector transfers via IPC data buffers. The build system creates a 32 MiB FAT32 disk image with pre-seeded files using `mkfs.fat` and `mtools`.
- **Kernel state**: each subsystem keeps its state in a `static` (`sync::Global`, e.g. the frame bitmap, the process table), reached by its functions.
- **Serial**: COM1 (`kernel/src/hardware/serial.rs`) carries the kernel log, the serial write syscall and test output.

## Rust

CassiOS was ported from C++ to Rust phase by phase (issue #188, integration branch `refactor/188-rust-migration`).

- Toolchain pinned in `rust-toolchain.toml`; components target `i586-unknown-linux-gnu` as `#![no_std]` with `panic = "abort"` (no SSE). No external crates.
- Each binary is a `staticlib` linked with the existing `ld` command and linker script (which must collect `.rodata*`, `.data*`, `.bss*`). Its Makefile runs `cargo build` and writes the same output path as before.
- Shared userspace library: `libs/cassio`, crate `cassio` (`os`, `ipc`, `ns`, `ps2`, `gfx`, the `ata`, `display`, `kbd`, `terminal` and `vfs` clients, global allocator, panic handler). Syscall wrappers are only compiled for `target_arch = "x86"` so pure logic runs as host tests.
- Memory functions: `libs/mem`, crate `mem` (`memcpy`, `memmove`, `memset`, `memcmp`, `bcmp`), built with `no_builtins` so its loops are not compiled back into calls to themselves. `kernel` and `cassio` link it with `extern crate mem as _`; the C symbols are only exported in the i586 build.
- Kernel/userspace shared definitions: `libs/sys`, crate `sys` (IPC `Message` and message types, syscall numbers, `ProcEntry`, `FramebufferInfo`, port I/O `Port`/`PortType`, the free-list `Heap` allocator). No global allocator, panic handler or syscalls; `cassio` re-exports them. Their tests pin every syscall number, message type and port.
- Kernel: crate `kernel` (`kernel/Cargo.toml`, sources under `kernel/src`: `core` as module `system` since `core` is Rust's core library, `hardware`, `memory`), a staticlib linked with the three assembly files by `ld --gc-sections`. It depends only on `mem` and `sys`. Kernel-global state goes in `sync::Global`, which relies on kernel code running with interrupts disabled (only sleep and the serial write enable them, outside any borrow); allocations sized by user input must be fallible. Frames come only from RAM between 1 MiB and the end of the direct map. The test kernel is the same crate built with the `ktest` feature: its `start()` runs the in-QEMU kernel tests, which live in `ktests` modules next to the code, listed with `tests!` and added to `KTESTS` in `kernel/src/lib.rs`.
- `make test-rust` (part of `make test`) runs `cargo test` on the host. Put unit tests in `#[cfg(test)]` modules next to the code. Black-box tests of a service go in the in-QEMU runner `userspace/tests` (crate `usertest`, run by `make test-userspace`): one module per service, each listing its tests with `tests!`.
- Format with `cargo fmt`. Copyright header as `//` comments. `main.rs` holds only the entry point and receive loop.
- Hardware in a Rust service is owned by that service alone: `_start` constructs its device type (e.g. the mouse `Device`) once and keeps it local. In the kernel, a device's state is a `static` in its module (e.g. `COM1`).

## Pull Request Policy

Never merge a pull request without explicit user approval. After opening a PR, stop and wait for the user to review and test it. Only merge when the user gives the go-ahead.

## Coding Standards

1. Keep it simple - NEVER over-engineer, ALWAYS simplify, NO unnecessary defensive programming. No extra features - focus on simplicity
2. Be concise. Keep README minimal.
3. NEVER use emojis.
4. Follow existing patterns: types with methods, one module per concern.
5. General-purpose types take their configuration when built (e.g. `Serial::new` with its ports); a specific device is a `static` of one (e.g. `COM1`).
6. I/O port addresses go in the `PortType` enum in `libs/sys/src/io.rs` -- no magic numbers. Do not add raw `u16` constructors to `Port`.
7. Functions called from assembly (`start`, `handleSyscall`, `handleInterrupt`, `handleException`) are `#[unsafe(no_mangle)] extern "C"`.
8. Tests are part of the implementation, not separate work items. Everything testable must be tested within the same issue. Only skip tests when something genuinely can't be tested or the effort is disproportionate -- but make every effort not to skip. Never create separate issues for testing.
9. Userspace services must have proper structure: logic split into modules under `src/`, with `main.rs` holding only the entry point and receive loop. Unit tests sit next to the code; black-box tests go in `userspace/tests`. For complex services like the shell, split further (e.g., `src/commands/`).
10. Use the kernel heap (`Box`/`Vec`) for dynamic data structures rather than defaulting to fixed-size arrays. Fixed arrays are appropriate only when the size is hardware-dictated (IDT, IRQ tables, page directories).

## Working Documentation

All documents for planning, working with and executing this project are in the docs/ directory. `docs/plans` and `docs/reviews` are dated records written before the Rust port, so they describe the C++ code.
Check @docs/WORKFLOW.md for instructions on how to use the project tooling, such as GitHub Issues and Git.
ALWAYS CHECK docs/DEBUGGING.md for the methodology to follow when fixing bugs and issues.
