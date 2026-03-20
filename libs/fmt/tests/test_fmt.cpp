#include <std/fmt.hpp>
#include <std/str.hpp>
#include <std/test.hpp>

using namespace std;
using str::StringView;

// --- signed integers ---

TEST(fmt_signed_positive) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "{}", 42);
    ASSERT(StringView(buf) == "42");
}

TEST(fmt_signed_negative) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "{}", -7);
    ASSERT(StringView(buf) == "-7");
}

TEST(fmt_signed_zero) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "{}", 0);
    ASSERT(StringView(buf) == "0");
}

TEST(fmt_signed_max) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "{}", 2147483647);
    ASSERT(StringView(buf) == "2147483647");
}

// --- unsigned integers ---

TEST(fmt_unsigned_basic) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "{}", (u32)12345);
    ASSERT(StringView(buf) == "12345");
}

TEST(fmt_unsigned_zero) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "{}", (u32)0);
    ASSERT(StringView(buf) == "0");
}

TEST(fmt_unsigned_max) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "{}", (u32)4294967295);
    ASSERT(StringView(buf) == "4294967295");
}

// --- hex ---

TEST(fmt_hex_lower) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "{:x}", (u32)255);
    ASSERT(StringView(buf) == "ff");
}

TEST(fmt_hex_upper) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "{:X}", (u32)255);
    ASSERT(StringView(buf) == "FF");
}

TEST(fmt_hex_zero) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "{:x}", (u32)0);
    ASSERT(StringView(buf) == "0");
}

TEST(fmt_hex_large) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "{:x}", (u32)0xDEADBEEF);
    ASSERT(StringView(buf) == "deadbeef");
}

// --- strings ---

TEST(fmt_string_basic) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "{}", "hello");
    ASSERT(StringView(buf) == "hello");
}

TEST(fmt_string_null) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "{}", (const char*)nullptr);
    ASSERT(StringView(buf) == "(null)");
}

TEST(fmt_string_empty) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "{}", "");
    ASSERT(StringView(buf) == "");
}

// --- char ---

TEST(fmt_char) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "{}", 'A');
    ASSERT(StringView(buf) == "A");
}

// --- literal brace ---

TEST(fmt_literal_brace) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "100{{}}");
    ASSERT(StringView(buf) == "100{}");
}

// --- mixed ---

TEST(fmt_mixed) {
    char buf[64];
    fmt::format(buf, sizeof(buf), "{}={} (0x{:x})", "val", 42, (u32)42);
    ASSERT(StringView(buf) == "val=42 (0x2a)");
}

TEST(fmt_plain_text) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "hello world");
    ASSERT(StringView(buf) == "hello world");
}

// --- truncation ---

TEST(fmt_truncation) {
    char buf[6];
    usize n = fmt::format(buf, sizeof(buf), "hello world");
    ASSERT(StringView(buf) == "hello");
    ASSERT_EQ(n, (usize)5);
}

TEST(fmt_truncation_exact) {
    char buf[6];
    fmt::format(buf, sizeof(buf), "hello");
    ASSERT(StringView(buf) == "hello");
}

// --- return value ---

TEST(fmt_returns_length) {
    char buf[64];
    usize n = fmt::format(buf, sizeof(buf), "abc{}", 42);
    ASSERT_EQ(n, (usize)5);
}

TEST(fmt_zero_size) {
    char buf[1] = {'X'};
    usize n = fmt::format(buf, 0, "hello");
    ASSERT_EQ(n, (usize)0);
    ASSERT_EQ(buf[0], 'X');
}

// --- width (right-aligned, space-padded) ---

TEST(fmt_width_right) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "{:5}", (u32)42);
    ASSERT(StringView(buf) == "   42");
}

TEST(fmt_width_signed_right) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "{:5}", -7);
    ASSERT(StringView(buf) == "   -7");
}

TEST(fmt_width_string_right) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "{:10}", "hello");
    ASSERT(StringView(buf) == "     hello");
}

TEST(fmt_width_hex_right) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "{:8X}", (u32)0xFF);
    ASSERT(StringView(buf) == "      FF");
}

TEST(fmt_width_no_effect) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "{:3}", (u32)12345);
    ASSERT(StringView(buf) == "12345");
}

// --- left-align ---

TEST(fmt_left_unsigned) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "{:<5}", (u32)42);
    ASSERT(StringView(buf) == "42   ");
}

TEST(fmt_left_string) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "{:<10}", "hello");
    ASSERT(StringView(buf) == "hello     ");
}

TEST(fmt_left_signed) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "{:<5}", -7);
    ASSERT(StringView(buf) == "-7   ");
}

// --- zero-pad ---

TEST(fmt_zeropad_unsigned) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "{:05}", (u32)42);
    ASSERT(StringView(buf) == "00042");
}

TEST(fmt_zeropad_negative) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "{:05}", -7);
    ASSERT(StringView(buf) == "-0007");
}

TEST(fmt_zeropad_hex) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "{:08X}", (u32)0xFF);
    ASSERT(StringView(buf) == "000000FF");
}

TEST(fmt_zeropad_small) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "{:03}", (u32)5);
    ASSERT(StringView(buf) == "005");
}

// --- left-align overrides zero-pad ---

TEST(fmt_left_overrides_zeropad) {
    char buf[32];
    fmt::format(buf, sizeof(buf), "{:<05}", (u32)42);
    ASSERT(StringView(buf) == "42   ");
}

// --- real-world table row ---

TEST(fmt_table_row) {
    char buf[64];
    fmt::format(buf, sizeof(buf), "{:3}  {:<9}  {:<14}  {} KB", (u32)3, "shell", "Ready", (u32)4);
    ASSERT(StringView(buf) == "  3  shell      Ready           4 KB");
}
