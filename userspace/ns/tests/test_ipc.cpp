#include <test.hpp>
#include <ns.hpp>
#include <string.hpp>

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

TEST(ns_ipc_list_all_returns_registered_services) {
    // Services already registered by earlier tests and by the real services
    // running in the system. Just verify we get a non-zero count and valid data.
    NsEntry buf[16];
    u32 count = Nameserver::listAll(buf, 16);
    ASSERT(count > 0);

    // Every returned entry should have a non-empty name and non-zero PID.
    for (u32 i = 0; i < count; ++i) {
        ASSERT(buf[i].name[0] != '\0');
        ASSERT(buf[i].pid != 0);
    }
}

TEST(ns_ipc_list_all_contains_known_service) {
    // The "kbd" service is always running in the test environment.
    NsEntry buf[16];
    u32 count = Nameserver::listAll(buf, 16);

    bool found = false;
    for (u32 i = 0; i < count; ++i) {
        if (streq(buf[i].name, "kbd")) {
            found = true;
            break;
        }
    }
    ASSERT(found);
}
