#include <test.hpp>
#include <ns.hpp>

using namespace cassio;

TEST(ns_ipc_register_and_lookup) {
    u32 ok = Nameserver::registerName("test_svc");
    ASSERT_EQ(ok, 1u);

    u32 pid = Nameserver::lookup("test_svc");
    ASSERT(pid != 0);
}

TEST(ns_ipc_lookup_unregistered_returns_zero) {
    u32 pid = Nameserver::lookup("nonexist");
    ASSERT_EQ(pid, 0u);
}

TEST(ns_ipc_register_multiple_and_lookup) {
    Nameserver::registerName("svc_a");
    Nameserver::registerName("svc_b");
    Nameserver::registerName("svc_c");

    // All registered by the same process, so all return the same PID.
    u32 a = Nameserver::lookup("svc_a");
    u32 b = Nameserver::lookup("svc_b");
    u32 c = Nameserver::lookup("svc_c");
    ASSERT(a != 0);
    ASSERT_EQ(a, b);
    ASSERT_EQ(b, c);
}

TEST(ns_ipc_register_duplicate_fails) {
    Nameserver::registerName("dup_test");
    u32 ok = Nameserver::registerName("dup_test");
    ASSERT_EQ(ok, 0u);
}
