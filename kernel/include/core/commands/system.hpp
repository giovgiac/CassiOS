/**
 * system.hpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef CORE_COMMANDS_SYSTEM_HPP_
#define CORE_COMMANDS_SYSTEM_HPP_

#include <core/commands/command.hpp>

namespace cassio {
namespace kernel {

class HelpCommand : public Command {
public:
    HelpCommand();
    bool execute(const char** args, usize argc,
                 filesystem::FileNode*& cwd) override;
};

class ClearCommand : public Command {
public:
    ClearCommand();
    bool execute(const char** args, usize argc,
                 filesystem::FileNode*& cwd) override;
};

class MemCommand : public Command {
public:
    MemCommand();
    bool execute(const char** args, usize argc,
                 filesystem::FileNode*& cwd) override;
};

class RebootCommand : public Command {
public:
    RebootCommand();
    bool execute(const char** args, usize argc,
                 filesystem::FileNode*& cwd) override;
};

class UptimeCommand : public Command {
public:
    UptimeCommand();
    bool execute(const char** args, usize argc,
                 filesystem::FileNode*& cwd) override;
};

class ShutdownCommand : public Command {
public:
    ShutdownCommand();
    bool execute(const char** args, usize argc,
                 filesystem::FileNode*& cwd) override;
};

class SyscallCommand : public Command {
public:
    SyscallCommand();
    bool execute(const char** args, usize argc,
                 filesystem::FileNode*& cwd) override;
};

} // kernel
} // cassio

#endif // CORE_COMMANDS_SYSTEM_HPP_
