// serial.rs -- 16550 UART driver
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// COM1 carries the kernel's log, the serial write syscall and the test
// reports.

use core::fmt;

use sys::io::{Port, PortType};

const LINE_CONTROL_DLAB: u8 = 0x80;
const LINE_CONTROL_8N1: u8 = 0x03;
/// Enable and clear both FIFOs, interrupt at 14 bytes.
const FIFO_CONTROL_ENABLE: u8 = 0xC7;
const MODEM_CONTROL_DTR_RTS: u8 = 0x03;
#[cfg(feature = "ktest")]
const MODEM_CONTROL_LOOPBACK: u8 = 0x10;
#[cfg(feature = "ktest")]
const LINE_STATUS_DATA_READY: u8 = 0x01;
const LINE_STATUS_TRANSMIT_EMPTY: u8 = 0x20;
/// Nothing left to send: the FIFO and the shift register are empty.
#[cfg(feature = "ktest")]
const LINE_STATUS_TRANSMITTER_IDLE: u8 = 0x40;
/// 115200 / 3 = 38400 baud.
const BAUD_DIVISOR: u8 = 3;

pub struct Serial {
    data: Port<u8>,
    interrupt_enable: Port<u8>,
    fifo_control: Port<u8>,
    line_control: Port<u8>,
    modem_control: Port<u8>,
    line_status: Port<u8>,
}

/// The first serial port.
pub static COM1: Serial = Serial::new(
    PortType::SerialCOM1Data,
    PortType::SerialCOM1InterruptEnable,
    PortType::SerialCOM1FIFOControl,
    PortType::SerialCOM1LineControl,
    PortType::SerialCOM1ModemControl,
    PortType::SerialCOM1LineStatus,
);

impl Serial {
    const fn new(
        data: PortType,
        interrupt_enable: PortType,
        fifo_control: PortType,
        line_control: PortType,
        modem_control: PortType,
        line_status: PortType,
    ) -> Self {
        Self {
            data: Port::new(data),
            interrupt_enable: Port::new(interrupt_enable),
            fifo_control: Port::new(fifo_control),
            line_control: Port::new(line_control),
            modem_control: Port::new(modem_control),
            line_status: Port::new(line_status),
        }
    }

    /// Program 38400 baud, 8N1, FIFOs on, interrupts off.
    pub fn init(&self) {
        // SAFETY: the kernel owns the UART.
        unsafe {
            self.interrupt_enable.write(0x00);
            self.line_control.write(LINE_CONTROL_DLAB);
            // With DLAB set, data and interrupt enable hold the divisor.
            self.data.write(BAUD_DIVISOR);
            self.interrupt_enable.write(0x00);
            self.line_control.write(LINE_CONTROL_8N1);
            self.fifo_control.write(FIFO_CONTROL_ENABLE);
            self.modem_control.write(MODEM_CONTROL_DTR_RTS);
        }
    }

    /// Send one byte, waiting until the transmitter can take it.
    pub fn putchar(&self, byte: u8) {
        // SAFETY: the kernel owns the UART.
        unsafe {
            while self.line_status.read() & LINE_STATUS_TRANSMIT_EMPTY == 0 {}
            self.data.write(byte);
        }
    }

    pub fn write(&self, bytes: &[u8]) {
        for &byte in bytes {
            self.putchar(byte);
        }
    }
}

impl fmt::Write for &Serial {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        self.write(s.as_bytes());
        Ok(())
    }
}

#[cfg(feature = "ktest")]
pub mod ktests {
    use super::*;
    use crate::ktest::Outcome;

    tests!(com1_is_8n1_without_interrupts, com1_loopback_round_trip);

    fn com1_is_8n1_without_interrupts() -> Outcome {
        // SAFETY: reads have no side effect on these registers.
        let (line, interrupts) =
            unsafe { (COM1.line_control.read(), COM1.interrupt_enable.read()) };
        check_eq!(line, LINE_CONTROL_8N1);
        check_eq!(interrupts, 0);
        Ok(())
    }

    fn com1_loopback_round_trip() -> Outcome {
        // In loopback mode the UART receives what it sends instead of
        // putting it on the line. Earlier output must leave first, or it
        // would loop back too, and nothing stale may wait to be read.
        // SAFETY: the kernel owns the UART; normal mode is restored below.
        let idle = (0..100_000)
            .any(|_| unsafe { COM1.line_status.read() } & LINE_STATUS_TRANSMITTER_IDLE != 0);
        check!(idle);
        let received = unsafe {
            while COM1.line_status.read() & LINE_STATUS_DATA_READY != 0 {
                COM1.data.read();
            }
            COM1.modem_control
                .write(MODEM_CONTROL_DTR_RTS | MODEM_CONTROL_LOOPBACK);
            COM1.putchar(0xA5);
            let mut received = None;
            for _ in 0..100_000 {
                if COM1.line_status.read() & LINE_STATUS_DATA_READY != 0 {
                    received = Some(COM1.data.read());
                    break;
                }
            }
            COM1.modem_control.write(MODEM_CONTROL_DTR_RTS);
            received
        };
        check_eq!(received, Some(0xA5));
        Ok(())
    }
}
