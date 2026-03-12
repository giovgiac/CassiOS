#include <hardware/terminal.hpp>
#include "test.hpp"

using namespace cassio;
using namespace cassio::hardware;

TEST(terminal_putchar) {
    VgaTerminal& vga = VgaTerminal::getTerminal();
    vga.clear();

    vga.putchar('X');

    u16* buf = reinterpret_cast<u16*>(0xB8000);
    ASSERT_EQ(static_cast<u32>(buf[0] & 0x00FF), static_cast<u32>('X'));
}

TEST(terminal_preserves_attribute) {
    VgaTerminal& vga = VgaTerminal::getTerminal();
    vga.clear();

    u16* buf = reinterpret_cast<u16*>(0xB8000);
    u16 attr_before = buf[0] & 0xFF00;

    vga.putchar('A');

    u16 attr_after = buf[0] & 0xFF00;
    ASSERT_EQ(static_cast<u32>(attr_before), static_cast<u32>(attr_after));
}

TEST(terminal_clear) {
    VgaTerminal& vga = VgaTerminal::getTerminal();

    // Write some characters first.
    vga.putchar('H');
    vga.putchar('i');

    vga.clear();

    u16* buf = reinterpret_cast<u16*>(0xB8000);

    // First few cells should be spaces.
    for (u32 i = 0; i < 10; ++i) {
        ASSERT_EQ(static_cast<u32>(buf[i] & 0x00FF), static_cast<u32>(' '));
    }

    // Cursor should be at (0,0) -- next putchar writes to buf[0].
    vga.putchar('Z');
    ASSERT_EQ(static_cast<u32>(buf[0] & 0x00FF), static_cast<u32>('Z'));
}

TEST(terminal_newline) {
    VgaTerminal& vga = VgaTerminal::getTerminal();
    vga.clear();

    vga.putchar('\n');

    // Cursor should be at row 1, col 0. Next char goes to buf[80].
    vga.putchar('N');

    u16* buf = reinterpret_cast<u16*>(0xB8000);
    ASSERT_EQ(static_cast<u32>(buf[VGA_WIDTH] & 0x00FF), static_cast<u32>('N'));
}

TEST(terminal_backspace) {
    VgaTerminal& vga = VgaTerminal::getTerminal();
    vga.clear();

    vga.putchar('A');
    vga.putchar('B');
    vga.putchar('\b');

    // Backspace should have erased 'B' (replaced with space) and moved cursor back.
    u16* buf = reinterpret_cast<u16*>(0xB8000);
    ASSERT_EQ(static_cast<u32>(buf[1] & 0x00FF), static_cast<u32>(' '));

    // Next char should overwrite position 1.
    vga.putchar('C');
    ASSERT_EQ(static_cast<u32>(buf[1] & 0x00FF), static_cast<u32>('C'));
}

TEST(terminal_get_cursor) {
    VgaTerminal& vga = VgaTerminal::getTerminal();
    vga.clear();

    ASSERT_EQ(static_cast<u32>(vga.getCursorX()), 0u);
    ASSERT_EQ(static_cast<u32>(vga.getCursorY()), 0u);

    vga.putchar('A');
    ASSERT_EQ(static_cast<u32>(vga.getCursorX()), 1u);
    ASSERT_EQ(static_cast<u32>(vga.getCursorY()), 0u);
}

TEST(terminal_scroll) {
    VgaTerminal& vga = VgaTerminal::getTerminal();
    vga.clear();

    u16* buf = reinterpret_cast<u16*>(0xB8000);

    // Place 'Z' on row 1, then trigger a scroll from the last row.
    vga.setCursor(0, 1);
    vga.putchar('Z');
    vga.setCursor(0, VGA_HEIGHT - 1);
    vga.putchar('\n');

    // 'Z' should have moved from row 1 to row 0.
    ASSERT_EQ(static_cast<u32>(buf[0] & 0x00FF), static_cast<u32>('Z'));

    // Last row should be blank.
    ASSERT_EQ(static_cast<u32>(buf[VGA_WIDTH * (VGA_HEIGHT - 1)] & 0x00FF), static_cast<u32>(' '));

    // Cursor should be at column 0 of the last row.
    ASSERT_EQ(static_cast<u32>(vga.getCursorY()), static_cast<u32>(VGA_HEIGHT - 1));
    ASSERT_EQ(static_cast<u32>(vga.getCursorX()), 0u);
}

TEST(terminal_set_cursor) {
    VgaTerminal& vga = VgaTerminal::getTerminal();
    vga.clear();

    vga.setCursor(10, 5);
    ASSERT_EQ(static_cast<u32>(vga.getCursorX()), 10u);
    ASSERT_EQ(static_cast<u32>(vga.getCursorY()), 5u);

    // Writing at the new position should place the character there.
    vga.putchar('Q');
    u16* buf = reinterpret_cast<u16*>(0xB8000);
    ASSERT_EQ(static_cast<u32>(buf[5 * VGA_WIDTH + 10] & 0x00FF), static_cast<u32>('Q'));
}
