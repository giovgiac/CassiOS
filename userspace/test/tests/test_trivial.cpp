#include <test.hpp>
#include <system.hpp>

using namespace cassio;

TEST(userspace_assert_true) {
    ASSERT(1 == 1);
}

TEST(userspace_assert_eq) {
    ASSERT_EQ(42u, 42u);
}

TEST(userspace_write_to_serial) {
    i32 result = System::write(2, "hello", 5);
    ASSERT_EQ(result, 5);
}

TEST(userspace_uptime_nonnegative) {
    i32 ticks = System::uptime();
    ASSERT(ticks >= 0);
}
