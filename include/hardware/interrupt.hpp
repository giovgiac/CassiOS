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

#include <common/types.hpp>
#include <core/gdt.hpp>
#include <hardware/driver.hpp>
#include <hardware/port.hpp>
#include <std/iostream.hpp>

namespace cassio {
namespace hardware {

/**
 * @brief Flag constants used when building IDT gate descriptors.
 *
 */
enum InterruptFlags : u8 {
    IDT_DESCRIPTOR_PRESENT = 0x80,
    IDT_INTERRUPT_GATE = 0xE
};

/**
 * @brief Singleton that owns the IDT and PIC, dispatching hardware interrupts to drivers.
 *
 */
class InterruptManager final {
friend class Driver;

private:
    /**
     * @brief An 8-byte packed IDT entry pointing to an interrupt handler.
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
     * @brief Pointer structure passed to the lidt instruction (size + base address).
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
     * @brief Constructs the manager and initializes PIC ports.
     *
     */
    InterruptManager();

    /**
     * @brief Destroys the interrupt manager.
     *
     */
    ~InterruptManager() = default;

public:
    /**
     * @brief Returns the singleton InterruptManager instance.
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
     * @brief Enables hardware interrupts by executing the sti instruction.
     *
     */
    void activate();

    /**
     * @brief Disables hardware interrupts by executing the cli instruction.
     *
     */
    void deactivate();

    /**
     * @brief Dispatches an interrupt to the registered driver or prints a warning.
     *
     */
    u32 handleInterrupt(u8 number, u32 esp);

    /**
     * @brief Writes a gate descriptor entry into the IDT at the given index.
     *
     */
    void setInterrupt(u8 number, u16 code_offset, void(*handler)(), u8 access, u8 flags);

    /**
     * @brief Populates the IDT, remaps the PICs, and loads the IDT via lidt.
     *
     */
    void load(cassio::kernel::GlobalDescriptorTable& gdt);

    /**
     * @brief Unloads the interrupt manager.
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
 * @brief C-linkage interrupt handler called from assembly stubs in stub.s.
 *
 */
extern "C" cassio::u32 handleInterrupt(cassio::u8 number, cassio::u32 esp);

#endif // HARDWARE_INTERRUPT_HPP_
