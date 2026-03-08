#include <drivers/keyboard.hpp>
#include <drivers/mouse.hpp>
#include "test.hpp"

using namespace cassio;
using namespace cassio::drivers;
using namespace cassio::hardware;

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
    ASSERT_EQ(static_cast<u32>(KeyCode::Escape), 0x1Bu);
}

TEST(keyboard_scancode_values) {
    ASSERT_EQ(static_cast<u32>(ScanCode::Escape), 0x01u);
    ASSERT_EQ(static_cast<u32>(ScanCode::One), 0x02u);
    ASSERT_EQ(static_cast<u32>(ScanCode::Enter), 0x1Cu);
    ASSERT_EQ(static_cast<u32>(ScanCode::LeftShift), 0x2Au);
    ASSERT_EQ(static_cast<u32>(ScanCode::RightShift), 0x36u);
    ASSERT_EQ(static_cast<u32>(ScanCode::LeftCtrl), 0x1Du);
    ASSERT_EQ(static_cast<u32>(ScanCode::LeftAlt), 0x38u);
    ASSERT_EQ(static_cast<u32>(ScanCode::CapsLock), 0x3Au);
    ASSERT_EQ(static_cast<u32>(ScanCode::F1), 0x3Bu);
    ASSERT_EQ(static_cast<u32>(ScanCode::F12), 0x58u);
    ASSERT_EQ(static_cast<u32>(ScanCode::Space), 0x39u);
}

TEST(keyboard_function_key_codes) {
    ASSERT_EQ(static_cast<u32>(KeyCode::F1), 0x80u);
    ASSERT_EQ(static_cast<u32>(KeyCode::F12), 0x8Bu);
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

TEST(keyboard_irq_enabled_after_activate) {
    // Regression: keyboard.activate() must drain the ACK from 0xF4, otherwise
    // mouse.activate() reads it as the command byte and disables keyboard IRQs.
    KeyboardEventHandler kbd_handler;
    MouseEventHandler mouse_handler;
    KeyboardDriver kbd(&kbd_handler);
    MouseDriver mouse(&mouse_handler);

    kbd.activate();
    mouse.activate();

    // Read back the PS/2 controller command byte
    Port<u8> cmd(PortType::KeyboardControllerCommand);
    Port<u8> data(PortType::KeyboardControllerData);
    cmd.write(KeyboardCommand::ReadCommandByte);
    u8 status = data.read();

    // Keyboard interrupt bit (bit 0) must still be enabled
    ASSERT(status & 0x01);
}
