/**
 * keyboard.hpp
 * 
 * Copyright (c) 2019 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef DRIVERS_KEYBOARD_HPP_
#define DRIVERS_KEYBOARD_HPP_

#include <core/types.hpp>
#include <hardware/driver.hpp>
#include <hardware/port.hpp>

namespace cassio {
namespace drivers {

/**
 * @brief Defines names for the PS/2 Keyboard commands that can be sent to the controller.
 * 
 * A namespace which contains constants for the various possible keyboard commands, these
 * include reading and writing to the 'command byte', as well as testing and enabling the
 * keyboard interrupts.
 * 
 * @see KeyboardCommandByte
 * @see https://www.avrfreaks.net/sites/default/files/PS2%20Keyboard.pdf
 * 
 */
namespace KeyboardCommand {
    constexpr u8 ReadCommand        = 0x20;
    constexpr u8 WriteCommand       = 0x60;
    constexpr u8 GetVersionNumber   = 0xA1;
    constexpr u8 TestKeyboard       = 0xAB;
    constexpr u8 EnableKeyboard     = 0xAE;
}

/**
 * @brief
 * 
 * 
 */
union KeyboardCommandByte {
    struct {
        // Whether to enable keyboard interrupts.
        bool keyboard_interrupt : 1;

        // Whether to enable mouse interrupts.
        bool mouse_interrupt : 1;

        // Manually sets the status SYS flag.
        bool system_flag : 1;

        // Unused byte, added for padding.
        bool ignore1 : 1;

        // Whether to disable the keyboard.
        bool disable_keyboard : 1;

        // Whether to disable the mouse.
        bool disable_mouse : 1;

        // Whether to translate scan codes to set 1.
        bool translate_scan : 1;

        // Unused byte, added for padding.
        bool ignore2 : 1;
    };
    
    u8 byte;
};

/**
 * @brief
 * 
 */
class KeyboardDriver : public hardware::Driver {
private:
    hardware::Port<u8> cmd;
    hardware::Port<u8> data;

private:
    KeyboardCommandByte readCommandByte();

public:
    /**
     * @brief
     * 
     */
    KeyboardDriver();

    /**
     * @brief
     * 
     */
    ~KeyboardDriver() = default;

    /**
     * @brief
     * 
     */
    virtual u32 handleInterrupt(u32 esp);


    /** Deleted Methods */
    KeyboardDriver(const KeyboardDriver&) = delete;
    KeyboardDriver(KeyboardDriver&&) = delete;
    KeyboardDriver& operator=(const KeyboardDriver&) = delete;
    KeyboardDriver& operator=(KeyboardDriver&&) = delete;

};

}
}

#endif // DRIVERS_KEYBOARD_HPP_
