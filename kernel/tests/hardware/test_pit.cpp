#include <hardware/pit.hpp>
#include <test.hpp>

using namespace cassio;
using namespace cassio::hardware;
using namespace cassio::hardware;

TEST(pit_port_constants) {
    ASSERT_EQ(static_cast<u32>(PortType::PitChannel0Data), 0x40u);
    ASSERT_EQ(static_cast<u32>(PortType::PitCommand), 0x43u);
}

TEST(pit_divisor_in_range) {
    // The 16-bit divisor must produce a valid frequency from the PIT base clock.
    ASSERT(PIT_DIVISOR > 0);
    ASSERT(PIT_DIVISOR <= 0xFFFF);
    ASSERT_EQ(static_cast<u32>(PIT_DIVISOR),
              PIT_BASE_FREQUENCY / PIT_FREQUENCY);
}

TEST(pit_ticks_increment) {
    PitTimer& pit = PitTimer::getTimer();
    u32 before = pit.getTicks();
    pit.handleInterrupt(0);
    u32 after = pit.getTicks();
    ASSERT_EQ(after, before + 1);
}
