#include <std/test.hpp>
#include <table.hpp>
#include <std/str.hpp>

using namespace cassio;
using namespace std;
using str::StringView;

TEST(ns_table_register_and_lookup) {
    NsTable table;
    ASSERT_EQ(table.registerName("test", 42), 1u);
    ASSERT_EQ(table.lookup("test"), 42u);
}

TEST(ns_table_lookup_unregistered) {
    NsTable table;
    ASSERT_EQ(table.lookup("nope"), 0u);
}

TEST(ns_table_register_multiple) {
    NsTable table;
    table.registerName("a", 1);
    table.registerName("b", 2);
    table.registerName("c", 3);
    ASSERT_EQ(table.lookup("a"), 1u);
    ASSERT_EQ(table.lookup("b"), 2u);
    ASSERT_EQ(table.lookup("c"), 3u);
}

TEST(ns_table_register_duplicate_fails) {
    NsTable table;
    ASSERT_EQ(table.registerName("dup", 1), 1u);
    ASSERT_EQ(table.registerName("dup", 2), 0u);
    ASSERT_EQ(table.lookup("dup"), 1u);
}

TEST(ns_table_beyond_old_limit) {
    NsTable table;
    char name[4];
    // Should be able to register more than the old fixed limit of 16.
    for (u32 i = 0; i < 20; i++) {
        name[0] = 'a' + static_cast<char>(i / 26);
        name[1] = 'a' + static_cast<char>(i % 26);
        name[2] = 'x';
        name[3] = '\0';
        ASSERT_EQ(table.registerName(name, i + 1), 1u);
    }
    ASSERT_EQ(table.count(), 20u);
}

TEST(ns_table_list_all_returns_entries) {
    NsTable table;
    table.registerName("vga", 3);
    table.registerName("kbd", 2);

    ns::Entry buf[8];
    u32 count = table.listAll(buf, 8);
    ASSERT_EQ(count, 2u);

    // Verify both entries are present (order may vary).
    bool foundVga = false;
    bool foundKbd = false;
    for (u32 i = 0; i < count; ++i) {
        if (StringView(buf[i].name) == "vga" && buf[i].pid == 3) foundVga = true;
        if (StringView(buf[i].name) == "kbd" && buf[i].pid == 2) foundKbd = true;
    }
    ASSERT(foundVga);
    ASSERT(foundKbd);
}

TEST(ns_table_list_all_empty) {
    NsTable table;
    ns::Entry buf[8];
    u32 count = table.listAll(buf, 8);
    ASSERT_EQ(count, 0u);
}

TEST(ns_table_list_all_respects_max) {
    NsTable table;
    table.registerName("a", 1);
    table.registerName("b", 2);
    table.registerName("c", 3);

    ns::Entry buf[2];
    u32 count = table.listAll(buf, 2);
    ASSERT_EQ(count, 2u);
}
