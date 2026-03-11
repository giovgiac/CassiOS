# CassiOS

A hobby operating system targeting i386 (32-bit x86), written in C++ and assembly. Boots via GRUB using the Multiboot specification.

## Technical Details

- Toolchain: `g++` (with `-m32`), `as` (with `--32`), `ld` (with `-melf_i386`)
- In-kernel test framework: `make test` builds a separate test kernel and runs it in QEMU (see `tests/` and `docs/plans/2026-03-07-in-kernel-testing-design.md`)
- Custom fixed-width types defined in `include/common/types.hpp`: `u8`, `u16`, `u32`, `u64`, `i8`, `i16`, `i32`, `i64`, `f32`, `f64`, `usize`, `isize`
- Headers use `#ifndef` include guards (not `#pragma once`)
- Assembly uses AT&T syntax
- Copy/move constructors and assignment operators are explicitly deleted on hardware-bound and singleton classes
- When issues or bugs are encountered, start by reproducing the issue yourself, determine the root cause and only then attempt to fix. Once applied, verify and prove that the issue is no longer reproducible through testing
- Testing is done with QEMU: `qemu-system-i386 -machine pc -kernel bin/cassio.bin -net none`
- For headless/automated testing, use the QEMU monitor with `screendump` to capture VGA screenshots (see `docs/DEBUGGING.md` for the full test script and techniques)

## Architecture

- **Boot flow**: GRUB loads the kernel via Multiboot (`src/core/loader.s`), which sets up a 2MiB stack, calls `ctors()` for global constructors, then calls `start()` in `kernel.cpp`. `start()` initializes GDT, InterruptManager, DriverManager, loads drivers, and enters the main loop.
- **Interrupt dispatch**: assembly stubs in `src/hardware/stub.s` use mangled C++ names to bridge IRQs to `InterruptManager::handleInterrupt()`. Adding a new IRQ handler requires: a `HandleInterruptRequest` macro call in `stub.s`, a static method declaration in `InterruptManager`, and a `setInterrupt()` call in `InterruptManager::load()`.
- **Singletons** for `InterruptManager`, `DriverManager`, and `COM1` (private constructors, static `instance`, accessed via `getManager()`/`getSerial()`)
- **Drivers** inherit from `hardware::Driver`, self-register with the `InterruptManager` in their constructor, and use an event handler pattern to decouple input handling from the driver itself.
- **Serial**: `Serial` is a general class taking `PortType` values; `COM1` is a singleton wrapping it for COM1 ports. Used by the test framework for output.

## Pull Request Policy

Never merge a pull request without explicit user approval. After opening a PR, stop and wait for the user to review and test it. Only merge when the user gives the go-ahead.

## Coding Standards

1. Keep it simple - NEVER over-engineer, ALWAYS simplify, NO unnecessary defensive programming. No extra features - focus on simplicity
2. Be concise. Keep README minimal.
3. NEVER use emojis.
4. Use proper C++ classes, not C-style free function APIs. Follow existing patterns (e.g., `KeyboardDriver`, `InterruptManager`).
5. Hardware-bound classes use singletons (private constructor, static instance, public getter). General-purpose classes (like `Serial`) take configuration via constructor and are wrapped by singletons for specific instances (like `COM1`).
6. I/O port addresses go in the `PortType` enum in `port.hpp` -- no magic numbers. Do not add raw `u16` constructors to `Port`.
7. Functions called from assembly (`ctors`, `start`) must be declared `extern "C"`.
8. Tests are part of the implementation, not separate work items. Everything testable must be tested within the same issue. Only skip tests when something genuinely can't be tested or the effort is disproportionate -- but make every effort not to skip. Never create separate issues for testing.

## Working Documentation

All documents for planning, working with and executing this project are in the docs/ directory. 
Check @docs/WORKFLOW.md for instructions on how to use the project tooling, such as GitHub Issues and Git. 
ALWAYS CHECK docs/DEBUGGING.md for the methodology to follow when fixing bugs and issues.
