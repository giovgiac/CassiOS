/**
 * display.cpp -- Display service client implementation
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <std/display.hpp>

#include <std/ipc.hpp>
#include <std/ns.hpp>

using namespace std;

display::Display::Display() : pid(0) {
    while (pid == 0) {
        pid = ns::lookup("display");
    }
}

void display::Display::fillRect(u32 x, u32 y, u32 w, u32 h, gfx::Color color) {
    ipc::Message msg = {};
    msg.type = ipc::MessageType::DisplayFillRect;
    msg.arg1 = x;
    msg.arg2 = y;
    msg.arg3 = w;
    msg.arg4 = h;
    msg.arg5 = color;
    ipc::notify(pid, &msg);
}

void display::Display::drawRect(u32 x, u32 y, u32 w, u32 h, gfx::Color color) {
    ipc::Message msg = {};
    msg.type = ipc::MessageType::DisplayDrawRect;
    msg.arg1 = x;
    msg.arg2 = y;
    msg.arg3 = w;
    msg.arg4 = h;
    msg.arg5 = color;
    ipc::notify(pid, &msg);
}

void display::Display::blit(u32 x, u32 y, u32 w, u32 h, const u32* pixels) {
    ipc::Message msg = {};
    msg.type = ipc::MessageType::DisplayBlit;
    msg.arg1 = x;
    msg.arg2 = y;
    msg.arg3 = w;
    msg.arg4 = h;
    ipc::send(pid, &msg, pixels, w * h * sizeof(u32));
}

void display::Display::drawChar(u32 x, u32 y, char ch, gfx::Color fg, gfx::Color bg) {
    ipc::Message msg = {};
    msg.type = ipc::MessageType::DisplayDrawChar;
    msg.arg1 = x;
    msg.arg2 = y;
    msg.arg3 = static_cast<u32>(static_cast<u8>(ch));
    msg.arg4 = fg;
    msg.arg5 = bg;
    ipc::notify(pid, &msg);
}

void display::Display::scroll(u32 pixels, gfx::Color color) {
    ipc::Message msg = {};
    msg.type = ipc::MessageType::DisplayScroll;
    msg.arg1 = pixels;
    msg.arg2 = color;
    ipc::notify(pid, &msg);
}

void display::Display::flush() {
    ipc::Message msg = {};
    msg.type = ipc::MessageType::DisplayFlush;
    ipc::notify(pid, &msg);
}

display::DisplayInfo display::Display::getInfo() {
    ipc::Message msg = {};
    msg.type = ipc::MessageType::DisplayGetInfo;
    ipc::send(pid, &msg);
    return {msg.arg1, msg.arg2, msg.arg3, msg.arg4};
}
