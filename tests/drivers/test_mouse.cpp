#include <drivers/mouse.hpp>
#include "test.hpp"

using namespace cassio;
using namespace cassio::drivers;

TEST(mouse_command_constants) {
    ASSERT_EQ(static_cast<u32>(MouseCommand::ReadCommand), 0x20u);
    ASSERT_EQ(static_cast<u32>(MouseCommand::WriteCommand), 0x60u);
    ASSERT_EQ(static_cast<u32>(MouseCommand::EnableMouse), 0xA8u);
    ASSERT_EQ(static_cast<u32>(MouseCommand::WriteMouse), 0xD4u);
}

TEST(mouse_event_handler_construction) {
    MouseEventHandler handler;
    // Default construction should not crash
    ASSERT(true);
}

TEST(mouse_singleton_access) {
    MouseDriver& mouse = MouseDriver::getDriver();
    MouseDriver& mouse2 = MouseDriver::getDriver();
    ASSERT_EQ(reinterpret_cast<u32>(&mouse), reinterpret_cast<u32>(&mouse2));
}
