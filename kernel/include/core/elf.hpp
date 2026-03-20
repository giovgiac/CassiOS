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

#include <std/types.hpp>

namespace cassio {
namespace kernel {

struct __attribute__((packed)) Elf32Header {
    std::u8 e_ident[16];
    std::u16 e_type;
    std::u16 e_machine;
    std::u32 e_version;
    std::u32 e_entry;
    std::u32 e_phoff;
    std::u32 e_shoff;
    std::u32 e_flags;
    std::u16 e_ehsize;
    std::u16 e_phentsize;
    std::u16 e_phnum;
    std::u16 e_shentsize;
    std::u16 e_shnum;
    std::u16 e_shstrndx;
};

struct __attribute__((packed)) Elf32ProgramHeader {
    std::u32 p_type;
    std::u32 p_offset;
    std::u32 p_vaddr;
    std::u32 p_paddr;
    std::u32 p_filesz;
    std::u32 p_memsz;
    std::u32 p_flags;
    std::u32 p_align;
};

// ELF identification indices.
static constexpr std::u32 EI_MAG0 = 0;
static constexpr std::u32 EI_CLASS = 4;

// ELF magic bytes.
static constexpr std::u8 ELFMAG0 = 0x7F;
static constexpr std::u8 ELFMAG1 = 'E';
static constexpr std::u8 ELFMAG2 = 'L';
static constexpr std::u8 ELFMAG3 = 'F';

// ELF class.
static constexpr std::u8 ELFCLASS32 = 1;

// ELF type.
static constexpr std::u16 ET_EXEC = 2;

// ELF machine.
static constexpr std::u16 EM_386 = 3;

// Program header types.
static constexpr std::u32 PT_LOAD = 1;

struct ElfLoadResult {
    std::u32 entryPoint;
    std::u32 heapStart; // Page-aligned end of loaded segments (initial sbrk break).
    bool success;
};

/**
 * @brief Loads ELF32 executables into a given address space.
 *
 */
class ElfLoader {
public:
    static ElfLoadResult load(std::u32 pdPhysical, const std::u8* elfData, std::u32 elfSize);
};

} // namespace kernel
} // namespace cassio

#endif // CORE_ELF_HPP_
