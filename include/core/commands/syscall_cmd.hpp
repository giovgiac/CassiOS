/**
 * syscall_cmd.hpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef CORE_COMMANDS_SYSCALL_CMD_HPP_
#define CORE_COMMANDS_SYSCALL_CMD_HPP_

#include <core/commands/command.hpp>

namespace cassio {
namespace kernel {

class SyscallCommand : public Command {
public:
    SyscallCommand();
    bool execute(const char** args, usize argc,
                 filesystem::FileNode*& cwd) override;
};

} // kernel
} // cassio

#endif // CORE_COMMANDS_SYSCALL_CMD_HPP_
