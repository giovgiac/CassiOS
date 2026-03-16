/**
 * message.hpp -- IPC message format and type constants
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Shared between kernel and userspace. No namespace or kernel-specific types.
 *
 */

#ifndef SHARED_MESSAGE_HPP_
#define SHARED_MESSAGE_HPP_

struct Message {
    unsigned int type;
    unsigned int arg1;
    unsigned int arg2;
    unsigned int arg3;
    unsigned int arg4;
    unsigned int arg5;
};

namespace MessageType {
    constexpr unsigned int IrqNotify  = 1;
    constexpr unsigned int NsRegister = 2;
    constexpr unsigned int NsLookup   = 3;
}

#endif // SHARED_MESSAGE_HPP_
