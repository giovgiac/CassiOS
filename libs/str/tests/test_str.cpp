#include <std/str.hpp>
#include <std/test.hpp>

using namespace std;
using str::StringView;

// -- StringView construction --

TEST(sv_default_empty) {
    StringView sv;
    ASSERT(sv.isEmpty());
    ASSERT_EQ(sv.length(), (usize)0);
}

TEST(sv_from_cstr) {
    StringView sv("hello");
    ASSERT_EQ(sv.length(), (usize)5);
    ASSERT_EQ(sv[0], 'h');
    ASSERT_EQ(sv[4], 'o');
}

TEST(sv_from_cstr_nullptr) {
    StringView sv(nullptr);
    ASSERT(sv.isEmpty());
}

TEST(sv_from_ptr_and_len) {
    const char* s = "hello world";
    StringView sv(s, 5);
    ASSERT_EQ(sv.length(), (usize)5);
    ASSERT_EQ(sv[0], 'h');
}

// -- Comparison --

TEST(sv_eq_same) {
    StringView a("hello");
    StringView b("hello");
    ASSERT(a == b);
}

TEST(sv_eq_different) {
    StringView a("hello");
    StringView b("world");
    ASSERT(a != b);
}

TEST(sv_eq_different_length) {
    StringView a("hello");
    StringView b("hell");
    ASSERT(a != b);
}

TEST(sv_eq_implicit_cstr) {
    StringView sv("hello");
    ASSERT(sv == "hello");
}

TEST(sv_compare_less) {
    StringView a("abc");
    StringView b("abd");
    ASSERT(a.compare(b) < 0);
}

TEST(sv_compare_greater) {
    StringView a("abd");
    StringView b("abc");
    ASSERT(a.compare(b) > 0);
}

TEST(sv_compare_prefix) {
    StringView a("ab");
    StringView b("abc");
    ASSERT(a.compare(b) < 0);
}

// -- Search --

TEST(sv_starts_with) {
    StringView sv("hello world");
    ASSERT(sv.startsWith("hello"));
    ASSERT(!sv.startsWith("world"));
}

TEST(sv_ends_with) {
    StringView sv("hello world");
    ASSERT(sv.endsWith("world"));
    ASSERT(!sv.endsWith("hello"));
}

TEST(sv_contains) {
    StringView sv("hello");
    ASSERT(sv.contains('e'));
    ASSERT(!sv.contains('z'));
}

TEST(sv_index_of) {
    StringView sv("hello");
    ASSERT_EQ(sv.indexOf('l'), (isize)2);
    ASSERT_EQ(sv.indexOf('z'), (isize)-1);
}

// -- Substrings --

TEST(sv_substr_from_pos) {
    StringView sv("hello world");
    StringView sub = sv.substr(6);
    ASSERT(sub == "world");
}

TEST(sv_substr_pos_and_len) {
    StringView sv("hello world");
    StringView sub = sv.substr(0, 5);
    ASSERT(sub == "hello");
}

TEST(sv_substr_past_end) {
    StringView sv("hello");
    StringView sub = sv.substr(10);
    ASSERT(sub.isEmpty());
}

// -- copyTo --

TEST(sv_copy_to) {
    StringView sv("hello");
    char buf[16];
    sv.copyTo(buf, 16);
    ASSERT(str::eq(buf, "hello"));
}

TEST(sv_copy_to_truncates) {
    StringView sv("hello");
    char buf[4];
    sv.copyTo(buf, 4);
    ASSERT(str::eq(buf, "hel"));
}

// -- to<T> --

TEST(sv_to_u32_basic) {
    StringView sv("12345");
    ASSERT_EQ(sv.to<u32>(), (u32)12345);
}

TEST(sv_to_u32_stops_at_nondigit) {
    StringView sv("42abc");
    ASSERT_EQ(sv.to<u32>(), (u32)42);
}

TEST(sv_to_u32_empty) {
    StringView sv("");
    ASSERT_EQ(sv.to<u32>(), (u32)0);
}

// -- Legacy free functions --

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
