#include <std/mem.hpp>
#include <test.hpp>

using namespace std;

TEST(mem_copy_basic) {
    u8 src[] = {1, 2, 3, 4, 5};
    u8 dst[5] = {};
    mem::copy(dst, src, 5);
    for (usize i = 0; i < 5; i++) {
        ASSERT_EQ(dst[i], src[i]);
    }
}

TEST(mem_copy_returns_dst) {
    u8 src[] = {1};
    u8 dst[1] = {};
    void* result = mem::copy(dst, src, 1);
    ASSERT_EQ(reinterpret_cast<usize>(result), reinterpret_cast<usize>(dst));
}

TEST(mem_move_forward_overlap) {
    u8 buf[] = {1, 2, 3, 4, 5};
    mem::move(buf, buf + 2, 3);
    ASSERT_EQ(buf[0], (u8)3);
    ASSERT_EQ(buf[1], (u8)4);
    ASSERT_EQ(buf[2], (u8)5);
}

TEST(mem_move_backward_overlap) {
    u8 buf[] = {1, 2, 3, 4, 5};
    mem::move(buf + 2, buf, 3);
    ASSERT_EQ(buf[2], (u8)1);
    ASSERT_EQ(buf[3], (u8)2);
    ASSERT_EQ(buf[4], (u8)3);
}

TEST(mem_move_returns_dst) {
    u8 buf[] = {1, 2, 3};
    void* result = mem::move(buf + 1, buf, 2);
    ASSERT_EQ(reinterpret_cast<usize>(result), reinterpret_cast<usize>(buf + 1));
}

TEST(mem_set_basic) {
    u8 buf[4] = {};
    mem::set(buf, 0xAB, 4);
    for (usize i = 0; i < 4; i++) {
        ASSERT_EQ(buf[i], (u8)0xAB);
    }
}

TEST(mem_set_returns_dst) {
    u8 buf[1] = {};
    void* result = mem::set(buf, 0, 1);
    ASSERT_EQ(reinterpret_cast<usize>(result), reinterpret_cast<usize>(buf));
}

TEST(mem_compare_equal) {
    u8 a[] = {1, 2, 3};
    u8 b[] = {1, 2, 3};
    ASSERT_EQ(mem::compare(a, b, 3), 0);
}

TEST(mem_compare_less) {
    u8 a[] = {1, 2, 3};
    u8 b[] = {1, 2, 4};
    ASSERT_EQ(mem::compare(a, b, 3), -1);
}

TEST(mem_compare_greater) {
    u8 a[] = {1, 3, 3};
    u8 b[] = {1, 2, 3};
    ASSERT_EQ(mem::compare(a, b, 3), 1);
}

TEST(mem_compare_zero_length) {
    u8 a[] = {1};
    u8 b[] = {2};
    ASSERT_EQ(mem::compare(a, b, 0), 0);
}
