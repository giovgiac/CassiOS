/**
 * vfs.hpp -- VFS service client
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Instance-based client for the VFS (virtual filesystem) service.
 * The constructor resolves the service PID from the nameserver
 * automatically, blocking until the service is registered.
 *
 */

#ifndef STD_VFS_HPP
#define STD_VFS_HPP

#include <std/types.hpp>

namespace std {
namespace vfs {

class Vfs {
public:
    /// Construct a VFS client. Blocks until the "vfs" service is
    /// registered with the nameserver.
    Vfs();

    /// Create a directory at the given path.
    /// Returns 0 on success, non-zero on failure.
    u32 mkdir(const char* path);

    /// Remove a file or empty directory at the given path.
    /// Returns 0 on success, non-zero on failure.
    u32 remove(const char* path);

    /// Open a file by path. If create is true, creates it if not found.
    /// Returns a non-zero handle on success, 0 on failure.
    u32 open(const char* path, bool create = false);

    /// Read from an open file handle at the given byte offset.
    /// Reads up to bufLen bytes into buf. Returns the number of bytes
    /// read, or 0 on failure/EOF.
    i32 read(u32 handle, u32 offset, u8* buf, u32 bufLen);

    /// Write data to an open file handle.
    /// Returns 0 on success, non-zero on failure.
    u32 write(u32 handle, const u8* data, u32 len);

    /// Query the type of an entry at the given path.
    /// Returns 0 = not found, 1 = file, 2 = directory.
    u32 stat(const char* path);

    /// List the entry at the given index within a directory.
    /// Writes the entry name into nameOut (up to nameMax bytes).
    /// Returns true if an entry was found, false if index is past the end.
    bool list(const char* path, u32 index, char* nameOut, u32 nameMax);

    Vfs(const Vfs&) = delete;
    Vfs& operator=(const Vfs&) = delete;

private:
    u32 pid;
};

} // vfs
} // std

#endif // STD_VFS_HPP
