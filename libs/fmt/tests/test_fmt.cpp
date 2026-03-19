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
