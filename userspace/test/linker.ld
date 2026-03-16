/**
 * linker.ld -- userspace test linker script
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
        start_ctors = .;
        KEEP(*(.ctors*))
        KEEP(*(.init_array*))
        end_ctors = .;
        *(.data)
    }

    .bss : {
        *(.bss)
    }

    /DISCARD/ : {
        *(.fini_array*)
        *(.comment)
        *(.eh_frame*)
    }
}
