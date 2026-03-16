/**
 * keyboard.hpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef DRIVERS_KEYBOARD_HPP_
#define DRIVERS_KEYBOARD_HPP_

#include <common/types.hpp>
#include <hardware/driver.hpp>
#include <hardware/port.hpp>

namespace cassio {
namespace drivers {

/**
 * @brief PS/2 scan set 1 scancodes as received from the keyboard hardware.
 *
 * @see https://wiki.osdev.org/PS/2_Keyboard#Scan_Code_Set_1
 *
 */
enum class ScanCode : u8 {
    Escape                                      = 0x01,
    One                                         = 0x02,
    Two                                         = 0x03,
    Three                                       = 0x04,
    Four                                        = 0x05,
    Five                                        = 0x06,
    Six                                         = 0x07,
    Seven                                       = 0x08,
    Eight                                       = 0x09,
    Nine                                        = 0x0A,
    Zero                                        = 0x0B,
    Minus                                       = 0x0C,
    Equals                                      = 0x0D,
    Backspace                                   = 0x0E,
    Tab                                         = 0x0F,
    Q                                           = 0x10,
    W                                           = 0x11,
    E                                           = 0x12,
    R                                           = 0x13,
    T                                           = 0x14,
    Y                                           = 0x15,
    U                                           = 0x16,
    I                                           = 0x17,
    O                                           = 0x18,
    P                                           = 0x19,
    LeftBracket                                 = 0x1A,
    RightBracket                                = 0x1B,
    Enter                                       = 0x1C,
    LeftCtrl                                    = 0x1D,
    A                                           = 0x1E,
    S                                           = 0x1F,
    D                                           = 0x20,
    F                                           = 0x21,
    G                                           = 0x22,
    H                                           = 0x23,
    J                                           = 0x24,
    K                                           = 0x25,
    L                                           = 0x26,
    Semicolon                                   = 0x27,
    Quote                                       = 0x28,
    Backquote                                   = 0x29,
    LeftShift                                   = 0x2A,
    Backslash                                   = 0x2B,
    Z                                           = 0x2C,
    X                                           = 0x2D,
    C                                           = 0x2E,
    V                                           = 0x2F,
    B                                           = 0x30,
    N                                           = 0x31,
    M                                           = 0x32,
    Comma                                       = 0x33,
    Period                                      = 0x34,
    Slash                                       = 0x35,
    RightShift                                  = 0x36,
    LeftAlt                                     = 0x38,
    Space                                       = 0x39,
    CapsLock                                    = 0x3A,
    F1                                          = 0x3B,
    F2                                          = 0x3C,
    F3                                          = 0x3D,
    F4                                          = 0x3E,
    F5                                          = 0x3F,
    F6                                          = 0x40,
    F7                                          = 0x41,
    F8                                          = 0x42,
    F9                                          = 0x43,
    F10                                         = 0x44,
    NumLock                                     = 0x45,
    F11                                         = 0x57,
    F12                                         = 0x58
};

/**
 * @brief Resolved key values dispatched to the event handler.
 *
 * ASCII-range values (0x00-0x7F) correspond to their ASCII characters.
 * Non-ASCII keys (modifiers, function keys) use values above 0x7F.
 *
 * @see https://simple.wikipedia.org/wiki/ASCII#/media/File:ASCII-Table-wide.svg
 *
 */
