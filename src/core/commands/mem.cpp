/**
 * mem.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "core/commands/mem.hpp"
#include "hardware/terminal.hpp"
#include "memory/physical.hpp"

using namespace cassio;
using namespace cassio::kernel;
using namespace cassio::hardware;
using namespace cassio::memory;

static MemCommand instance;

MemCommand::MemCommand() : Command("mem", "Show memory statistics") {}

bool MemCommand::execute(const char** args, usize argc,
                         filesystem::FileNode*& cwd) {
    VgaTerminal& vga = VgaTerminal::getTerminal();
    PhysicalMemoryManager& pmm = PhysicalMemoryManager::getManager();
    u32 free = pmm.getFreeFrames();
    u32 total = pmm.getTotalFrames();
    u32 used = pmm.getUsedFrames();

    vga.print("Physical memory:\n");
    vga.print("  Total: ");
    vga.print_dec(total * 4);
    vga.print(" KiB (");
    vga.print_dec(total);
    vga.print(" frames)\n");
    vga.print("  Used:  ");
    vga.print_dec(used * 4);
    vga.print(" KiB (");
    vga.print_dec(used);
    vga.print(" frames)\n");
    vga.print("  Free:  ");
    vga.print_dec(free * 4);
    vga.print(" KiB (");
    vga.print_dec(free);
    vga.print(" frames)\n");

    return true;
}
