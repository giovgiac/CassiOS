/**
 * port.hpp
 * 
 * Copyright (c) 2019 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef CORE_PORT_HPP_
#define CORE_PORT_HPP_

#include <core/types.hpp>

namespace cassio {
namespace kernel {

template <typename T> class Port;

/**
 * @brief
 * 
 */
template <> class Port<u8> {
private:
    u16 number;

public:
    /**
     * @brief
     * 
     */
    Port(u16 num);

    /**
     * @brief
     * 
     */
    ~Port() = default;

    /**
     * @brief
     * 
     */
    u8 read();

    /**
     * @brief
     * 
     */
    void write(u8 data);

    /**
     * @brief
     * 
     */
    void writeSlow(u8 data);

};

/**
 * @brief
 * 
 */
template <> class Port<u16> {
private:
    u16 number;

public:
    /**
     * @brief
     * 
     */
    Port(u16 num);

    /**
     * @brief
     * 
     */
    ~Port() = default;

    /**
     * @brief
     * 
     */
    u16 read();

    /**
     * @brief
     * 
     */
    void write(u16 data);

};

/**
 * @brief
 * 
 */
template <> class Port<u32> {
private:
    u16 number;

public:
    /**
     * @brief
     * 
     */
    Port(u16 num);

    /**
     * @brief
     * 
     */
    ~Port() = default;

    /**
     * @brief
     * 
     */
    u32 read();

    /**
     * @brief
     * 
     */
    void write(u32 data);

};

} // kernel
} // cassio

#endif // CORE_PORT_HPP_