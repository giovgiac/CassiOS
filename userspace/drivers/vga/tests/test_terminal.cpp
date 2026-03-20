/**
 * test_terminal.cpp -- VgaTerminal unit tests
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <std/test.hpp>

#include <terminal.hpp>

using namespace cassio;
using namespace std;

static u16 test_buf[80 * 25];

static VgaTerminal make_terminal() {
    VgaTerminal t(test_buf);
    t.clear();
    return t;
}

TEST(vga_putchar) {
    VgaTerminal t = make_terminal();

    t.putchar('X');

    ASSERT_EQ(static_cast<u32>(test_buf[0] & 0x00FF), static_cast<u32>('X'));
}

TEST(vga_preserves_attribute) {
    VgaTerminal t = make_terminal();

    u16 attr_before = test_buf[0] & 0xFF00;
    t.putchar('A');
    u16 attr_after = test_buf[0] & 0xFF00;

    ASSERT_EQ(static_cast<u32>(attr_before), static_cast<u32>(attr_after));
}

TEST(vga_clear) {
    VgaTerminal t = make_terminal();

    t.putchar('H');
    t.putchar('i');
    t.clear();

    for (u32 i = 0; i < 10; ++i) {
        ASSERT_EQ(static_cast<u32>(test_buf[i] & 0x00FF), static_cast<u32>(' '));
    }

    t.putchar('Z');
    ASSERT_EQ(static_cast<u32>(test_buf[0] & 0x00FF), static_cast<u32>('Z'));
}

TEST(vga_newline) {
    VgaTerminal t = make_terminal();

    t.putchar('\n');
    t.putchar('N');

    ASSERT_EQ(static_cast<u32>(test_buf[VGA_WIDTH] & 0x00FF), static_cast<u32>('N'));
}

TEST(vga_backspace) {
    VgaTerminal t = make_terminal();

    t.putchar('A');
    t.putchar('B');
    t.putchar('\b');

    ASSERT_EQ(static_cast<u32>(test_buf[1] & 0x00FF), static_cast<u32>(' '));

    t.putchar('C');
    ASSERT_EQ(static_cast<u32>(test_buf[1] & 0x00FF), static_cast<u32>('C'));
}

TEST(vga_get_cursor) {
    VgaTerminal t = make_terminal();

    ASSERT_EQ(static_cast<u32>(t.getCursorX()), 0u);
    ASSERT_EQ(static_cast<u32>(t.getCursorY()), 0u);

    t.putchar('A');
    ASSERT_EQ(static_cast<u32>(t.getCursorX()), 1u);
    ASSERT_EQ(static_cast<u32>(t.getCursorY()), 0u);
}

TEST(vga_scroll) {
    VgaTerminal t = make_terminal();

    t.setCursor(0, 1);
    t.putchar('Z');
    t.setCursor(0, VGA_HEIGHT - 1);
    t.putchar('\n');

    ASSERT_EQ(static_cast<u32>(test_buf[0] & 0x00FF), static_cast<u32>('Z'));
    ASSERT_EQ(static_cast<u32>(test_buf[VGA_WIDTH * (VGA_HEIGHT - 1)] & 0x00FF),
              static_cast<u32>(' '));
    ASSERT_EQ(static_cast<u32>(t.getCursorY()), static_cast<u32>(VGA_HEIGHT - 1));
    ASSERT_EQ(static_cast<u32>(t.getCursorX()), 0u);
}

TEST(vga_set_cursor) {
    VgaTerminal t = make_terminal();

    t.setCursor(10, 5);
    ASSERT_EQ(static_cast<u32>(t.getCursorX()), 10u);
    ASSERT_EQ(static_cast<u32>(t.getCursorY()), 5u);

    t.putchar('Q');
    ASSERT_EQ(static_cast<u32>(test_buf[5 * VGA_WIDTH + 10] & 0x00FF), static_cast<u32>('Q'));
}

TEST(vga_tab) {
    VgaTerminal t = make_terminal();

    t.putchar('\t');

    ASSERT_EQ(static_cast<u32>(t.getCursorX()), 8u);
    for (u32 i = 0; i < 8; ++i) {
        ASSERT_EQ(static_cast<u32>(test_buf[i] & 0x00FF), static_cast<u32>(' '));
    }
}

TEST(vga_delete) {
    VgaTerminal t = make_terminal();

    t.putchar('X');
    t.setCursor(0, 0);
    t.putchar(0x7F);

    ASSERT_EQ(static_cast<u32>(test_buf[0] & 0x00FF), static_cast<u32>(' '));
}

TEST(vga_line_wrap) {
    VgaTerminal t = make_terminal();

    for (u32 i = 0; i < VGA_WIDTH; ++i) {
        t.putchar('A');
    }

    ASSERT_EQ(static_cast<u32>(t.getCursorX()), 0u);
    ASSERT_EQ(static_cast<u32>(t.getCursorY()), 1u);
}
