/**
 * interrupt.hpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef HARDWARE_INTERRUPT_HPP_
#define HARDWARE_INTERRUPT_HPP_

#include <common/types.hpp>
#include <core/gdt.hpp>
#include <hardware/port.hpp>

namespace cassio {
namespace hardware {

/**
 * @brief Flag constants used when building IDT gate descriptors.
 *
 */
enum InterruptFlags : u8 {
    IDT_DESCRIPTOR_PRESENT = 0x80,
    IDT_INTERRUPT_GATE = 0xE,
    IDT_TRAP_GATE = 0xF
};

/**
 * @brief Singleton that owns the IDT. Delegates to ExceptionHandler, IrqManager, and SyscallHandler.
 *
 */
class InterruptManager final {
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
    u16 code_offset;

    static InterruptManager instance;

private:
    /**
     * @brief Constructs the manager.
     *
     */
    InterruptManager();

    /**
     * @brief Destroys the interrupt manager.
     *
     */
    ~InterruptManager() = default;

    /**
     * @brief Writes a gate descriptor entry into the IDT at the given index.
     *
     */
    void setInterrupt(u8 number, u16 code_offset, void(*handler)(), u8 access, u8 flags);

public:
    /**
     * @brief Returns the singleton InterruptManager instance.
     *
     */
    inline static InterruptManager& getManager() {
        return instance;
    }

    static void ignoreInterruptRequest();

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
     * @brief Sets an interrupt gate (DPL=0) at the given vector.
     *
     */
    void setInterruptGate(u8 vector, void(*handler)());

    /**
     * @brief Sets a trap gate at the given vector with the specified DPL.
     *
     */
    void setTrapGate(u8 vector, void(*handler)(), u8 dpl);

    /**
     * @brief Populates the IDT, delegates to sub-managers, and loads the IDT via lidt.
     *
     */
    void load(cassio::kernel::GlobalDescriptorTable& gdt);

    /** Deleted Methods */
    InterruptManager(const InterruptManager&) = delete;
    InterruptManager(InterruptManager&&) = delete;
    InterruptManager& operator=(const InterruptManager&) = delete;
    InterruptManager& operator=(InterruptManager&&) = delete;

};

}
}

#endif // HARDWARE_INTERRUPT_HPP_
