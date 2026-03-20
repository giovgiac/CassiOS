/**
 * mouse.hpp -- mouse service client
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Instance-based client for the mouse driver service. The
 * constructor resolves the service PID from the nameserver
 * automatically, blocking until the service is registered.
 *
 */

#ifndef STD_MOUSE_HPP
#define STD_MOUSE_HPP

#include <std/types.hpp>

namespace std {
namespace mouse {

class Mouse {
  public:
    /// Construct a mouse client. Blocks until the "mouse" service is
    /// registered with the nameserver.
    Mouse();

    /// Read the current mouse state. Returns button flags in buttons,
    /// and accumulated movement deltas in dx/dy since the last read.
    void read(u8& buttons, i32& dx, i32& dy);

    Mouse(const Mouse&) = delete;
    Mouse& operator=(const Mouse&) = delete;

  private:
    u32 pid;
};

} // namespace mouse
} // namespace std

#endif // STD_MOUSE_HPP
