/**
 * filesystem.hpp -- In-memory filesystem for the VFS service
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef USERSPACE_VFS_FILESYSTEM_HPP_
#define USERSPACE_VFS_FILESYSTEM_HPP_

#include <types.hpp>

namespace cassio {
namespace vfs {

constexpr u32 MAX_NODES = 32;
constexpr u32 MAX_NAME = 32;
constexpr u32 MAX_FILE_DATA = 256;
constexpr u8 INVALID = 0xFF;
constexpr u8 ROOT_INDEX = 0;

enum class NodeType : u8 {
    Unused = 0,
    File = 1,
    Directory = 2
};

struct FileNode {
    char name[MAX_NAME];
    NodeType type;
    u8 parent;
    u8 firstChild;
    u8 nextSibling;
    u8 data[MAX_FILE_DATA];
    u32 size;
};

class Filesystem {
private:
    FileNode nodes[MAX_NODES];

    u8 allocNode();
    void freeNode(u8 index);
    u8 findChild(u8 dir, const char* name);
    void addChild(u8 dir, u8 child);
    void removeChild(u8 dir, u8 child);
    u8 resolveParent(const char* path, char* nameOut, u32 nameMax);

public:
    void init();

    u8 resolve(const char* path);
    u8 createFile(const char* path);
    u8 createDirectory(const char* path);
    bool remove(const char* path);

    i32 read(u8 node, u32 offset, u8* buf, u32 len);
    bool write(u8 node, const u8* data, u32 len);

    bool listEntry(u8 dir, u32 index, char* nameOut, u32 nameMax, NodeType& type);

    bool isValid(u8 index) const;
    bool isFile(u8 index) const;
    bool isDirectory(u8 index) const;
};

} // vfs
} // cassio

#endif // USERSPACE_VFS_FILESYSTEM_HPP_
