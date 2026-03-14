#include <drivers/pit.hpp>
#include "test.hpp"

using namespace cassio;
using namespace cassio::drivers;
using namespace cassio::hardware;

TEST(pit_port_constants) {
    ASSERT_EQ(static_cast<u32>(PortType::PitChannel0Data), 0x40u);
    ASSERT_EQ(static_cast<u32>(PortType::PitCommand), 0x43u);
}

TEST(pit_driver_type) {
    ASSERT_EQ(static_cast<u32>(DriverType::SystemTimer), 0x20u);
}

TEST(pit_constants) {
    ASSERT_EQ(static_cast<u32>(PIT_FREQUENCY), 100u);
    ASSERT_EQ(static_cast<u32>(PIT_CMD_CHANNEL0_MODE2), 0x34u);
}

TEST(pit_ticks_increment) {
    PitTimer pit;
    u32 before = pit.getTicks();
    pit.handleInterrupt(0);
    u32 after = pit.getTicks();
    ASSERT_EQ(after, before + 1);
}
