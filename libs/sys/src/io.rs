// io.rs -- x86 I/O port access, shared by the kernel and userspace drivers
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

    // Every value, mirrored in libs/io/tests/test_io.cpp, so a port changed
    // on one side only fails the tests.
    #[test]
    fn port_numbers_match_the_kernel() {
        assert_eq!(PortType::MasterPicCommand as u16, 0x20);
        assert_eq!(PortType::MasterPicData as u16, 0x21);
        assert_eq!(PortType::PitChannel0Data as u16, 0x40);
        assert_eq!(PortType::PitCommand as u16, 0x43);
        assert_eq!(PortType::KbdData as u16, 0x60);
        assert_eq!(PortType::KbdCommand as u16, 0x64);
        assert_eq!(PortType::SlavePicCommand as u16, 0xA0);
        assert_eq!(PortType::SlavePicData as u16, 0xA1);
        assert_eq!(PortType::PrimaryAtaData as u16, 0x1F0);
        assert_eq!(PortType::PrimaryAtaError as u16, 0x1F1);
        assert_eq!(PortType::PrimaryAtaSectorCount as u16, 0x1F2);
        assert_eq!(PortType::PrimaryAtaLbaLow as u16, 0x1F3);
        assert_eq!(PortType::PrimaryAtaLbaMid as u16, 0x1F4);
        assert_eq!(PortType::PrimaryAtaLbaHigh as u16, 0x1F5);
        assert_eq!(PortType::PrimaryAtaDriveSelect as u16, 0x1F6);
        assert_eq!(PortType::PrimaryAtaCommandStatus as u16, 0x1F7);
        assert_eq!(PortType::VgaCrtcIndex as u16, 0x3D4);
        assert_eq!(PortType::VgaCrtcData as u16, 0x3D5);
        assert_eq!(PortType::PrimaryAtaDeviceControl as u16, 0x3F6);
        assert_eq!(PortType::SerialCOM1Data as u16, 0x3F8);
        assert_eq!(PortType::SerialCOM1InterruptEnable as u16, 0x3F9);
        assert_eq!(PortType::SerialCOM1FIFOControl as u16, 0x3FA);
        assert_eq!(PortType::SerialCOM1LineControl as u16, 0x3FB);
        assert_eq!(PortType::SerialCOM1ModemControl as u16, 0x3FC);
        assert_eq!(PortType::SerialCOM1LineStatus as u16, 0x3FD);
        assert_eq!(PortType::QemuDebugExit as u16, 0xF4);
    }

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

    impl PortValue for u16 {
        unsafe fn read_from(port: u16) -> Self {
            let value: u16;
            unsafe { asm!("in ax, dx", out("ax") value, in("dx") port, options(nostack)) };
            value
        }

        unsafe fn write_to(port: u16, value: Self) {
            unsafe { asm!("out dx, ax", in("dx") port, in("ax") value, options(nostack)) };
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

#[cfg(test)]
mod tests {
    use super::PortType;

    // Every value, mirrored in libs/io/tests/test_io.cpp, so a port changed
    // on one side only fails the tests.
    #[test]
    fn port_numbers_match_the_kernel() {
        assert_eq!(PortType::MasterPicCommand as u16, 0x20);
        assert_eq!(PortType::MasterPicData as u16, 0x21);
        assert_eq!(PortType::PitChannel0Data as u16, 0x40);
        assert_eq!(PortType::PitCommand as u16, 0x43);
        assert_eq!(PortType::KbdData as u16, 0x60);
        assert_eq!(PortType::KbdCommand as u16, 0x64);
        assert_eq!(PortType::SlavePicCommand as u16, 0xA0);
        assert_eq!(PortType::SlavePicData as u16, 0xA1);
        assert_eq!(PortType::PrimaryAtaData as u16, 0x1F0);
        assert_eq!(PortType::PrimaryAtaError as u16, 0x1F1);
        assert_eq!(PortType::PrimaryAtaSectorCount as u16, 0x1F2);
        assert_eq!(PortType::PrimaryAtaLbaLow as u16, 0x1F3);
        assert_eq!(PortType::PrimaryAtaLbaMid as u16, 0x1F4);
        assert_eq!(PortType::PrimaryAtaLbaHigh as u16, 0x1F5);
        assert_eq!(PortType::PrimaryAtaDriveSelect as u16, 0x1F6);
        assert_eq!(PortType::PrimaryAtaCommandStatus as u16, 0x1F7);
        assert_eq!(PortType::VgaCrtcIndex as u16, 0x3D4);
        assert_eq!(PortType::VgaCrtcData as u16, 0x3D5);
        assert_eq!(PortType::PrimaryAtaDeviceControl as u16, 0x3F6);
        assert_eq!(PortType::SerialCOM1Data as u16, 0x3F8);
        assert_eq!(PortType::SerialCOM1InterruptEnable as u16, 0x3F9);
        assert_eq!(PortType::SerialCOM1FIFOControl as u16, 0x3FA);
        assert_eq!(PortType::SerialCOM1LineControl as u16, 0x3FB);
        assert_eq!(PortType::SerialCOM1ModemControl as u16, 0x3FC);
        assert_eq!(PortType::SerialCOM1LineStatus as u16, 0x3FD);
        assert_eq!(PortType::QemuDebugExit as u16, 0xF4);
    }

    #[test]
    fn ata_ports() {
        let ports = [
            PortType::PrimaryAtaData,
            PortType::PrimaryAtaError,
            PortType::PrimaryAtaSectorCount,
            PortType::PrimaryAtaLbaLow,
            PortType::PrimaryAtaLbaMid,
            PortType::PrimaryAtaLbaHigh,
            PortType::PrimaryAtaDriveSelect,
            PortType::PrimaryAtaCommandStatus,
        ];
        for (offset, port) in ports.into_iter().enumerate() {
            assert_eq!(port as u16, 0x1F0 + offset as u16);
        }
        assert_eq!(PortType::PrimaryAtaDeviceControl as u16, 0x3F6);
    }
}
