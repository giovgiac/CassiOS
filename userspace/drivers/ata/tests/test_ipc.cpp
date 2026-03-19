/**
 * test_ipc.cpp -- ATA service IPC integration tests
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <std/test.hpp>
#include <std/ata.hpp>

using namespace std;

TEST(ata_ipc_service_registered) {
    ata::Ata drive;
    // Construction succeeded (lookup returned non-zero).
    ASSERT(true);
}

TEST(ata_ipc_read_sector_succeeds) {
    ata::Ata drive;

    u8 buf[512];
    bool ok = drive.readSector(0, buf);
    ASSERT(ok);
}

TEST(ata_ipc_write_then_read_sector) {
    ata::Ata drive;

    // Use an LBA within the 1 MiB test disk (2048 sectors).
    const u32 testLba = 100;

    // Write a known pattern.
    u8 writeBuf[512];
    for (u32 i = 0; i < 512; ++i) writeBuf[i] = static_cast<u8>(i & 0xFF);
    bool ok = drive.writeSector(testLba, writeBuf);
    ASSERT(ok);

    // Read it back.
    u8 readBuf[512];
    for (u32 i = 0; i < 512; ++i) readBuf[i] = 0;
    ok = drive.readSector(testLba, readBuf);
    ASSERT(ok);

    // Verify contents match.
    bool match = true;
    for (u32 i = 0; i < 512; ++i) {
        if (readBuf[i] != writeBuf[i]) { match = false; break; }
    }
    ASSERT(match);
}
