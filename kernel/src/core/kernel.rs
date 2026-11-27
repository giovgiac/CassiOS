// kernel.rs -- kernel entry point
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// loader.s maps the kernel into the higher half and calls start() with
// the multiboot information. start() brings up the hardware and memory,
// loads every multiboot module as a user process, and becomes the idle
// loop (the kernel task). The test kernel, built with the `ktest`
// feature, runs the kernel tests instead and exits QEMU with the result.

use core::arch::asm;

use crate::hardware::serial::COM1;
use crate::hardware::{interrupt, pit};
use sys::os::FramebufferInfo;

use crate::memory::multiboot::{FLAG_FRAMEBUFFER, Info};
use crate::memory::{heap, paging, physical};
use crate::system::process::PROCESSES;
use crate::system::{gdt, syscall};

/// What both kernels need: COM1, the GDT, the IDT with the PICs, memory,
/// the timer, the kernel task and the framebuffer details. Interrupts
/// stay disabled.
///
/// # Safety
///
/// `info` is the boot loader's multiboot information; called once.
unsafe fn bring_up(info: &Info) {
    COM1.init();
    gdt::init();
    interrupt::init();
    unsafe { physical::init(info) };
    // Paging first: until it is up only the first 4 MiB are mapped. The
    // heap's region lies above them, reached through the direct map.
    unsafe { paging::init(info) };
    heap::init();
    pit::activate();
    // The kernel task: the idle loop, run when nothing else is ready.
    PROCESSES.with(|table| table.init());
    if info.flags & FLAG_FRAMEBUFFER != 0 {
        syscall::set_framebuffer(FramebufferInfo {
            address: info.framebuffer_addr as u32,
            width: info.framebuffer_width,
            height: info.framebuffer_height,
            pitch: info.framebuffer_pitch,
            bpp: info.framebuffer_bpp as u32,
        });
    }
}

fn halt() -> ! {
    loop {
        unsafe { asm!("hlt", options(nomem, nostack)) };
    }
}

#[cfg(not(feature = "ktest"))]
mod boot {
    use super::*;
    use crate::memory::multiboot::{FLAG_MODS, Module};
    use crate::memory::phys_to_virt;
    use crate::system::elf;
    use crate::system::gdt::{USER_CODE, USER_DATA};

    /// Load every module as a user process; a module that fails to load
    /// is skipped.
    ///
    /// # Safety
    ///
    /// As for `start`.
    unsafe fn load_modules(info: &Info) {
        if info.flags & FLAG_MODS == 0 {
            return;
        }
        let modules = unsafe {
            core::slice::from_raw_parts(
                phys_to_virt(info.mods_addr) as *const Module,
                info.mods_count as usize,
            )
        };
        // Ring 3 selectors.
        let (cs, ds) = (USER_CODE as u32 | 3, USER_DATA as u32 | 3);
        for module in modules {
            let image = unsafe {
                core::slice::from_raw_parts(
                    phys_to_virt(module.start),
                    (module.end - module.start) as usize,
                )
            };
            let Some(directory) = paging::create_address_space() else {
                continue;
            };
            let spawned = elf::load(directory, image).ok().and_then(|program| {
                PROCESSES.with(|t| t.spawn(directory, program.entry, program.heap_start, cs, ds))
            });
            if spawned.is_none() {
                paging::destroy_address_space(directory);
            }
        }
        // Every process has its own copy of its image now.
        unsafe { physical::release_modules(info) };
    }

    /// Called by loader.s.
    ///
    /// # Safety
    ///
    /// `info` points to the boot loader's multiboot information, in the
    /// direct map.
    #[unsafe(no_mangle)]
    unsafe extern "C" fn start(info: *const Info, _magic: u32) -> ! {
        let info = unsafe { &*info };
        unsafe { bring_up(info) };
        unsafe { load_modules(info) };
        interrupt::enable();
        // Userspace services handle all I/O via IPC.
        halt()
    }
}

#[cfg(feature = "ktest")]
mod test {
    use core::fmt::Write;

    use sys::io::{Port, PortType};

    use super::*;

    /// Called by loader.s in the test kernel: run every kernel test, print
    /// the summary and exit QEMU (exit code 1 when all pass, 3 if not).
    ///
    /// # Safety
    ///
    /// As for the production `start`.
    #[unsafe(no_mangle)]
    unsafe extern "C" fn start(info: *const Info, _magic: u32) -> ! {
        unsafe { bring_up(&*info) };
        let (mut passed, mut failed) = (0, 0);
        crate::ktest::run(&crate::KTESTS, &mut &COM1, &mut passed, &mut failed);
        let _ = writeln!(&COM1, "[DONE] {passed} passed, {failed} failed");
        let exit = if failed == 0 { 0x00 } else { 0x01 };
        unsafe { Port::<u8>::new(PortType::QemuDebugExit).write(exit) };
        halt()
    }
}
