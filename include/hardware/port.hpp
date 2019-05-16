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
namespace hardware {

/**
 * @brief
 * 
 * @see https://wiki.osdev.org/I/O_Ports
 * 
 */
enum class PortType : u16 {
    MasterProgrammableInterfaceControllerCommand        = 0x20,
    MasterProgrammableInterfaceControllerData           = 0x21,
    KeyboardControllerData                              = 0x60,
    KeyboardControllerCommand                           = 0x64,
    SlaveProgrammableInterfaceControllerCommand         = 0xA0,
    SlaveProgrammableInterfaceControllerData            = 0xA1
};

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
    Port(PortType type);

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
    Port(PortType type);

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
    Port(PortType type);

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