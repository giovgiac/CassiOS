/**
 * linker.ld
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

ENTRY(loader)
OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386:i386)

KERNEL_VBASE = 0xC0000000;
KERNEL_LMA   = 0x0100000;
KERNEL_VMA   = KERNEL_VBASE + KERNEL_LMA;

SECTIONS {
    . = KERNEL_VMA;
    _kernel_start = .;
    .text : AT(ADDR(.text) - KERNEL_VBASE)
    {
        KEEP(*(.multiboot))
        *(.text*)
        *(.rodata*)
    }

    .data : AT(ADDR(.data) - KERNEL_VBASE)
    {
        *(.data*)
        *(.got*)
    }

    .bss : AT(ADDR(.bss) - KERNEL_VBASE)
    {
        *(.bss*)
        *(COMMON)
    }
    _kernel_end = .;

    /* Nothing unwinds: the kernel is built with panic = "abort". */
    /DISCARD/ :
    {
        *(.fini_array*)
        *(.comment)
        *(.eh_frame*)
    }
}

/* loader.s maps only the first 4 MiB before paging is set up. */
ASSERT(_kernel_end - KERNEL_VBASE <= 0x400000, "kernel does not fit the 4 MiB boot mapping")
