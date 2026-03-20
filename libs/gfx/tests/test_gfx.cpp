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

TEST(gfx_pixel_buffer_size) {
    ASSERT_EQ(sizeof(PixelBuffer), (usize)20);
}

TEST(gfx_draw_pixel) {
    u32 data[4 * 4];
    mem::set(data, 0, sizeof(data));
    PixelBuffer buf(data, 4, 4, 4 * sizeof(u32));

    buf.drawPixel(2, 1, 0x00FF0000);
    ASSERT_EQ(data[1 * 4 + 2], 0x00FF0000u);

    // Unchanged neighbors.
    ASSERT_EQ(data[1 * 4 + 1], 0u);
    ASSERT_EQ(data[1 * 4 + 3], 0u);
}

TEST(gfx_draw_pixel_out_of_bounds) {
    u32 data[4 * 4];
    mem::set(data, 0, sizeof(data));
    PixelBuffer buf(data, 4, 4, 4 * sizeof(u32));

    buf.drawPixel(4, 0, 0xFF);
    buf.drawPixel(0, 4, 0xFF);
    buf.drawPixel(100, 100, 0xFF);

    for (u32 i = 0; i < 16; ++i) {
        ASSERT_EQ(data[i], 0u);
    }
}

TEST(gfx_fill_rect) {
    u32 data[8 * 8];
    mem::set(data, 0, sizeof(data));
    PixelBuffer buf(data, 8, 8, 8 * sizeof(u32));

    buf.fillRect(1, 2, 3, 2, 0xAA);

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
    PixelBuffer buf(data, 4, 4, 4 * sizeof(u32));

    buf.fillRect(2, 2, 10, 10, 0xBB);

    ASSERT_EQ(data[2 * 4 + 2], 0xBBu);
    ASSERT_EQ(data[3 * 4 + 3], 0xBBu);
}

TEST(gfx_draw_rect) {
    u32 data[8 * 8];
    mem::set(data, 0, sizeof(data));
    PixelBuffer buf(data, 8, 8, 8 * sizeof(u32));

    buf.drawRect(1, 1, 4, 3, 0xCC);

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
    PixelBuffer buf(data, 8, 16, 8 * sizeof(u32));

    buf.drawChar(0, 0, ' ', 0xFFFFFF, 0x000080);

    for (u32 i = 0; i < 8 * 16; ++i) {
        ASSERT_EQ(data[i], 0x000080u);
    }
}

TEST(gfx_draw_char_A_has_fg_pixels) {
    u32 data[8 * 16];
    mem::set(data, 0, sizeof(data));
    PixelBuffer buf(data, 8, 16, 8 * sizeof(u32));

    buf.drawChar(0, 0, 'A', 0xFFFFFF, 0x000000);

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
    PixelBuffer buf(data, 24, 16, 24 * sizeof(u32));

    buf.drawText(0, 0, "Hi", 0xFFFFFF, 0x000000);

    bool firstCharDrawn = false;
    for (u32 y = 0; y < 16; ++y) {
        for (u32 x = 0; x < 8; ++x) {
            if (data[y * 24 + x] == 0xFFFFFF) {
                firstCharDrawn = true;
            }
        }
    }
    ASSERT(firstCharDrawn);

    // Third character region should be untouched.
    for (u32 y = 0; y < 16; ++y) {
        for (u32 x = 16; x < 24; ++x) {
            ASSERT_EQ(data[y * 24 + x], 0u);
        }
    }
}

TEST(gfx_scroll_advances_offset) {
    u32 data[4 * 4];
    mem::set(data, 0, sizeof(data));
    PixelBuffer buf(data, 4, 4, 4 * sizeof(u32));

    ASSERT_EQ(buf.getScrollOffset(), 0u);
    buf.scroll(1, 0x00);
    ASSERT_EQ(buf.getScrollOffset(), 1u);
    buf.scroll(2, 0x00);
    ASSERT_EQ(buf.getScrollOffset(), 3u);
}

