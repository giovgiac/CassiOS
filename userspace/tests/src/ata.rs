// ata.rs -- ATA service tests
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.

use cassio::ata::{Ata, SECTOR_SIZE};

use crate::harness::Outcome;

tests!(ata_read_sector_succeeds, ata_write_then_read_sector);

/// The last sector of the 32 MiB test disk, past the FATs. The test puts
/// its original contents back.
const TEST_LBA: u32 = 32 * 1024 * 2 - 1;

fn ata_read_sector_succeeds() -> Outcome {
    let mut sector = [0u8; SECTOR_SIZE];
    check!(Ata::connect().read_sector(0, &mut sector));
    // A FAT32 boot sector ends with the 0x55AA signature.
    check_eq!((sector[510], sector[511]), (0x55, 0xAA));
    Ok(())
}

fn ata_write_then_read_sector() -> Outcome {
    let ata = Ata::connect();
    let mut original = [0u8; SECTOR_SIZE];
    check!(ata.read_sector(TEST_LBA, &mut original));

    let mut pattern = [0u8; SECTOR_SIZE];
    for (i, byte) in pattern.iter_mut().enumerate() {
        *byte = i as u8;
    }
    check!(ata.write_sector(TEST_LBA, &pattern));
    let mut read = [0u8; SECTOR_SIZE];
    check!(ata.read_sector(TEST_LBA, &mut read));
    let matches = read == pattern;

    check!(ata.write_sector(TEST_LBA, &original));
    check!(matches);
    Ok(())
}
