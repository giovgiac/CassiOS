/**
 * port.hpp
 * 
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef CORE_PORT_HPP_
#define CORE_PORT_HPP_

#include <common/types.hpp>

namespace cassio {
namespace hardware {

/**
 * @brief Enumerates known x86 I/O port addresses for hardware devices.
 *
 * @see https://wiki.osdev.org/I/O_Ports
 *
 */
enum class PortType : u16 {
    MasterProgrammableInterfaceControllerCommand         = 0x20,
    MasterProgrammableInterfaceControllerData            = 0x21,
    PitChannel0Data                                      = 0x40,
    PitCommand                                           = 0x43,
    KeyboardControllerData                              = 0x60,
    KeyboardControllerCommand                           = 0x64,
    SlaveProgrammableInterfaceControllerCommand         = 0xA0,
    SlaveProgrammableInterfaceControllerData            = 0xA1,
    SerialCOM1Data                                      = 0x3F8,
    SerialCOM1InterruptEnable                           = 0x3F9,
    SerialCOM1FIFOControl                               = 0x3FA,
    SerialCOM1LineControl                               = 0x3FB,
    SerialCOM1ModemControl                              = 0x3FC,
    SerialCOM1LineStatus                                = 0x3FD,
    PrimaryAtaData                                       = 0x1F0,
    PrimaryAtaError                                      = 0x1F1,
    PrimaryAtaSectorCount                                = 0x1F2,
    PrimaryAtaLbaLow                                     = 0x1F3,
    PrimaryAtaLbaMid                                     = 0x1F4,
    PrimaryAtaLbaHigh                                    = 0x1F5,
    PrimaryAtaDriveSelect                                = 0x1F6,
    PrimaryAtaCommandStatus                              = 0x1F7,
    PrimaryAtaDeviceControl                              = 0x3F6,
    VgaCrtcIndex                                         = 0x3D4,
    VgaCrtcData                                          = 0x3D5,
    QemuDebugExit                                        = 0xF4
};

template <typename T> class Port;

/**
 * @brief 8-bit I/O port for reading and writing single bytes via in/out instructions.
 *
 */
template <> class Port<u8> {
private:
    u16 number;

public:
    /**
     * @brief Constructs a port bound to the given I/O address.
     *
     */
    Port(PortType type);

    /**
     * @brief Destroys the port.
     *
     */
    ~Port() = default;

    /**
     * @brief Reads a byte from this I/O port using the inb instruction.
     *
     */
    u8 read();

    /**
     * @brief Writes a byte to this I/O port using the outb instruction.
     *
     */
    void write(u8 data);

    /**
     * @brief Writes a byte and waits for the bus to settle via a dummy I/O cycle.
     *
     */
    void writeSlow(u8 data);

};

/**
 * @brief 16-bit I/O port for reading and writing words via in/out instructions.
 *
 */
template <> class Port<u16> {
private:
    u16 number;

public:
    /**
     * @brief Constructs a port bound to the given I/O address.
     *
     */
    Port(PortType type);

    /**
     * @brief Destroys the port.
     *
     */
    ~Port() = default;

    /**
     * @brief Reads a 16-bit word from this I/O port using the inw instruction.
     *
     */
    u16 read();

    /**
     * @brief Writes a 16-bit word to this I/O port using the outw instruction.
     *
     */
    void write(u16 data);

};

/**
 * @brief 32-bit I/O port for reading and writing dwords via in/out instructions.
 *
 */
template <> class Port<u32> {
private:
    u16 number;

public:
    /**
     * @brief Constructs a port bound to the given I/O address.
     *
     */
    Port(PortType type);

    /**
     * @brief Destroys the port.
     *
     */
    ~Port() = default;

    /**
     * @brief Reads a 32-bit dword from this I/O port using the inl instruction.
     *
     */
    u32 read();

    /**
     * @brief Writes a 32-bit dword to this I/O port using the outl instruction.
     *
     */
    void write(u32 data);

};

} // kernel
} // cassio

#endif // CORE_PORT_HPP_