/**
 * filesystem.cpp -- In-memory filesystem implementation
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <filesystem.hpp>
#include <string.hpp>

using namespace cassio;
using namespace cassio::vfs;

void Filesystem::init() {
    for (u32 i = 0; i < MAX_NODES; i++) {
        nodes[i].type = NodeType::Unused;
    }

    nodes[ROOT_INDEX].name[0] = '/';
    nodes[ROOT_INDEX].name[1] = '\0';
    nodes[ROOT_INDEX].type = NodeType::Directory;
    nodes[ROOT_INDEX].parent = ROOT_INDEX;
    nodes[ROOT_INDEX].firstChild = INVALID;
    nodes[ROOT_INDEX].nextSibling = INVALID;
    nodes[ROOT_INDEX].size = 0;
}

u8 Filesystem::allocNode() {
    for (u32 i = 1; i < MAX_NODES; i++) {
        if (nodes[i].type == NodeType::Unused) {
            return static_cast<u8>(i);
        }
    }
    return INVALID;
}

void Filesystem::freeNode(u8 index) {
    nodes[index].type = NodeType::Unused;
}

u8 Filesystem::findChild(u8 dir, const char* name) {
    u8 child = nodes[dir].firstChild;
    while (child != INVALID) {
        if (streq(nodes[child].name, name)) {
            return child;
        }
        child = nodes[child].nextSibling;
    }
    return INVALID;
}

void Filesystem::addChild(u8 dir, u8 child) {
    nodes[child].parent = dir;
    nodes[child].nextSibling = nodes[dir].firstChild;
    nodes[dir].firstChild = child;
}

void Filesystem::removeChild(u8 dir, u8 child) {
    if (nodes[dir].firstChild == child) {
        nodes[dir].firstChild = nodes[child].nextSibling;
        return;
    }
    u8 prev = nodes[dir].firstChild;
    while (prev != INVALID && nodes[prev].nextSibling != child) {
        prev = nodes[prev].nextSibling;
    }
    if (prev != INVALID) {
        nodes[prev].nextSibling = nodes[child].nextSibling;
    }
}

u8 Filesystem::resolveParent(const char* path, char* nameOut, u32 nameMax) {
    isize lastSlash = -1;
    for (usize i = 0; path[i] != '\0'; i++) {
        if (path[i] == '/') {
            lastSlash = static_cast<isize>(i);
        }
    }

    u8 parent;
    const char* name;

    if (lastSlash < 0) {
        // No slash: parent is root.
        parent = ROOT_INDEX;
        name = path;
    } else if (lastSlash == 0) {
        // Leading slash only (e.g., "/foo").
        parent = ROOT_INDEX;
        name = path + 1;
    } else {
        char parentPath[MAX_NAME];
        usize k = 0;
        while (k < MAX_NAME - 1 && k < static_cast<usize>(lastSlash)) {
            parentPath[k] = path[k];
            k++;
        }
        parentPath[k] = '\0';
        parent = resolve(parentPath);
        name = path + lastSlash + 1;
    }

    if (parent == INVALID || nodes[parent].type != NodeType::Directory) {
        return INVALID;
    }

    strcpy(nameOut, name, nameMax);
    return parent;
}

u8 Filesystem::resolve(const char* path) {
    if (path == nullptr || path[0] == '\0') {
        return INVALID;
    }

    u8 current;
    usize i = 0;

    if (path[0] == '/') {
        current = ROOT_INDEX;
        i = 1;
        if (path[1] == '\0') {
            return ROOT_INDEX;
        }
    } else {
        current = ROOT_INDEX;
    }

    char component[MAX_NAME];
    while (path[i] != '\0') {
        usize j = 0;
        while (path[i] != '\0' && path[i] != '/' && j < MAX_NAME - 1) {
            component[j++] = path[i++];
        }
        component[j] = '\0';

        if (path[i] == '/') {
            i++;
        }

        if (j == 0) {
            continue;
        }

        if (streq(component, ".")) {
            continue;
        }
        if (streq(component, "..")) {
            current = nodes[current].parent;
            continue;
        }

        if (nodes[current].type != NodeType::Directory) {
            return INVALID;
        }

        u8 child = findChild(current, component);
        if (child == INVALID) {
            return INVALID;
        }
        current = child;
    }

    return current;
}

u8 Filesystem::createFile(const char* path) {
    char name[MAX_NAME];
    u8 parent = resolveParent(path, name, MAX_NAME);
    if (parent == INVALID || name[0] == '\0') {
        return INVALID;
    }
    if (findChild(parent, name) != INVALID) {
        return INVALID;
    }

    u8 index = allocNode();
    if (index == INVALID) {
        return INVALID;
    }

    strcpy(nodes[index].name, name, MAX_NAME);
    nodes[index].type = NodeType::File;
    nodes[index].firstChild = INVALID;
    nodes[index].nextSibling = INVALID;
    nodes[index].size = 0;

    addChild(parent, index);
    return index;
}

u8 Filesystem::createDirectory(const char* path) {
    char name[MAX_NAME];
    u8 parent = resolveParent(path, name, MAX_NAME);
    if (parent == INVALID || name[0] == '\0') {
        return INVALID;
    }
    if (findChild(parent, name) != INVALID) {
        return INVALID;
    }

    u8 index = allocNode();
    if (index == INVALID) {
        return INVALID;
    }

    strcpy(nodes[index].name, name, MAX_NAME);
    nodes[index].type = NodeType::Directory;
    nodes[index].firstChild = INVALID;
    nodes[index].nextSibling = INVALID;
    nodes[index].size = 0;

    addChild(parent, index);
    return index;
}

bool Filesystem::remove(const char* path) {
    u8 index = resolve(path);
    if (index == INVALID || index == ROOT_INDEX) {
        return false;
    }

    if (nodes[index].type == NodeType::Directory &&
        nodes[index].firstChild != INVALID) {
        return false;
    }

    removeChild(nodes[index].parent, index);
    freeNode(index);
    return true;
}

i32 Filesystem::read(u8 node, u32 offset, u8* buf, u32 len) {
    if (node == INVALID || nodes[node].type != NodeType::File) {
        return -1;
    }
    if (offset >= nodes[node].size) {
        return 0;
    }

    u32 available = nodes[node].size - offset;
    u32 toRead = len < available ? len : available;

    for (u32 i = 0; i < toRead; i++) {
        buf[i] = nodes[node].data[offset + i];
    }

    return static_cast<i32>(toRead);
}

bool Filesystem::write(u8 node, const u8* data, u32 len) {
    if (node == INVALID || nodes[node].type != NodeType::File) {
        return false;
    }
    if (len > MAX_FILE_DATA) {
        return false;
    }

    for (u32 i = 0; i < len; i++) {
        nodes[node].data[i] = data[i];
    }
    nodes[node].size = len;
    return true;
}

bool Filesystem::listEntry(u8 dir, u32 index, char* nameOut, u32 nameMax,
                           NodeType& type) {
    if (dir == INVALID || nodes[dir].type != NodeType::Directory) {
        return false;
    }

    u8 child = nodes[dir].firstChild;
    u32 count = 0;
    while (child != INVALID) {
        if (count == index) {
            strcpy(nameOut, nodes[child].name, nameMax);
            type = nodes[child].type;
            return true;
        }
        count++;
        child = nodes[child].nextSibling;
    }
    return false;
}

bool Filesystem::isValid(u8 index) const {
    return index < MAX_NODES && nodes[index].type != NodeType::Unused;
}

bool Filesystem::isFile(u8 index) const {
    return index < MAX_NODES && nodes[index].type == NodeType::File;
}

bool Filesystem::isDirectory(u8 index) const {
    return index < MAX_NODES && nodes[index].type == NodeType::Directory;
}
