/**
 * ata.hpp -- ATA service client
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Instance-based client for the ATA disk driver service. The
 * constructor resolves the service PID from the nameserver
 * automatically, blocking until the service is registered.
 *
 */

#ifndef STD_ATA_HPP
#define STD_ATA_HPP

#include <std/types.hpp>

namespace std {
namespace ata {

/// Sector size in bytes (standard ATA).
constexpr u32 SECTOR_SIZE = 512;

class Ata {
  public:
    /// Construct an ATA client. Blocks until the "ata" service is
    /// registered with the nameserver.
    Ata();

    /// Read a single 512-byte sector at the given LBA into buf.
    /// Returns true on success, false on failure.
    bool readSector(u32 lba, u8* buf);

    /// Write a single 512-byte sector from buf to the given LBA.
    /// Returns true on success, false on failure.
    bool writeSector(u32 lba, const u8* buf);

    Ata(const Ata&) = delete;
    Ata& operator=(const Ata&) = delete;

  private:
    u32 pid;
};

} // namespace ata
} // namespace std

#endif // STD_ATA_HPP
