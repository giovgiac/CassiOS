// irq.rs -- PIC setup and IRQ dispatch
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// The two PICs are remapped to vectors 0x20-0x2F, with only the lines in
// LINES unmasked (timer 0, keyboard 1, mouse 12, primary ATA 14). IRQ 0
// drives the PIT; each line can also be forwarded to one userspace driver,
// which receives an IrqNotify message, at once if it is blocked in
// receive and otherwise on its next receive. Spurious IRQs (7 and 15 not
// in service) are dropped. The entry stubs are in stub.s.

// Most of it is only used by the i586 build.
#![cfg_attr(not(target_arch = "x86"), allow(dead_code))]

/// Vector of IRQ 0; the CPU's exceptions use the vectors below.
pub const IRQ_OFFSET: u8 = 0x20;
const IRQS: usize = 16;

/// The lines with a stub that are unmasked: timer, keyboard, mouse and
/// primary ATA. Only these can be forwarded.
pub const LINES: [u8; 4] = [0, 1, 12, 14];

/// Whether IRQ `irq`, with `in_service` the in-service register of the
/// PIC it came from, is spurious. A PIC reports a request that vanished
/// before it was acknowledged on its lowest-priority line (7 on each PIC)
/// without marking it in service.
pub fn spurious(irq: u8, in_service: u8) -> bool {
    (irq == 7 || irq == 15) && in_service & 0x80 == 0
}

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

    /// Forward `irq` to `pid` (0 stops forwarding). False unless `irq` is
    /// one of LINES.
    pub fn register(&mut self, irq: u32, pid: u32) -> bool {
        if !LINES.iter().any(|&line| line as u32 == irq) {
            return false;
        }
        self.pid[irq as usize] = pid;
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
    use crate::system::process::{PROCESSES, State, set_return};
    use crate::system::syscall::write_message;

    static FORWARDING: Global<Forwarding> = Global::new(Forwarding::new());

    const MASTER_COMMAND: Port<u8> = Port::new(PortType::MasterPicCommand);
    const MASTER_DATA: Port<u8> = Port::new(PortType::MasterPicData);
    const SLAVE_COMMAND: Port<u8> = Port::new(PortType::SlavePicCommand);
    const SLAVE_DATA: Port<u8> = Port::new(PortType::SlavePicData);
    const END_OF_INTERRUPT: u8 = 0x20;
    /// Makes the next read of the command port return the in-service
    /// register.
    const READ_IN_SERVICE: u8 = 0x0B;
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
        fn irq_stub_0x07();
        fn irq_stub_0x0F();

    }

    /// Point the IRQ vectors in use (timer, keyboard, mouse, primary ATA)
    /// at their stubs and remap the PICs to them.
    pub fn install() {
        set_gate(IRQ_OFFSET, irq_stub_0x00, 0);
        set_gate(IRQ_OFFSET + 0x01, irq_stub_0x01, 0);
        set_gate(IRQ_OFFSET + 0x0C, irq_stub_0x0C, 0);
        set_gate(IRQ_OFFSET + 0x0E, irq_stub_0x0E, 0);
        set_gate(IRQ_OFFSET + 0x07, irq_stub_0x07, 0);
        set_gate(IRQ_OFFSET + 0x0F, irq_stub_0x0F, 0);

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
        if irq == 7 || irq == 15 {
            let command = if irq == 7 {
                &MASTER_COMMAND
            } else {
                &SLAVE_COMMAND
            };
            // SAFETY: the kernel owns the PICs.
            let in_service = unsafe {
                command.write_slow(READ_IN_SERVICE);
                command.read()
            };
            if spurious(irq, in_service) {
                // The slave's spurious IRQ still occupied the master's
                // cascade line, which needs its end-of-interrupt.
                if irq == 15 {
                    unsafe { MASTER_COMMAND.write_slow(END_OF_INTERRUPT) };
                }
                return esp;
            }
        }
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
            if !deliver(pid, irq) {
                FORWARDING.with(|f| f.set_pending(irq));
            }
        }
        esp
    }

    /// Hand IrqNotify for `irq` to `pid` at once if it is blocked in
    /// receive. False if it is not receiving (the IRQ stays pending); a
    /// process that is gone gets nothing.
    fn deliver(pid: u32, irq: u8) -> bool {
        PROCESSES.with(|table| {
            let Some(process) = table.get_mut(pid) else {
                return true;
            };
            if process.state != State::ReceiveBlocked {
                return false;
            }
            let notify = Message {
                kind: message_type::IRQ_NOTIFY,
                arg1: irq as u32,
                ..Message::default()
            };
            write_message(process.directory, process.msg_ptr, &notify);
            // receive() returns 0: a kernel notification.
            set_return(process, 0);
            process.state = State::Ready;
            true
        })
    }

    /// Called by the stubs with the vector and the saved frame.
    #[unsafe(no_mangle)]
    extern "C" fn handleInterrupt(vector: u32, esp: u32) -> u32 {
        dispatch(vector as u8, esp)
    }

    /// Forward `irq` to `pid` (0 stops it). 0, or -1 unless `irq` is one
    /// of LINES.
    pub fn register_forward(irq: u32, pid: u32) -> i32 {
        if FORWARDING.with(|f| f.register(irq, pid)) {
            0
        } else {
            -1
        }
    }

    /// Take the lowest IRQ pending for `pid`.
    pub fn take_pending(pid: u32) -> Option<u8> {
        FORWARDING.with(|f| f.take_pending(pid))
    }

    #[cfg(feature = "ktest")]
    pub mod ktests {
        use super::*;
        use crate::ktest::Outcome;

        tests!(
            irq_0_drives_the_pit,
            irq_without_a_handler_returns_the_frame,
            irq_only_lines_with_a_stub_are_unmasked,
            irq_spurious_ones_are_dropped,
        );

        fn irq_spurious_ones_are_dropped() -> Outcome {
            // Nothing is in service in the test kernel, so both count as
            // spurious: no PIT tick, the frame comes back unchanged.
            let before = pit::ticks();
            check_eq!(dispatch(IRQ_OFFSET + 7, 0x1234), 0x1234);
            check_eq!(dispatch(IRQ_OFFSET + 15, 0x1234), 0x1234);
            check_eq!(pit::ticks(), before);
            Ok(())
        }

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
    fn only_lines_in_use_can_be_forwarded() {
        let mut f = Forwarding::new();
        for line in LINES {
            assert!(f.register(line as u32, 7));
            assert_eq!(f.target(line), Some(7));
        }
        for irq in [2, 5, 7, 15, 16, 255, 256, u32::MAX] {
            assert!(!f.register(irq, 7), "irq {irq}");
        }
        assert_eq!(f.target(5), None);
    }

    #[test]
    fn spurious_irqs_are_those_not_in_service() {
        assert!(spurious(7, 0x00));
        assert!(spurious(15, 0x7F));
        assert!(!spurious(7, 0x80));
        assert!(!spurious(15, 0x80));
        assert!(!spurious(1, 0x00));
    }

    #[test]
    fn pending_irqs_are_taken_lowest_first_and_once() {
        let mut f = Forwarding::new();
        f.register(1, 7);
        f.register(12, 7);
        f.register(14, 9);
        f.set_pending(12);
        f.set_pending(1);
        f.set_pending(14);
        assert_eq!(f.take_pending(7), Some(1));
        assert_eq!(f.take_pending(7), Some(12));
        assert_eq!(f.take_pending(7), None);
        assert_eq!(f.take_pending(9), Some(14));
    }

    #[test]
    fn unregistering_drops_pending_irqs() {
        let mut f = Forwarding::new();
        f.register(1, 7);
        f.set_pending(1);
        f.register(1, 0);
        assert_eq!(f.target(1), None);
        assert_eq!(f.take_pending(7), None);
        assert_eq!(f.take_pending(0), None);
    }
}
