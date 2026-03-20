/**
 * main.cpp -- Display service entry point
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Userspace VESA framebuffer display service. Registers as "display"
 * with the nameserver and handles pixel-level drawing requests via IPC.
 * Maps the VESA framebuffer into its address space and allocates a
 * back buffer via sbrk. All drawing goes to the back buffer;
 * DisplayFlush copies it to the framebuffer.
 *
 */

#include <std/ipc.hpp>
#include <std/ns.hpp>
#include <std/os.hpp>
#include <std/types.hpp>

#include <display.hpp>

using namespace cassio;
using namespace std;

extern "C" void _start() {
    ns::registerName("display");

    os::FramebufferInfo fb = os::framebufferInfo();

    // Calculate page count for MapDevice.
    u32 fbSizeBytes = fb.pitch * fb.height;
    u32 pages = (fbSizeBytes + 0xFFF) / 0x1000;

    // Map framebuffer into our address space. The physical address is
    // typically above KERNEL_VBASE (e.g., 0xFD000000), so we map it
    // at a fixed userspace virtual address.
    static constexpr u32 FB_VIRT = 0x10000000;
    os::mapDevice(fb.address, FB_VIRT, pages);
    u32* framebuffer = reinterpret_cast<u32*>(FB_VIRT);

    // Allocate back buffer via sbrk.
    u32* backBuffer = static_cast<u32*>(os::sbrk(fbSizeBytes));

    Display display(framebuffer, backBuffer, fb.width, fb.height, fb.pitch);

    // Clear to black and flush.
    display.fillRect(0, 0, fb.width, fb.height, 0x000000);
    display.flush();

    while (true) {
        ipc::Message msg;
        // Data buffer for DisplayBlit pixel data.
        // Max blit: 8x16 glyph = 512 bytes (one character cell at 32bpp).
        // Larger blits are possible but 1024 bytes covers typical usage.
        u8 dataBuf[1024];
        i32 sender = ipc::receive(&msg, dataBuf, sizeof(dataBuf));

        switch (msg.type) {
        case ipc::MessageType::DisplayFillRect:
            display.fillRect(msg.arg1, msg.arg2, msg.arg3, msg.arg4, msg.arg5);
            break;

        case ipc::MessageType::DisplayDrawRect:
            display.drawRect(msg.arg1, msg.arg2, msg.arg3, msg.arg4, msg.arg5);
            break;

        case ipc::MessageType::DisplayBlit:
            display.blit(msg.arg1, msg.arg2, msg.arg3, msg.arg4,
                         reinterpret_cast<const u32*>(dataBuf));
            break;

        case ipc::MessageType::DisplayDrawChar:
            display.drawChar(msg.arg1, msg.arg2, static_cast<char>(msg.arg3), msg.arg4, msg.arg5);
            break;

        case ipc::MessageType::DisplayScroll:
            display.scroll(msg.arg1, msg.arg2);
            break;

        case ipc::MessageType::DisplayFlush:
            display.flush();
            break;

        case ipc::MessageType::DisplayGetInfo:
            break;

        default:
            break;
        }

        if (sender > 0) {
            ipc::Message reply = {};
            reply.arg1 = display.getWidth();
            reply.arg2 = display.getHeight();
            reply.arg3 = display.getPitch();
            reply.arg4 = display.getBpp();
            ipc::reply(static_cast<u32>(sender), &reply);
        }
    }
}
