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

#include <common/types.hpp>
#include <hardware/driver.hpp>
#include <hardware/port.hpp>
#include <std/iostream.hpp>

namespace cassio {
namespace drivers {

/**
 * @brief Enumerates the keycodes and maps them to their ASCII code equivalents.
 * 
 * This enumeration has the purpose of naming the keys appropriatelly as well as mapping
 * them to their correct ASCII codes for ease of printing and manipulating.
 * 
 * @see https://simple.wikipedia.org/wiki/ASCII#/media/File:ASCII-Table-wide.svg
 * 
 */
enum class KeyCode : u8 {
    Backspace                                   = 0x08,
    Tab                                         = 0x09,
    Enter                                       = 0x0D,
    Space                                       = 0x20,
    DoubleQuote                                 = 0x22,
    Hash                                        = 0x23,
    Dollar                                      = 0x24,
    Percent                                     = 0x25,
    Ampersand                                   = 0x26,
    Quote                                       = 0x27,
    LeftParethensis                             = 0x28,
    RightParenthesis                            = 0x29,
    Asterisk                                    = 0x2A,
    Plus                                        = 0x2B,
    Comma                                       = 0x2C,
    Minus                                       = 0x2D,
    Period                                      = 0x2E,
    Slash                                       = 0x2F,
    Zero                                        = 0x30,
    One                                         = 0x31,
    Two                                         = 0x32,
    Three                                       = 0x33,
    Four                                        = 0x34,
    Five                                        = 0x35,
    Six                                         = 0x36,
    Seven                                       = 0x37,
    Eight                                       = 0x38,
    Nine                                        = 0x39,
    Colon                                       = 0x3A,
    Semicolon                                   = 0x3B,
    LessThan                                    = 0x3C,
    Equals                                      = 0x3D,
    GreaterThan                                 = 0x3E,
    Question                                    = 0x3F,
    At                                          = 0x40,
    A                                           = 0x41,
    B                                           = 0x42,
    C                                           = 0x43,
    D                                           = 0x44,
    E                                           = 0x45,
    F                                           = 0x46,
    G                                           = 0x47,
    H                                           = 0x48,
    I                                           = 0x49,
    J                                           = 0x4A,
    K                                           = 0x4B,
    L                                           = 0x4C,
    M                                           = 0x4D,
    N                                           = 0x4E,
    O                                           = 0x4F,
    P                                           = 0x50,
    Q                                           = 0x51,
    R                                           = 0x52,
    S                                           = 0x53,
    T                                           = 0x54,
    U                                           = 0x55,
    V                                           = 0x56,
    W                                           = 0x57,
    X                                           = 0x58,
    Y                                           = 0x59,
    Z                                           = 0x5A,
    LeftBracket                                 = 0x5B,
    BackSlash                                   = 0x5C,
    RightBracket                                = 0x5D,
    Caret                                       = 0x5E,
    Underscore                                  = 0x5F,
    Backquote                                   = 0x60,
    LeftCurly                                   = 0x7B,
    Pipe                                        = 0x7C,
    RightCurly                                  = 0x7D,
    Tilde                                       = 0x7E,
    Delete                                      = 0x7F
};

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
    constexpr u8 ReadCommandByte                = 0x20;
    constexpr u8 WriteCommandByte               = 0x60;
    constexpr u8 GetVersionNumber               = 0xA1;
    constexpr u8 TestKeyboardInterface          = 0xAB;
    constexpr u8 EnableKeyboardInterface        = 0xAE;
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
class KeyboardEventHandler {
public:
    KeyboardEventHandler() = default;

    /**
     * @brief
     *
     */
    virtual void OnKeyDown(KeyCode key);

    /**
     * @brief
     * 
     */
    virtual void OnKeyUp(KeyCode key);
};

/**
 * @brief
 * 
 */
class KeyboardDriver : public hardware::Driver {
private:
    hardware::Port<u8> cmd;
    hardware::Port<u8> data;

    KeyboardEventHandler* handler;

private:
    KeyboardCommandByte readCommandByte();

public:
    /**
     * @brief
     * 
     */
    KeyboardDriver(KeyboardEventHandler* han);

    /**
     * @brief
     * 
     */
    ~KeyboardDriver() = default;

    /**
     * @brief
     * 
     */
    virtual void activate() override;

    /**
     * @brief
     * 
     */
    virtual u32 handleInterrupt(u32 esp) override;


    /** Deleted Methods */
    KeyboardDriver(const KeyboardDriver&) = delete;
    KeyboardDriver(KeyboardDriver&&) = delete;
    KeyboardDriver& operator=(const KeyboardDriver&) = delete;
    KeyboardDriver& operator=(KeyboardDriver&&) = delete;

};

}
}

#endif // DRIVERS_KEYBOARD_HPP_
