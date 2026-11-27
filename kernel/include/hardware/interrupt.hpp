/**
 * interrupt.hpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * The IDT is Rust (kernel/src/hardware/interrupt.rs). InterruptManager
 * forwards to it until its callers are ported.
 *
 */

#ifndef HARDWARE_INTERRUPT_HPP_
#define HARDWARE_INTERRUPT_HPP_

#include <std/types.hpp>

#include <core/gdt.hpp>

extern "C" void interrupt_init();
extern "C" void interrupt_enable();

namespace cassio {
namespace hardware {

class InterruptManager final {
public:
    inline static InterruptManager& getManager() { return instance; }

    /** Enables hardware interrupts (sti). */
    void activate() { interrupt_enable(); }

    /**
     * @brief Fills and loads the IDT (exceptions, IRQs with the PICs
     * remapped, the syscall gate). The GDT must be loaded.
     *
     */
    void load(cassio::kernel::GlobalDescriptorTable& gdt);

    /** Deleted Methods */
    InterruptManager(const InterruptManager&) = delete;
    InterruptManager(InterruptManager&&) = delete;
    InterruptManager& operator=(const InterruptManager&) = delete;
    InterruptManager& operator=(InterruptManager&&) = delete;

private:
    InterruptManager() = default;

    static InterruptManager instance;
};

} // namespace hardware
} // namespace cassio

#endif // HARDWARE_INTERRUPT_HPP_
