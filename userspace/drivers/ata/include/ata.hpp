/**
 * ata.hpp -- ATA PIO driver for the userspace ATA service
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef USERSPACE_ATA_ATA_HPP_
#define USERSPACE_ATA_ATA_HPP_

#include <types.hpp>
#include <port.hpp>

namespace cassio {
namespace ata {

namespace Status {
    constexpr u8 Err = 0x01;
    constexpr u8 Drq = 0x08;
    constexpr u8 Df  = 0x20;
    constexpr u8 Rdy = 0x40;
    constexpr u8 Bsy = 0x80;
}

namespace Command {
    constexpr u8 ReadSectors  = 0x20;
    constexpr u8 WriteSectors = 0x30;
    constexpr u8 CacheFlush   = 0xE7;
    constexpr u8 Identify     = 0xEC;
}

constexpr u32 SECTOR_SIZE = 512;
constexpr u8 MASTER_LBA = 0xE0;

class Ata {
private:
    hardware::Port<u16> data{hardware::PortType::PrimaryAtaData};
    hardware::Port<u8> error{hardware::PortType::PrimaryAtaError};
    hardware::Port<u8> sectorCount{hardware::PortType::PrimaryAtaSectorCount};
    hardware::Port<u8> lbaLow{hardware::PortType::PrimaryAtaLbaLow};
    hardware::Port<u8> lbaMid{hardware::PortType::PrimaryAtaLbaMid};
    hardware::Port<u8> lbaHigh{hardware::PortType::PrimaryAtaLbaHigh};
    hardware::Port<u8> driveSelect{hardware::PortType::PrimaryAtaDriveSelect};
    hardware::Port<u8> commandStatus{hardware::PortType::PrimaryAtaCommandStatus};
    hardware::Port<u8> deviceControl{hardware::PortType::PrimaryAtaDeviceControl};

    bool present;
    u32 sectors;
    char model[41];

    u8 sectorBuf[SECTOR_SIZE];
    u32 cachedLba;
    bool cacheValid;

    u8 poll();
    bool waitForData();
    void delay400ns();
    bool readSectorInternal(u32 lba);
    bool writeSectorInternal(u32 lba);

public:
    void init();
    void handleIrq();

    bool isPresent() const;
    u32 getSectors() const;
    const char* getModel() const;

    /**
     * @brief Read a full 512-byte sector into buf.
     *
     * @return true on success.
     *
     */
    bool readSector(u32 lba, u8* buf);

    /**
     * @brief Write a full 512-byte sector from buf to disk.
     *
     * @return true on success.
     *
     */
    bool writeSector(u32 lba, const u8* buf);
};

} // ata
} // cassio

#endif // USERSPACE_ATA_ATA_HPP_
