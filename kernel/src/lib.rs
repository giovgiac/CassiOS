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

    fn mem_copy_copies_and_returns_dst() -> Outcome {
        // Odd lengths and offsets exercise the word and byte loops.
        let src: [u8; 37] = core::array::from_fn(|i| i as u8 * 3);
        let mut dst = [0u8; 40];
        let out = unsafe { memcpy(dst[1..].as_mut_ptr().cast(), src.as_ptr().cast(), 37) };
        check_eq!(out, dst[1..].as_mut_ptr().cast());
        check!(dst[1..38] == src);
        check_eq!((dst[0], dst[38]), (0, 0));
        Ok(())
    }

    fn mem_move_handles_both_overlaps() -> Outcome {
        let mut forward = [1u8, 2, 3, 4, 5];
        unsafe { memmove(forward.as_mut_ptr().cast(), forward[2..].as_ptr().cast(), 3) };
        check_eq!(forward, [3, 4, 5, 4, 5]);
        let mut backward = [1u8, 2, 3, 4, 5];
        let out = unsafe {
            memmove(
                backward[2..].as_mut_ptr().cast(),
                backward.as_ptr().cast(),
                3,
            )
        };
        check_eq!(backward, [1, 2, 1, 2, 3]);
        check_eq!(out, backward[2..].as_mut_ptr().cast());
        Ok(())
    }

    fn mem_set_fills_and_returns_dst() -> Outcome {
        let mut buf = [0u8; 33];
        let out = unsafe { memset(buf[1..].as_mut_ptr().cast(), 0x1AB, 31) };
        check_eq!(out, buf[1..].as_mut_ptr().cast());
        // Only the low byte of the value is stored.
        check!(buf[1..32].iter().all(|&b| b == 0xAB));
        check_eq!((buf[0], buf[32]), (0, 0));
        Ok(())
    }

    fn mem_compare_orders_bytes() -> Outcome {
        let cmp =
            |a: &[u8], b: &[u8], n| unsafe { memcmp(a.as_ptr().cast(), b.as_ptr().cast(), n) };
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
