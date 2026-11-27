// drive.rs -- ATA PIO access to the primary master drive
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// 28-bit LBA, one sector per command, polled with the drive's interrupt
// disabled (nIEN). Every wait is bounded so a missing or failing drive
// cannot hang the driver.

pub const SECTOR_SIZE: usize = 512;

// Status register bits.
pub const STATUS_ERR: u8 = 0x01;
pub const STATUS_DRQ: u8 = 0x08;
pub const STATUS_DF: u8 = 0x20;
pub const STATUS_RDY: u8 = 0x40;
pub const STATUS_BSY: u8 = 0x80;

/// Error register bit: the drive aborted the command (e.g. unsupported).
pub const ERROR_ABRT: u8 = 0x04;

// Commands.
pub const CMD_READ_SECTORS: u8 = 0x20;
pub const CMD_WRITE_SECTORS: u8 = 0x30;
pub const CMD_CACHE_FLUSH: u8 = 0xE7;
pub const CMD_IDENTIFY: u8 = 0xEC;

/// Drive/head register value selecting the master drive in LBA mode.
pub const MASTER_LBA: u8 = 0xE0;

/// The drive/head, LBA low, LBA mid and LBA high register values that
/// address `lba`.
pub fn lba_registers(lba: u32) -> [u8; 4] {
    [
        MASTER_LBA | ((lba >> 24) & 0x0F) as u8,
        lba as u8,
        (lba >> 8) as u8,
        (lba >> 16) as u8,
    ]
}

/// The sector count from IDENTIFY words 60 (low) and 61 (high).
pub fn sector_count(low: u16, high: u16) -> u32 {
    low as u32 | (high as u32) << 16
}

/// Whether a status byte read right after IDENTIFY means no drive: 0 when
/// the channel has no device, 0xFF when the bus floats.
pub fn no_drive(status: u8) -> bool {
    status == 0 || status == 0xFF
}

/// Whether a write succeeded, given the status after its data phase and
/// the status and error registers after the following cache flush. A
/// flush the drive aborts is unsupported (optional before ATA-6), which
/// does not undo the write.
pub fn write_succeeded(written: u8, flushed: u8, flush_error: u8) -> bool {
    let failed = |status: u8| status & (STATUS_ERR | STATUS_DF) != 0;
    if failed(written) {
        return false;
    }
    !failed(flushed) || (flushed & STATUS_DF == 0 && flush_error & ERROR_ABRT != 0)
}

#[cfg(target_arch = "x86")]
pub use imp::Drive;

#[cfg(target_arch = "x86")]
mod imp {
    use cassio::io::{Port, PortType};

    use super::*;

    // Status polls before giving up (generous: a real disk can take
    // seconds to spin up).
    const TIMEOUT: u32 = 1_000_000;

    /// Device control bit that disables the drive's interrupt.
    const CONTROL_NIEN: u8 = 0x02;

    /// IDENTIFY words holding the 28-bit addressable sector count.
    const IDENTIFY_SECTORS_LOW: usize = 60;
    const IDENTIFY_SECTORS_HIGH: usize = 61;

    /// The primary master drive.
    pub struct Drive {
        data: Port<u16>,
        error: Port<u8>,
        sector_count: Port<u8>,
        lba_low: Port<u8>,
        lba_mid: Port<u8>,
        lba_high: Port<u8>,
        drive_select: Port<u8>,
        command: Port<u8>,
        control: Port<u8>,
        /// Addressable sectors; 0 until `identify` finds a drive.
        sectors: u32,
    }

    impl Drive {
        pub const fn new() -> Self {
            Self {
                data: Port::new(PortType::PrimaryAtaData),
                error: Port::new(PortType::PrimaryAtaError),
                sector_count: Port::new(PortType::PrimaryAtaSectorCount),
                lba_low: Port::new(PortType::PrimaryAtaLbaLow),
                lba_mid: Port::new(PortType::PrimaryAtaLbaMid),
                lba_high: Port::new(PortType::PrimaryAtaLbaHigh),
                drive_select: Port::new(PortType::PrimaryAtaDriveSelect),
                command: Port::new(PortType::PrimaryAtaCommandStatus),
                control: Port::new(PortType::PrimaryAtaDeviceControl),
                sectors: 0,
            }
        }

        /// Detect an ATA drive on the primary master and record its size.
        /// Returns false if there is none (or it is ATAPI/SATA).
        pub fn identify(&mut self) -> bool {
            self.sectors = self.try_identify().unwrap_or(0);
            self.sectors != 0
        }

        /// Read sector `lba` into `buf`.
        pub fn read_sector(&self, lba: u32, buf: &mut [u8; SECTOR_SIZE]) -> bool {
            self.try_read(lba, buf).is_some()
        }

        /// Write `buf` to sector `lba` and flush the drive's cache.
        pub fn write_sector(&self, lba: u32, buf: &[u8; SECTOR_SIZE]) -> bool {
            self.try_write(lba, buf).is_some()
        }

        fn try_identify(&self) -> Option<u32> {
            unsafe { self.drive_select.write(MASTER_LBA) };
            self.delay();
            // A floating bus reads 0xFF, which also looks busy; otherwise let
            // a drive still spinning up finish, as it ignores commands
            // while busy.
            if self.status() == 0xFF {
                return None;
            }
            self.wait_not_busy()?;
            unsafe {
                // Polling only: keep the drive from raising IRQ 14.
                self.control.write(CONTROL_NIEN);
                self.sector_count.write(0);
                self.lba_low.write(0);
                self.lba_mid.write(0);
                self.lba_high.write(0);
                self.command.write(CMD_IDENTIFY);
            }
            self.delay();
            if no_drive(self.status()) {
                return None;
            }
            self.wait_not_busy()?;

            // Non-zero signature bytes mean ATAPI or SATA, not ATA.
            if unsafe { self.lba_mid.read() != 0 || self.lba_high.read() != 0 } {
                return None;
            }
            self.wait_data()?;

            // Keep only the sector count words; the user stack is small.
            let (mut low, mut high) = (0, 0);
            for word in 0..SECTOR_SIZE / 2 {
                let value = unsafe { self.data.read() };
                match word {
                    IDENTIFY_SECTORS_LOW => low = value,
                    IDENTIFY_SECTORS_HIGH => high = value,
                    _ => {}
                }
            }
            Some(sector_count(low, high))
        }

