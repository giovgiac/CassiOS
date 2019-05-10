/**
 * interrupt.hpp
 * 
 * Copyright (c) 2019 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef HARDWARE_INTERRUPT_HPP_
#define HARDWARE_INTERRUPT_HPP_

#include <core/gdt.hpp>
#include <core/types.hpp>
#include <hardware/driver.hpp>
#include <hardware/port.hpp>

namespace cassio {
namespace hardware {

/**
 * @brief
 * 
 */
enum InterruptFlags : u8 {
    IDT_DESCRIPTOR_PRESENT = 0x80,
    IDT_INTERRUPT_GATE = 0xE
};

/**
 * @brief
 * 
 */
class InterruptManager final {
friend class Driver;

private:
    /**
     * @brief
     * 
     */
    struct __attribute__((packed)) GateDescriptor {
        u16 handler_low;
        u16 code_offset;
        u8  reserved;
        u8  access;
        u16 handler_high;
    };

    /**
     * @brief
     * 
     */
    struct __attribute__((packed)) InterruptDescriptorTable {
        u16 size;
        u32 base;
    };

private:
    GateDescriptor idt[256];
    Driver* drv[256];

    hardware::Port<u8> pic_master_cmd;
    hardware::Port<u8> pic_master_data;
    hardware::Port<u8> pic_slave_cmd;
    hardware::Port<u8> pic_slave_data;

    static InterruptManager instance;

private:
    /**
     * @brief
     * 
     */
    InterruptManager();

    /**
     * @brief
     * 
     */
    ~InterruptManager() = default;

public:
    /**
     * @brief
     * 
     */
    inline static InterruptManager& getManager() {
        return instance;
    }

    static void ignoreInterruptRequest();
    static void handleInterruptRequest0x00();
    static void handleInterruptRequest0x01();
    static void handleInterruptRequest0x0C();

    /**
     * @brief
     * 
     */
    void activate();

    /**
     * @brief
     * 
     */
    void deactivate();

    /**
     * @brief
     * 
     */
    u32 handleInterrupt(u8 number, u32 esp);

    /**
     * @brief
     * 
     */
    void setInterrupt(u8 number, u16 code_offset, void(*handler)(), u8 access, u8 flags);

    /**
     * @brief
     * 
     */
    void load(cassio::kernel::GlobalDescriptorTable& gdt);

    /**
     * @brief
     * 
     */
    void unload();

    /** Deleted Methods */
    InterruptManager(const InterruptManager&) = delete;
    InterruptManager(InterruptManager&&) = delete;
    InterruptManager& operator=(const InterruptManager&) = delete;
    InterruptManager& operator=(InterruptManager&&) = delete;

};

}
}

/**
 * @brief
 *
 */
extern "C" u32 handleInterrupt(u8 number, u32 esp);

#endif // HARDWARE_INTERRUPT_HPP_
