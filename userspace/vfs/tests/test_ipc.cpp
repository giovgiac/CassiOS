/**
 * test_ipc.cpp -- VFS service IPC integration tests
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <test.hpp>
#include <ns.hpp>
#include <vfs.hpp>
#include <string.hpp>

using namespace cassio;

TEST(vfs_ipc_mkdir_and_list) {
    u32 pid = Nameserver::lookup("vfs");
    ASSERT(pid != 0);

    u32 ret = Vfs::mkdir(pid, "/testdir");
    ASSERT_EQ(ret, 0u);

    // Verify the directory appears in the root listing.
    char name[32];
    bool found = false;
    for (u32 i = 0; i < 32; i++) {
        if (!Vfs::list(pid, "/", i, name, sizeof(name))) {
            break;
        }
        if (streq(name, "testdir")) {
            found = true;
            break;
        }
    }
    ASSERT(found);
}

TEST(vfs_ipc_open_write_read) {
    u32 pid = Nameserver::lookup("vfs");
    ASSERT(pid != 0);

    u32 handle = Vfs::open(pid, "/hello");
    ASSERT(handle != 0);

    const u8 text[] = {'h', 'e', 'l', 'l', 'o'};
    u32 wret = Vfs::write(pid, handle, text, 5);
    ASSERT_EQ(wret, 0u);

    u8 buf[256] = {};
    i32 n = Vfs::read(pid, handle, 0, buf, sizeof(buf));
    ASSERT_EQ(n, 5);
    ASSERT_EQ(buf[0], (u8)'h');
    ASSERT_EQ(buf[1], (u8)'e');
    ASSERT_EQ(buf[2], (u8)'l');
    ASSERT_EQ(buf[3], (u8)'l');
    ASSERT_EQ(buf[4], (u8)'o');
}

TEST(vfs_ipc_delete_file) {
    u32 pid = Nameserver::lookup("vfs");
    ASSERT(pid != 0);

    Vfs::open(pid, "/delme");
    u32 ret = Vfs::remove(pid, "/delme");
    ASSERT_EQ(ret, 0u);

    // Verify the file no longer appears in listing.
    char name[32];
    for (u32 i = 0; i < 32; i++) {
        if (!Vfs::list(pid, "/", i, name, sizeof(name))) {
            break;
        }
        ASSERT(!streq(name, "delme"));
    }
}

TEST(vfs_ipc_remove_directory) {
    u32 pid = Nameserver::lookup("vfs");
    ASSERT(pid != 0);

    Vfs::mkdir(pid, "/rmdir");
    u32 ret = Vfs::remove(pid, "/rmdir");
    ASSERT_EQ(ret, 0u);

    // Verify the directory no longer appears in listing.
    char name[32];
    for (u32 i = 0; i < 32; i++) {
        if (!Vfs::list(pid, "/", i, name, sizeof(name))) {
            break;
        }
        ASSERT(!streq(name, "rmdir"));
    }
}

TEST(vfs_ipc_long_path) {
    u32 pid = Nameserver::lookup("vfs");
    ASSERT(pid != 0);

    // Create a nested directory structure with path > 20 chars.
    u32 ret = Vfs::mkdir(pid, "/longdir");
    ASSERT_EQ(ret, 0u);

    ret = Vfs::mkdir(pid, "/longdir/subdir");
    ASSERT_EQ(ret, 0u);

    // Create a file in the nested directory.
    u32 handle = Vfs::open(pid, "/longdir/subdir/file");
    ASSERT(handle != 0);

    // Verify nested listing.
    char name[32];
    bool found = false;
    for (u32 i = 0; i < 32; i++) {
        if (!Vfs::list(pid, "/longdir/subdir", i, name, sizeof(name))) {
            break;
        }
        if (streq(name, "file")) {
            found = true;
            break;
        }
    }
    ASSERT(found);
}

TEST(vfs_ipc_large_write_read) {
    u32 pid = Nameserver::lookup("vfs");
    ASSERT(pid != 0);

    u32 handle = Vfs::open(pid, "/bigfile");
    ASSERT(handle != 0);

    // Write 64 bytes (exceeds old 12-byte limit).
    u8 data[64];
    for (u32 i = 0; i < 64; i++) data[i] = static_cast<u8>(i + 1);

    u32 wret = Vfs::write(pid, handle, data, 64);
    ASSERT_EQ(wret, 0u);

    // Read it back.
    u8 buf[256] = {};
    i32 n = Vfs::read(pid, handle, 0, buf, sizeof(buf));
    ASSERT_EQ(n, 64);
    for (i32 i = 0; i < 64; i++) {
        ASSERT_EQ(buf[i], static_cast<u8>(i + 1));
    }
}

TEST(vfs_ipc_remove_root_fails) {
    u32 pid = Nameserver::lookup("vfs");
    ASSERT(pid != 0);

    u32 ret = Vfs::remove(pid, "/");
    ASSERT(ret != 0);
}

TEST(vfs_ipc_duplicate_name_fails) {
    u32 pid = Nameserver::lookup("vfs");
    ASSERT(pid != 0);

    u32 ret = Vfs::mkdir(pid, "/dupdir");
    ASSERT_EQ(ret, 0u);

    ret = Vfs::mkdir(pid, "/dupdir");
    ASSERT(ret != 0);
}

TEST(vfs_ipc_remove_nonempty_dir_fails) {
    u32 pid = Nameserver::lookup("vfs");
    ASSERT(pid != 0);

    Vfs::mkdir(pid, "/nonempty");
    Vfs::open(pid, "/nonempty/child");

    u32 ret = Vfs::remove(pid, "/nonempty");
    ASSERT(ret != 0);
}

TEST(vfs_ipc_read_with_offset) {
    u32 pid = Nameserver::lookup("vfs");
    ASSERT(pid != 0);

    u32 handle = Vfs::open(pid, "/offtest");
    ASSERT(handle != 0);

    const u8 text[] = {'a', 'b', 'c', 'd'};
    Vfs::write(pid, handle, text, 4);

    u8 buf[256] = {};
    i32 n = Vfs::read(pid, handle, 2, buf, sizeof(buf));
    ASSERT_EQ(n, 2);
    ASSERT_EQ(buf[0], (u8)'c');
    ASSERT_EQ(buf[1], (u8)'d');
}

TEST(vfs_ipc_seed_file_readable) {
    u32 pid = Nameserver::lookup("vfs");
    ASSERT(pid != 0);

    // The build-time seed file README.TXT should exist.
    u32 handle = Vfs::open(pid, "/README.TXT");
    ASSERT(handle != 0);

    u8 buf[256] = {};
    i32 n = Vfs::read(pid, handle, 0, buf, sizeof(buf));
    ASSERT(n > 0);

    // First line starts with "Welcome".
    ASSERT_EQ(buf[0], (u8)'W');
    ASSERT_EQ(buf[1], (u8)'e');
    ASSERT_EQ(buf[2], (u8)'l');
}
