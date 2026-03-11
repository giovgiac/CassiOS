/**
 * command.hpp
 *
 * Copyright (c) 2019 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef CORE_COMMANDS_COMMAND_HPP_
#define CORE_COMMANDS_COMMAND_HPP_

#include <common/types.hpp>

namespace cassio {
namespace kernel {

constexpr u8 MAX_COMMANDS = 32;

/**
 * @brief Base class for shell commands.
 *
 * Commands self-register into a static registry at construction time
 * (via global constructors). The shell looks up commands by name and
 * calls execute().
 *
 */
class Command {
private:
    static Command* registry[MAX_COMMANDS];
    static u8 count;

    const char* name;
    const char* description;

public:
    Command(const char* name, const char* description);
    ~Command() = default;

    const char* getName() const;
    const char* getDescription() const;

    /**
     * @brief Runs the command with the given arguments.
     *
     * @return true if the shell should continue, false to exit.
     *
     */
    virtual bool execute(const char** args, usize argc) = 0;

    static Command* find(const char* name);
    static Command** getRegistry();
    static u8 getCount();
};

} // kernel
} // cassio

#endif // CORE_COMMANDS_COMMAND_HPP_
