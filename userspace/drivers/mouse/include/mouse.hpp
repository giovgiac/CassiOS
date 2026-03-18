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

#include <types.hpp>

namespace cassio {

class Mouse {
private:
    u8 buffer[3];
    u8 offset;
    u8 buttons;
    i32 dx;
    i32 dy;

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
    void handleByte(u8 byte);

    /**
     * @brief Read and reset the accumulated mouse state.
     *
     * Movement deltas are reset to zero after reading.
     *
     */
    void readState(u8& btns, i32& outDx, i32& outDy);

    u8 getButtons() const;
    i32 getDx() const;
    i32 getDy() const;

    Mouse(const Mouse&) = delete;
    Mouse(Mouse&&) = delete;
    Mouse& operator=(const Mouse&) = delete;
    Mouse& operator=(Mouse&&) = delete;
};

} // cassio

#endif // USERSPACE_MOUSE_MOUSE_HPP_
