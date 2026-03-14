/**
 * ata.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "drivers/ata.hpp"
#include "hardware/serial.hpp"

using namespace cassio;
using namespace cassio::drivers;
using namespace cassio::hardware;

AtaPioDriver AtaPioDriver::instance;

AtaPioDriver::AtaPioDriver()
    : Driver(DriverType::PrimaryAdvancedTechnologyAttachment),
      data(PortType::PrimaryAtaData),
      error(PortType::PrimaryAtaError),
      sector_count(PortType::PrimaryAtaSectorCount),
      lba_low(PortType::PrimaryAtaLbaLow),
      lba_mid(PortType::PrimaryAtaLbaMid),
      lba_high(PortType::PrimaryAtaLbaHigh),
      drive_select(PortType::PrimaryAtaDriveSelect),
      command_status(PortType::PrimaryAtaCommandStatus),
      device_control(PortType::PrimaryAtaDeviceControl),
      present(false),
      sectors(0) {
    model[0] = '\0';
}

void AtaPioDriver::delay400ns() {
    // Reading the alternate status register 4 times provides ~400ns delay.
    device_control.read();
    device_control.read();
    device_control.read();
    device_control.read();
}

u8 AtaPioDriver::poll() {
    u8 status;
    while ((status = command_status.read()) & ATA_STATUS_BSY) {
        // Spin until BSY clears.
    }
    return status;
}

bool AtaPioDriver::waitForData() {
    u8 status = poll();
    if (status & ATA_STATUS_ERR) return false;
    if (status & ATA_STATUS_DF) return false;
    if (!(status & ATA_STATUS_DRQ)) return false;
    return true;
}

void AtaPioDriver::activate() {
    Serial& serial = COM1::getSerial();

    // Select master drive.
    drive_select.write(ATA_MASTER_LBA);
    delay400ns();

    // Zero out the sector count and LBA ports.
    sector_count.write(0);
    lba_low.write(0);
    lba_mid.write(0);
    lba_high.write(0);

    // Send IDENTIFY command.
    command_status.write(ATA_CMD_IDENTIFY);
    delay400ns();

    // Check if drive exists.
    u8 status = command_status.read();
    if (status == 0) {
        serial.puts("ATA: no drive detected on primary channel\n");
        present = false;
        return;
    }

    // Wait for BSY to clear.
    status = poll();

    // Check that LBA mid and high are still zero (rules out ATAPI/SATA).
    if (lba_mid.read() != 0 || lba_high.read() != 0) {
        serial.puts("ATA: non-ATA device on primary channel\n");
        present = false;
        return;
    }

    // Wait for DRQ or ERR.
    while (!(status & (ATA_STATUS_DRQ | ATA_STATUS_ERR))) {
        status = command_status.read();
    }

    if (status & ATA_STATUS_ERR) {
        serial.puts("ATA: IDENTIFY command returned error\n");
        present = false;
        return;
    }

    // Read the 256-word IDENTIFY data.
    u16 identify[256];
    for (u32 i = 0; i < 256; ++i) {
        identify[i] = data.read();
    }

    // Extract 28-bit LBA sector count from words 60-61.
    sectors = static_cast<u32>(identify[60]) | (static_cast<u32>(identify[61]) << 16);

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

    serial.puts("ATA: detected drive: ");
    serial.puts(model);
    serial.puts(" (");
    serial.put_dec(sectors);
    serial.puts(" sectors)\n");
}

void AtaPioDriver::deactivate() {}

u32 AtaPioDriver::handleInterrupt(u32 esp) {
    // Read status to acknowledge the IRQ.
    command_status.read();
    return esp;
}

bool AtaPioDriver::isPresent() const {
    return present;
}

u32 AtaPioDriver::getSectors() const {
    return sectors;
}

const char* AtaPioDriver::getModel() const {
    return model;
}

bool AtaPioDriver::readSector(u32 lba, u8* buffer) {
    if (!present) return false;
    if (lba >= sectors) return false;

    // Select master, LBA mode, bits 24-27 of LBA.
    drive_select.write(ATA_MASTER_LBA | ((lba >> 24) & 0x0F));
    delay400ns();

    sector_count.write(1);
    lba_low.write(static_cast<u8>(lba));
    lba_mid.write(static_cast<u8>(lba >> 8));
    lba_high.write(static_cast<u8>(lba >> 16));

    command_status.write(ATA_CMD_READ_SECTORS);

    delay400ns();

    if (!waitForData()) return false;

    // Read 256 words (512 bytes).
    u16* buf16 = reinterpret_cast<u16*>(buffer);
    for (u32 i = 0; i < 256; ++i) {
        buf16[i] = data.read();
    }

    return true;
}

bool AtaPioDriver::writeSector(u32 lba, const u8* buffer) {
    if (!present) return false;
    if (lba >= sectors) return false;

    // Select master, LBA mode, bits 24-27 of LBA.
    drive_select.write(ATA_MASTER_LBA | ((lba >> 24) & 0x0F));
    delay400ns();

    sector_count.write(1);
    lba_low.write(static_cast<u8>(lba));
    lba_mid.write(static_cast<u8>(lba >> 8));
    lba_high.write(static_cast<u8>(lba >> 16));

    command_status.write(ATA_CMD_WRITE_SECTORS);

    delay400ns();

    if (!waitForData()) return false;

    // Write 256 words (512 bytes).
    const u16* buf16 = reinterpret_cast<const u16*>(buffer);
    for (u32 i = 0; i < 256; ++i) {
        data.write(buf16[i]);
    }

    // Flush the write cache.
    command_status.write(ATA_CMD_CACHE_FLUSH);
    poll();

    return true;
}
