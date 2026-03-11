/**
 * clear.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "core/commands/clear.hpp"
#include "hardware/terminal.hpp"

using namespace cassio;
using namespace cassio::kernel;
using namespace cassio::hardware;

static ClearCommand instance;

ClearCommand::ClearCommand() : Command("clear", "Clear the screen") {}

bool ClearCommand::execute(const char** args, usize argc) {
    VgaTerminal::getTerminal().clear();
    return true;
}
