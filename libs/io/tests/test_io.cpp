#include <std/io.hpp>
#include <std/test.hpp>

using namespace std;
using namespace std::io;

TEST(port_u8_size) {
    // Port<u8> stores a u16 port number, so sizeof should be 2
    ASSERT_EQ(static_cast<u32>(sizeof(Port<u8>)), 2u);
}

TEST(port_u16_size) {
    ASSERT_EQ(static_cast<u32>(sizeof(Port<u16>)), 2u);
}

TEST(port_u32_size) {
    ASSERT_EQ(static_cast<u32>(sizeof(Port<u32>)), 2u);
}

TEST(port_type_enum_values) {
    // Every value, mirrored in libs/sys/src/io.rs.
    ASSERT_EQ(static_cast<u32>(PortType::MasterPicCommand), 0x20u);
    ASSERT_EQ(static_cast<u32>(PortType::MasterPicData), 0x21u);
    ASSERT_EQ(static_cast<u32>(PortType::PitChannel0Data), 0x40u);
    ASSERT_EQ(static_cast<u32>(PortType::PitCommand), 0x43u);
    ASSERT_EQ(static_cast<u32>(PortType::KbdData), 0x60u);
    ASSERT_EQ(static_cast<u32>(PortType::KbdCommand), 0x64u);
    ASSERT_EQ(static_cast<u32>(PortType::SlavePicCommand), 0xA0u);
    ASSERT_EQ(static_cast<u32>(PortType::SlavePicData), 0xA1u);
    ASSERT_EQ(static_cast<u32>(PortType::PrimaryAtaData), 0x1F0u);
    ASSERT_EQ(static_cast<u32>(PortType::PrimaryAtaError), 0x1F1u);
    ASSERT_EQ(static_cast<u32>(PortType::PrimaryAtaSectorCount), 0x1F2u);
    ASSERT_EQ(static_cast<u32>(PortType::PrimaryAtaLbaLow), 0x1F3u);
    ASSERT_EQ(static_cast<u32>(PortType::PrimaryAtaLbaMid), 0x1F4u);
    ASSERT_EQ(static_cast<u32>(PortType::PrimaryAtaLbaHigh), 0x1F5u);
    ASSERT_EQ(static_cast<u32>(PortType::PrimaryAtaDriveSelect), 0x1F6u);
    ASSERT_EQ(static_cast<u32>(PortType::PrimaryAtaCommandStatus), 0x1F7u);
    ASSERT_EQ(static_cast<u32>(PortType::VgaCrtcIndex), 0x3D4u);
    ASSERT_EQ(static_cast<u32>(PortType::VgaCrtcData), 0x3D5u);
    ASSERT_EQ(static_cast<u32>(PortType::PrimaryAtaDeviceControl), 0x3F6u);
    ASSERT_EQ(static_cast<u32>(PortType::SerialCOM1Data), 0x3F8u);
    ASSERT_EQ(static_cast<u32>(PortType::SerialCOM1InterruptEnable), 0x3F9u);
    ASSERT_EQ(static_cast<u32>(PortType::SerialCOM1FIFOControl), 0x3FAu);
    ASSERT_EQ(static_cast<u32>(PortType::SerialCOM1LineControl), 0x3FBu);
    ASSERT_EQ(static_cast<u32>(PortType::SerialCOM1ModemControl), 0x3FCu);
    ASSERT_EQ(static_cast<u32>(PortType::SerialCOM1LineStatus), 0x3FDu);
    ASSERT_EQ(static_cast<u32>(PortType::QemuDebugExit), 0xF4u);
}
