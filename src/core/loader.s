# Copyright (c) 2019 Giovanni Giacomo. All Rights Reserved.
# Use of this source code is governed by a MIT-style
# license that can be found in the LICENSE file.

.set MAGIC, 0x1BADB002
.set FLAGS, (1 << 0 | 1 << 1)
.set CHECKSUM, -(MAGIC + FLAGS)


.section .multiboot
    .long   MAGIC
    .long   FLAGS
    .long   CHECKSUM


.section .text
.extern ctors
.extern start
.global loader

loader:
    mov     $stack, %esp
    call    ctors

    push    %eax
    push    %ebx
    call    start

stop:
    cli
    hlt
    jmp     stop


.section .bss
.space 2*1024*1024; # 2 MiB
stack:
