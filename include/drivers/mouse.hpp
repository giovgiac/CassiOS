/**
 * mouse.hpp
 * 
 * Copyright (c) 2019 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef DRIVERS_MOUSE_HPP_
#define DRIVERS_MOUSE_HPP_

#include <common/types.hpp>
#include <hardware/driver.hpp>
#include <hardware/port.hpp>

namespace cassio {
namespace drivers {

/**
 * @brief Defines names for the PS/2 Mouse commands that can be sent to the controller.
 * 
 * A namespace which contains constants for the various possible mouse commands, these
 * include reading and writing to the 'command byte', as well as testing and enabling the
 * mouse interrupts.
 * 
 * @see https://www.avrfreaks.net/sites/default/files/PS2%20Keyboard.pdf
 * 
 */
namespace MouseCommand {
    constexpr u8 ReadCommand        = 0x20;
    constexpr u8 WriteCommand       = 0x60;
    constexpr u8 GetVersionNumber   = 0xA1;
    constexpr u8 EnableMouse        = 0xA8;
    constexpr u8 TestMouse          = 0xA9;
    constexpr u8 WriteMouse         = 0xD4;
}

/**
 * @brief
 * 
 */
class MouseEventHandler {
public:
    MouseEventHandler() = default;

    /**
     * @brief
     * 
     */
    virtual void OnActivate();

    /**
     * @brief
     *
     */
    virtual void OnMouseDown(u8 btn);

    /**
     * @brief
     * 
     */
    virtual void OnMouseUp(u8 btn);

    /**
     * @brief
     * 
     */
    virtual void OnMouseMove(i8 dx, i8 dy);
};

/**
 * @brief
 * 
 */
class MouseDriver : public hardware::Driver {
private:
    hardware::Port<u8> cmd;
    hardware::Port<u8> data;

    u8 buffer[3];
    u8 offset;
    u8 button;

    MouseEventHandler* handler;
    
public:
    /**
     * @brief
     * 
     */
    MouseDriver(MouseEventHandler* han);

    /**
     * @brief
     * 
     */
    ~MouseDriver() = default;

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
    MouseDriver(const MouseDriver&) = delete;
    MouseDriver(MouseDriver&&) = delete;
    MouseDriver& operator=(const MouseDriver&) = delete;
    MouseDriver& operator=(MouseDriver&&) = delete;

};

}
}

#endif // DRIVERS_MOUSE_HPP_
