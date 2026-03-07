# CassiOS

A hobby operating system targeting i386 (32-bit x86), written in C++ and assembly. Boots via GRUB using the Multiboot specification.

## Technical Details

- Toolchain: `g++` (with `-m32`), `as` (with `--32`), `ld` (with `-melf_i386`)
- No tests or linting tools are configured
- Custom fixed-width types defined in `include/common/types.hpp`: `u8`, `u16`, `u32`, `u64`, `i8`, `i16`, `i32`, `i64`, `f32`, `f64`, `usize`, `isize`
- Headers use `#ifndef` include guards (not `#pragma once`)
- Assembly uses AT&T syntax
- Copy/move constructors and assignment operators are explicitly deleted on hardware-bound and singleton classes
- When issues or bugs are encountered, start by reproducing the issue yourself, determine the root cause and only then attempt to fix. Once applied, verify and prove that the issue is no longer reproducible through testing

## Architecture

- **Boot flow**: GRUB loads the kernel via Multiboot (`src/core/loader.s`), which sets up a 2MiB stack, calls `ctors()` for global constructors, then calls `start()` in `kernel.cpp`. `start()` initializes GDT, InterruptManager, DriverManager, loads drivers, and enters the main loop.
- **Interrupt dispatch**: assembly stubs in `src/hardware/stub.s` use mangled C++ names to bridge IRQs to `InterruptManager::handleInterrupt()`. Adding a new IRQ handler requires: a `HandleInterruptRequest` macro call in `stub.s`, a static method declaration in `InterruptManager`, and a `setInterrupt()` call in `InterruptManager::load()`.
- **Singletons** for `InterruptManager` and `DriverManager` (private constructors, static `instance`, accessed via `getManager()`)
- **Drivers** inherit from `hardware::Driver`, self-register with the `InterruptManager` in their constructor, and use an event handler pattern to decouple input handling from the driver itself.

## Pull Request Policy

Never merge a pull request without explicit user approval. After opening a PR, stop and wait for the user to review and test it. Only merge when the user gives the go-ahead.

## Coding Standards

1. Keep it simple - NEVER over-engineer, ALWAYS simplify, NO unnecessary defensive programming. No extra features - focus on simplicity
2. Be concise. Keep README minimal.
3. NEVER use emojis.

## Working Documentation

All documents for planning, working with and executing this project are in the docs/ directory. Check docs/WORKFLOW.md for instructions on how to use the project tooling, such as GitHub Issues and Git. Check docs/DEBUGGING.md for the methodology to follow when fixing bugs and issues.
