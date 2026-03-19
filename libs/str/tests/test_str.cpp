#include <std/str.hpp>
#include <std/test.hpp>

using namespace std;

TEST(str_eq_identical) {
    ASSERT(str::eq("hello", "hello"));
}

TEST(str_eq_different) {
    ASSERT(!str::eq("hello", "world"));
}

TEST(str_eq_prefix) {
    ASSERT(!str::eq("hello", "hell"));
}

TEST(str_eq_empty) {
    ASSERT(str::eq("", ""));
}

TEST(str_copy_basic) {
    char dst[16];
    str::copy(dst, "hello", 16);
    ASSERT(str::eq(dst, "hello"));
}

TEST(str_copy_truncates) {
    char dst[4];
    str::copy(dst, "hello", 4);
    ASSERT(str::eq(dst, "hel"));
}

TEST(str_copy_null_terminates) {
    char dst[6] = "xxxxx";
    str::copy(dst, "ab", 6);
    ASSERT_EQ(dst[2], '\0');
}

TEST(str_len_basic) {
    ASSERT_EQ(str::len("hello"), (usize)5);
}

TEST(str_len_empty) {
    ASSERT_EQ(str::len(""), (usize)0);
}

TEST(str_to_u32_basic) {
    ASSERT_EQ(str::to_u32("12345"), (u32)12345);
}

TEST(str_to_u32_zero) {
    ASSERT_EQ(str::to_u32("0"), (u32)0);
}

TEST(str_to_u32_stops_at_non_digit) {
    ASSERT_EQ(str::to_u32("42abc"), (u32)42);
}

TEST(str_to_u32_empty) {
    ASSERT_EQ(str::to_u32(""), (u32)0);
}
