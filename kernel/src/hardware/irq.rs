// irq.rs -- PIC setup and IRQ dispatch
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// The two PICs are remapped to vectors 0x20-0x2F. IRQ 0 drives the PIT;
// any IRQ can also be forwarded to one userspace driver, which receives an
// IrqNotify message, at once if it is blocked in receive and otherwise on
// its next receive. The entry stubs are in stub.s. The C++ IrqManager
// (kernel/include/hardware/irq.hpp) forwards here until its callers are
// ported.

// Most of it is only used by the i586 build.
#![cfg_attr(not(target_arch = "x86"), allow(dead_code))]

/// Vector of IRQ 0; the CPU's exceptions use the vectors below.
pub const IRQ_OFFSET: u8 = 0x20;
const IRQS: usize = 16;

/// Which process each IRQ is forwarded to, and IRQs it has yet to receive.
pub struct Forwarding {
    pid: [u32; IRQS],
    pending: [bool; IRQS],
}

impl Forwarding {
    pub const fn new() -> Self {
        Self {
            pid: [0; IRQS],
            pending: [false; IRQS],
        }
    }

    /// Forward `irq` to `pid` (0 stops forwarding). False if `irq` is not
    /// an IRQ.
    pub fn register(&mut self, irq: u8, pid: u32) -> bool {
        let Some(slot) = self.pid.get_mut(irq as usize) else {
            return false;
        };
        *slot = pid;
        self.pending[irq as usize] = false;
        true
    }

    /// The process `irq` is forwarded to, if any.
    pub fn target(&self, irq: u8) -> Option<u32> {
        self.pid.get(irq as usize).copied().filter(|&pid| pid != 0)
    }

    pub fn set_pending(&mut self, irq: u8) {
        self.pending[irq as usize] = true;
    }

    /// Take the lowest pending IRQ forwarded to `pid`.
    pub fn take_pending(&mut self, pid: u32) -> Option<u8> {
        let irq = (0..IRQS).find(|&irq| self.pid[irq] == pid && self.pending[irq])?;
        self.pending[irq] = false;
        Some(irq as u8)
    }
}

#[cfg(target_arch = "x86")]
pub use imp::*;

#[cfg(target_arch = "x86")]
mod imp {
    use sys::io::{Port, PortType};
    use sys::ipc::{Message, message_type};

    use super::*;
    use crate::hardware::interrupt::set_gate;
    use crate::hardware::pit;
    use crate::sync::Global;

    static FORWARDING: Global<Forwarding> = Global::new(Forwarding::new());

    const MASTER_COMMAND: Port<u8> = Port::new(PortType::MasterPicCommand);
    const MASTER_DATA: Port<u8> = Port::new(PortType::MasterPicData);
    const SLAVE_COMMAND: Port<u8> = Port::new(PortType::SlavePicCommand);
    const SLAVE_DATA: Port<u8> = Port::new(PortType::SlavePicData);
    const END_OF_INTERRUPT: u8 = 0x20;
    /// Unmasked lines: the timer (0), keyboard (1) and the slave's cascade
    /// (2); on the slave, the mouse (12) and the primary ATA channel (14).
    /// Only these have stubs; an IRQ on another line would reach the
    /// ignoring stub, get no end-of-interrupt, and block every IRQ of
    /// lower priority for good.
    const MASTER_MASK: u8 = !0b0000_0111;
    const SLAVE_MASK: u8 = !0b0101_0000;

    unsafe extern "C" {
        fn irq_stub_0x00();
        fn irq_stub_0x01();
        fn irq_stub_0x0C();
        fn irq_stub_0x0E();

        /// Deliver IrqNotify for `irq` to process `pid` if it is blocked
        /// in receive (C++, kernel/src/hardware/irq.cpp, until processes
        /// are ported). Returns 0 if there is no such process, 1 if it was
        /// delivered, 2 if the process is not receiving.
        fn process_deliver_irq(pid: u32, irq: u8) -> u32;
    }

    /// Point the IRQ vectors in use (timer, keyboard, mouse, primary ATA)
    /// at their stubs and remap the PICs to them.
    pub fn install() {
        set_gate(IRQ_OFFSET, irq_stub_0x00, 0);
        set_gate(IRQ_OFFSET + 0x01, irq_stub_0x01, 0);
        set_gate(IRQ_OFFSET + 0x0C, irq_stub_0x0C, 0);
        set_gate(IRQ_OFFSET + 0x0E, irq_stub_0x0E, 0);

        // SAFETY: the kernel owns the PICs.
        unsafe {
            // Initialize, with a fourth configuration word.
            MASTER_COMMAND.write_slow(0x11);
            SLAVE_COMMAND.write_slow(0x11);
            MASTER_DATA.write_slow(IRQ_OFFSET);
            SLAVE_DATA.write_slow(IRQ_OFFSET + 8);
            // The slave hangs off the master's IRQ 2.
            MASTER_DATA.write_slow(0x04);
            SLAVE_DATA.write_slow(0x02);
            // 8086 mode, then unmask the lines with a stub.
            MASTER_DATA.write_slow(0x01);
            SLAVE_DATA.write_slow(0x01);
            MASTER_DATA.write_slow(MASTER_MASK);
            SLAVE_DATA.write_slow(SLAVE_MASK);
        }
    }