        fn try_read(&self, lba: u32, buf: &mut [u8; SECTOR_SIZE]) -> Option<()> {
            self.start(lba, CMD_READ_SECTORS)?;
            for pair in buf.chunks_exact_mut(2) {
                pair.copy_from_slice(&unsafe { self.data.read() }.to_le_bytes());
            }
            Some(())
        }

        fn try_write(&self, lba: u32, buf: &[u8; SECTOR_SIZE]) -> Option<()> {
            self.start(lba, CMD_WRITE_SECTORS)?;
            for pair in buf.chunks_exact(2) {
                unsafe { self.data.write(u16::from_le_bytes([pair[0], pair[1]])) };
            }
            // The drive ignores commands while busy, so let it finish the
            // write before flushing.
            let written = self.wait_not_busy()?;
            unsafe { self.command.write(CMD_CACHE_FLUSH) };
            let flushed = self.wait_not_busy()?;
            let flush_error = unsafe { self.error.read() };
            write_succeeded(written, flushed, flush_error).then_some(())
        }

        /// Address `lba`, issue `command` for one sector, and wait until
        /// the drive is ready to transfer data.
        fn start(&self, lba: u32, command: u8) -> Option<()> {
            if lba >= self.sectors {
                return None;
            }
            let [select, low, mid, high] = lba_registers(lba);
            unsafe { self.drive_select.write(select) };
            self.delay();
            // Commands are only accepted once the drive is idle and ready.
            self.wait_ready()?;
            unsafe {
                self.sector_count.write(1);
                self.lba_low.write(low);
                self.lba_mid.write(mid);
                self.lba_high.write(high);
                self.command.write(command);
            }
            self.delay();
            self.wait_data()
        }

        fn status(&self) -> u8 {
            unsafe { self.command.read() }
        }

        /// Give the drive 400 ns to update its status (four reads of the
        /// alternate status register).
        fn delay(&self) {
            for _ in 0..4 {
                unsafe { self.control.read() };
            }
        }

        fn wait_ready(&self) -> Option<()> {
            (0..TIMEOUT)
                .any(|_| self.status() & (STATUS_BSY | STATUS_RDY) == STATUS_RDY)
                .then_some(())
        }

        fn wait_not_busy(&self) -> Option<u8> {
            (0..TIMEOUT)
                .map(|_| self.status())
                .find(|status| status & STATUS_BSY == 0)
        }

        /// Wait until the drive requests data; None on error or timeout.
        fn wait_data(&self) -> Option<()> {
            let status = (0..TIMEOUT).map(|_| self.status()).find(|status| {
                status & STATUS_BSY == 0 && status & (STATUS_DRQ | STATUS_ERR) != 0
            })?;
            (status & (STATUS_ERR | STATUS_DF) == 0).then_some(())
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn lba_register_split() {
        assert_eq!(lba_registers(0), [0xE0, 0, 0, 0]);
        assert_eq!(lba_registers(0x0ABC_DEF1), [0xEA, 0xF1, 0xDE, 0xBC]);
    }

    #[test]
    fn lba_register_drops_bits_above_28() {
        assert_eq!(lba_registers(0xFFFF_FFFF), [0xEF, 0xFF, 0xFF, 0xFF]);
    }

    #[test]
    fn sector_count_combines_words() {
        assert_eq!(sector_count(0x0000, 0x0001), 0x1_0000);
        assert_eq!(sector_count(0xF000, 0x0000), 0xF000);
        assert_eq!(sector_count(0x5678, 0x1234), 0x1234_5678);
    }

    #[test]
    fn no_drive_status() {
        assert!(no_drive(0x00));
        assert!(no_drive(0xFF));
        assert!(!no_drive(0x58));
    }

    #[test]
    fn write_success() {
        assert!(write_succeeded(0x50, 0x50, 0));
    }

    #[test]
    fn write_error_fails() {
        assert!(!write_succeeded(0x51, 0x50, 0));
        assert!(!write_succeeded(0x70, 0x50, 0));
    }

    #[test]
    fn unsupported_flush_does_not_fail_the_write() {
        assert!(write_succeeded(0x50, 0x51, ERROR_ABRT));
    }

    #[test]
    fn other_flush_errors_fail_the_write() {
        assert!(!write_succeeded(0x50, 0x51, 0x40));
        assert!(!write_succeeded(0x50, 0x71, ERROR_ABRT));
    }

    #[test]
    fn constants_match_the_ata_spec() {
        assert_eq!(SECTOR_SIZE, 512);
        assert_eq!(
            [
                CMD_READ_SECTORS,
                CMD_WRITE_SECTORS,
                CMD_CACHE_FLUSH,
                CMD_IDENTIFY
            ],
            [0x20, 0x30, 0xE7, 0xEC]
        );
        assert_eq!(
            [STATUS_ERR, STATUS_DRQ, STATUS_DF, STATUS_RDY, STATUS_BSY],
            [0x01, 0x08, 0x20, 0x40, 0x80]
        );
    }
}
