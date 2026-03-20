/**
 * test_display.cpp -- Tests for the Display class
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <std/mem.hpp>
#include <std/test.hpp>

#include <display.hpp>

using namespace cassio;
using namespace std;

static constexpr u32 W = 16;
static constexpr u32 H = 8;
static constexpr u32 PITCH = W * sizeof(u32);

TEST(display_fill_rect) {
    u32 fb[W * H];
    u32 back[W * H];
    mem::set(fb, 0, sizeof(fb));
    mem::set(back, 0, sizeof(back));
    Display display(fb, back, W, H, PITCH);

    display.fillRect(1, 1, 3, 2, 0xAA);

    // Back buffer should have the fill, framebuffer should not (no flush).
    ASSERT_EQ(back[1 * W + 1], 0xAAu);
    ASSERT_EQ(back[2 * W + 3], 0xAAu);
    ASSERT_EQ(fb[1 * W + 1], 0u);
}

TEST(display_flush_copies_to_framebuffer) {
    u32 fb[W * H];
    u32 back[W * H];
    mem::set(fb, 0, sizeof(fb));
    mem::set(back, 0, sizeof(back));
    Display display(fb, back, W, H, PITCH);

    display.fillRect(0, 0, W, H, 0xBB);
    display.flush();

    ASSERT_EQ(fb[0], 0xBBu);
    ASSERT_EQ(fb[W * H - 1], 0xBBu);
}

TEST(display_draw_rect) {
    u32 fb[W * H];
    u32 back[W * H];
    mem::set(fb, 0, sizeof(fb));
    mem::set(back, 0, sizeof(back));
    Display display(fb, back, W, H, PITCH);

    display.drawRect(0, 0, 4, 3, 0xCC);

    // Outline pixels.
    ASSERT_EQ(back[0 * W + 0], 0xCCu);
    ASSERT_EQ(back[0 * W + 3], 0xCCu);
    ASSERT_EQ(back[2 * W + 0], 0xCCu);

    // Interior.
    ASSERT_EQ(back[1 * W + 1], 0u);
}

TEST(display_scroll) {
    u32 fb[W * H];
    u32 back[W * H];
    mem::set(fb, 0, sizeof(fb));
    mem::set(back, 0, sizeof(back));
    Display display(fb, back, W, H, PITCH);

    // Fill row 2 with marker.
    for (u32 x = 0; x < W; ++x) {
        back[2 * W + x] = 0xDD;
    }

    display.scroll(1, 0x00);

    // Row 2 should now be in row 1.
    ASSERT_EQ(back[1 * W + 0], 0xDDu);

    // Bottom row should be fill color.
    ASSERT_EQ(back[(H - 1) * W + 0], 0u);
}

TEST(display_blit) {
    u32 fb[W * H];
    u32 back[W * H];
    mem::set(fb, 0, sizeof(fb));
    mem::set(back, 0, sizeof(back));
    Display display(fb, back, W, H, PITCH);

    u32 pixels[2 * 2] = {0x11, 0x22, 0x33, 0x44};
    display.blit(1, 1, 2, 2, pixels);

    ASSERT_EQ(back[1 * W + 1], 0x11u);
    ASSERT_EQ(back[1 * W + 2], 0x22u);
    ASSERT_EQ(back[2 * W + 1], 0x33u);
    ASSERT_EQ(back[2 * W + 2], 0x44u);
}

TEST(display_getters) {
    u32 fb[W * H];
    u32 back[W * H];
    Display display(fb, back, W, H, PITCH);

    ASSERT_EQ(display.getWidth(), W);
    ASSERT_EQ(display.getHeight(), H);
    ASSERT_EQ(display.getPitch(), PITCH);
    ASSERT_EQ(display.getBpp(), 32u);
}
