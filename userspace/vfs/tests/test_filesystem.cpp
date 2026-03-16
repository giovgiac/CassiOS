/**
 * test_filesystem.cpp -- VFS filesystem unit tests
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <test.hpp>
#include <filesystem.hpp>

using namespace cassio;
using namespace cassio::vfs;

static Filesystem fs;

TEST(vfs_create_directory) {
    fs.init();
    u8 node = fs.createDirectory("/mydir");
    ASSERT(node != INVALID);
    ASSERT(fs.isDirectory(node));
}

TEST(vfs_create_file) {
    fs.init();
    u8 node = fs.createFile("/myfile");
    ASSERT(node != INVALID);
    ASSERT(fs.isFile(node));
}

TEST(vfs_resolve_root) {
    fs.init();
    u8 node = fs.resolve("/");
    ASSERT_EQ(node, ROOT_INDEX);
    ASSERT(fs.isDirectory(node));
}

TEST(vfs_resolve_path) {
    fs.init();
    fs.createDirectory("/adir");
    u8 node = fs.resolve("/adir");
    ASSERT(node != INVALID);
    ASSERT(fs.isDirectory(node));
}

TEST(vfs_resolve_nested) {
    fs.init();
    fs.createDirectory("/a");
    fs.createDirectory("/a/b");
    u8 node = fs.resolve("/a/b");
    ASSERT(node != INVALID);
    ASSERT(fs.isDirectory(node));
}

TEST(vfs_duplicate_name_fails) {
    fs.init();
    fs.createFile("/dup");
    u8 second = fs.createFile("/dup");
    ASSERT_EQ(second, INVALID);
}

TEST(vfs_remove_file) {
    fs.init();
    fs.createFile("/rmme");
    ASSERT(fs.remove("/rmme"));
    ASSERT_EQ(fs.resolve("/rmme"), INVALID);
}

TEST(vfs_remove_empty_directory) {
    fs.init();
    fs.createDirectory("/emptydir");
    ASSERT(fs.remove("/emptydir"));
    ASSERT_EQ(fs.resolve("/emptydir"), INVALID);
}

TEST(vfs_remove_nonempty_directory_fails) {
    fs.init();
    fs.createDirectory("/parent");
    fs.createFile("/parent/child");
    ASSERT(!fs.remove("/parent"));
    ASSERT(fs.isValid(fs.resolve("/parent")));
}

TEST(vfs_write_and_read) {
    fs.init();
    u8 node = fs.createFile("/data");
    ASSERT(node != INVALID);

    const u8 text[] = {'h', 'e', 'l', 'l', 'o'};
    ASSERT(fs.write(node, text, 5));

    u8 buf[16] = {};
    i32 n = fs.read(node, 0, buf, sizeof(buf));
    ASSERT_EQ(n, 5);
    ASSERT_EQ(buf[0], (u8)'h');
    ASSERT_EQ(buf[1], (u8)'e');
    ASSERT_EQ(buf[2], (u8)'l');
    ASSERT_EQ(buf[3], (u8)'l');
    ASSERT_EQ(buf[4], (u8)'o');
}

TEST(vfs_read_with_offset) {
    fs.init();
    u8 node = fs.createFile("/off");
    const u8 text[] = {'a', 'b', 'c', 'd'};
    fs.write(node, text, 4);

    u8 buf[16] = {};
    i32 n = fs.read(node, 2, buf, sizeof(buf));
    ASSERT_EQ(n, 2);
    ASSERT_EQ(buf[0], (u8)'c');
    ASSERT_EQ(buf[1], (u8)'d');
}

TEST(vfs_list_directory) {
    fs.init();
    fs.createDirectory("/ldir");
    fs.createFile("/lfile");

    char name[32];
    NodeType type;

    ASSERT(fs.listEntry(ROOT_INDEX, 0, name, sizeof(name), type));
    ASSERT(fs.listEntry(ROOT_INDEX, 1, name, sizeof(name), type));
    ASSERT(!fs.listEntry(ROOT_INDEX, 2, name, sizeof(name), type));
}

TEST(vfs_remove_root_fails) {
    fs.init();
    ASSERT(!fs.remove("/"));
}
