/**
 * irq.hpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef HARDWARE_IRQ_HPP_
#define HARDWARE_IRQ_HPP_

#include <types.hpp>
#include <hardware/port.hpp>

namespace cassio {
namespace hardware {

// Offset added to PIC IRQs to avoid conflict with CPU exceptions.
constexpr u8 IRQ_OFFSET = 0x20;

class Driver; // forward declaration

/**
 * @brief Singleton that owns the PIC and dispatches hardware IRQs to drivers.
 *
 * Manages the 8259 PIC pair (master and slave), routes IRQs 0-15 to
 * registered in-kernel drivers, and sends EOI after handling.
 *
 */
class IrqManager final {
private:
    Driver* drv[16];

    Port<u8> pic_master_cmd;
    Port<u8> pic_master_data;
    Port<u8> pic_slave_cmd;
    Port<u8> pic_slave_data;

    static IrqManager instance;

private:
    IrqManager();
    ~IrqManager() = default;

public:
    /**
     * @brief Returns the singleton IrqManager instance.
     *
     */
    inline static IrqManager& getManager() {
        return instance;
    }

    /**
     * @brief Assembly entry points for IRQ vectors (defined in stub.s).
     *
     */
    static void handleInterruptRequest0x00();
    static void handleInterruptRequest0x01();
    static void handleInterruptRequest0x0C();
    static void handleInterruptRequest0x0E();

    /**
     * @brief Registers IDT entries for IRQ vectors and initializes the PICs.
     *
     */
    void load();

    /**
     * @brief Dispatches an IRQ to the registered driver and sends EOI.
     *
     */
    u32 handleIrq(u8 number, u32 esp);

    /**
     * @brief Registers a driver for the given IRQ vector number.
     *
     */
    void registerDriver(u8 vector, Driver* driver);

    /**
     * @brief Unregisters a driver for the given IRQ vector number.
     *
     */
    void unregisterDriver(u8 vector, Driver* driver);

    /** Deleted Methods */
    IrqManager(const IrqManager&) = delete;
    IrqManager(IrqManager&&) = delete;
    IrqManager& operator=(const IrqManager&) = delete;
    IrqManager& operator=(IrqManager&&) = delete;
};

} // hardware
} // cassio

/**
 * @brief C-linkage interrupt handler called from assembly stubs in stub.s.
 *
 */
extern "C" cassio::u32 handleInterrupt(cassio::u8 number, cassio::u32 esp);

#endif // HARDWARE_IRQ_HPP_
