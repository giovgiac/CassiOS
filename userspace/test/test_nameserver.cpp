#include <test.hpp>
#include <nameserver.hpp>

using namespace cassio;

TEST(ns_register_and_lookup) {
    u32 ok = Nameserver::registerName("test_svc");
    ASSERT_EQ(ok, 1u);

    u32 pid = Nameserver::lookup("test_svc");
    // Our PID is 2 (kernel=0, nameserver=1, us=2).
    ASSERT_EQ(pid, 2u);
}

TEST(ns_lookup_unregistered_returns_zero) {
    u32 pid = Nameserver::lookup("nonexist");
    ASSERT_EQ(pid, 0u);
}

TEST(ns_register_multiple_and_lookup) {
    Nameserver::registerName("svc_a");
    Nameserver::registerName("svc_b");
    Nameserver::registerName("svc_c");

    // All should resolve to our PID (2).
    ASSERT_EQ(Nameserver::lookup("svc_a"), 2u);
    ASSERT_EQ(Nameserver::lookup("svc_b"), 2u);
    ASSERT_EQ(Nameserver::lookup("svc_c"), 2u);
}

TEST(ns_register_duplicate_fails) {
    Nameserver::registerName("dup_test");
    u32 ok = Nameserver::registerName("dup_test");
    ASSERT_EQ(ok, 0u);
}
