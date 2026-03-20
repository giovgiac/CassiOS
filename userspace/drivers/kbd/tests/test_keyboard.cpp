#include <std/test.hpp>

#include <keyboard.hpp>

using namespace cassio;
using namespace std;

TEST(kbd_scancode_to_keycode) {
    Keyboard kb;
    // 'a' scancode = 0x1E, press (no release bit)
    kb.handleScancode(0x1E);
    ASSERT_EQ(kb.readBuffer(), 'a');
}

TEST(kbd_shift_letter_uppercase) {
    Keyboard kb;
    kb.handleScancode(0x2A); // Left Shift down
    kb.handleScancode(0x1E); // 'a' -> 'A'
    ASSERT_EQ(kb.readBuffer(), 'A');
}

TEST(kbd_shift_symbol) {
    Keyboard kb;
    kb.handleScancode(0x2A); // Left Shift down
    kb.handleScancode(0x02); // '1' -> '!'
    ASSERT_EQ(kb.readBuffer(), '!');
}

TEST(kbd_caps_lock_toggle) {
    Keyboard kb;
    kb.handleScancode(0x3A); // Caps Lock on
    kb.handleScancode(0x1E); // 'a' -> 'A'
    ASSERT_EQ(kb.readBuffer(), 'A');

    // Caps Lock + Shift = lowercase
    kb.handleScancode(0x2A); // Left Shift down
    kb.handleScancode(0x1E); // 'A' back to 'a' (shift XOR caps)
    ASSERT_EQ(kb.readBuffer(), 'a');
}

TEST(kbd_release_ignored) {
    Keyboard kb;
    kb.handleScancode(0x1E);        // 'a' press
    kb.handleScancode(0x1E | 0x80); // 'a' release
    // Only one character should be buffered.
    ASSERT_EQ(kb.readBuffer(), 'a');
    ASSERT_EQ(kb.readBuffer(), '\0');
}

TEST(kbd_e0_prefix_arrow_buffered) {
    Keyboard kb;
    kb.handleScancode(0xE0); // Extended prefix
    kb.handleScancode(0x4B); // Left Arrow
    ASSERT_EQ(static_cast<u8>(kb.readBuffer()), static_cast<u8>(KeyCode::LeftArrow));
}

TEST(kbd_enter_buffers_keycode) {
    Keyboard kb;
    kb.handleScancode(0x1C); // Enter
    ASSERT_EQ(static_cast<u8>(kb.readBuffer()), static_cast<u8>(KeyCode::Enter));
}

TEST(kbd_backspace_buffered) {
    Keyboard kb;
    kb.handleScancode(0x0E); // Backspace
    ASSERT_EQ(static_cast<u8>(kb.readBuffer()), static_cast<u8>(KeyCode::Backspace));
}

TEST(kbd_buffer_empty_returns_null) {
    Keyboard kb;
    ASSERT_EQ(kb.readBuffer(), '\0');
}

TEST(kbd_buffer_multiple_keys) {
    Keyboard kb;
    kb.handleScancode(0x23); // 'h'
    kb.handleScancode(0x17); // 'i'
    ASSERT_EQ(kb.readBuffer(), 'h');
    ASSERT_EQ(kb.readBuffer(), 'i');
    ASSERT_EQ(kb.readBuffer(), '\0');
}

TEST(kbd_shift_resolves_all_symbols) {
    ASSERT_EQ(static_cast<u8>(Keyboard::resolveShift(KeyCode::One)),
              static_cast<u8>(KeyCode::Exclamation));
    ASSERT_EQ(static_cast<u8>(Keyboard::resolveShift(KeyCode::Two)), static_cast<u8>(KeyCode::At));
    ASSERT_EQ(static_cast<u8>(Keyboard::resolveShift(KeyCode::Nine)),
              static_cast<u8>(KeyCode::LeftParenthesis));
    ASSERT_EQ(static_cast<u8>(Keyboard::resolveShift(KeyCode::Minus)),
              static_cast<u8>(KeyCode::Underscore));
    ASSERT_EQ(static_cast<u8>(Keyboard::resolveShift(KeyCode::Equals)),
              static_cast<u8>(KeyCode::Plus));
    ASSERT_EQ(static_cast<u8>(Keyboard::resolveShift(KeyCode::LeftBracket)),
              static_cast<u8>(KeyCode::LeftCurly));
    ASSERT_EQ(static_cast<u8>(Keyboard::resolveShift(KeyCode::Semicolon)),
              static_cast<u8>(KeyCode::Colon));
    ASSERT_EQ(static_cast<u8>(Keyboard::resolveShift(KeyCode::Quote)),
              static_cast<u8>(KeyCode::DoubleQuote));
    ASSERT_EQ(static_cast<u8>(Keyboard::resolveShift(KeyCode::Comma)),
              static_cast<u8>(KeyCode::LessThan));
    ASSERT_EQ(static_cast<u8>(Keyboard::resolveShift(KeyCode::Period)),
              static_cast<u8>(KeyCode::GreaterThan));
    ASSERT_EQ(static_cast<u8>(Keyboard::resolveShift(KeyCode::Slash)),
              static_cast<u8>(KeyCode::Question));
}

TEST(kbd_modifier_release_clears_state) {
    Keyboard kb;
    kb.handleScancode(0x2A);        // Left Shift down
    kb.handleScancode(0x2A | 0x80); // Left Shift up
    kb.handleScancode(0x1E);        // 'a' (should be lowercase now)
    ASSERT_EQ(kb.readBuffer(), 'a');
}

TEST(kbd_function_key_buffered) {
    Keyboard kb;
    kb.handleScancode(0x3B); // F1
    ASSERT_EQ(static_cast<u8>(kb.readBuffer()), static_cast<u8>(KeyCode::F1));
}
