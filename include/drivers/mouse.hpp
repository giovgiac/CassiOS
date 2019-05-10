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

#include <core/types.hpp>
#include <hardware/driver.hpp>
#include <hardware/port.hpp>

namespace cassio {
namespace drivers {

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
    
public:
    /**
     * @brief
     * 
     */
    MouseDriver();

    /**
     * @brief
     * 
     */
    ~MouseDriver() = default;

    /**
     * @brief
     * 
     */
    virtual u32 handleInterrupt(u32 esp);

    /** Deleted Methods */
    MouseDriver(const MouseDriver&) = delete;
    MouseDriver(MouseDriver&&) = delete;
    MouseDriver& operator=(const MouseDriver&) = delete;
    MouseDriver& operator=(MouseDriver&&) = delete;

};

}
}

#endif // DRIVERS_MOUSE_HPP_
