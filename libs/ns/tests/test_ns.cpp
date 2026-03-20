#include <std/ns.hpp>
#include <std/test.hpp>

using namespace std;

TEST(ns_entry_size) {
    ASSERT_EQ(sizeof(ns::Entry), 24u);
}

TEST(ns_pid_is_one) {
    ASSERT_EQ(ns::PID, 1u);
}

TEST(ns_max_name_len) {
    ASSERT_EQ(ns::MAX_NAME_LEN, 16u);
}