TEST(gfx_scroll_draw_at_new_bottom) {
    u32 data[4 * 4];
    mem::set(data, 0, sizeof(data));
    PixelBuffer buf(data, 4, 4, 4 * sizeof(u32));

    // Draw a marker at row 2.
    buf.fillRect(0, 2, 4, 1, 0xDD);

    // Scroll up by 1 -- marker should now be at logical row 1.
    buf.scroll(1, 0x00);

    // Draw at the new bottom row (row 3).
    buf.drawPixel(0, 3, 0xEE);

    // Verify logical row 1 still has the marker.
    // Row 1 wraps to physical row (1+1)%4 = 2, which has the 0xDD data.
    u32 physRow = (1 + buf.getScrollOffset()) % 4;
    ASSERT_EQ(data[physRow * 4 + 0], 0xDDu);

    // Verify the new bottom row has our pixel.
    u32 physBottom = (3 + buf.getScrollOffset()) % 4;
    ASSERT_EQ(data[physBottom * 4 + 0], 0xEEu);
}

TEST(gfx_scroll_clears_new_rows) {
    u32 data[4 * 4];
    PixelBuffer buf(data, 4, 4, 4 * sizeof(u32));

    // Fill everything with 0xFF.
    buf.fillRect(0, 0, 4, 4, 0xFF);

    // Scroll by 2 -- bottom 2 rows should be cleared.
    buf.scroll(2, 0xAA);

    // Read the bottom 2 logical rows via physical mapping.
    for (u32 row = 2; row < 4; ++row) {
        u32 physRow = (row + buf.getScrollOffset()) % 4;
        for (u32 x = 0; x < 4; ++x) {
            ASSERT_EQ(data[physRow * 4 + x], 0xAAu);
        }
    }

    // Top 2 rows should retain original data.
    for (u32 row = 0; row < 2; ++row) {
        u32 physRow = (row + buf.getScrollOffset()) % 4;
        for (u32 x = 0; x < 4; ++x) {
            ASSERT_EQ(data[physRow * 4 + x], 0xFFu);
        }
    }
}

TEST(gfx_scroll_full_height_clears) {
    u32 data[4 * 4];
    PixelBuffer buf(data, 4, 4, 4 * sizeof(u32));

    for (u32 i = 0; i < 16; ++i) {
        data[i] = 0xFF;
    }

    buf.scroll(4, 0xAA);

    for (u32 i = 0; i < 16; ++i) {
        ASSERT_EQ(data[i], 0xAAu);
    }
    // Full scroll resets offset.
    ASSERT_EQ(buf.getScrollOffset(), 0u);
}

TEST(gfx_scroll_wraps_around) {
    u32 data[4 * 4];
    mem::set(data, 0, sizeof(data));
    PixelBuffer buf(data, 4, 4, 4 * sizeof(u32));

    // Scroll 3 times by 2 rows (total 6 in a height-4 buffer).
    buf.scroll(2, 0x00);
    buf.scroll(2, 0x00);
    buf.scroll(2, 0x00);

    // Offset should wrap: (2+2+2) % 4 = 2.
    ASSERT_EQ(buf.getScrollOffset(), 2u);

    // Drawing at logical (0,0) should hit physical row 2.
    buf.drawPixel(0, 0, 0x77);
    ASSERT_EQ(data[2 * 4 + 0], 0x77u);
}

TEST(gfx_blit_copies_region) {
    u32 srcData[4 * 4];
    u32 dstData[4 * 4];
    mem::set(srcData, 0, sizeof(srcData));
    mem::set(dstData, 0, sizeof(dstData));
    PixelBuffer src(srcData, 4, 4, 4 * sizeof(u32));
    PixelBuffer dst(dstData, 4, 4, 4 * sizeof(u32));

    srcData[1 * 4 + 1] = 0x11;
    srcData[1 * 4 + 2] = 0x22;
    srcData[2 * 4 + 1] = 0x33;
    srcData[2 * 4 + 2] = 0x44;

    dst.blit(0, 0, src, 1, 1, 2, 2);

    ASSERT_EQ(dstData[0 * 4 + 0], 0x11u);
    ASSERT_EQ(dstData[0 * 4 + 1], 0x22u);
    ASSERT_EQ(dstData[1 * 4 + 0], 0x33u);
    ASSERT_EQ(dstData[1 * 4 + 1], 0x44u);

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

TEST(gfx_getters) {
    u32 data[8 * 4];
    PixelBuffer buf(data, 8, 4, 8 * sizeof(u32));

    ASSERT_EQ(buf.getWidth(), 8u);
    ASSERT_EQ(buf.getHeight(), 4u);
    ASSERT_EQ(buf.getPitch(), 32u);
    ASSERT(buf.getData() == data);
    ASSERT_EQ(buf.getScrollOffset(), 0u);
}
