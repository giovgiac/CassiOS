// interrupt.rs -- Interrupt Descriptor Table
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// Every vector gets an interrupt gate (interrupts stay disabled while its
// handler runs): CPU exceptions, the remapped PIC IRQs, the syscall gate
// (kernel/src/core/syscall.rs), and an ignoring stub for the rest. The
// entry stubs are in stub.s and syscall_stub.s. The C++ InterruptManager
// (kernel/include/hardware/interrupt.hpp) forwards here until its callers
// are ported.

// Most of it is only used by the i586 build.
#![cfg_attr(not(target_arch = "x86"), allow(dead_code))]

/// Present, 32-bit interrupt gate.
const GATE_INTERRUPT: u8 = 0x8E;

/// An 8-byte IDT entry: the handler's address and code selector, and
/// whether code at `dpl` may raise it with `int`.
#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Gate([u8; 8]);

impl Gate {
    pub const fn new(handler: u32, selector: u16, dpl: u8) -> Self {
        Self([
            handler as u8,
            (handler >> 8) as u8,
            selector as u8,
            (selector >> 8) as u8,
            0,
            GATE_INTERRUPT | ((dpl & 3) << 5),
            (handler >> 16) as u8,
            (handler >> 24) as u8,
        ])
    }

    #[cfg(any(test, feature = "ktest"))]
    pub fn handler(&self) -> u32 {
        u32::from_le_bytes([self.0[0], self.0[1], self.0[6], self.0[7]])
    }

    #[cfg(any(test, feature = "ktest"))]
    pub fn selector(&self) -> u16 {
        u16::from_le_bytes([self.0[2], self.0[3]])
    }

    #[cfg(any(test, feature = "ktest"))]
    pub fn access(&self) -> u8 {
        self.0[5]
    }
}

#[cfg(target_arch = "x86")]
pub use imp::*;

#[cfg(target_arch = "x86")]
mod imp {
    use core::arch::asm;

    use super::*;
    use crate::hardware::{exception, irq};
    use crate::sync::Global;
    use crate::system::gdt::KERNEL_CODE;

    const VECTORS: usize = 256;

    #[repr(C, align(8))]
    struct Table([Gate; VECTORS]);

    static IDT: Global<Table> = Global::new(Table([Gate([0; 8]); VECTORS]));

    unsafe extern "C" {
        fn ignore_interrupt();
    }

    /// The address of an entry stub.
    fn address(stub: unsafe extern "C" fn()) -> u32 {
        stub as *const () as u32
    }

    /// The operand of lidt and sidt.
    #[repr(C, packed)]
    struct Pointer {
        limit: u16,
        base: u32,
    }

    /// Point `vector` at `handler`, an entry stub. DPL 3 lets user mode
    /// raise it with `int`.
    pub fn set_gate(vector: u8, handler: unsafe extern "C" fn(), dpl: u8) {
        let gate = Gate::new(address(handler), KERNEL_CODE, dpl);
        IDT.with(|idt| idt.0[vector as usize] = gate);
    }

    /// Fill the table, program the PICs and load it.
    pub fn init() {
        let ignore = Gate::new(address(ignore_interrupt), KERNEL_CODE, 0);
        IDT.with(|idt| idt.0 = [ignore; VECTORS]);
        exception::install();
        irq::install();
        crate::system::syscall::install();
        let pointer = IDT.with(|idt| Pointer {
            limit: (size_of::<Table>() - 1) as u16,
            base: idt.0.as_ptr() as u32,
        });
        unsafe { asm!("lidt [{}]", in(reg) &pointer, options(nostack)) };
    }

    pub fn enable() {
        unsafe { asm!("sti", options(nomem, nostack)) };
    }

    #[cfg(feature = "ktest")]
    pub mod ktests {
        use super::*;
        use crate::ktest::Outcome;

        tests!(
            interrupt_loaded_table_is_ours,
            interrupt_used_vectors_have_distinct_handlers,
            interrupt_unused_vectors_are_ignored,
            interrupt_syscall_gate_is_dpl_3,
        );

        /// The table the CPU uses, read back through sidt.
        fn loaded() -> (u16, &'static [Gate; VECTORS]) {
            let mut pointer = Pointer { limit: 0, base: 0 };
            unsafe {
                asm!("sidt [{}]", in(reg) &mut pointer, options(nostack));
                (pointer.limit, &*(pointer.base as *const [Gate; VECTORS]))
            }
        }

        const USED: [usize; 10] = [0x00, 0x06, 0x0D, 0x0E, 0x20, 0x21, 0x27, 0x2C, 0x2E, 0x2F];

        fn interrupt_loaded_table_is_ours() -> Outcome {
            let (limit, table) = loaded();
            let ours = IDT.with(|idt| idt.0.as_ptr());
            check_eq!(limit as usize, VECTORS * 8 - 1);
            check!(table.as_ptr() == ours);
            check!(table.iter().all(|gate| gate.selector() == KERNEL_CODE));
            Ok(())
        }

        fn interrupt_used_vectors_have_distinct_handlers() -> Outcome {
            let (_, table) = loaded();
            let ignore = address(ignore_interrupt);
            for (i, &a) in USED.iter().enumerate() {
                check!(table[a].handler() != ignore);
                check_eq!(table[a].access(), GATE_INTERRUPT);
                for &b in &USED[i + 1..] {
                    check!(table[a].handler() != table[b].handler());
                }
            }
            Ok(())
        }

        fn interrupt_unused_vectors_are_ignored() -> Outcome {
            let (_, table) = loaded();
            let ignore = address(ignore_interrupt);
            let unused = (0..VECTORS).filter(|v| !USED.contains(v) && *v != 0x80);
            for vector in unused {
                check_eq!((vector, table[vector].handler()), (vector, ignore));
            }
            Ok(())
        }

        fn interrupt_syscall_gate_is_dpl_3() -> Outcome {
            let (_, table) = loaded();
            check_eq!(table[0x80].access(), GATE_INTERRUPT | (3 << 5));
            check!(table[0x80].handler() != address(ignore_interrupt));
            Ok(())
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn gates_split_the_handler_address() {
        let gate = Gate::new(0xC012_3456, 0x08, 0);
        assert_eq!(gate.0, [0x56, 0x34, 0x08, 0, 0, 0x8E, 0x12, 0xC0]);
        assert_eq!(gate.handler(), 0xC012_3456);
        assert_eq!(gate.selector(), 0x08);
    }

    #[test]
    fn dpl_3_gates_can_be_raised_from_user_mode() {
        assert_eq!(Gate::new(0, 0x08, 3).access(), 0xEE);
        assert_eq!(Gate::new(0, 0x08, 0).access(), 0x8E);
    }
}
