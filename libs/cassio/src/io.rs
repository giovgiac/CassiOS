// io.rs -- x86 I/O port access
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.

/// Known I/O port addresses.
#[repr(u16)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum PortType {
    MasterPicCommand = 0x20,
    MasterPicData = 0x21,
    PitChannel0Data = 0x40,
    PitCommand = 0x43,
    KbdData = 0x60,
    KbdCommand = 0x64,
    SlavePicCommand = 0xA0,
    SlavePicData = 0xA1,
    PrimaryAtaData = 0x1F0,
    PrimaryAtaError = 0x1F1,
    PrimaryAtaSectorCount = 0x1F2,
    PrimaryAtaLbaLow = 0x1F3,
    PrimaryAtaLbaMid = 0x1F4,
    PrimaryAtaLbaHigh = 0x1F5,
    PrimaryAtaDriveSelect = 0x1F6,
    PrimaryAtaCommandStatus = 0x1F7,
    VgaCrtcIndex = 0x3D4,
    VgaCrtcData = 0x3D5,
    PrimaryAtaDeviceControl = 0x3F6,
    SerialCOM1Data = 0x3F8,
    SerialCOM1InterruptEnable = 0x3F9,
    SerialCOM1FIFOControl = 0x3FA,
    SerialCOM1LineControl = 0x3FB,
    SerialCOM1ModemControl = 0x3FC,
    SerialCOM1LineStatus = 0x3FD,
    QemuDebugExit = 0xF4,
}

#[cfg(target_arch = "x86")]
pub use imp::*;

#[cfg(target_arch = "x86")]
mod imp {
    use core::arch::asm;
    use core::marker::PhantomData;

    use super::PortType;

    /// A value that can be transferred through an I/O port.
    pub trait PortValue: Copy {
        unsafe fn read_from(port: u16) -> Self;
        unsafe fn write_to(port: u16, value: Self);
    }

    impl PortValue for u8 {
        unsafe fn read_from(port: u16) -> Self {
            let value: u8;
            unsafe { asm!("in al, dx", out("al") value, in("dx") port, options(nostack)) };
            value
        }

        unsafe fn write_to(port: u16, value: Self) {
            unsafe { asm!("out dx, al", in("dx") port, in("al") value, options(nostack)) };
        }
    }

    /// An I/O port carrying values of type `T`.
    pub struct Port<T: PortValue> {
        number: u16,
        _value: PhantomData<T>,
    }

    impl<T: PortValue> Port<T> {
        pub const fn new(port: PortType) -> Self {
            Self {
                number: port as u16,
                _value: PhantomData,
            }
        }

        /// Read a value from the port.
        ///
        /// # Safety
        ///
        /// Port I/O drives hardware directly and can affect the whole
        /// system; the caller must own the device behind the port.
        pub unsafe fn read(&self) -> T {
            unsafe { T::read_from(self.number) }
        }

        /// Write a value to the port.
        ///
        /// # Safety
        ///
        /// See [`Port::read`].
        pub unsafe fn write(&self, value: T) {
            unsafe { T::write_to(self.number, value) }
        }
    }
}
