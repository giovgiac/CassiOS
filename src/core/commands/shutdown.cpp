/**
 * shutdown.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "core/commands/shutdown.hpp"
#include "hardware/terminal.hpp"

using namespace cassio;
using namespace cassio::kernel;
using namespace cassio::hardware;

static ShutdownCommand instance;

ShutdownCommand::ShutdownCommand() : Command("shutdown", "Halt the system") {}

bool ShutdownCommand::execute(const char** args, usize argc) {
    VgaTerminal::getTerminal().print("Shutting down...\n");
    return false;
}
