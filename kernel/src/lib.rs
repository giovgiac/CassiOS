// lib.rs -- CassiOS kernel, Rust part
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// The whole kernel apart from three assembly files: the multiboot loader
// (kernel/src/core/loader.s), which calls start() in core/kernel.rs, and
// the interrupt and syscall entry stubs. It is a staticlib linked with
// them by ld. Hardware access is only compiled for the i586 target, so
// pure logic runs as host tests. Built with the `ktest` feature it is the
// test kernel, which runs the kernel tests (the `ktests` modules).

#![cfg_attr(not(test), no_std)]

extern crate alloc;

#[cfg(any(test, feature = "ktest"))]
#[macro_use]
mod ktest;
#[cfg(target_arch = "x86")]
mod sync;

// kernel/src/core; `core` itself names Rust's core library.
#[path = "core"]
mod system {
    pub mod elf;
    pub mod gdt;
    #[cfg(all(target_arch = "x86", not(test)))]
    pub mod kernel;
    pub mod process;
    #[cfg(target_arch = "x86")]
    pub mod scheduler;
    pub mod syscall;
}

mod hardware {
    #[cfg(target_arch = "x86")]
    pub mod exception;
    pub mod interrupt;
    pub mod irq;
    pub mod pit;
    #[cfg(target_arch = "x86")]
    pub mod serial;
}

mod memory {
    #[cfg(target_arch = "x86")]
    pub mod heap;
    pub mod multiboot;
    pub mod paging;
    pub mod physical;

    /// Where the direct map of physical memory starts.
    pub const KERNEL_VBASE: u32 = 0xC000_0000;
    /// Physical memory the direct map reaches: the rest of the address
    /// space after KERNEL_VBASE.
    pub const DIRECT_MAP_END: u64 = 0x1_0000_0000 - KERNEL_VBASE as u64;

    /// The direct-map address of physical address `phys`.
    #[cfg(target_arch = "x86")]
    pub fn phys_to_virt(phys: u32) -> *mut u8 {
        (phys + KERNEL_VBASE) as *mut u8
    }
}

/// Every group of kernel tests, in the order the test kernel runs them.
#[cfg(feature = "ktest")]
const KTESTS: [&[ktest::Test]; 13] = [
    libmem::TESTS,
    hardware::serial::ktests::TESTS,
    system::gdt::ktests::TESTS,
    hardware::interrupt::ktests::TESTS,
    hardware::irq::ktests::TESTS,
    hardware::pit::ktests::TESTS,
    memory::physical::ktests::TESTS,
    memory::heap::ktests::TESTS,
    memory::paging::ktests::TESTS,
    system::elf::ktests::TESTS,
    system::process::ktests::TESTS,
    system::scheduler::ktests::TESTS,
    system::syscall::ktests::TESTS,
];

/// Tests of libstd_mem (libs/mem), which supplies memcpy, memmove, memset
/// and memcmp to the kernel and every program until it is replaced.
#[cfg(feature = "ktest")]
mod libmem {
    use core::ffi::c_void;
    use core::hint::black_box;

    use crate::ktest::Outcome;

    unsafe extern "C" {
        fn memcpy(dst: *mut c_void, src: *const c_void, n: usize) -> *mut c_void;
        fn memmove(dst: *mut c_void, src: *const c_void, n: usize) -> *mut c_void;
        fn memset(dst: *mut c_void, value: i32, n: usize) -> *mut c_void;
        fn memcmp(a: *const c_void, b: *const c_void, n: usize) -> i32;
    }

    tests!(
        mem_copy_copies_and_returns_dst,
        mem_move_handles_both_overlaps,
        mem_set_fills_and_returns_dst,
        mem_compare_orders_bytes,
    );

    /// A word-aligned buffer, so offsets choose which loops run: libs/mem
    /// copies words only when both pointers have the same alignment.
    #[repr(align(4))]
    #[derive(Clone, Copy)]
    struct Buffer([u8; 64]);

    impl Buffer {
        /// 0, 1, 2, ...
        fn counting() -> Self {
            Buffer(core::array::from_fn(|i| i as u8))
        }
    }

