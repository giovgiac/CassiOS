/**
 * filesystem.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "filesystem/filesystem.hpp"
#include "common/string.hpp"

using namespace cassio;
using namespace cassio::filesystem;

Filesystem Filesystem::instance;

Filesystem::Filesystem() : root{} {
    root.name[0] = '/';
    root.name[1] = '\0';
    root.type = FileNodeType::Directory;
    root.parent = &root; // root's parent is itself
    root.children = nullptr;
    root.next = nullptr;
    root.data = nullptr;
    root.size = 0;
}

Filesystem& Filesystem::getFilesystem() {
    return instance;
}

FileNode* Filesystem::getRoot() {
    return &root;
}

FileNode* Filesystem::findChild(FileNode* dir, const char* name) {
    FileNode* child = dir->children;
    while (child != nullptr) {
        if (streq(child->name, name)) {
            return child;
        }
        child = child->next;
    }
    return nullptr;
}

void Filesystem::addChild(FileNode* dir, FileNode* child) {
    child->parent = dir;
    child->next = dir->children;
    dir->children = child;
}

void Filesystem::removeChild(FileNode* dir, FileNode* child) {
    if (dir->children == child) {
        dir->children = child->next;
        return;
    }
    FileNode* prev = dir->children;
    while (prev != nullptr && prev->next != child) {
        prev = prev->next;
    }
    if (prev != nullptr) {
        prev->next = child->next;
    }
}

FileNode* Filesystem::resolve(const char* path, FileNode* cwd) {
    if (path == nullptr || path[0] == '\0') {
        return nullptr;
    }

    FileNode* current;
    usize i = 0;

    // Absolute or relative?
    if (path[0] == '/') {
        current = &root;
        i = 1;
        // Handle bare "/" path.
        if (path[1] == '\0') {
            return &root;
        }
    } else {
        current = cwd;
    }

    // Walk path components separated by '/'.
    char component[MAX_NAME_LENGTH];
    while (path[i] != '\0') {
        // Extract next component.
        usize j = 0;
        while (path[i] != '\0' && path[i] != '/' && j < MAX_NAME_LENGTH - 1) {
            component[j++] = path[i++];
        }
        component[j] = '\0';

        // Skip trailing slash.
        if (path[i] == '/') {
            ++i;
        }

        // Skip empty components (e.g., "//").
        if (j == 0) {
            continue;
        }

        // Handle special components.
        if (streq(component, ".")) {
            continue;
        }
        if (streq(component, "..")) {
            current = current->parent;
            continue;
        }

        // Must be a directory to descend into.
        if (current->type != FileNodeType::Directory) {
            return nullptr;
        }

        FileNode* child = findChild(current, component);
        if (child == nullptr) {
            return nullptr;
        }
        current = child;
    }

    return current;
}

// Splits a path into parent path + final component name.
// Returns the parent node and writes the name into 'name_out'.
// Returns nullptr if the parent path doesn't resolve to a directory.
static FileNode* resolveParent(Filesystem& fs, const char* path,
                               FileNode* cwd, char* name_out, usize max) {
    // Find the last '/'.
    isize last_slash = -1;
    for (usize i = 0; path[i] != '\0'; ++i) {
        if (path[i] == '/') {
            last_slash = static_cast<isize>(i);
        }
    }

    FileNode* parent;
    const char* name;

    if (last_slash < 0) {
        // No slash: parent is cwd, name is the whole path.
        parent = cwd;
        name = path;
    } else if (last_slash == 0) {
        // Leading slash only (e.g., "/foo"): parent is root.
        parent = fs.getRoot();
        name = path + 1;
    } else {
        // Split at last slash: resolve parent path.
        // Temporarily null-terminate at the slash.
        // We need a local copy since path might be const.
        char parent_path[256];
        usize k = 0;
        while (k < 255 && k < static_cast<usize>(last_slash)) {
            parent_path[k] = path[k];
            ++k;
        }
        parent_path[k] = '\0';

        parent = fs.resolve(parent_path, cwd);
        name = path + last_slash + 1;
    }

    if (parent == nullptr || parent->type != FileNodeType::Directory) {
        return nullptr;
    }

    cassio::strcpy(name_out, name, max);

    return parent;
}

FileNode* Filesystem::createFile(const char* path, FileNode* cwd) {
    char name[MAX_NAME_LENGTH];
    FileNode* parent = resolveParent(*this, path, cwd, name, MAX_NAME_LENGTH);
    if (parent == nullptr || name[0] == '\0') {
        return nullptr;
    }

    // Check for duplicate name.
    if (findChild(parent, name) != nullptr) {
        return nullptr;
    }

    FileNode* node = new FileNode{};
    strcpy(node->name, name, MAX_NAME_LENGTH);
    node->type = FileNodeType::File;
    node->children = nullptr;
    node->next = nullptr;
    node->data = nullptr;
    node->size = 0;

    addChild(parent, node);
    return node;
}

FileNode* Filesystem::createDirectory(const char* path, FileNode* cwd) {
    char name[MAX_NAME_LENGTH];
    FileNode* parent = resolveParent(*this, path, cwd, name, MAX_NAME_LENGTH);
    if (parent == nullptr || name[0] == '\0') {
        return nullptr;
    }

    // Check for duplicate name.
    if (findChild(parent, name) != nullptr) {
        return nullptr;
    }

    FileNode* node = new FileNode{};
    strcpy(node->name, name, MAX_NAME_LENGTH);
    node->type = FileNodeType::Directory;
    node->children = nullptr;
    node->next = nullptr;
    node->data = nullptr;
    node->size = 0;

    addChild(parent, node);
    return node;
}

bool Filesystem::remove(FileNode* node) {
    if (node == nullptr || node == &root) {
        return false;
    }

    // Non-empty directories cannot be removed.
    if (node->type == FileNodeType::Directory && node->children != nullptr) {
        return false;
    }

    // Free file data.
    if (node->data != nullptr) {
        delete[] node->data;
    }

    removeChild(node->parent, node);
    delete node;
    return true;
}

bool Filesystem::write(FileNode* file, const u8* data, usize size) {
    if (file == nullptr || file->type != FileNodeType::File) {
        return false;
    }

    // Free existing data.
    if (file->data != nullptr) {
        delete[] file->data;
        file->data = nullptr;
        file->size = 0;
    }

    if (size == 0 || data == nullptr) {
        return true;
    }

    file->data = new u8[size];
    for (usize i = 0; i < size; ++i) {
        file->data[i] = data[i];
    }
    file->size = size;
    return true;
}

bool Filesystem::move(FileNode* node, const char* destPath, FileNode* cwd) {
    if (node == nullptr || node == &root) {
        return false;
    }

    char name[MAX_NAME_LENGTH];
    FileNode* destParent = resolveParent(*this, destPath, cwd, name, MAX_NAME_LENGTH);

    if (destParent == nullptr || name[0] == '\0') {
        return false;
    }

    // If destination resolves to an existing directory, move into it
    // keeping the original name.
    FileNode* existing = findChild(destParent, name);
    if (existing != nullptr && existing->type == FileNodeType::Directory) {
        destParent = existing;
        // Keep original name.
        strcpy(name, node->name, MAX_NAME_LENGTH);
    } else if (existing != nullptr) {
        // Destination exists and is not a directory.
        return false;
    }

    // Check for duplicate name in target directory.
    if (findChild(destParent, name) != nullptr) {
        return false;
    }

    // Detach from old parent, attach to new parent.
    removeChild(node->parent, node);
    strcpy(node->name, name, MAX_NAME_LENGTH);
    addChild(destParent, node);
    return true;
}

FileNode* Filesystem::copy(FileNode* node, const char* destPath, FileNode* cwd) {
    if (node == nullptr || node->type != FileNodeType::File) {
        return nullptr;
    }

    FileNode* newNode = createFile(destPath, cwd);
    if (newNode == nullptr) {
        return nullptr;
    }

    if (node->data != nullptr && node->size > 0) {
        write(newNode, node->data, node->size);
    }

    return newNode;
}
