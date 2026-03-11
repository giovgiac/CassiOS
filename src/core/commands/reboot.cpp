/**
 * reboot.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "core/commands/reboot.hpp"
#include "hardware/terminal.hpp"
#include "hardware/port.hpp"

using namespace cassio;
using namespace cassio::kernel;
using namespace cassio::hardware;

static RebootCommand instance;

RebootCommand::RebootCommand() : Command("reboot", "Reboot the system") {}

bool RebootCommand::execute(const char** args, usize argc) {
    VgaTerminal::getTerminal().print("Rebooting...\n");
    Port<u8> cmd(PortType::KeyboardControllerCommand);
    cmd.write(0xFE);
    return false;
}