    /// Byte `offset` from `base`. Like every length below, it is hidden
    /// from the compiler (black_box), which would otherwise work out or
    /// inline a call instead of reaching libstd_mem. Every pointer comes
    /// from one `base`, so none invalidates another.
    fn at(base: *mut u8, offset: usize) -> *mut c_void {
        black_box(base.wrapping_add(offset).cast())
    }

    fn mem_copy_copies_and_returns_dst() -> Outcome {
        // (dst, src): same alignment (byte, word and byte loops), then
        // different alignment (byte loop only).
        for (dst, src) in [(1, 33), (1, 34)] {
            let mut buf = Buffer::counting();
            let mut expected = buf;
            expected.0.copy_within(src..src + 29, dst);
            let base = buf.0.as_mut_ptr();
            let out = unsafe { memcpy(at(base, dst), at(base, src), black_box(29)) };
            check_eq!(out, at(base, dst));
            check!(buf.0 == expected.0);
        }
        Ok(())
    }

    fn mem_move_handles_both_overlaps() -> Outcome {
        // (dst, src): forward and backward, each with the same alignment
        // (alignment, word and byte loops) and a different one (byte loop).
        // 26 bytes leave neither end of the range on a word boundary.
        for (dst, src) in [(1, 9), (2, 9), (9, 1), (9, 2)] {
            let mut buf = Buffer::counting();
            let mut expected = buf;
            expected.0.copy_within(src..src + 26, dst);
            let base = buf.0.as_mut_ptr();
            let out = unsafe { memmove(at(base, dst), at(base, src), black_box(26)) };
            check_eq!(out, at(base, dst));
            check!(buf.0 == expected.0);
        }
        Ok(())
    }

    fn mem_set_fills_and_returns_dst() -> Outcome {
        // 30 bytes from offset 1: alignment, word and trailing byte loops.
        let mut buf = Buffer([0; 64]);
        let base = buf.0.as_mut_ptr();
        let out = unsafe { memset(at(base, 1), black_box(0x1AB), black_box(30)) };
        check_eq!(out, at(base, 1));
        // Only the low byte of the value is stored.
        let mut expected = [0u8; 64];
        expected[1..31].fill(0xAB);
        check!(buf.0 == expected);
        Ok(())
    }

    fn mem_compare_orders_bytes() -> Outcome {
        let cmp = |a: &[u8], b: &[u8], n| unsafe {
            memcmp(
                black_box(a.as_ptr().cast()),
                black_box(b.as_ptr().cast()),
                black_box(n),
            )
        };
        check_eq!(cmp(b"abc", b"abc", 3), 0);
        check!(cmp(b"abc", b"abd", 3) < 0);
        check!(cmp(b"abd", b"abc", 3) > 0);
        // Bytes compare unsigned.
        check!(cmp(&[0x80], &[0x01], 1) > 0);
        check_eq!(cmp(b"a", b"b", 0), 0);
        Ok(())
    }
}

#[cfg(all(target_arch = "x86", not(test)))]
mod rt {
    use core::arch::asm;
    use core::ffi::c_void;
    use core::fmt::Write;
    use core::panic::PanicInfo;

    use sys::io::{Port, PortType};

    use crate::hardware::serial::COM1;

    // The prebuilt alloc crate is compiled with unwinding and references
    // the personality routine and, from its formatting code, the unwinder.
    // With panic = "abort" neither is ever called.
    #[unsafe(no_mangle)]
    extern "C" fn rust_eh_personality() {}

    #[unsafe(no_mangle)]
    extern "C" fn _Unwind_Resume() -> ! {
        panic!("unwinding is not supported")
    }

    // libstd_mem provides memcmp but not bcmp, which LLVM emits for
    // slice equality.
    unsafe extern "C" {
        fn memcmp(a: *const c_void, b: *const c_void, n: usize) -> i32;
    }

    #[unsafe(no_mangle)]
    unsafe extern "C" fn bcmp(a: *const c_void, b: *const c_void, n: usize) -> i32 {
        unsafe { memcmp(a, b, n) }
    }

    #[panic_handler]
    fn panic(info: &PanicInfo) -> ! {
        let _ = writeln!(&COM1, "KERNEL PANIC: {info}");
        // Fails a test run; without QEMU's debug-exit device the write
        // does nothing.
        unsafe { Port::<u8>::new(PortType::QemuDebugExit).write(0x01) };
        loop {
            unsafe { asm!("cli", "hlt") };
        }
    }
}
