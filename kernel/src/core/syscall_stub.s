# Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
# Use of this source code is governed by a MIT-style
# license that can be found in the LICENSE file.

.section .text
.extern handleSyscall

.global syscallEntry
syscallEntry:
    pusha
    pushl   %ds
    pushl   %es
    pushl   %fs
    pushl   %gs

    # Pass syscall arguments: EAX (number), EBX, ECX, EDX.
    # EAX is at offset 28 in the pusha frame (last pushed = EDI at top).
    # pusha order: EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI
    # So on stack (low to high): EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX
    # After pushl ds/es/fs/gs (4*4=16 bytes), offsets from ESP:
    #   gs=0, fs=4, es=8, ds=12, EDI=16, ESI=20, EBP=24, ESP=28,
    #   EBX=32, EDX=36, ECX=40, EAX=44

    pushl   %edx
    pushl   %ecx
    pushl   %ebx
    pushl   %eax
    call    handleSyscall
    addl    $16, %esp

    # Store return value into EAX's slot in the pusha frame so popa
    # restores it to the caller's EAX.
    movl    %eax, 44(%esp)

    pop     %gs
    pop     %fs
    pop     %es
    pop     %ds
    popa
    iret
