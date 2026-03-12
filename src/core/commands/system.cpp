/**
 * system.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "core/commands/system.hpp"
#include "common/string.hpp"
#include "hardware/port.hpp"
#include "hardware/terminal.hpp"
#include "memory/physical.hpp"

using namespace cassio;
using namespace cassio::kernel;
using namespace cassio::filesystem;
using namespace cassio::hardware;
using namespace cassio::memory;

// --- help ---

static HelpCommand helpInstance;

HelpCommand::HelpCommand() : Command("help", "Show available commands") {}

bool HelpCommand::execute(const char** args, usize argc,
                          FileNode*& cwd) {
    VgaTerminal& vga = VgaTerminal::getTerminal();
    vga.print("Available commands:\n");

    Command** cmds = Command::getRegistry();
    u8 count = Command::getCount();
    for (u8 i = 0; i < count; ++i) {
        vga.print("  ");
        vga.print(cmds[i]->getName());

        // Pad to 12 characters for alignment.
        usize len = strlen(cmds[i]->getName());
        for (usize j = len; j < 12; ++j) {
            vga.putchar(' ');
        }

        vga.print("- ");
        vga.print(cmds[i]->getDescription());
        vga.putchar('\n');
    }

    return true;
}

// --- clear ---

static ClearCommand clearInstance;

ClearCommand::ClearCommand() : Command("clear", "Clear the screen") {}

bool ClearCommand::execute(const char** args, usize argc,
                           FileNode*& cwd) {
    VgaTerminal::getTerminal().clear();
    return true;
}

// --- mem ---

static MemCommand memInstance;

MemCommand::MemCommand() : Command("mem", "Show memory statistics") {}

bool MemCommand::execute(const char** args, usize argc,
                         FileNode*& cwd) {
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

// --- reboot ---

static RebootCommand rebootInstance;

RebootCommand::RebootCommand() : Command("reboot", "Reboot the system") {}

bool RebootCommand::execute(const char** args, usize argc,
                            FileNode*& cwd) {
    VgaTerminal::getTerminal().print("Rebooting...\n");
    Port<u8> cmd(PortType::KeyboardControllerCommand);
    cmd.write(0xFE);
    return false;
}

// --- shutdown ---

static ShutdownCommand shutdownInstance;

ShutdownCommand::ShutdownCommand() : Command("shutdown", "Halt the system") {}

bool ShutdownCommand::execute(const char** args, usize argc,
                              FileNode*& cwd) {
    VgaTerminal::getTerminal().print("Shutting down...\n");
    return false;
}
