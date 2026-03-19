#include <std/test.hpp>
#include <std/ns.hpp>
#include <std/ipc.hpp>
#include <std/mem.hpp>

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
