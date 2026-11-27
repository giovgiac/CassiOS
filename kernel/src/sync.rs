// sync.rs -- kernel-global state
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.

use core::cell::UnsafeCell;

/// State shared across the kernel, such as the frame allocator.
///
/// The kernel runs on one CPU, and kernel code runs with interrupts
/// disabled: syscalls enter through an interrupt gate, and only sleep and
/// the serial write enable interrupts, without touching any `Global`
/// meanwhile. So at most one `with` call is active at a time, as long as
/// `f` does not reach the same `Global` again.
pub struct Global<T>(UnsafeCell<T>);

// SAFETY: see the type's documentation.
unsafe impl<T> Sync for Global<T> {}

impl<T> Global<T> {
    pub const fn new(value: T) -> Self {
        Self(UnsafeCell::new(value))
    }

    /// Run `f` with exclusive access to the value.
    pub fn with<R>(&self, f: impl FnOnce(&mut T) -> R) -> R {
        // SAFETY: see the type's documentation.
        f(unsafe { &mut *self.0.get() })
    }
}