enum class KeyCode : u8 {
    Backspace                                   = 0x08,
    Tab                                         = 0x09,
    Enter                                       = 0x0D,
    Escape                                      = 0x1B,
    Space                                       = 0x20,
    Exclamation                                 = 0x21,
    DoubleQuote                                 = 0x22,
    Hash                                        = 0x23,
    Dollar                                      = 0x24,
    Percent                                     = 0x25,
    Ampersand                                   = 0x26,
    Quote                                       = 0x27,
    LeftParenthesis                             = 0x28,
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
    a                                           = 0x61,
    b                                           = 0x62,
    c                                           = 0x63,
    d                                           = 0x64,
    e                                           = 0x65,
    f                                           = 0x66,
    g                                           = 0x67,
    h                                           = 0x68,
    i                                           = 0x69,
    j                                           = 0x6A,
    k                                           = 0x6B,
    l                                           = 0x6C,
    m                                           = 0x6D,
    n                                           = 0x6E,
    o                                           = 0x6F,
    p                                           = 0x70,
    q                                           = 0x71,
    r                                           = 0x72,
    s                                           = 0x73,
    t                                           = 0x74,
    u                                           = 0x75,
    v                                           = 0x76,
    w                                           = 0x77,
    x                                           = 0x78,
    y                                           = 0x79,
    z                                           = 0x7A,
    LeftCurly                                   = 0x7B,
    Pipe                                        = 0x7C,
    RightCurly                                  = 0x7D,
    Tilde                                       = 0x7E,
    Delete                                      = 0x7F,

    // Non-ASCII keys.
    F1                                          = 0x80,
    F2                                          = 0x81,
    F3                                          = 0x82,
    F4                                          = 0x83,
    F5                                          = 0x84,
    F6                                          = 0x85,
    F7                                          = 0x86,
    F8                                          = 0x87,
    F9                                          = 0x88,
    F10                                         = 0x89,
    F11                                         = 0x8A,
    F12                                         = 0x8B,
    LeftArrow                                   = 0x8C,
    RightArrow                                  = 0x8D
};

/**
 * @brief PS/2 controller commands sent to the command port (0x64).
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
 * @brief Bitfield layout of the PS/2 controller command byte (port 0x64).
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
 * @brief Interface for receiving keyboard events from the KeyboardDriver.
 *
 */
class KeyboardEventHandler {
public:
    KeyboardEventHandler() = default;

    /**
     * @brief Called when a key is pressed. Override to handle key-down events.
     *
     */
    virtual void OnKeyDown(KeyCode key);

    /**
     * @brief Called when a key is released. Override to handle key-up events.
     *
     */
    virtual void OnKeyUp(KeyCode key);
};

/**
 * @brief PS/2 keyboard driver that translates scancodes into KeyCode events.
 *
 * Tracks modifier state (Shift, Ctrl, Alt, Caps Lock) internally and uses
 * it to resolve scancodes into the correct KeyCode before dispatching.
 *
 */
constexpr u16 KEYBOARD_BUFFER_SIZE = 256;

class KeyboardDriver : public hardware::Driver {
private:
    hardware::Port<u8> cmd;
    hardware::Port<u8> data;

    KeyboardEventHandler* handler;

    bool shift_held;
    bool ctrl_held;
    bool alt_held;
    bool caps_lock_on;
    bool e0_prefix;

    // Ring buffer for buffering typed characters for syscall read().
    char ring[KEYBOARD_BUFFER_SIZE];
    u16 ring_head;
    u16 ring_tail;

    // Lookup table mapping scancodes (0x00-0x58) to their default KeyCode.
    // A value of 0x00 means the scancode has no KeyCode mapping.
    static const KeyCode scancode_table[0x59];

    // Returns the shifted variant of a KeyCode (e.g., a -> A, One -> Exclamation).
    static KeyCode resolveShift(KeyCode key);

    static KeyboardDriver instance;

private:
    KeyboardDriver();
    ~KeyboardDriver() = default;

    KeyboardCommandByte readCommandByte();

public:
    /**
     * @brief Returns the singleton KeyboardDriver instance.
     *
     */
    inline static KeyboardDriver& getDriver() {
        return instance;
    }

    /**
     * @brief Sets the event handler for keyboard events.
     *
     */
    void setHandler(KeyboardEventHandler* han);

    /**
     * @brief Reads and removes one character from the input buffer.
     *
     * @return The character, or '\0' if the buffer is empty.
     *
     */
    char readBuffer();

    /**
     * @brief Enables keyboard interrupts and clears any pending scancodes.
     *
     */
    virtual void activate() override;
    virtual void deactivate() override;

    /**
     * @brief Reads a scancode from the data port and dispatches it to the event handler.
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
