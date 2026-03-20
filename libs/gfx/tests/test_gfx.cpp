/**
 * test_gfx.cpp -- Tests for std::gfx graphics primitives
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <std/gfx.hpp>
#include <std/mem.hpp>
#include <std/test.hpp>

using namespace std;
using namespace std::gfx;

static PixelBuffer makeBuf(u32* data, u32 w, u32 h) {
    return {data, w, h, w * sizeof(u32)};
}

TEST(gfx_pixel_buffer_size) {
    ASSERT_EQ(sizeof(PixelBuffer), (usize)16);
}

TEST(gfx_draw_pixel) {
    u32 data[4 * 4];
    mem::set(data, 0, sizeof(data));
    auto buf = makeBuf(data, 4, 4);

    drawPixel(buf, 2, 1, 0x00FF0000);
    ASSERT_EQ(data[1 * 4 + 2], 0x00FF0000u);

    // Unchanged neighbors.
    ASSERT_EQ(data[1 * 4 + 1], 0u);
    ASSERT_EQ(data[1 * 4 + 3], 0u);
}

TEST(gfx_draw_pixel_out_of_bounds) {
    u32 data[4 * 4];
    mem::set(data, 0, sizeof(data));
    auto buf = makeBuf(data, 4, 4);

    drawPixel(buf, 4, 0, 0xFF);
    drawPixel(buf, 0, 4, 0xFF);
    drawPixel(buf, 100, 100, 0xFF);

    // Nothing should have been written.
    for (u32 i = 0; i < 16; ++i) {
        ASSERT_EQ(data[i], 0u);
    }
}

TEST(gfx_fill_rect) {
    u32 data[8 * 8];
    mem::set(data, 0, sizeof(data));
    auto buf = makeBuf(data, 8, 8);

    fillRect(buf, 1, 2, 3, 2, 0xAA);

    // Inside the rect.
    ASSERT_EQ(data[2 * 8 + 1], 0xAAu);
    ASSERT_EQ(data[2 * 8 + 3], 0xAAu);
    ASSERT_EQ(data[3 * 8 + 2], 0xAAu);

    // Outside the rect.
    ASSERT_EQ(data[2 * 8 + 0], 0u);
    ASSERT_EQ(data[2 * 8 + 4], 0u);
    ASSERT_EQ(data[1 * 8 + 1], 0u);
    ASSERT_EQ(data[4 * 8 + 1], 0u);
}

TEST(gfx_fill_rect_clips) {
    u32 data[4 * 4];
    mem::set(data, 0, sizeof(data));
    auto buf = makeBuf(data, 4, 4);

    // Rect extends past buffer bounds -- should clip, not crash.
    fillRect(buf, 2, 2, 10, 10, 0xBB);

    ASSERT_EQ(data[2 * 4 + 2], 0xBBu);
    ASSERT_EQ(data[3 * 4 + 3], 0xBBu);
}

TEST(gfx_draw_rect) {
    u32 data[8 * 8];
    mem::set(data, 0, sizeof(data));
    auto buf = makeBuf(data, 8, 8);

    drawRect(buf, 1, 1, 4, 3, 0xCC);

    // Top edge.
    ASSERT_EQ(data[1 * 8 + 1], 0xCCu);
    ASSERT_EQ(data[1 * 8 + 4], 0xCCu);

    // Bottom edge.
    ASSERT_EQ(data[3 * 8 + 1], 0xCCu);
    ASSERT_EQ(data[3 * 8 + 4], 0xCCu);

    // Left/right edges.
    ASSERT_EQ(data[2 * 8 + 1], 0xCCu);
    ASSERT_EQ(data[2 * 8 + 4], 0xCCu);

    // Interior should be empty.
    ASSERT_EQ(data[2 * 8 + 2], 0u);
    ASSERT_EQ(data[2 * 8 + 3], 0u);
}

TEST(gfx_draw_char_space_is_bg) {
    u32 data[8 * 16];
    mem::set(data, 0, sizeof(data));
    auto buf = makeBuf(data, 8, 16);

    drawChar(buf, 0, 0, ' ', 0xFFFFFF, 0x000080);

    // Space glyph is all zeros, so every pixel should be bg.
    for (u32 i = 0; i < 8 * 16; ++i) {
        ASSERT_EQ(data[i], 0x000080u);
    }
}

TEST(gfx_draw_char_A_has_fg_pixels) {
    u32 data[8 * 16];
    mem::set(data, 0, sizeof(data));
    auto buf = makeBuf(data, 8, 16);

    drawChar(buf, 0, 0, 'A', 0xFFFFFF, 0x000000);

    // 'A' glyph has nonzero rows -- at least some pixels should be fg.
    u32 fgCount = 0;
    for (u32 i = 0; i < 8 * 16; ++i) {
        if (data[i] == 0xFFFFFF) {
            ++fgCount;
        }
    }
    ASSERT(fgCount > 0);
}

TEST(gfx_draw_text) {
    u32 data[24 * 16];
    mem::set(data, 0, sizeof(data));
    auto buf = makeBuf(data, 24, 16);

    drawText(buf, 0, 0, "Hi", 2, 0xFFFFFF, 0x000000);

    // First char occupies columns 0-7, second occupies 8-15.
    // Columns 16-23 should be untouched.
    bool firstCharDrawn = false;
    for (u32 y = 0; y < 16; ++y) {
        for (u32 x = 0; x < 8; ++x) {
            if (data[y * 24 + x] == 0xFFFFFF) {
                firstCharDrawn = true;
            }
        }
    }
    ASSERT(firstCharDrawn);

    // Third character region should be untouched (still 0).
    for (u32 y = 0; y < 16; ++y) {
        for (u32 x = 16; x < 24; ++x) {
            ASSERT_EQ(data[y * 24 + x], 0u);
        }
    }
}

TEST(gfx_scroll_shifts_up) {
    u32 data[4 * 4];
    mem::set(data, 0, sizeof(data));
    auto buf = makeBuf(data, 4, 4);

    // Fill row 2 with a marker.
    for (u32 x = 0; x < 4; ++x) {
        data[2 * 4 + x] = 0xDD;
    }

    scroll(buf, 1, 0x00);

    // Row 2 should now be in row 1.
    for (u32 x = 0; x < 4; ++x) {
        ASSERT_EQ(data[1 * 4 + x], 0xDDu);
    }

    // Bottom row should be fill color.
    for (u32 x = 0; x < 4; ++x) {
        ASSERT_EQ(data[3 * 4 + x], 0u);
    }
}

TEST(gfx_scroll_full_height_clears) {
    u32 data[4 * 4];
    auto buf = makeBuf(data, 4, 4);

    for (u32 i = 0; i < 16; ++i) {
        data[i] = 0xFF;
    }

    scroll(buf, 4, 0xAA);

    for (u32 i = 0; i < 16; ++i) {
        ASSERT_EQ(data[i], 0xAAu);
    }
}

TEST(gfx_blit_copies_region) {
    u32 srcData[4 * 4];
    u32 dstData[4 * 4];
    mem::set(srcData, 0, sizeof(srcData));
    mem::set(dstData, 0, sizeof(dstData));
    auto src = makeBuf(srcData, 4, 4);
    auto dst = makeBuf(dstData, 4, 4);

    // Set a 2x2 region in src at (1,1).
    srcData[1 * 4 + 1] = 0x11;
    srcData[1 * 4 + 2] = 0x22;
    srcData[2 * 4 + 1] = 0x33;
    srcData[2 * 4 + 2] = 0x44;

    blit(dst, 0, 0, src, 1, 1, 2, 2);

    ASSERT_EQ(dstData[0 * 4 + 0], 0x11u);
    ASSERT_EQ(dstData[0 * 4 + 1], 0x22u);
    ASSERT_EQ(dstData[1 * 4 + 0], 0x33u);
    ASSERT_EQ(dstData[1 * 4 + 1], 0x44u);

    // Rest should be untouched.
    ASSERT_EQ(dstData[0 * 4 + 2], 0u);
}

TEST(gfx_font_constants) {
    ASSERT_EQ(FONT_WIDTH, 8u);
    ASSERT_EQ(FONT_HEIGHT, 16u);
}

TEST(gfx_font_space_is_blank) {
    for (u32 row = 0; row < 16; ++row) {
        ASSERT_EQ(font[' '][row], 0u);
    }
}

TEST(gfx_font_A_is_nonzero) {
    u32 nonzero = 0;
    for (u32 row = 0; row < 16; ++row) {
        if (font['A'][row] != 0) {
            ++nonzero;
        }
    }
    ASSERT(nonzero > 0);
}
