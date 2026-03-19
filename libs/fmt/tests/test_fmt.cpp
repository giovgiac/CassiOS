#include <std/fmt.hpp>
#include <std/str.hpp>
#include <test.hpp>

using namespace std;

// --- %d (signed decimal) ---

TEST(fmt_d_positive) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "%d", 42);
    ASSERT(str::eq(buf, "42"));
}

TEST(fmt_d_negative) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "%d", -7);
    ASSERT(str::eq(buf, "-7"));
}

TEST(fmt_d_zero) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "%d", 0);
    ASSERT(str::eq(buf, "0"));
}

TEST(fmt_d_max) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "%d", 2147483647);
    ASSERT(str::eq(buf, "2147483647"));
}

// --- %u (unsigned decimal) ---

TEST(fmt_u_basic) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "%u", (u32)12345);
    ASSERT(str::eq(buf, "12345"));
}

TEST(fmt_u_zero) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "%u", (u32)0);
    ASSERT(str::eq(buf, "0"));
}

TEST(fmt_u_max) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "%u", (u32)4294967295);
    ASSERT(str::eq(buf, "4294967295"));
}

// --- %x / %X (hex) ---

TEST(fmt_x_basic) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "%x", (u32)255);
    ASSERT(str::eq(buf, "ff"));
}

TEST(fmt_X_basic) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "%X", (u32)255);
    ASSERT(str::eq(buf, "FF"));
}

TEST(fmt_x_zero) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "%x", (u32)0);
    ASSERT(str::eq(buf, "0"));
}

TEST(fmt_x_large) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "%x", (u32)0xDEADBEEF);
    ASSERT(str::eq(buf, "deadbeef"));
}

// --- %s (string) ---

TEST(fmt_s_basic) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "%s", "hello");
    ASSERT(str::eq(buf, "hello"));
}

TEST(fmt_s_null) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "%s", (const char*)nullptr);
    ASSERT(str::eq(buf, "(null)"));
}

TEST(fmt_s_empty) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "%s", "");
    ASSERT(str::eq(buf, ""));
}

// --- %c (character) ---

TEST(fmt_c_basic) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "%c", 'A');
    ASSERT(str::eq(buf, "A"));
}

// --- %% (literal percent) ---

TEST(fmt_percent) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "100%%");
    ASSERT(str::eq(buf, "100%"));
}

// --- mixed format ---

TEST(fmt_mixed) {
    char buf[64];
    fmt::format(buf, sizeof(buf), "%s=%d (0x%x)", "val", 42, (u32)42);
    ASSERT(str::eq(buf, "val=42 (0x2a)"));
}

TEST(fmt_plain_text) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "hello world");
    ASSERT(str::eq(buf, "hello world"));
}

// --- truncation ---

TEST(fmt_truncation) {
    char buf[6];
    usize n = fmt::format(buf, sizeof(buf), "hello world");
    ASSERT(str::eq(buf, "hello"));
    ASSERT_EQ(n, (usize)5);
}

TEST(fmt_truncation_exact) {
    char buf[6];
    fmt::format(buf, sizeof(buf), "hello");
    ASSERT(str::eq(buf, "hello"));
}

// --- return value ---

TEST(fmt_returns_length) {
    char buf[64];
    usize n = fmt::format(buf, sizeof(buf), "abc%d", 42);
    ASSERT_EQ(n, (usize)5);
}

TEST(fmt_zero_size) {
    char buf[1] = {'X'};
    usize n = fmt::format(buf, 0, "hello");
    ASSERT_EQ(n, (usize)0);
    ASSERT_EQ(buf[0], 'X');
}

// --- unknown specifier ---

TEST(fmt_unknown_specifier) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "%z");
    ASSERT(str::eq(buf, "%z"));
}

// --- width (right-aligned, space-padded) ---

TEST(fmt_width_u_right) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "%5u", (u32)42);
    ASSERT(str::eq(buf, "   42"));
}

TEST(fmt_width_d_right) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "%5d", -7);
    ASSERT(str::eq(buf, "   -7"));
}

TEST(fmt_width_s_right) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "%10s", "hello");
    ASSERT(str::eq(buf, "     hello"));
}

TEST(fmt_width_x_right) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "%8X", (u32)0xFF);
    ASSERT(str::eq(buf, "      FF"));
}

TEST(fmt_width_no_effect) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "%3u", (u32)12345);
    ASSERT(str::eq(buf, "12345"));
}

// --- left-align flag ---

TEST(fmt_left_u) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "%-5u", (u32)42);
    ASSERT(str::eq(buf, "42   "));
}

TEST(fmt_left_s) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "%-10s", "hello");
    ASSERT(str::eq(buf, "hello     "));
}

TEST(fmt_left_d) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "%-5d", -7);
    ASSERT(str::eq(buf, "-7   "));
}

// --- zero-pad flag ---

TEST(fmt_zeropad_u) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "%05u", (u32)42);
    ASSERT(str::eq(buf, "00042"));
}

TEST(fmt_zeropad_d_negative) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "%05d", -7);
    ASSERT(str::eq(buf, "-0007"));
}

TEST(fmt_zeropad_x) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "%08X", (u32)0xFF);
    ASSERT(str::eq(buf, "000000FF"));
}

TEST(fmt_zeropad_d_positive) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "%03u", (u32)5);
    ASSERT(str::eq(buf, "005"));
}

// --- left-align overrides zero-pad ---

TEST(fmt_left_overrides_zeropad) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "%-05u", (u32)42);
    ASSERT(str::eq(buf, "42   "));
}

// --- combined (real-world table row) ---

TEST(fmt_table_row) {
    char buf[64];
    fmt::format(buf, sizeof(buf), "%3u  %-9s  %-14s  %u KB",
                (u32)3, "shell", "Ready", (u32)4);
    ASSERT(str::eq(buf, "  3  shell      Ready           4 KB"));
}
