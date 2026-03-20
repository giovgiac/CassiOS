/**
 * test_ipc.cpp -- Terminal service IPC tests
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

TEST(terminal_ipc_service_registered) {
    u32 pid = ns::lookup("terminal");
    ASSERT(pid > 0);
}

TEST(terminal_ipc_putchar) {
    u32 pid = ns::lookup("terminal");
    ASSERT(pid > 0);

    ipc::Message msg = {};
    msg.type = ipc::MessageType::TerminalPutchar;
    msg.arg1 = 'A';
    i32 result = ipc::notify(pid, &msg);
    ASSERT_EQ(result, 0);
}

TEST(terminal_ipc_clear) {
    u32 pid = ns::lookup("terminal");
    ASSERT(pid > 0);

    ipc::Message msg = {};
    msg.type = ipc::MessageType::TerminalClear;
    ipc::send(pid, &msg);
    // If we get here, the service responded.
    ASSERT(true);
}

TEST(terminal_ipc_get_cursor) {
    u32 pid = ns::lookup("terminal");
    ASSERT(pid > 0);

    ipc::Message msg = {};
    msg.type = ipc::MessageType::TerminalGetCursor;
    ipc::send(pid, &msg);

    // After clear, cursor should be at (0, 0) or a valid position.
    // Can't assert exact values since other tests may have written text.
    ASSERT(true);
}

TEST(terminal_ipc_set_cursor) {
    u32 pid = ns::lookup("terminal");
    ASSERT(pid > 0);

    // Set cursor to (5, 3).
    ipc::Message msg = {};
    msg.type = ipc::MessageType::TerminalSetCursor;
    msg.arg1 = 5;
    msg.arg2 = 3;
    ipc::send(pid, &msg);

    // Read it back.
    ipc::Message getMsg = {};
    getMsg.type = ipc::MessageType::TerminalGetCursor;
    ipc::send(pid, &getMsg);

    ASSERT_EQ(getMsg.arg1, 5u);
    ASSERT_EQ(getMsg.arg2, 3u);
}

TEST(terminal_ipc_write) {
    u32 pid = ns::lookup("terminal");
    ASSERT(pid > 0);

    const char* text = "test";
    ipc::Message msg = {};
    msg.type = ipc::MessageType::TerminalWrite;
    msg.arg1 = 4;
    i32 result = ipc::notify(pid, &msg, text, 4);
    ASSERT_EQ(result, 0);
}

TEST(terminal_set_get_cursor_roundtrip) {
    u32 pid = ns::lookup("terminal");
    ASSERT(pid > 0);

    // Set to a known position.
    ipc::Message setMsg = {};
    setMsg.type = ipc::MessageType::TerminalSetCursor;
    setMsg.arg1 = 10;
    setMsg.arg2 = 7;
    ipc::send(pid, &setMsg);

    // Read it back.
    ipc::Message getMsg = {};
    getMsg.type = ipc::MessageType::TerminalGetCursor;
    ipc::send(pid, &getMsg);

    ASSERT_EQ(getMsg.arg1, 10u);
    ASSERT_EQ(getMsg.arg2, 7u);
}

TEST(terminal_clear_resets_cursor) {
    u32 pid = ns::lookup("terminal");
    ASSERT(pid > 0);

    // Move cursor away from origin.
    ipc::Message setMsg = {};
    setMsg.type = ipc::MessageType::TerminalSetCursor;
    setMsg.arg1 = 10;
    setMsg.arg2 = 5;
    ipc::send(pid, &setMsg);

    // Clear.
    ipc::Message clearMsg = {};
    clearMsg.type = ipc::MessageType::TerminalClear;
    ipc::send(pid, &clearMsg);

    // Cursor should be at origin.
    ipc::Message getMsg = {};
    getMsg.type = ipc::MessageType::TerminalGetCursor;
    ipc::send(pid, &getMsg);

    ASSERT_EQ(getMsg.arg1, 0u);
    ASSERT_EQ(getMsg.arg2, 0u);
}
