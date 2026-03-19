#include <std/test.hpp>
#include <std/ns.hpp>
#include <std/str.hpp>

using namespace std;

TEST(ns_ipc_register_and_lookup) {
    u32 ok = ns::registerName("test_svc");
    ASSERT_EQ(ok, 1u);

    u32 pid = ns::lookup("test_svc");
    ASSERT(pid != 0);
}

TEST(ns_ipc_lookup_unregistered_returns_zero) {
    u32 pid = ns::lookup("nonexist");
    ASSERT_EQ(pid, 0u);
}

TEST(ns_ipc_register_multiple_and_lookup) {
    ns::registerName("svc_a");
    ns::registerName("svc_b");
    ns::registerName("svc_c");

    // All registered by the same process, so all return the same PID.
    u32 a = ns::lookup("svc_a");
    u32 b = ns::lookup("svc_b");
    u32 c = ns::lookup("svc_c");
    ASSERT(a != 0);
    ASSERT_EQ(a, b);
    ASSERT_EQ(b, c);
}

TEST(ns_ipc_register_duplicate_fails) {
    ns::registerName("dup_test");
    u32 ok = ns::registerName("dup_test");
    ASSERT_EQ(ok, 0u);
}

TEST(ns_ipc_list_all_returns_registered_services) {
    // Services already registered by earlier tests and by the real services
    // running in the system. Just verify we get a non-zero count and valid data.
    ns::Entry buf[16];
    u32 count = ns::listAll(buf, 16);
    ASSERT(count > 0);

    // Every returned entry should have a non-empty name and non-zero PID.
    for (u32 i = 0; i < count; ++i) {
        ASSERT(buf[i].name[0] != '\0');
        ASSERT(buf[i].pid != 0);
    }
}

TEST(ns_pack_unpack_roundtrip) {
    ipc::Message msg = {};
    ns::packName("hello", msg);

    char out[17];
    ns::unpackName(msg, out);
    ASSERT(out[0] == 'h');
    ASSERT(out[1] == 'e');
    ASSERT(out[2] == 'l');
    ASSERT(out[3] == 'l');
    ASSERT(out[4] == 'o');
    ASSERT(out[5] == '\0');
}

TEST(ns_pack_unpack_max_length) {
    ipc::Message msg = {};
    ns::packName("0123456789abcdef", msg);

    char out[17];
    ns::unpackName(msg, out);
    ASSERT(out[0] == '0');
    ASSERT(out[15] == 'f');
    ASSERT(out[16] == '\0');
}

TEST(ns_pack_empty_string) {
    ipc::Message msg = {};
    ns::packName("", msg);

    char out[17];
    ns::unpackName(msg, out);
    ASSERT(out[0] == '\0');
}

TEST(ns_ipc_list_all_contains_known_service) {
    // The "kbd" service is always running in the test environment.
    ns::Entry buf[16];
    u32 count = ns::listAll(buf, 16);

    bool found = false;
    for (u32 i = 0; i < count; ++i) {
        if (str::eq(buf[i].name, "kbd")) {
            found = true;
            break;
        }
    }
    ASSERT(found);
}
