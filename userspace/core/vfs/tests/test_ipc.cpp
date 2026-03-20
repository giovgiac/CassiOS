/**
 * test_ipc.cpp -- VFS service IPC integration tests
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <std/str.hpp>
#include <std/test.hpp>
#include <std/vfs.hpp>

using namespace std;
using str::StringView;

TEST(vfs_ipc_mkdir_and_list) {
    vfs::Vfs fs;

    u32 ret = fs.mkdir("/testdir");
    ASSERT_EQ(ret, 0u);

    // Verify the directory appears in the root listing.
    char name[32];
    bool found = false;
    for (u32 i = 0; i < 32; i++) {
        if (!fs.list("/", i, name, sizeof(name))) {
            break;
        }
        if (StringView(name) == "testdir") {
            found = true;
            break;
        }
    }
    ASSERT(found);
}

TEST(vfs_ipc_open_write_read) {
    vfs::Vfs fs;

    u32 handle = fs.open("/hello", true);
    ASSERT(handle != 0);

    const u8 text[] = {'h', 'e', 'l', 'l', 'o'};
    u32 wret = fs.write(handle, text, 5);
    ASSERT_EQ(wret, 0u);

    u8 buf[256] = {};
    i32 n = fs.read(handle, 0, buf, sizeof(buf));
    ASSERT_EQ(n, 5);
    ASSERT_EQ(buf[0], (u8)'h');
    ASSERT_EQ(buf[1], (u8)'e');
    ASSERT_EQ(buf[2], (u8)'l');
    ASSERT_EQ(buf[3], (u8)'l');
    ASSERT_EQ(buf[4], (u8)'o');
}

TEST(vfs_ipc_delete_file) {
    vfs::Vfs fs;

    fs.open("/delme", true);
    u32 ret = fs.remove("/delme");
    ASSERT_EQ(ret, 0u);

    // Verify the file no longer appears in listing.
    char name[32];
    for (u32 i = 0; i < 32; i++) {
        if (!fs.list("/", i, name, sizeof(name))) {
            break;
        }
        ASSERT(!(StringView(name) == "delme"));
    }
}

TEST(vfs_ipc_remove_directory) {
    vfs::Vfs fs;

    fs.mkdir("/rmdir");
    u32 ret = fs.remove("/rmdir");
    ASSERT_EQ(ret, 0u);

    // Verify the directory no longer appears in listing.
    char name[32];
    for (u32 i = 0; i < 32; i++) {
        if (!fs.list("/", i, name, sizeof(name))) {
            break;
        }
        ASSERT(!(StringView(name) == "rmdir"));
    }
}

TEST(vfs_ipc_long_path) {
    vfs::Vfs fs;

    // Create a nested directory structure with path > 20 chars.
    u32 ret = fs.mkdir("/longdir");
    ASSERT_EQ(ret, 0u);

    ret = fs.mkdir("/longdir/subdir");
    ASSERT_EQ(ret, 0u);

    // Create a file in the nested directory.
    u32 handle = fs.open("/longdir/subdir/file", true);
    ASSERT(handle != 0);

    // Verify nested listing.
    char name[32];
    bool found = false;
    for (u32 i = 0; i < 32; i++) {
        if (!fs.list("/longdir/subdir", i, name, sizeof(name))) {
            break;
        }
        if (StringView(name) == "file") {
            found = true;
            break;
        }
    }
    ASSERT(found);
}

TEST(vfs_ipc_large_write_read) {
    vfs::Vfs fs;

    u32 handle = fs.open("/bigfile", true);
    ASSERT(handle != 0);

    // Write 64 bytes (exceeds old 12-byte limit).
    u8 data[64];
    for (u32 i = 0; i < 64; i++)
        data[i] = static_cast<u8>(i + 1);

    u32 wret = fs.write(handle, data, 64);
    ASSERT_EQ(wret, 0u);

    // Read it back.
    u8 buf[256] = {};
    i32 n = fs.read(handle, 0, buf, sizeof(buf));
    ASSERT_EQ(n, 64);
    for (i32 i = 0; i < 64; i++) {
        ASSERT_EQ(buf[i], static_cast<u8>(i + 1));
    }
}

TEST(vfs_ipc_remove_root_fails) {
    vfs::Vfs fs;

    u32 ret = fs.remove("/");
    ASSERT(ret != 0);
}

TEST(vfs_ipc_duplicate_name_fails) {
    vfs::Vfs fs;

    u32 ret = fs.mkdir("/dupdir");
    ASSERT_EQ(ret, 0u);

    ret = fs.mkdir("/dupdir");
    ASSERT(ret != 0);
}

TEST(vfs_ipc_remove_nonempty_dir_fails) {
    vfs::Vfs fs;

    fs.mkdir("/nonempty");
    fs.open("/nonempty/child", true);

    u32 ret = fs.remove("/nonempty");
    ASSERT(ret != 0);
}

TEST(vfs_ipc_read_with_offset) {
    vfs::Vfs fs;

    u32 handle = fs.open("/offtest", true);
    ASSERT(handle != 0);

    const u8 text[] = {'a', 'b', 'c', 'd'};
    fs.write(handle, text, 4);

    u8 buf[256] = {};
    i32 n = fs.read(handle, 2, buf, sizeof(buf));
    ASSERT_EQ(n, 2);
    ASSERT_EQ(buf[0], (u8)'c');
    ASSERT_EQ(buf[1], (u8)'d');
}

TEST(vfs_ipc_empty_file_in_subdir) {
    vfs::Vfs fs;

    // Reproduce: mkdir, touch inside it, then open the empty file.
    fs.mkdir("/subtest");
    u32 h1 = fs.open("/subtest/file.txt", true);
    ASSERT(h1 != 0);

    // Open the same empty file without create -- must find it.
    u32 h2 = fs.open("/subtest/file.txt");
    ASSERT(h2 != 0);

    // Write to it, then read back.
    const u8 text[] = {'O', 'K'};
    u32 wret = fs.write(h2, text, 2);
    ASSERT_EQ(wret, 0u);

    u8 buf[32] = {};
    i32 n = fs.read(h2, 0, buf, sizeof(buf));
    ASSERT_EQ(n, 2);
    ASSERT_EQ(buf[0], (u8)'O');
    ASSERT_EQ(buf[1], (u8)'K');

    // Listing should show exactly one file.txt, not two.
    char name[32];
    u32 count = 0;
    for (u32 i = 0; i < 32; i++) {
        if (!fs.list("/subtest", i, name, sizeof(name)))
            break;
        if (StringView(name) == "file.txt")
            count++;
    }
    ASSERT_EQ(count, 1u);
}

TEST(vfs_ipc_open_nonexistent_fails) {
    vfs::Vfs fs;

    u32 handle = fs.open("/doesnotexist");
    ASSERT_EQ(handle, 0u);
}

TEST(vfs_ipc_dotdot_traversal) {
    vfs::Vfs fs;

    fs.mkdir("/dottest");
    u32 handle = fs.open("/dottest/../dottest/../README.TXT");
    ASSERT(handle != 0);

    u8 buf[32] = {};
    i32 n = fs.read(handle, 0, buf, sizeof(buf));
    ASSERT(n > 0);
    ASSERT_EQ(buf[0], (u8)'W');
}

TEST(vfs_ipc_dot_traversal) {
    vfs::Vfs fs;

    u32 handle = fs.open("/./README.TXT");
    ASSERT(handle != 0);

    u8 buf[32] = {};
    i32 n = fs.read(handle, 0, buf, sizeof(buf));
    ASSERT(n > 0);
    ASSERT_EQ(buf[0], (u8)'W');
}

TEST(vfs_ipc_seed_file_readable) {
    vfs::Vfs fs;

    // The build-time seed file README.TXT should exist.
    u32 handle = fs.open("/README.TXT");
    ASSERT(handle != 0);

    u8 buf[256] = {};
    i32 n = fs.read(handle, 0, buf, sizeof(buf));
    ASSERT(n > 0);

    // First line starts with "Welcome".
    ASSERT_EQ(buf[0], (u8)'W');
    ASSERT_EQ(buf[1], (u8)'e');
    ASSERT_EQ(buf[2], (u8)'l');
}