    /// Handle the IRQ at `vector`, whose interrupted context is saved at
    /// `esp`. Returns the context to resume (the PIT may switch process).
    pub fn dispatch(vector: u8, mut esp: u32) -> u32 {
        let irq = vector - IRQ_OFFSET;
        if irq == 0 {
            esp = pit::tick(esp);
        }

        // SAFETY: the kernel owns the PICs.
        unsafe {
            MASTER_COMMAND.write_slow(END_OF_INTERRUPT);
            if irq >= 8 {
                SLAVE_COMMAND.write_slow(END_OF_INTERRUPT);
            }
        }

        if let Some(pid) = FORWARDING.with(|f| f.target(irq)) {
            // SAFETY: the C++ side only touches the process table.
            if unsafe { process_deliver_irq(pid, irq) } == 2 {
                FORWARDING.with(|f| f.set_pending(irq));
            }
        }
        esp
    }

    /// Called by the stubs with the vector and the saved frame.
    #[unsafe(no_mangle)]
    extern "C" fn handleInterrupt(vector: u32, esp: u32) -> u32 {
        dispatch(vector as u8, esp)
    }

    #[unsafe(no_mangle)]
    extern "C" fn irq_dispatch(vector: u8, esp: u32) -> u32 {
        dispatch(vector, esp)
    }

    /// 0 on success, -1 if `irq` is not an IRQ.
    #[unsafe(no_mangle)]
    extern "C" fn irq_register_forward(irq: u8, pid: u32) -> i32 {
        if FORWARDING.with(|f| f.register(irq, pid)) {
            0
        } else {
            -1
        }
    }

    /// Fill `msg` with an IrqNotify for the lowest IRQ pending for `pid`.
    /// False if none is pending.
    ///
    /// # Safety
    ///
    /// `msg` must be writable.
    #[unsafe(no_mangle)]
    unsafe extern "C" fn irq_deliver_pending(pid: u32, msg: *mut Message) -> bool {
        let Some(irq) = FORWARDING.with(|f| f.take_pending(pid)) else {
            return false;
        };
        let notify = Message {
            kind: message_type::IRQ_NOTIFY,
            arg1: irq as u32,
            ..Message::default()
        };
        unsafe { msg.write(notify) };
        true
    }

    #[cfg(not(test))]
    pub mod ktests {
        use super::*;
        use crate::ktest::Outcome;

        tests!(
            irq_0_drives_the_pit,
            irq_without_a_handler_returns_the_frame,
            irq_only_lines_with_a_stub_are_unmasked,
        );

        fn irq_only_lines_with_a_stub_are_unmasked() -> Outcome {
            let masks = unsafe { (MASTER_DATA.read(), SLAVE_DATA.read()) };
            check_eq!(masks, (0xF8, 0xAF));
            Ok(())
        }

        fn irq_0_drives_the_pit() -> Outcome {
            let before = pit::ticks();
            dispatch(IRQ_OFFSET, 0);
            check_eq!(pit::ticks(), before + 1);
            Ok(())
        }

        fn irq_without_a_handler_returns_the_frame() -> Outcome {
            let before = pit::ticks();
            check_eq!(dispatch(IRQ_OFFSET + 5, 0x1234_5678), 0x1234_5678);
            check_eq!(pit::ticks(), before);
            Ok(())
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn only_irqs_0_to_15_can_be_forwarded() {
        let mut f = Forwarding::new();
        assert!(f.register(15, 7));
        assert!(!f.register(16, 7));
        assert!(!f.register(255, 7));
        assert_eq!(f.target(15), Some(7));
        assert_eq!(f.target(16), None);
    }

    #[test]
    fn pending_irqs_are_taken_lowest_first_and_once() {
        let mut f = Forwarding::new();
        f.register(1, 7);
        f.register(12, 7);
        f.register(5, 9);
        f.set_pending(12);
        f.set_pending(1);
        f.set_pending(5);
        assert_eq!(f.take_pending(7), Some(1));
        assert_eq!(f.take_pending(7), Some(12));
        assert_eq!(f.take_pending(7), None);
        assert_eq!(f.take_pending(9), Some(5));
    }

    #[test]
    fn unregistering_drops_pending_irqs() {
        let mut f = Forwarding::new();
        f.register(3, 7);
        f.set_pending(3);
        f.register(3, 0);
        assert_eq!(f.target(3), None);
        assert_eq!(f.take_pending(7), None);
        assert_eq!(f.take_pending(0), None);
    }
}
