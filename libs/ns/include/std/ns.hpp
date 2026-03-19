/**
 * ns.hpp -- nameserver client library
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Client interface for the nameserver service. The nameserver is the
 * first userspace service (well-known PID 1) and provides name-to-PID
 * lookup for service discovery.
 *
 * Since the nameserver always runs at PID 1, this module uses free
 * functions rather than an instance class.
 *
 */

#ifndef STD_NS_HPP
#define STD_NS_HPP

#include <std/types.hpp>
#include <std/ipc.hpp>

namespace std {
namespace ns {

/// Well-known PID of the nameserver (always the first userspace process).
constexpr u32 PID = 1;

/// Maximum length of a service name (excluding null terminator).
constexpr u32 MAX_NAME_LEN = 16;

/// Entry returned by listAll(), containing a service name and its PID.
struct Entry {
    char name[20];  // MAX_NAME_LEN + 1 + padding for u32 alignment.
    u32 pid;
};

/// Pack a service name into IPC message args for nameserver requests.
/// Names longer than MAX_NAME_LEN are silently truncated.
void packName(const char* name, ipc::Message& msg);

/// Unpack a service name from IPC message args into a buffer.
/// The output buffer must hold at least MAX_NAME_LEN + 1 bytes.
void unpackName(const ipc::Message& msg, char* out);

/// Register a service name with the nameserver. The calling process's
/// PID is automatically associated with the name. Returns 1 on success,
/// 0 if the name is already registered.
u32 registerName(const char* name);

/// Look up a service PID by name. Returns the PID on success, or 0 if
/// the name is not registered. Callers typically retry in a loop until
/// the service registers.
u32 lookup(const char* name);

/// Retrieve all registered service entries. Writes up to maxEntries
/// entries into buf and returns the number actually written.
u32 listAll(Entry* buf, u32 maxEntries);

} // ns
} // std

#endif // STD_NS_HPP
