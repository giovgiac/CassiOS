/**
 * exception.hpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef HARDWARE_EXCEPTION_HPP_
#define HARDWARE_EXCEPTION_HPP_

#include <common/types.hpp>

namespace cassio {
namespace hardware {

/**
 * @brief Singleton that handles CPU exceptions (vectors 0-31).
 *
 * Registers IDT entries for div-by-zero, invalid opcode, GPF, and page fault.
 * Logs the fault to serial and halts.
 *
 */
class ExceptionHandler final {
private:
    static ExceptionHandler instance;

private:
    ExceptionHandler();
    ~ExceptionHandler() = default;

public:
    /**
     * @brief Returns the singleton ExceptionHandler instance.
     *
     */
    inline static ExceptionHandler& getHandler() {
        return instance;
    }

    /**
     * @brief Assembly entry points for exception vectors (defined in stub.s).
     *
     */
    static void handleException0x00();
    static void handleException0x06();
    static void handleException0x0D();
    static void handleException0x0E();

    /**
     * @brief Registers exception IDT entries for vectors 0, 6, 13, and 14.
     *
     */
    void load();

    /**
     * @brief Handles a CPU exception by logging to serial and halting.
     *
     */
    u32 handle(u8 vector, u32 error_code, u32 esp);

    /** Deleted Methods */
    ExceptionHandler(const ExceptionHandler&) = delete;
    ExceptionHandler(ExceptionHandler&&) = delete;
    ExceptionHandler& operator=(const ExceptionHandler&) = delete;
    ExceptionHandler& operator=(ExceptionHandler&&) = delete;
};

} // hardware
} // cassio

/**
 * @brief C-linkage exception handler called from assembly stubs in stub.s.
 *
 */
extern "C" cassio::u32 handleException(cassio::u8 vector, cassio::u32 error_code, cassio::u32 esp);

#endif // HARDWARE_EXCEPTION_HPP_
