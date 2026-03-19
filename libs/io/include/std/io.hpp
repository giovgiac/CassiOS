/**
 * io.hpp -- x86 I/O port access
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef STD_IO_HPP
#define STD_IO_HPP

#include <std/types.hpp>

namespace std {
namespace io {

/**
 * Enumerates known x86 I/O port addresses for hardware devices.
 *
 * @see https://wiki.osdev.org/I/O_Ports
 */
enum class PortType : u16 {
    MasterPicCommand        = 0x20,
    MasterPicData           = 0x21,
    PitChannel0Data         = 0x40,
    PitCommand              = 0x43,
    KbdData                 = 0x60,
    KbdCommand              = 0x64,
    SlavePicCommand         = 0xA0,
    SlavePicData            = 0xA1,
    PrimaryAtaData          = 0x1F0,
    PrimaryAtaError         = 0x1F1,
    PrimaryAtaSectorCount   = 0x1F2,
    PrimaryAtaLbaLow        = 0x1F3,
    PrimaryAtaLbaMid        = 0x1F4,
    PrimaryAtaLbaHigh       = 0x1F5,
    PrimaryAtaDriveSelect   = 0x1F6,
    PrimaryAtaCommandStatus = 0x1F7,
    VgaCrtcIndex            = 0x3D4,
    VgaCrtcData             = 0x3D5,
    PrimaryAtaDeviceControl = 0x3F6,
    SerialCOM1Data          = 0x3F8,
    SerialCOM1InterruptEnable = 0x3F9,
    SerialCOM1FIFOControl   = 0x3FA,
    SerialCOM1LineControl   = 0x3FB,
    SerialCOM1ModemControl  = 0x3FC,
    SerialCOM1LineStatus    = 0x3FD,
    QemuDebugExit           = 0xF4
};

/**
 * Typed x86 I/O port. T must be u8, u16, or u32.
 *
 * Reads and writes use inline asm with the appropriate in/out
 * instruction selected at compile time via if constexpr.
 */
template <typename T>
class Port {
public:
    Port(PortType type) : number(static_cast<u16>(type)) {}

    T read() {
        T result;
        if constexpr (sizeof(T) == 1)
            asm volatile("inb %1, %0" : "=a"(result) : "Nd"(number));
        else if constexpr (sizeof(T) == 2)
            asm volatile("inw %1, %0" : "=a"(result) : "Nd"(number));
        else if constexpr (sizeof(T) == 4)
            asm volatile("inl %1, %0" : "=a"(result) : "Nd"(number));
        return result;
    }

    void write(T data) {
        if constexpr (sizeof(T) == 1)
            asm volatile("outb %0, %1" : : "a"(data), "Nd"(number));
        else if constexpr (sizeof(T) == 2)
            asm volatile("outw %0, %1" : : "a"(data), "Nd"(number));
        else if constexpr (sizeof(T) == 4)
            asm volatile("outl %0, %1" : : "a"(data), "Nd"(number));
    }

    void writeSlow(T data) {
        static_assert(sizeof(T) == 1, "writeSlow is only valid for 8-bit ports");
        asm volatile("outb %0, %1\njmp 1f\n1: jmp 1f\n1:" : : "a"(data), "Nd"(number));
    }

private:
    u16 number;
};

}
}

#endif // STD_IO_HPP
