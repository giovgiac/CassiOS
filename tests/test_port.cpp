#include <hardware/port.hpp>
#include "test.hpp"

using namespace cassio;
using namespace cassio::hardware;

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
    // Verify key PortType enum values match expected I/O addresses
    ASSERT_EQ(static_cast<u32>(PortType::KeyboardControllerData), 0x60u);
    ASSERT_EQ(static_cast<u32>(PortType::SerialCOM1Data), 0x3F8u);
    ASSERT_EQ(static_cast<u32>(PortType::QemuDebugExit), 0xF4u);
    ASSERT_EQ(static_cast<u32>(PortType::MasterProgrammableInterfaceControllerCommand), 0x20u);
}
