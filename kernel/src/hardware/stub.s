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
    pushl   $\num
    jmp     exception
.endm

# Exceptions that push a hardware error code onto the stack.
.macro HandleExceptionWithError num
.global _ZN6cassio8hardware16ExceptionHandler19handleException\num\()Ev
_ZN6cassio8hardware16ExceptionHandler19handleException\num\()Ev:
    pushl   $\num
    jmp     exception
.endm

# IRQ stubs -- route to IrqManager.
.macro HandleInterruptRequest num
.global _ZN6cassio8hardware10IrqManager26handleInterruptRequest\num\()Ev
_ZN6cassio8hardware10IrqManager26handleInterruptRequest\num\()Ev:
    pushl   $0
    pushl   $(\num + IRQ_BASE)
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

# Stack layout after pusha + segment pushes (offsets from ESP):
#   +0:  gs  +4:  fs  +8:  es  +12: ds
#   +16: edi +20: esi +24: ebp +28: esp(pusha)
#   +32: ebx +36: edx +40: ecx +44: eax
#   +48: number  +52: error_code
#   +56: eip  +60: cs  +64: eflags  [+68: user_esp  +72: user_ss]

exception:
    pusha
    pushl   %ds
    pushl   %es
    pushl   %fs
    pushl   %gs

    pushl   %esp            # arg3: saved state pointer
    pushl   56(%esp)        # arg2: error_code (52 + 4 for pushl %esp)
    pushl   56(%esp)        # arg1: number (48 + 8 for two pushes above)
    call    handleException
    addl    $12, %esp
    mov     %eax, %esp

    pop     %gs
    pop     %fs
    pop     %es
    pop     %ds
    popa
    addl    $8, %esp        # skip number + error_code
    iret

interrupt:
    pusha
    pushl   %ds
    pushl   %es
    pushl   %fs
    pushl   %gs

    pushl    %esp           # arg2: saved state pointer
    pushl   52(%esp)        # arg1: number (48 + 4 for pushl %esp)
    call    handleInterrupt
    add     $8, %esp
    mov     %eax, %esp

    pop    %gs
    pop    %fs
    pop    %es
    pop    %ds
    popa
    addl    $8, %esp        # skip number + error_code
    iret

.global _ZN6cassio8hardware16InterruptManager22ignoreInterruptRequestEv
_ZN6cassio8hardware16InterruptManager22ignoreInterruptRequestEv:
    iret
