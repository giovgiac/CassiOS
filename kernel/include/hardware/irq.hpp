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
#include <message.hpp>
#include <hardware/port.hpp>

namespace cassio {
namespace hardware {

// Offset added to PIC IRQs to avoid conflict with CPU exceptions.
constexpr u8 IRQ_OFFSET = 0x20;

// IRQ handler function pointer: takes ESP, returns (possibly modified) ESP.
typedef u32 (*IrqHandler)(u32 esp);

/**
 * @brief Singleton that owns the PIC and dispatches hardware IRQs.
 *
 * Manages the 8259 PIC pair (master and slave), routes IRQs 0-15 to
 * registered in-kernel handlers or forwards them to userspace, and
 * sends EOI after handling.
 *
 */
class IrqManager final {
private:
    IrqHandler handlers[16];
    u32 forwardPid[16];
    bool pendingIrq[16];

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
     * @brief Dispatches an IRQ to the registered handler and sends EOI.
     *
     */
    u32 handleIrq(u8 number, u32 esp);

    /**
     * @brief Registers a handler function for the given IRQ number (0-15).
     *
     */
    void registerHandler(u8 irq, IrqHandler handler);

    /**
     * @brief Unregisters the handler for the given IRQ number.
     *
     */
    void unregisterHandler(u8 irq);

    /**
     * @brief Registers a userspace process to receive IRQ notifications.
     *
     * One process per IRQ. Returns 0 on success, -1 if irq is out of range.
     *
     */
    i32 registerForward(u8 irq, u32 pid);

    /**
     * @brief Delivers a pending IRQ notification to a process.
     *
     * Scans for pending IRQs registered to the given PID and fills msg
     * with the first one found. Returns true if a notification was
     * delivered, false if none pending.
     *
     */
    bool deliverPending(u32 pid, Message* msg);

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
