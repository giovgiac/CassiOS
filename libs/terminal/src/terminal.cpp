/**
 * terminal.cpp -- Terminal service client implementation
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <std/terminal.hpp>

#include <std/ipc.hpp>
#include <std/ns.hpp>

using namespace std;

terminal::Terminal::Terminal() : pid(0) {
    while (pid == 0) {
        pid = ns::lookup("terminal");
    }
}

void terminal::Terminal::putchar(char c) {
    ipc::Message msg = {};
    msg.type = ipc::MessageType::TerminalPutchar;
    msg.arg1 = static_cast<u8>(c);
    ipc::notify(pid, &msg);
}

void terminal::Terminal::write(const char* str) {
    u32 len = 0;
    while (str[len] != '\0')
        len++;
    write(str, len);
}

void terminal::Terminal::write(const char* buf, u32 len) {
    ipc::Message msg = {};
    msg.type = ipc::MessageType::TerminalWrite;
    msg.arg1 = len;
    ipc::notify(pid, &msg, buf, len);
}

void terminal::Terminal::clear() {
    ipc::Message msg = {};
    msg.type = ipc::MessageType::TerminalClear;
    ipc::send(pid, &msg);
}

void terminal::Terminal::flush() {
    ipc::Message msg = {};
    msg.type = ipc::MessageType::TerminalFlush;
    ipc::notify(pid, &msg);
}

void terminal::Terminal::setCursor(u8 col, u8 row) {
    ipc::Message msg = {};
    msg.type = ipc::MessageType::TerminalSetCursor;
    msg.arg1 = col;
    msg.arg2 = row;
    ipc::send(pid, &msg);
}

void terminal::Terminal::getCursor(u8& col, u8& row) {
    ipc::Message msg = {};
    msg.type = ipc::MessageType::TerminalGetCursor;
    ipc::send(pid, &msg);
    col = static_cast<u8>(msg.arg1);
    row = static_cast<u8>(msg.arg2);
}
