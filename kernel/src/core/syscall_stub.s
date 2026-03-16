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

    # Pass the saved-frame ESP to the C handler.
    pushl   %esp
    call    handleSyscall
    addl    $4, %esp

    # The handler returns the ESP to restore. This may be a different
    # process's stack if the caller blocked (IPC send/receive).
    movl    %eax, %esp

    pop     %gs
    pop     %fs
    pop     %es
    pop     %ds
    popa
    iret
