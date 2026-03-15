#include <drivers/ata.hpp>
#include "test.hpp"

using namespace cassio;
using namespace cassio::drivers;
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

TEST(ata_driver_type) {
    ASSERT_EQ(static_cast<u32>(DriverType::PrimaryAdvancedTechnologyAttachment), 0x2Eu);
}

TEST(ata_constants) {
    ASSERT_EQ(static_cast<u32>(AtaCommand::ReadSectors), 0x20u);
    ASSERT_EQ(static_cast<u32>(AtaCommand::WriteSectors), 0x30u);
    ASSERT_EQ(static_cast<u32>(AtaCommand::CacheFlush), 0xE7u);
    ASSERT_EQ(static_cast<u32>(AtaCommand::Identify), 0xECu);
    ASSERT_EQ(static_cast<u32>(ATA_SECTOR_SIZE), 512u);
}

TEST(ata_drive_detected) {
    AtaPioDriver& ata = AtaPioDriver::getDriver();
    ata.activate();
    ASSERT(ata.isPresent());
    ASSERT(ata.getSectors() > 0);
}

TEST(ata_read_write_sector) {
    AtaPioDriver& ata = AtaPioDriver::getDriver();
    if (!ata.isPresent()) ata.activate();

    // Write a known pattern to sector 0.
    u8 write_buf[ATA_SECTOR_SIZE];
    for (u32 i = 0; i < ATA_SECTOR_SIZE; ++i) {
        write_buf[i] = static_cast<u8>(i & 0xFF);
    }

    ASSERT(ata.writeSector(0, write_buf));

    // Read it back.
    u8 read_buf[ATA_SECTOR_SIZE];
    ASSERT(ata.readSector(0, read_buf));

    // Compare.
    for (u32 i = 0; i < ATA_SECTOR_SIZE; ++i) {
        ASSERT_EQ(write_buf[i], read_buf[i]);
    }
}

TEST(ata_read_write_nonzero_lba) {
    AtaPioDriver& ata = AtaPioDriver::getDriver();
    if (!ata.isPresent()) ata.activate();

    // Write a different pattern to sector 1.
    u8 write_buf[ATA_SECTOR_SIZE];
    for (u32 i = 0; i < ATA_SECTOR_SIZE; ++i) {
        write_buf[i] = static_cast<u8>(0xFF - (i & 0xFF));
    }

    ASSERT(ata.writeSector(1, write_buf));

    u8 read_buf[ATA_SECTOR_SIZE];
    ASSERT(ata.readSector(1, read_buf));

    for (u32 i = 0; i < ATA_SECTOR_SIZE; ++i) {
        ASSERT_EQ(write_buf[i], read_buf[i]);
    }
}

TEST(ata_read_out_of_bounds) {
    AtaPioDriver& ata = AtaPioDriver::getDriver();
    if (!ata.isPresent()) ata.activate();

    u8 buf[ATA_SECTOR_SIZE];
    // Sector beyond the drive capacity should fail.
    ASSERT(!ata.readSector(ata.getSectors(), buf));
}
