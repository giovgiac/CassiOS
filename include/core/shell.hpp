/**
 * shell.hpp
 *
 * Copyright (c) 2019 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef CORE_SHELL_HPP_
#define CORE_SHELL_HPP_

#include <common/types.hpp>
#include <drivers/keyboard.hpp>
#include <hardware/terminal.hpp>

namespace cassio {
namespace kernel {

constexpr u8 SHELL_MAX_INPUT = 78;

/**
 * @brief Simple command shell that provides a `$ ` prompt and built-in commands.
 *
 * Handles line editing with cursor movement (left/right arrows, backspace)
 * and dispatches entered commands: shutdown, help, clear.
 *
 */
class Shell : public drivers::KeyboardEventHandler {
private:
    hardware::VgaTerminal& vga;

    char buffer[SHELL_MAX_INPUT];
    u8 length;
    u8 cursor;
    u8 prompt_x;
    u8 prompt_y;

    void printPrompt();
    void redrawLine();
    void execute();

    bool streq(const char* a, const char* b);

public:
    Shell();

    virtual void OnKeyDown(drivers::KeyCode key) override;

    /** Deleted Methods */
    Shell(const Shell&) = delete;
    Shell(Shell&&) = delete;
    Shell& operator=(const Shell&) = delete;
    Shell& operator=(Shell&&) = delete;
};

} // kernel
} // cassio

#endif // CORE_SHELL_HPP_
