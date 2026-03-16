/**
 * multiboot.hpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef MEMORY_MULTIBOOT_HPP_
#define MEMORY_MULTIBOOT_HPP_

#include <common/types.hpp>

namespace cassio {
namespace memory {

struct __attribute__((packed)) MultibootInfo {
    u32 flags;
    u32 mem_lower;
    u32 mem_upper;
    u32 boot_device;
    u32 cmdline;
    u32 mods_count;
    u32 mods_addr;
    u32 syms[4];
    u32 mmap_length;
    u32 mmap_addr;
};

struct __attribute__((packed)) MultibootMmapEntry {
    u32 size;
    u64 base_addr;
    u64 length;
    u32 type;
};

struct __attribute__((packed)) MultibootModule {
    u32 mod_start;
    u32 mod_end;
    u32 string;
    u32 reserved;
};

static constexpr u32 MULTIBOOT_FLAG_MODS = (1 << 3);
static constexpr u32 MULTIBOOT_FLAG_MMAP = (1 << 6);
static constexpr u32 MULTIBOOT_MMAP_AVAILABLE = 1;

} // memory
} // cassio

#endif // MEMORY_MULTIBOOT_HPP_
