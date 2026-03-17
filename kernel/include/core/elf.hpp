/**
 * elf.hpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef CORE_ELF_HPP_
#define CORE_ELF_HPP_

#include <types.hpp>

namespace cassio {
namespace kernel {

struct __attribute__((packed)) Elf32Header {
    u8  e_ident[16];
    u16 e_type;
    u16 e_machine;
    u32 e_version;
    u32 e_entry;
    u32 e_phoff;
    u32 e_shoff;
    u32 e_flags;
    u16 e_ehsize;
    u16 e_phentsize;
    u16 e_phnum;
    u16 e_shentsize;
    u16 e_shnum;
    u16 e_shstrndx;
};

struct __attribute__((packed)) Elf32ProgramHeader {
    u32 p_type;
    u32 p_offset;
    u32 p_vaddr;
    u32 p_paddr;
    u32 p_filesz;
    u32 p_memsz;
    u32 p_flags;
    u32 p_align;
};

// ELF identification indices.
static constexpr u32 EI_MAG0    = 0;
static constexpr u32 EI_CLASS   = 4;

// ELF magic bytes.
static constexpr u8 ELFMAG0 = 0x7F;
static constexpr u8 ELFMAG1 = 'E';
static constexpr u8 ELFMAG2 = 'L';
static constexpr u8 ELFMAG3 = 'F';

// ELF class.
static constexpr u8 ELFCLASS32 = 1;

// ELF type.
static constexpr u16 ET_EXEC = 2;

// ELF machine.
static constexpr u16 EM_386 = 3;

// Program header types.
static constexpr u32 PT_LOAD = 1;

struct ElfLoadResult {
    u32 entryPoint;
    u32 heapStart;  // Page-aligned end of loaded segments (initial sbrk break).
    bool success;
};

/**
 * @brief Loads ELF32 executables into a given address space.
 *
 */
class ElfLoader {
public:
    static ElfLoadResult load(u32 pdPhysical, const u8* elfData, u32 elfSize);
};

} // kernel
} // cassio

#endif // CORE_ELF_HPP_
