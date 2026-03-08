#include <drivers/keyboard.hpp>
#include "test.hpp"

using namespace cassio;
using namespace cassio::drivers;

TEST(keyboard_command_byte_size) {
    ASSERT_EQ(static_cast<u32>(sizeof(KeyboardCommandByte)), 1u);
}

TEST(keyboard_command_constants) {
    ASSERT_EQ(static_cast<u32>(KeyboardCommand::ReadCommandByte), 0x20u);
    ASSERT_EQ(static_cast<u32>(KeyboardCommand::WriteCommandByte), 0x60u);
    ASSERT_EQ(static_cast<u32>(KeyboardCommand::EnableKeyboardInterface), 0xAEu);
}

TEST(keyboard_keycode_values) {
    ASSERT_EQ(static_cast<u32>(KeyCode::A), 0x41u);
    ASSERT_EQ(static_cast<u32>(KeyCode::Z), 0x5Au);
    ASSERT_EQ(static_cast<u32>(KeyCode::Zero), 0x30u);
    ASSERT_EQ(static_cast<u32>(KeyCode::Nine), 0x39u);
    ASSERT_EQ(static_cast<u32>(KeyCode::Enter), 0x0Du);
    ASSERT_EQ(static_cast<u32>(KeyCode::Space), 0x20u);
}

TEST(keyboard_command_byte_bits) {
    KeyboardCommandByte cb;
    cb.byte = 0;

    cb.keyboard_interrupt = true;
    ASSERT_EQ(static_cast<u32>(cb.byte & 0x01), 1u);

    cb.byte = 0;
    cb.mouse_interrupt = true;
    ASSERT_EQ(static_cast<u32>(cb.byte & 0x02), 2u);
}

TEST(keyboard_driver_construction) {
    KeyboardEventHandler handler;
    KeyboardDriver kbd(&handler);
    // Construction should not crash; driver self-registers with InterruptManager
    ASSERT(true);
}
