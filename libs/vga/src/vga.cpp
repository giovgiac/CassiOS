/**
 * vga.cpp -- VGA terminal service client implementation
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <std/ipc.hpp>
#include <std/ns.hpp>
#include <std/vga.hpp>

using namespace std;

vga::Vga::Vga() : pid(0) {
    while (pid == 0) {
        pid = ns::lookup("vga");
    }
}

void vga::Vga::putchar(char c) {
    ipc::Message msg = {};
    msg.type = ipc::MessageType::VgaPutchar;
    msg.arg1 = static_cast<u8>(c);
    ipc::notify(pid, &msg);
}

void vga::Vga::write(const char* str) {
    u32 len = 0;
    while (str[len] != '\0')
        len++;
    write(str, len);
}

void vga::Vga::write(const char* buf, u32 len) {
    ipc::Message msg = {};
    msg.type = ipc::MessageType::VgaWrite;
    msg.arg1 = len;
    ipc::notify(pid, &msg, buf, len);
}

void vga::Vga::clear() {
    ipc::Message msg = {};
    msg.type = ipc::MessageType::VgaClear;
    ipc::send(pid, &msg);
}

void vga::Vga::setCursor(u8 col, u8 row) {
    ipc::Message msg = {};
    msg.type = ipc::MessageType::VgaSetCursor;
    msg.arg1 = col;
    msg.arg2 = row;
    ipc::send(pid, &msg);
}

void vga::Vga::getCursor(u8& col, u8& row) {
    ipc::Message msg = {};
    msg.type = ipc::MessageType::VgaGetCursor;
    ipc::send(pid, &msg);
    col = static_cast<u8>(msg.arg1);
    row = static_cast<u8>(msg.arg2);
}
