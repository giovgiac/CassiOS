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

#include <std/types.hpp>

namespace cassio {
namespace memory {

struct __attribute__((packed)) MultibootInfo {
    std::u32 flags;
    std::u32 mem_lower;
    std::u32 mem_upper;
    std::u32 boot_device;
    std::u32 cmdline;
    std::u32 mods_count;
    std::u32 mods_addr;
    std::u32 syms[4];
    std::u32 mmap_length;
    std::u32 mmap_addr;
    std::u32 drives_length;
    std::u32 drives_addr;
    std::u32 config_table;
    std::u32 boot_loader_name;
    std::u32 apm_table;
    std::u32 vbe_control_info;
    std::u32 vbe_mode_info;
    std::u16 vbe_mode;
    std::u16 vbe_interface_seg;
    std::u16 vbe_interface_off;
    std::u16 vbe_interface_len;
    std::u64 framebuffer_addr;
    std::u32 framebuffer_pitch;
    std::u32 framebuffer_width;
    std::u32 framebuffer_height;
    std::u8 framebuffer_bpp;
    std::u8 framebuffer_type;
};

struct __attribute__((packed)) MultibootMmapEntry {
    std::u32 size;
    std::u64 base_addr;
    std::u64 length;
    std::u32 type;
};

struct __attribute__((packed)) MultibootModule {
    std::u32 mod_start;
    std::u32 mod_end;
    std::u32 string;
    std::u32 reserved;
};

static constexpr std::u32 MULTIBOOT_FLAG_MODS = (1 << 3);
static constexpr std::u32 MULTIBOOT_FLAG_MMAP = (1 << 6);
static constexpr std::u32 MULTIBOOT_FLAG_FRAMEBUFFER = (1 << 12);
static constexpr std::u32 MULTIBOOT_MMAP_AVAILABLE = 1;

} // namespace memory
} // namespace cassio

#endif // MEMORY_MULTIBOOT_HPP_
