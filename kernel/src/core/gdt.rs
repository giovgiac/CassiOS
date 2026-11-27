// gdt.rs -- Global Descriptor Table and Task State Segment
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// Flat segments for the kernel and for user mode, and one TSS whose esp0
// is the kernel stack the CPU switches to on entry from ring 3.

// Most of it is only used by the i586 build.
#![cfg_attr(not(target_arch = "x86"), allow(dead_code))]

use core::mem::size_of;

/// Segment selectors: byte offsets of the descriptors in the table.
pub const KERNEL_CODE: u16 = 0x08;
pub const KERNEL_DATA: u16 = 0x10;
pub const USER_CODE: u16 = 0x18;
pub const USER_DATA: u16 = 0x20;
pub const TSS_SELECTOR: u16 = 0x28;

const ENTRIES: usize = 6;

// Access bytes.
const ACCESS_KERNEL_CODE: u8 = 0x9A;
const ACCESS_KERNEL_DATA: u8 = 0x92;
const ACCESS_USER_CODE: u8 = 0xFA;
const ACCESS_USER_DATA: u8 = 0xF2;
/// Present, 32-bit TSS, available.
const ACCESS_TSS: u8 = 0x89;
/// Set in the TSS descriptor's type by ltr.
#[cfg(any(test, feature = "ktest"))]
#[cfg(target_arch = "x86")]
const TSS_BUSY: u8 = 0x02;
/// Set by the CPU in a segment descriptor it loads.
#[cfg(any(test, feature = "ktest"))]
#[cfg(target_arch = "x86")]
const ACCESSED: u8 = 0x01;

// Flags (high nibble of byte 6).
const FLAG_32_BIT: u8 = 0x40;
const FLAG_PAGE_GRANULAR: u8 = 0x80;

/// An 8-byte segment descriptor, as the CPU reads it.
#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Descriptor([u8; 8]);

impl Descriptor {
    pub const NULL: Self = Self([0; 8]);

    /// A 32-bit segment. Limits up to 64 KiB are in bytes; larger ones
    /// are stored in 4 KiB pages.
    pub const fn segment(base: u32, limit: u32, access: u8) -> Self {
        let (limit, flags) = if limit <= 0x1_0000 {
            (limit, FLAG_32_BIT)
        } else if limit & 0xFFF == 0xFFF {
            (limit >> 12, FLAG_32_BIT | FLAG_PAGE_GRANULAR)
        } else {
            ((limit >> 12) - 1, FLAG_32_BIT | FLAG_PAGE_GRANULAR)
        };
        Self([
            limit as u8,
            (limit >> 8) as u8,
            base as u8,
            (base >> 8) as u8,
            (base >> 16) as u8,
            access,
            flags | ((limit >> 16) & 0xF) as u8,
            (base >> 24) as u8,
        ])
    }

    /// A TSS descriptor: like a byte-granular segment, but a TSS must not
    /// have the 32-bit (D/B) flag.
    pub const fn tss(base: u32, limit: u32) -> Self {
        let mut descriptor = Self::segment(base, limit, ACCESS_TSS);
        descriptor.0[6] &= 0x0F;
        descriptor
    }

    #[cfg(any(test, feature = "ktest"))]
    pub fn base(&self) -> u32 {
        u32::from_le_bytes([self.0[2], self.0[3], self.0[4], self.0[7]])
    }

    #[cfg(test)]
    pub fn limit(&self) -> u32 {
        let limit = u32::from_le_bytes([self.0[0], self.0[1], self.0[6] & 0xF, 0]);
        if self.0[6] & FLAG_PAGE_GRANULAR != 0 {
            (limit << 12) | 0xFFF
        } else {
            limit
        }
    }

    #[cfg(test)]
    pub fn access(&self) -> u8 {
        self.0[5]
    }
}

/// The 104-byte Task State Segment. Only esp0 and ss0 are used: the CPU
/// loads SS:ESP from them on entry to ring 0 from ring 3.
#[repr(C)]
pub struct TaskStateSegment {
    prev_tss: u32,
    pub esp0: u32,
    pub ss0: u32,
    unused: [u32; 22],
    trap: u16,
    pub iomap_base: u16,
}

impl TaskStateSegment {
    pub const fn new() -> Self {
        Self {
            prev_tss: 0,
            esp0: 0,
            ss0: KERNEL_DATA as u32,
            unused: [0; 22],
            trap: 0,
            // No I/O permission bitmap: it would start past the end.
            iomap_base: size_of::<TaskStateSegment>() as u16,
        }
    }
}

