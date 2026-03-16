#include <test.hpp>
#include <table.hpp>

using namespace cassio;

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

TEST(ns_table_full_capacity) {
    NsTable table;
    char name[3];
    for (u32 i = 0; i < NsTable::MAX_ENTRIES; i++) {
        name[0] = 'a' + static_cast<char>(i / 26);
        name[1] = 'a' + static_cast<char>(i % 26);
        name[2] = '\0';
        ASSERT_EQ(table.registerName(name, i + 1), 1u);
    }
    ASSERT_EQ(table.count(), NsTable::MAX_ENTRIES);
    ASSERT_EQ(table.registerName("overflow", 99), 0u);
}
