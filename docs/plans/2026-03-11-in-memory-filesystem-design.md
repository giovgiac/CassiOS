# In-Memory Filesystem Design

## Overview

A hierarchical in-memory filesystem with a refactored command architecture. Files store data in contiguous heap-allocated buffers. Commands are dispatched via a registry of `Command` subclasses, replacing the current if-chain in the shell.

## Filesystem Data Model

The filesystem is a tree of nodes. Each node is either a file or a directory, distinguished by a type field.

```cpp
enum FileNodeType { FILE, DIRECTORY };

class FileNode {
    char name[128];
    FileNodeType type;
    FileNode* parent;       // null for root

    // Directory: linked list of children
    FileNode* children;     // first child
    FileNode* next;         // next sibling

    // File: contiguous buffer
    u8* data;               // null if empty
    usize size;
};
```

- Directories use `children`/`next` as an intrusive linked list. Files ignore these fields.
- Files use `data`/`size` for content. Directories ignore these fields.
- Fixed 128-byte name avoids dynamic string allocation.
- Nodes are heap-allocated with `new`/`delete`.

## Filesystem Singleton

The `Filesystem` class is a singleton that owns the root node and provides all filesystem operations.

### Operations

- `resolve(const char* path, FileNode* cwd)` -- resolve a path to a node, returns null if not found
- `createFile(const char* path, FileNode* cwd)` -- create an empty file at path
- `createDirectory(const char* path, FileNode* cwd)` -- create a directory at path
- `remove(FileNode* node)` -- remove a file or empty directory, frees data buffer
- `write(FileNode* file, const u8* data, usize size)` -- replace file contents
- `move(FileNode* node, const char* destPath, FileNode* cwd)` -- move or rename a node
- `copy(FileNode* node, const char* destPath, FileNode* cwd)` -- deep copy a file node

### Path Resolution

1. If path starts with `/`, start from root. Otherwise start from `cwd`.
2. Split by `/`, walk each component:
   - `.` -- stay at current node
   - `..` -- move to parent (root's parent is itself)
   - Otherwise -- search children linked list for matching name
3. Return final node or null.

For create operations, resolve the parent path (everything except the last component), then create the new node as a child.

## Command Architecture

### Base Class

```cpp
class Command {
    const char* name;
    const char* description;

    virtual void execute(const char** args, usize argc) = 0;
};
```

### Registry

A static array of `Command*` pointers (max 32) owned by the shell. Commands register via `shell.registerCommand(command)`. Commands are instantiated as globals -- constructors run during `ctors()`.

### Argument Parsing

The shell splits input on spaces into a `const char*` array and `argc` count, looks up the command by name, and calls `execute(args, argc)`.

For commands like `echo` and `write` that take free-form text, the command implementation treats everything from a certain argument index onward as raw text.

### Filesystem Access

Commands access the filesystem via `Filesystem::getFilesystem()`. The current working directory is a `FileNode*` owned by the shell, accessible to commands that need it.

## Commands

### Filesystem Commands

- `ls [path]` -- list directory contents (current directory if no argument)
- `cd <path>` -- change current directory
- `mkdir <path>` -- create a directory
- `rmdir <path>` -- remove an empty directory
- `touch <filename>` -- create an empty file
- `rm <filename>` -- remove a file
- `cat <filename>` -- print file contents
- `write <filename> <text>` -- write text to a file (overwrite)
- `mv <source> <destination>` -- move or rename a file or directory
- `cp <source> <destination>` -- copy a file

### General Commands

- `pwd` -- print current working directory
- `echo <text>` -- print text to screen
- `help` -- list available commands
- `clear` -- clear screen
- `mem` -- display memory statistics
- `reboot` -- reboot the system
- `shutdown` -- halt the system

## File Layout

```
include/core/commands/
    command.hpp
    ls.hpp, cd.hpp, mkdir.hpp, rmdir.hpp, touch.hpp, rm.hpp,
    cat.hpp, write.hpp, mv.hpp, cp.hpp, pwd.hpp, echo.hpp,
    help.hpp, clear.hpp, mem.hpp, reboot.hpp, shutdown.hpp

src/core/commands/
    command.cpp
    ls.cpp, cd.cpp, mkdir.cpp, rmdir.cpp, touch.cpp, rm.cpp,
    cat.cpp, write.cpp, mv.cpp, cp.cpp, pwd.cpp, echo.cpp,
    help.cpp, clear.cpp, mem.cpp, reboot.cpp, shutdown.cpp

include/filesystem/
    filesystem.hpp

src/filesystem/
    filesystem.cpp
```
