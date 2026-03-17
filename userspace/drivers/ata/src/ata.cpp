/**
 * ata.cpp -- ATA PIO driver implementation
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <ata.hpp>

using namespace cassio;
using namespace cassio::ata;
using namespace cassio::hardware;

void Ata::delay400ns() {
    deviceControl.read();
    deviceControl.read();
    deviceControl.read();
    deviceControl.read();
}

u8 Ata::poll() {
    u8 status;
    while ((status = commandStatus.read()) & Status::Bsy) {}
    return status;
}

bool Ata::waitForData() {
    u8 status = poll();
    if (status & Status::Err) return false;
    if (status & Status::Df) return false;
    if (!(status & Status::Drq)) return false;
    return true;
}

void Ata::init() {
    present = false;
    sectors = 0;
    cachedLba = 0;
    cacheValid = false;

    // Select master drive.
    driveSelect.write(MASTER_LBA);
    delay400ns();

    sectorCount.write(0);
    lbaLow.write(0);
    lbaMid.write(0);
    lbaHigh.write(0);

    // Send IDENTIFY command.
    commandStatus.write(Command::Identify);
    delay400ns();

    u8 status = commandStatus.read();
    if (status == 0) {
        return;
    }

    status = poll();

    // Check LBA mid/high are zero (rules out ATAPI/SATA).
    if (lbaMid.read() != 0 || lbaHigh.read() != 0) {
        return;
    }

    // Wait for DRQ or ERR.
    while (!(status & (Status::Drq | Status::Err))) {
        status = commandStatus.read();
    }

    if (status & Status::Err) {
        return;
    }

    // Read 256-word IDENTIFY data.
    u16 identify[256];
    for (u32 i = 0; i < 256; ++i) {
        identify[i] = data.read();
    }

    // Extract 28-bit LBA sector count from words 60-61.
    sectors = static_cast<u32>(identify[60]) |
              (static_cast<u32>(identify[61]) << 16);

    // Extract model string from words 27-46 (40 ASCII chars, byte-swapped).
    for (u32 i = 0; i < 20; ++i) {
        model[i * 2]     = static_cast<char>(identify[27 + i] >> 8);
        model[i * 2 + 1] = static_cast<char>(identify[27 + i] & 0xFF);
    }
    model[40] = '\0';

    // Trim trailing spaces.
    for (i32 i = 39; i >= 0 && model[i] == ' '; --i) {
        model[i] = '\0';
    }

    present = true;
}

void Ata::handleIrq() {
    commandStatus.read();
}

bool Ata::isPresent() const {
    return present;
}

u32 Ata::getSectors() const {
    return sectors;
}

const char* Ata::getModel() const {
    return model;
}

bool Ata::readSectorInternal(u32 lba) {
    if (!present || lba >= sectors) return false;

    driveSelect.write(MASTER_LBA | ((lba >> 24) & 0x0F));
    delay400ns();

    sectorCount.write(1);
    lbaLow.write(static_cast<u8>(lba));
    lbaMid.write(static_cast<u8>(lba >> 8));
    lbaHigh.write(static_cast<u8>(lba >> 16));

    commandStatus.write(Command::ReadSectors);
    delay400ns();

    if (!waitForData()) return false;

    u16* buf16 = reinterpret_cast<u16*>(sectorBuf);
    for (u32 i = 0; i < 256; ++i) {
        buf16[i] = data.read();
    }

    cachedLba = lba;
    cacheValid = true;
    return true;
}

bool Ata::writeSectorInternal(u32 lba) {
    if (!present || lba >= sectors) return false;

    driveSelect.write(MASTER_LBA | ((lba >> 24) & 0x0F));
    delay400ns();

    sectorCount.write(1);
    lbaLow.write(static_cast<u8>(lba));
    lbaMid.write(static_cast<u8>(lba >> 8));
    lbaHigh.write(static_cast<u8>(lba >> 16));

    commandStatus.write(Command::WriteSectors);
    delay400ns();

    if (!waitForData()) return false;

    const u16* buf16 = reinterpret_cast<const u16*>(sectorBuf);
    for (u32 i = 0; i < 256; ++i) {
        data.write(buf16[i]);
    }

    commandStatus.write(Command::CacheFlush);
    poll();

    return true;
}

i32 Ata::read(u32 lba, u32 offset, u8* buf, u32 len) {
    if (offset >= SECTOR_SIZE) return -1;

    // Use cache if available.
    if (!cacheValid || cachedLba != lba) {
        if (!readSectorInternal(lba)) return -1;
    }

    u32 available = SECTOR_SIZE - offset;
    u32 toRead = len < available ? len : available;

    for (u32 i = 0; i < toRead; ++i) {
        buf[i] = sectorBuf[offset + i];
    }

    return static_cast<i32>(toRead);
}

bool Ata::write(u32 lba, u32 offset, const u8* buf, u32 len) {
    if (offset >= SECTOR_SIZE) return false;
    if (offset + len > SECTOR_SIZE) return false;

    // Read-modify-write: load current sector first.
    if (!cacheValid || cachedLba != lba) {
        if (!readSectorInternal(lba)) return false;
    }

    for (u32 i = 0; i < len; ++i) {
        sectorBuf[offset + i] = buf[i];
    }

    return writeSectorInternal(lba);
}
