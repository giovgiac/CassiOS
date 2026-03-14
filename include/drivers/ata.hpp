/**
 * ata.hpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef DRIVERS_ATA_HPP_
#define DRIVERS_ATA_HPP_

#include <common/types.hpp>
#include <hardware/driver.hpp>
#include <hardware/port.hpp>

namespace cassio {
namespace drivers {

// ATA status register bits.
constexpr u8 ATA_STATUS_ERR  = 0x01;
constexpr u8 ATA_STATUS_DRQ  = 0x08;
constexpr u8 ATA_STATUS_SRV  = 0x10;
constexpr u8 ATA_STATUS_DF   = 0x20;
constexpr u8 ATA_STATUS_RDY  = 0x40;
constexpr u8 ATA_STATUS_BSY  = 0x80;

// ATA commands.
constexpr u8 ATA_CMD_READ_SECTORS  = 0x20;
constexpr u8 ATA_CMD_WRITE_SECTORS = 0x30;
constexpr u8 ATA_CMD_CACHE_FLUSH   = 0xE7;
constexpr u8 ATA_CMD_IDENTIFY      = 0xEC;

// Bytes per sector.
constexpr u32 ATA_SECTOR_SIZE = 512;

// Drive select byte for master drive with LBA mode.
constexpr u8 ATA_MASTER_LBA = 0xE0;

/**
 * @brief ATA PIO driver singleton for the primary IDE channel.
 *
 * Reads and writes 512-byte sectors using PIO polling with 28-bit LBA.
 * Supports only the master drive on the primary channel.
 *
 */
class AtaPioDriver : public hardware::Driver {
private:
    hardware::Port<u16> data;
    hardware::Port<u8> error;
    hardware::Port<u8> sector_count;
    hardware::Port<u8> lba_low;
    hardware::Port<u8> lba_mid;
    hardware::Port<u8> lba_high;
    hardware::Port<u8> drive_select;
    hardware::Port<u8> command_status;
    hardware::Port<u8> device_control;

    bool present;
    u32 sectors;
    char model[41];

    static AtaPioDriver instance;

private:
    AtaPioDriver();
    ~AtaPioDriver() = default;

    /**
     * @brief Polls the status register until BSY clears. Returns the final status byte.
     *
     */
    u8 poll();

    /**
     * @brief Waits for DRQ after polling BSY. Returns true on success, false on error.
     *
     */
    bool waitForData();

    /**
     * @brief Performs a 400ns delay by reading the alternate status register.
     *
     */
    void delay400ns();

public:
    /**
     * @brief Returns the AtaPioDriver singleton instance.
     *
     */
    inline static AtaPioDriver& getDriver() {
        return instance;
    }

    /**
     * @brief Identifies the drive and reads its capacity and model string.
     *
     */
    virtual void activate() override;
    virtual void deactivate() override;

    /**
     * @brief Acknowledges the IRQ (PIO polling does not use IRQ-driven transfers).
     *
     */
    virtual u32 handleInterrupt(u32 esp) override;

    /**
     * @brief Returns true if a drive was detected on the primary channel.
     *
     */
    bool isPresent() const;

    /**
     * @brief Returns the total number of 28-bit LBA sectors on the drive.
     *
     */
    u32 getSectors() const;

    /**
     * @brief Returns the drive model string (null-terminated, up to 40 chars).
     *
     */
    const char* getModel() const;

    /**
     * @brief Reads one sector at the given 28-bit LBA into the buffer.
     *
     * @param lba    Logical block address (28-bit).
     * @param buffer Output buffer, must be at least 512 bytes.
     * @return true on success, false on error or no drive.
     *
     */
    bool readSector(u32 lba, u8* buffer);

    /**
     * @brief Writes one sector from the buffer to the given 28-bit LBA.
     *
     * @param lba    Logical block address (28-bit).
     * @param buffer Input buffer, must be at least 512 bytes.
     * @return true on success, false on error or no drive.
     *
     */
    bool writeSector(u32 lba, const u8* buffer);

    /** Deleted Methods */
    AtaPioDriver(const AtaPioDriver&) = delete;
    AtaPioDriver(AtaPioDriver&&) = delete;
    AtaPioDriver& operator=(const AtaPioDriver&) = delete;
    AtaPioDriver& operator=(AtaPioDriver&&) = delete;
};

} // drivers
} // cassio

#endif // DRIVERS_ATA_HPP_
