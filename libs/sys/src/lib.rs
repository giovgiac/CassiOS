// lib.rs -- definitions shared by the kernel and userspace
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// The IPC message layout, message types, syscall numbers and the
// structures syscalls return. Plain data only: no allocator, panic handler
// or syscalls, so both the kernel and userspace can depend on it. Until the
// kernel is ported, libs/ipc and libs/os hold C++ copies; keep them in step.

#![no_std]

pub mod ipc;
pub mod os;