/// The table for a TSS at `tss_base`.
pub const fn table(tss_base: u32) -> [Descriptor; ENTRIES] {
    let flat = 0xFFFF_FFFF;
    [
        Descriptor::NULL,
        Descriptor::segment(0, flat, ACCESS_KERNEL_CODE),
        Descriptor::segment(0, flat, ACCESS_KERNEL_DATA),
        Descriptor::segment(0, flat, ACCESS_USER_CODE),
        Descriptor::segment(0, flat, ACCESS_USER_DATA),
        Descriptor::tss(tss_base, size_of::<TaskStateSegment>() as u32 - 1),
    ]
}

#[cfg(target_arch = "x86")]
pub use imp::*;

#[cfg(target_arch = "x86")]
mod imp {
    use core::arch::asm;

    use super::*;
    use crate::sync::Global;

    #[repr(C, align(8))]
    struct Table([Descriptor; ENTRIES]);

    static GDT: Global<Table> = Global::new(Table([Descriptor::NULL; ENTRIES]));
    static TSS: Global<TaskStateSegment> = Global::new(TaskStateSegment::new());
    static LOADED: Global<bool> = Global::new(false);

    /// The operand of lgdt.
    #[repr(C, packed)]
    struct Pointer {
        limit: u16,
        base: u32,
    }

    /// Build the table, load it, reload every segment register and load
    /// the TSS. There is one table: later calls do nothing (a second ltr
    /// on the now busy TSS would fault).
    pub fn init() {
        if LOADED.with(|loaded| core::mem::replace(loaded, true)) {
            return;
        }
        let tss = TSS.with(|tss| tss as *mut TaskStateSegment as u32);
        let pointer = GDT.with(|gdt| {
            gdt.0 = table(tss);
            Pointer {
                limit: (size_of::<Table>() - 1) as u16,
                base: gdt.0.as_ptr() as u32,
            }
        });
        // SAFETY: the table describes the flat segments the kernel already
        // runs in, at the selectors the reloads use.
        unsafe {
            asm!(
                "lgdt [{pointer}]",
                // A far return reloads CS.
                "push {code}",
                "lea {scratch}, [2f]",
                "push {scratch}",
                "retf",
                "2:",
                "mov ds, {data:x}",
                "mov es, {data:x}",
                "mov fs, {data:x}",
                "mov gs, {data:x}",
                "mov ss, {data:x}",
                "ltr {tss:x}",
                pointer = in(reg) &pointer,
                code = const KERNEL_CODE as u32,
                scratch = out(reg) _,
                data = in(reg) KERNEL_DATA as u32,
                tss = in(reg) TSS_SELECTOR as u32,
            )
        };
    }

    /// Set the kernel stack the CPU switches to on entry from ring 3.
    pub fn set_kernel_stack(esp0: u32) {
        TSS.with(|tss| tss.esp0 = esp0);
    }

    #[cfg(feature = "ktest")]
    pub mod ktests {
        use super::*;
        use crate::ktest::Outcome;

        tests!(
            gdt_segment_registers_hold_the_kernel_selectors,
            gdt_task_register_holds_the_tss,
            gdt_loaded_table_is_ours,
            gdt_set_kernel_stack_reaches_the_tss,
            gdt_init_again_changes_nothing,
            gdt_user_selectors_name_ring_3_segments,
        );

        fn gdt_user_selectors_name_ring_3_segments() -> Outcome {
            // Processes run with these, ORed with ring 3.
            let table = loaded();
            let access = |selector: u16| table[selector as usize / 8].0[5] & !ACCESSED;
            check_eq!((access(USER_CODE), access(USER_DATA)), (0xFA, 0xF2));
            Ok(())
        }

        fn gdt_segment_registers_hold_the_kernel_selectors() -> Outcome {
            let (cs, ds, ss): (u16, u16, u16);
            unsafe {
                asm!("mov {:x}, cs", out(reg) cs);
                asm!("mov {:x}, ds", out(reg) ds);
                asm!("mov {:x}, ss", out(reg) ss);
            }
            check_eq!((cs, ds, ss), (KERNEL_CODE, KERNEL_DATA, KERNEL_DATA));
            Ok(())
        }

