# Copyright (c) 2019 Giovanni Giacomo. All Rights Reserved.
# Use of this source code is governed by a MIT-style
# license that can be found in the LICENSE file.

.set IRQ_BASE, 0x20

.section .text
.extern handleInterrupt

.macro HandleException num
.global _ZN6cassio8hardware16InterruptManager19handleException\num\()Ev
_ZN6cassio8hardware16InterruptManager19handleException\num\()Ev:
    movb    $\num, (number)
    jmp     interrupt
.endm

.macro HandleInterruptRequest num
.global _ZN6cassio8hardware16InterruptManager26handleInterruptRequest\num\()Ev
_ZN6cassio8hardware16InterruptManager26handleInterruptRequest\num\()Ev:
    movb    $\num + IRQ_BASE, (number)
    jmp     interrupt
.endm

HandleInterruptRequest  0x00
HandleInterruptRequest  0x01

interrupt:
    pusha
    pushl   %ds
    pushl   %es
    pushl   %fs
    pushl   %gs

    pushl    %esp
    push    (number)
    call    handleInterrupt
    add     %esp, 6
    mov     %eax, %esp

    pop    %gs
    pop    %fs
    pop    %es
    pop    %ds
    popa

.global _ZN6cassio8hardware16InterruptManager22ignoreInterruptRequestEv
_ZN6cassio8hardware16InterruptManager22ignoreInterruptRequestEv:
    iret

.data
    number: .byte 0
