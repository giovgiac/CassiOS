#include <std/types.hpp>
#include <std/test.hpp>

TEST(types_sizes) {
    ASSERT_EQ(sizeof(std::u8), 1u);
    ASSERT_EQ(sizeof(std::u16), 2u);
    ASSERT_EQ(sizeof(std::u32), 4u);
    ASSERT_EQ(sizeof(std::u64), 8u);
    ASSERT_EQ(sizeof(std::i8), 1u);
    ASSERT_EQ(sizeof(std::i16), 2u);
    ASSERT_EQ(sizeof(std::i32), 4u);
    ASSERT_EQ(sizeof(std::i64), 8u);
    ASSERT_EQ(sizeof(std::f32), 4u);
    ASSERT_EQ(sizeof(std::f64), 8u);
    ASSERT_EQ(sizeof(std::usize), 4u);
    ASSERT_EQ(sizeof(std::isize), 4u);
}