        fn gdt_task_register_holds_the_tss() -> Outcome {
            let tr: u16;
            unsafe { asm!("str {:x}", out(reg) tr) };
            check_eq!(tr, TSS_SELECTOR);
            Ok(())
        }

        /// The table the CPU uses, read back through sgdt.
        fn loaded() -> &'static [Descriptor; ENTRIES] {
            let mut pointer = Pointer { limit: 0, base: 0 };
            unsafe {
                asm!("sgdt [{}]", in(reg) &mut pointer);
                &*(pointer.base as *const [Descriptor; ENTRIES])
            }
        }

        fn gdt_loaded_table_is_ours() -> Outcome {
            let tss = TSS.with(|tss| tss as *mut TaskStateSegment as u32);
            // The CPU sets the accessed bit of segments it loads, and ltr
            // marks the TSS busy (bit 1 of the TSS type).
            let mask = |mut entries: [Descriptor; ENTRIES]| {
                for entry in &mut entries {
                    entry.0[5] &= !(ACCESSED | TSS_BUSY);
                }
                entries
            };
            check!(mask(*loaded()) == mask(table(tss)));
            Ok(())
        }

        fn gdt_init_again_changes_nothing() -> Outcome {
            let before = *loaded();
            init();
            check!(*loaded() == before);
            Ok(())
        }

        fn gdt_set_kernel_stack_reaches_the_tss() -> Outcome {
            let before = TSS.with(|tss| tss.esp0);
            set_kernel_stack(0xDEAD_BEEF);
            let tss = loaded()[TSS_SELECTOR as usize / 8].base() as *const TaskStateSegment;
            let esp0 = unsafe { (*tss).esp0 };
            set_kernel_stack(before);
            check_eq!(esp0, 0xDEAD_BEEF);
            Ok(())
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn descriptors_are_8_bytes_and_the_tss_104() {
        assert_eq!(size_of::<Descriptor>(), 8);
        assert_eq!(size_of::<TaskStateSegment>(), 104);
    }

    #[test]
    fn selectors_follow_the_table_order() {
        for (index, selector) in [KERNEL_CODE, KERNEL_DATA, USER_CODE, USER_DATA, TSS_SELECTOR]
            .into_iter()
            .enumerate()
        {
            assert_eq!(selector as usize, (index + 1) * size_of::<Descriptor>());
        }
    }

    #[test]
    fn base_round_trips() {
        let descriptor = Descriptor::segment(0x1234_5678, 0xFFFF, ACCESS_KERNEL_DATA);
        assert_eq!(descriptor.base(), 0x1234_5678);
    }

    #[test]
    fn small_limits_are_in_bytes() {
        let descriptor = Descriptor::segment(0, 0x1_0000, ACCESS_KERNEL_CODE);
        assert_eq!(descriptor.limit(), 0x1_0000);
        assert_eq!(descriptor.0[6] & 0xF0, FLAG_32_BIT);
    }

    #[test]
    fn large_limits_are_in_pages() {
        let flat = Descriptor::segment(0, 0xFFFF_FFFF, ACCESS_KERNEL_CODE);
        assert_eq!(flat.limit(), 0xFFFF_FFFF);
        assert_eq!(flat.0, [0xFF, 0xFF, 0, 0, 0, 0x9A, 0xCF, 0]);
        // A limit short of a whole page loses the partial page.
        assert_eq!(Descriptor::segment(0, 0x2_0800, 0x92).limit(), 0x1_FFFF);
    }

    #[test]
    fn the_table_has_flat_kernel_and_user_segments() {
        let entries = table(0xC010_0000);
        assert_eq!(entries[0], Descriptor::NULL);
        let access: Vec<u8> = entries[1..5].iter().map(|d| d.access()).collect();
        assert_eq!(access, [0x9A, 0x92, 0xFA, 0xF2]);
        assert!(
            entries[1..5]
                .iter()
                .all(|d| d.base() == 0 && d.limit() == 0xFFFF_FFFF)
        );
    }

    #[test]
    fn the_tss_descriptor_is_16_bit_clear_and_covers_the_tss() {
        let tss = table(0xC010_0000)[TSS_SELECTOR as usize / 8];
        assert_eq!(tss.base(), 0xC010_0000);
        assert_eq!(tss.limit(), 103);
        assert_eq!(tss.access(), 0x89);
        assert_eq!(tss.0[6] & 0xF0, 0);
    }
}
