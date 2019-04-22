/**
 * driver.hpp
 * 
 * Copyright (c) 2019 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef HARDWARE_DRIVER_HPP_
#define HARDWARE_DRIVER_HPP_

#include <core/types.hpp>

namespace cassio {
namespace hardware {

/**
 * @brief
 *  
 */
class Driver {
protected:
    u8 number;

protected:
    /**
     * @brief
     * 
     */
    Driver(u8 num);

    /**
     * @brief
     * 
     */
    ~Driver();

public:
    /**
     * @brief
     * 
     */
    virtual u32 handleInterrupt(u32 esp);

    /** Deleted Methods */
    Driver(const Driver&) = delete;
    Driver(Driver&&) = delete;
    Driver& operator=(const Driver&) = delete;
    Driver& operator=(Driver&&) = delete;

};

}
}

#endif // HARDWARE_DRIVER_HPP_
