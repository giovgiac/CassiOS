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
        *(.multiboot)
        *(.text*)
        *(.rodata)
    }

    .data : AT(ADDR(.data) - KERNEL_VBASE)
    {
        start_ctors = .;
        KEEP(*(.init_array));
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*)));
        end_ctors = .;

        *(.data)
    }

    .bss : AT(ADDR(.bss) - KERNEL_VBASE)
    {
        *(.bss)
    }
    _kernel_end = .;

    /DISCARD/ :
    {
        *(.fini_array*)
        *(.comment)
    }
}
