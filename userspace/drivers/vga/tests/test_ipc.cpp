/**
 * test_ipc.cpp -- VGA service IPC integration tests
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <std/ipc.hpp>
#include <std/ns.hpp>
#include <std/test.hpp>

using namespace std;

TEST(vga_ipc_putchar) {
    u32 pid = ns::lookup("vga");
    ASSERT(pid != 0);

    ipc::Message msg = {};
    msg.type = ipc::MessageType::VgaPutchar;
    msg.arg1 = 'T';
    i32 ret = ipc::send(pid, &msg);
    ASSERT_EQ(ret, 0);
}

TEST(vga_ipc_clear) {
    u32 pid = ns::lookup("vga");
    ASSERT(pid != 0);

    ipc::Message msg = {};
    msg.type = ipc::MessageType::VgaClear;
    i32 ret = ipc::send(pid, &msg);
    ASSERT_EQ(ret, 0);
}

TEST(vga_ipc_set_cursor) {
    u32 pid = ns::lookup("vga");
    ASSERT(pid != 0);

    ipc::Message msg = {};
    msg.type = ipc::MessageType::VgaSetCursor;
    msg.arg1 = 5;
    msg.arg2 = 3;
    i32 ret = ipc::send(pid, &msg);
    ASSERT_EQ(ret, 0);

    // Reply carries cursor position back.
    ASSERT_EQ(msg.arg1, 5u);
    ASSERT_EQ(msg.arg2, 3u);
}

TEST(vga_ipc_write) {
    u32 pid = ns::lookup("vga");
    ASSERT(pid != 0);

    const char* text = "Hi";
    ipc::Message msg = {};
    msg.type = ipc::MessageType::VgaWrite;
    msg.arg1 = 2;
    i32 ret = ipc::send(pid, &msg, text, 2);
    ASSERT_EQ(ret, 0);
}

TEST(vga_ipc_write_long) {
    u32 pid = ns::lookup("vga");
    ASSERT(pid != 0);

    const char* text = "This is a long string that exceeds 20 characters!";
    u32 len = 0;
    while (text[len])
        len++;

    ipc::Message msg = {};
    msg.type = ipc::MessageType::VgaWrite;
    msg.arg1 = len;
    i32 ret = ipc::send(pid, &msg, text, len);
    ASSERT_EQ(ret, 0);
}
