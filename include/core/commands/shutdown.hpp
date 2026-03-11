/**
 * shutdown.hpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef CORE_COMMANDS_SHUTDOWN_HPP_
#define CORE_COMMANDS_SHUTDOWN_HPP_

#include <core/commands/command.hpp>

namespace cassio {
namespace kernel {

class ShutdownCommand : public Command {
public:
    ShutdownCommand();
    bool execute(const char** args, usize argc) override;
};

} // kernel
} // cassio

#endif // CORE_COMMANDS_SHUTDOWN_HPP_
