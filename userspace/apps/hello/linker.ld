/**
 * linker.ld -- hello app linker script
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

ENTRY(_start)
OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386:i386)

SECTIONS {
    . = 0x00400000;

    .text : {
        *(.text*)
        *(.rodata*)
    }

    .data : {
        *(.data)
    }

    .bss : {
        *(.bss)
    }

    /DISCARD/ : {
        *(.fini_array*)
        *(.init_array*)
        *(.comment)
        *(.eh_frame*)
    }
}
