/**
 * command.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "core/commands/command.hpp"
#include <string.hpp>

using namespace cassio;
using namespace cassio::kernel;

Command* Command::registry[MAX_COMMANDS] = {};
u8 Command::count = 0;

Command::Command(const char* name, const char* description)
    : name(name), description(description) {
    if (count < MAX_COMMANDS) {
        registry[count++] = this;
    }
}

const char* Command::getName() const {
    return name;
}

const char* Command::getDescription() const {
    return description;
}

Command* Command::find(const char* name) {
    for (u8 i = 0; i < count; ++i) {
        if (streq(name, registry[i]->name)) {
            return registry[i];
        }
    }
    return nullptr;
}

Command** Command::getRegistry() {
    return registry;
}

u8 Command::getCount() {
    return count;
}
