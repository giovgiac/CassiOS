/**
 * filesystem.hpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef FILESYSTEM_FILESYSTEM_HPP_
#define FILESYSTEM_FILESYSTEM_HPP_

#include <common/types.hpp>

namespace cassio {
namespace filesystem {

constexpr usize MAX_NAME_LENGTH = 128;

enum class FileNodeType : u8 {
    File,
    Directory
};

struct FileNode {
    char name[MAX_NAME_LENGTH];
    FileNodeType type;
    FileNode* parent;

    // Directory: intrusive linked list of children.
    FileNode* children;
    FileNode* next;

    // File: contiguous data buffer.
    u8* data;
    usize size;
};

/**
 * @brief In-memory hierarchical filesystem singleton.
 *
 * Owns the root directory node and provides operations for creating,
 * removing, reading, writing, moving, and copying files and directories.
 *
 */
class Filesystem {
private:
    FileNode root;

    static Filesystem instance;

    Filesystem();

    bool streq(const char* a, const char* b);
    void strcpy(char* dst, const char* src, usize max);
    FileNode* findChild(FileNode* dir, const char* name);
    void addChild(FileNode* dir, FileNode* child);
    void removeChild(FileNode* dir, FileNode* child);

public:
    ~Filesystem() = default;

    static Filesystem& getFilesystem();

    /**
     * @brief Returns the root directory node.
     *
     */
    FileNode* getRoot();

    /**
     * @brief Resolves a path to a node, starting from cwd for relative paths.
     *
     * Supports absolute paths (/foo/bar), relative paths (foo/bar),
     * current directory (.), and parent directory (..).
     *
     * @return The resolved node, or nullptr if not found.
     *
     */
    FileNode* resolve(const char* path, FileNode* cwd);

    /**
     * @brief Creates an empty file at the given path.
     *
     * @return The new file node, or nullptr on failure.
     *
     */
    FileNode* createFile(const char* path, FileNode* cwd);

    /**
     * @brief Creates a directory at the given path.
     *
     * @return The new directory node, or nullptr on failure.
     *
     */
    FileNode* createDirectory(const char* path, FileNode* cwd);

    /**
     * @brief Removes a file or empty directory and frees its resources.
     *
     * @return true on success, false if the node is a non-empty directory
     *         or the root.
     *
     */
    bool remove(FileNode* node);

    /**
     * @brief Replaces the contents of a file with new data.
     *
     * Frees any existing data buffer and allocates a new one.
     *
     * @return true on success, false if the node is not a file.
     *
     */
    bool write(FileNode* file, const u8* data, usize size);

    /**
     * @brief Moves a node to a new location.
     *
     * @return true on success, false on failure.
     *
     */
    bool move(FileNode* node, const char* destPath, FileNode* cwd);

    /**
     * @brief Deep copies a file node to a new location.
     *
     * @return The new node, or nullptr on failure. Only files can be copied.
     *
     */
    FileNode* copy(FileNode* node, const char* destPath, FileNode* cwd);

    /** Deleted Methods */
    Filesystem(const Filesystem&) = delete;
    Filesystem(Filesystem&&) = delete;
    Filesystem& operator=(const Filesystem&) = delete;
    Filesystem& operator=(Filesystem&&) = delete;
};

} // filesystem
} // cassio

#endif // FILESYSTEM_FILESYSTEM_HPP_
