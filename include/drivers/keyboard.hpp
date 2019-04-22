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
 * @brief
 * 
 */
class KeyboardDriver : public hardware::Driver {
private:
    hardware::Port<u8> cmd;
    hardware::Port<u8> data;

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
