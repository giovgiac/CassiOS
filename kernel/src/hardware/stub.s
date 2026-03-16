# Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
# Use of this source code is governed by a MIT-style
# license that can be found in the LICENSE file.

.set IRQ_BASE, 0x20

.section .text
.extern handleInterrupt
.extern handleException

# Exception stubs -- route to ExceptionHandler.
# Exceptions without error codes push a dummy 0 for uniform stack layout.
.macro HandleException num
.global _ZN6cassio8hardware16ExceptionHandler19handleException\num\()Ev
_ZN6cassio8hardware16ExceptionHandler19handleException\num\()Ev:
    pushl   $0
    movb    $\num, (number)
    jmp     exception
.endm

# Exceptions that push a hardware error code onto the stack.
.macro HandleExceptionWithError num
.global _ZN6cassio8hardware16ExceptionHandler19handleException\num\()Ev
_ZN6cassio8hardware16ExceptionHandler19handleException\num\()Ev:
    movb    $\num, (number)
    jmp     exception
.endm

# IRQ stubs -- route to IrqManager.
.macro HandleInterruptRequest num
.global _ZN6cassio8hardware10IrqManager26handleInterruptRequest\num\()Ev
_ZN6cassio8hardware10IrqManager26handleInterruptRequest\num\()Ev:
    movb    $\num + IRQ_BASE, (number)
    jmp     interrupt
.endm

HandleException             0x00
HandleException             0x06
HandleExceptionWithError    0x0D
HandleExceptionWithError    0x0E

HandleInterruptRequest  0x00
HandleInterruptRequest  0x01
HandleInterruptRequest  0x0C
HandleInterruptRequest  0x0E

exception:
    popl    (error_code)

    pusha
    pushl   %ds
    pushl   %es
    pushl   %fs
    pushl   %gs

    pushl   %esp
    pushl   (error_code)
    push    (number)
    call    handleException
    addl    $12, %esp
    mov     %eax, %esp

    pop     %gs
    pop     %fs
    pop     %es
    pop     %ds
    popa
    iret

interrupt:
    pusha
    pushl   %ds
    pushl   %es
    pushl   %fs
    pushl   %gs

    pushl    %esp
    push    (number)
    call    handleInterrupt
    add     $8, %esp
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
    number:     .byte 0
    error_code: .long 0
