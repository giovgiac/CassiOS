/**
 * mouse.hpp
 * 
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
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
 * @brief PS/2 controller commands for mouse operation, sent to the command port (0x64).
 *
 * @see https://www.avrfreaks.net/sites/default/files/PS2%20Keyboard.pdf
 *
 */
enum class MouseCommand : u8 {
    ReadCommand                                 = 0x20,
    WriteCommand                                = 0x60,
    GetVersionNumber                            = 0xA1,
    EnableMouse                                 = 0xA8,
    TestMouse                                   = 0xA9,
    WriteMouse                                  = 0xD4
};

/**
 * @brief Interface for receiving mouse events from the MouseDriver.
 *
 */
class MouseEventHandler {
public:
    MouseEventHandler() = default;

    /**
     * @brief Called when the mouse driver is activated. Override to set initial state.
     *
     */
    virtual void OnActivate();

    /**
     * @brief Called when a mouse button is pressed.
     *
     */
    virtual void OnMouseDown(u8 btn);

    /**
     * @brief Called when a mouse button is released.
     *
     */
    virtual void OnMouseUp(u8 btn);

    /**
     * @brief Called when the mouse moves by (dx, dy) units.
     *
     */
    virtual void OnMouseMove(i8 dx, i8 dy);
};

/**
 * @brief PS/2 mouse driver that reads 3-byte packets and dispatches movement/button events.
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

    static MouseDriver instance;

private:
    MouseDriver();
    ~MouseDriver() = default;

public:
    /**
     * @brief Returns the singleton MouseDriver instance.
     *
     */
    inline static MouseDriver& getDriver() {
        return instance;
    }

    /**
     * @brief Sets the event handler for mouse events.
     *
     */
    void setHandler(MouseEventHandler* han);

    /**
     * @brief Enables the mouse on the PS/2 controller and starts receiving interrupts.
     *
     */
    virtual void activate() override;
    virtual void deactivate() override;

    /**
     * @brief Reads a byte into the 3-byte packet buffer and dispatches events when complete.
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
