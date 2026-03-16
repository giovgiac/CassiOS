/**
 * message.hpp -- IPC message format and type constants
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef COMMON_MESSAGE_HPP_
#define COMMON_MESSAGE_HPP_

#include <types.hpp>

namespace cassio {

struct Message {
    u32 type;
    u32 arg1;
    u32 arg2;
    u32 arg3;
    u32 arg4;
    u32 arg5;
};

namespace MessageType {
    constexpr u32 IrqNotify  = 1;
    constexpr u32 NsRegister = 2;
    constexpr u32 NsLookup   = 3;
}

} // cassio

#endif // COMMON_MESSAGE_HPP_
