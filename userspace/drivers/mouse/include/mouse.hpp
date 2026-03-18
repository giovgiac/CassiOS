/**
 * mouse.hpp -- PS/2 mouse packet parser and state tracker
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef USERSPACE_MOUSE_MOUSE_HPP_
#define USERSPACE_MOUSE_MOUSE_HPP_

#include <std/types.hpp>

namespace cassio {

class Mouse {
private:
    std::u8 buffer[3];
    std::u8 offset;
    std::u8 buttons;
    std::i32 dx;
    std::i32 dy;

public:
    Mouse() = default;

    void init();

    /**
     * @brief Feed a single byte from the PS/2 data port.
     *
     * Bytes are collected into a 3-byte packet. When a full packet
     * is assembled, internal state (buttons, dx, dy) is updated.
     *
     */
    void handleByte(std::u8 byte);

    /**
     * @brief Read and reset the accumulated mouse state.
     *
     * Movement deltas are reset to zero after reading.
     *
     */
    void readState(std::u8& btns, std::i32& outDx, std::i32& outDy);

    std::u8 getButtons() const;
    std::i32 getDx() const;
    std::i32 getDy() const;

    Mouse(const Mouse&) = delete;
    Mouse(Mouse&&) = delete;
    Mouse& operator=(const Mouse&) = delete;
    Mouse& operator=(Mouse&&) = delete;
};

} // cassio

#endif // USERSPACE_MOUSE_MOUSE_HPP_
