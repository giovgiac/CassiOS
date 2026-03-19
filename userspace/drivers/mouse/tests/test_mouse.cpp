/**
 * test_mouse.cpp -- mouse packet parser unit tests
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <std/test.hpp>
#include <mouse.hpp>

using namespace cassio;
using namespace std;

static Mouse mouse;

TEST(mouse_init_state) {
    mouse.init();
    ASSERT_EQ(mouse.getButtons(), 0u);
    ASSERT_EQ(mouse.getDx(), 0);
    ASSERT_EQ(mouse.getDy(), 0);
}

TEST(mouse_single_packet_movement) {
    mouse.init();
    // Packet: no buttons, dx=+5, dy=-3 (raw byte 3, negated = -3)
    mouse.handleByte(0x00);  // buttons
    mouse.handleByte(5);     // dx
    mouse.handleByte(3);     // raw dy (negated to -3)

    ASSERT_EQ(mouse.getButtons(), 0u);
    ASSERT_EQ(mouse.getDx(), 5);
    ASSERT_EQ(mouse.getDy(), static_cast<i32>(-3));
}

TEST(mouse_button_left) {
    mouse.init();
    mouse.handleByte(0x01);  // left button pressed
    mouse.handleByte(0);
    mouse.handleByte(0);

    ASSERT_EQ(mouse.getButtons(), 1u);
}

TEST(mouse_button_right) {
    mouse.init();
    mouse.handleByte(0x02);  // right button pressed
    mouse.handleByte(0);
    mouse.handleByte(0);

    ASSERT_EQ(mouse.getButtons(), 2u);
}

TEST(mouse_button_middle) {
    mouse.init();
    mouse.handleByte(0x04);  // middle button pressed
    mouse.handleByte(0);
    mouse.handleByte(0);

    ASSERT_EQ(mouse.getButtons(), 4u);
}

TEST(mouse_accumulate_movement) {
    mouse.init();
    // First packet: dx=+10, dy=-5
    mouse.handleByte(0x00);
    mouse.handleByte(10);
    mouse.handleByte(5);
    // Second packet: dx=+3, dy=-2
    mouse.handleByte(0x00);
    mouse.handleByte(3);
    mouse.handleByte(2);

    ASSERT_EQ(mouse.getDx(), 13);
    ASSERT_EQ(mouse.getDy(), static_cast<i32>(-7));
}

TEST(mouse_read_state_resets_deltas) {
    mouse.init();
    mouse.handleByte(0x01);
    mouse.handleByte(10);
    mouse.handleByte(5);

    u8 btns;
    i32 dx, dy;
    mouse.readState(btns, dx, dy);

    ASSERT_EQ(btns, 1u);
    ASSERT_EQ(dx, 10);
    ASSERT_EQ(dy, static_cast<i32>(-5));

    // After read, deltas should be reset.
    ASSERT_EQ(mouse.getDx(), 0);
    ASSERT_EQ(mouse.getDy(), 0);
    // Buttons persist.
    ASSERT_EQ(mouse.getButtons(), 1u);
}

TEST(mouse_incomplete_packet_no_update) {
    mouse.init();
    // Only 2 bytes -- no state change yet.
    mouse.handleByte(0x01);
    mouse.handleByte(10);

    ASSERT_EQ(mouse.getButtons(), 0u);
    ASSERT_EQ(mouse.getDx(), 0);
    ASSERT_EQ(mouse.getDy(), 0);
}
