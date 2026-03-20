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

#include <std/io.hpp>
#include <std/types.hpp>

namespace cassio {
namespace ata {

namespace Status {
constexpr std::u8 Err = 0x01;
constexpr std::u8 Drq = 0x08;
constexpr std::u8 Df = 0x20;
constexpr std::u8 Rdy = 0x40;
constexpr std::u8 Bsy = 0x80;
} // namespace Status

namespace Command {
constexpr std::u8 ReadSectors = 0x20;
constexpr std::u8 WriteSectors = 0x30;
constexpr std::u8 CacheFlush = 0xE7;
constexpr std::u8 Identify = 0xEC;
} // namespace Command

constexpr std::u32 SECTOR_SIZE = 512;
constexpr std::u8 MASTER_LBA = 0xE0;

class Ata {
private:
    std::io::Port<std::u16> data{std::io::PortType::PrimaryAtaData};
    std::io::Port<std::u8> error{std::io::PortType::PrimaryAtaError};
    std::io::Port<std::u8> sectorCount{std::io::PortType::PrimaryAtaSectorCount};
    std::io::Port<std::u8> lbaLow{std::io::PortType::PrimaryAtaLbaLow};
    std::io::Port<std::u8> lbaMid{std::io::PortType::PrimaryAtaLbaMid};
    std::io::Port<std::u8> lbaHigh{std::io::PortType::PrimaryAtaLbaHigh};
    std::io::Port<std::u8> driveSelect{std::io::PortType::PrimaryAtaDriveSelect};
    std::io::Port<std::u8> commandStatus{std::io::PortType::PrimaryAtaCommandStatus};
    std::io::Port<std::u8> deviceControl{std::io::PortType::PrimaryAtaDeviceControl};

    bool present;
    std::u32 sectors;
    char model[41];

    std::u8 sectorBuf[SECTOR_SIZE];
    std::u32 cachedLba;
    bool cacheValid;

    std::u8 poll();
    bool waitForData();
    void delay400ns();
    bool readSectorInternal(std::u32 lba);
    bool writeSectorInternal(std::u32 lba);

public:
    Ata() = default;

    void init();
    void handleIrq();

    bool isPresent() const;
    std::u32 getSectors() const;
    const char* getModel() const;

    /**
     * @brief Read a full 512-byte sector into buf.
     *
     * @return true on success.
     *
     */
    bool readSector(std::u32 lba, std::u8* buf);

    /**
     * @brief Write a full 512-byte sector from buf to disk.
     *
     * @return true on success.
     *
     */
    bool writeSector(std::u32 lba, const std::u8* buf);

    Ata(const Ata&) = delete;
    Ata(Ata&&) = delete;
    Ata& operator=(const Ata&) = delete;
    Ata& operator=(Ata&&) = delete;
};

} // namespace ata
} // namespace cassio

#endif // USERSPACE_ATA_ATA_HPP_
