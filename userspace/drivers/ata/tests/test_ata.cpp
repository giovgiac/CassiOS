/**
 * test_ata.cpp -- ATA constant unit tests
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <test.hpp>
#include <port.hpp>
#include <ata.hpp>

using namespace cassio;
using namespace cassio::ata;
using namespace cassio::hardware;

TEST(ata_port_constants) {
    ASSERT_EQ(static_cast<u32>(PortType::PrimaryAtaData), 0x1F0u);
    ASSERT_EQ(static_cast<u32>(PortType::PrimaryAtaError), 0x1F1u);
    ASSERT_EQ(static_cast<u32>(PortType::PrimaryAtaSectorCount), 0x1F2u);
    ASSERT_EQ(static_cast<u32>(PortType::PrimaryAtaLbaLow), 0x1F3u);
    ASSERT_EQ(static_cast<u32>(PortType::PrimaryAtaLbaMid), 0x1F4u);
    ASSERT_EQ(static_cast<u32>(PortType::PrimaryAtaLbaHigh), 0x1F5u);
    ASSERT_EQ(static_cast<u32>(PortType::PrimaryAtaDriveSelect), 0x1F6u);
    ASSERT_EQ(static_cast<u32>(PortType::PrimaryAtaCommandStatus), 0x1F7u);
    ASSERT_EQ(static_cast<u32>(PortType::PrimaryAtaDeviceControl), 0x3F6u);
}

TEST(ata_command_constants) {
    ASSERT_EQ(static_cast<u32>(Command::ReadSectors), 0x20u);
    ASSERT_EQ(static_cast<u32>(Command::WriteSectors), 0x30u);
    ASSERT_EQ(static_cast<u32>(Command::CacheFlush), 0xE7u);
    ASSERT_EQ(static_cast<u32>(Command::Identify), 0xECu);
}

TEST(ata_status_constants) {
    ASSERT_EQ(static_cast<u32>(Status::Bsy), 0x80u);
    ASSERT_EQ(static_cast<u32>(Status::Drq), 0x08u);
    ASSERT_EQ(static_cast<u32>(Status::Err), 0x01u);
    ASSERT_EQ(static_cast<u32>(Status::Df), 0x20u);
    ASSERT_EQ(static_cast<u32>(Status::Rdy), 0x40u);
}

TEST(ata_sector_size) {
    ASSERT_EQ(SECTOR_SIZE, 512u);
}
