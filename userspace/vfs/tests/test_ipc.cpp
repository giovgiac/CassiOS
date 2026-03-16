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

    u8 buf[16] = {};
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
