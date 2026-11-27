/**
 * irq.hpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * IRQ dispatch is Rust (kernel/src/hardware/irq.rs). IrqManager forwards
 * to it until its callers are ported.
 *
 */

#ifndef HARDWARE_IRQ_HPP_
#define HARDWARE_IRQ_HPP_

#include <std/ipc.hpp>
#include <std/types.hpp>

extern "C" std::u32 irq_dispatch(std::u8 vector, std::u32 esp);
extern "C" std::i32 irq_register_forward(std::u8 irq, std::u32 pid);
extern "C" bool irq_deliver_pending(std::u32 pid, std::ipc::Message* msg);

/**
 * @brief Delivers IrqNotify for irq to process pid if it is blocked in
 * receive. Called by the Rust IRQ dispatch. Returns 0 if there is no such
 * process, 1 if the message was delivered, 2 if the process is not
 * receiving (the IRQ stays pending).
 *
 */
extern "C" std::u32 process_deliver_irq(std::u32 pid, std::u8 irq);

namespace cassio {
namespace hardware {

// Offset added to PIC IRQs to avoid conflict with CPU exceptions.
constexpr std::u8 IRQ_OFFSET = 0x20;

class IrqManager final {
public:
    inline static IrqManager& getManager() { return instance; }

    /** Dispatches the IRQ at the given vector and sends EOI. */
    std::u32 handleIrq(std::u8 number, std::u32 esp) { return irq_dispatch(number, esp); }

    /**
     * @brief Registers a userspace process to receive IRQ notifications.
     *
     * One process per IRQ. Returns 0 on success, -1 if irq is out of range.
     *
     */
    std::i32 registerForward(std::u8 irq, std::u32 pid) { return irq_register_forward(irq, pid); }

    /**
     * @brief Fills msg with the first IRQ pending for pid. Returns false
     * if none is pending.
     *
     */
    bool deliverPending(std::u32 pid, std::ipc::Message* msg) {
        return irq_deliver_pending(pid, msg);
    }

    /** Deleted Methods */
    IrqManager(const IrqManager&) = delete;
    IrqManager(IrqManager&&) = delete;
    IrqManager& operator=(const IrqManager&) = delete;
    IrqManager& operator=(IrqManager&&) = delete;

private:
    IrqManager() = default;

    static IrqManager instance;
};

} // namespace hardware
} // namespace cassio

#endif // HARDWARE_IRQ_HPP_
