# Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
# Use of this source code is governed by a MIT-style
# license that can be found in the LICENSE file.

.set MAGIC, 0x1BADB002
.set FLAGS, (1 << 0 | 1 << 1 | 1 << 2)
.set CHECKSUM, -(MAGIC + FLAGS)

.set KERNEL_VBASE, 0xC0000000
.set KERNEL_PDE_INDEX, (KERNEL_VBASE >> 22)

.set PAGE_PRESENT,   0x01
.set PAGE_READWRITE, 0x02


.section .multiboot
    .long   MAGIC
    .long   FLAGS
    .long   CHECKSUM

    # Address fields (offsets 12-28). Not used (bit 16 not set),
    # but must be present so video fields land at offset 32.
    .long   0               # header_addr
    .long   0               # load_addr
    .long   0               # load_end_addr
    .long   0               # bss_end_addr
    .long   0               # entry_addr

    # Video mode fields (offsets 32-44). Request linear framebuffer.
    .long   0               # mode_type: 0 = linear framebuffer
    .long   1024            # width
    .long   768             # height
    .long   32              # depth (bpp)


.section .text
.extern ctors
.extern start
.global loader

# Entry point. GRUB jumps here at physical ~0x100000.
# EAX = multiboot magic, EBX = multiboot info pointer (physical).
loader:
    # Save multiboot registers.
    mov     %eax, %esi
    mov     %ebx, %edi

    # Build bootstrap page table: identity-map first 4 MiB.
    # Each PTE maps a 4 KiB page: addr | PRESENT | READWRITE.
    lea     (boot_page_table - KERNEL_VBASE), %ebx
    xor     %ecx, %ecx
    movl    $(PAGE_PRESENT | PAGE_READWRITE), %edx

1:  movl    %edx, (%ebx, %ecx, 4)
    addl    $0x1000, %edx
    incl    %ecx
    cmpl    $1024, %ecx
    jl      1b

    # Set up the page directory:
    # PDE[0]                -> boot_page_table (identity map, low)
    # PDE[KERNEL_PDE_INDEX] -> boot_page_table (high map at 0xC0000000)
    lea     (boot_page_directory - KERNEL_VBASE), %ebx
    lea     (boot_page_table - KERNEL_VBASE), %eax
    orl     $(PAGE_PRESENT | PAGE_READWRITE), %eax

    movl    %eax, (%ebx)
    movl    %eax, (KERNEL_PDE_INDEX * 4)(%ebx)

    # Load page directory into CR3.
    mov     %ebx, %cr3

    # Enable paging (set CR0.PG).
    mov     %cr0, %eax
    orl     $0x80000000, %eax
    mov     %eax, %cr0

    # Jump to high virtual address. This moves EIP from ~0x100xxx to ~0xC01xxxxx.
    lea     higher_half, %eax
    jmp     *%eax

higher_half:
    # Invalidate the identity mapping (PDE[0]).
    lea     boot_page_directory, %ebx
    movl    $0, (%ebx)

    # Flush TLB by reloading CR3.
    mov     %cr3, %eax
    mov     %eax, %cr3

    # Set up the stack (virtual address).
    mov     $stack, %esp

    # Translate multiboot pointer to virtual address.
    addl    $KERNEL_VBASE, %edi

    # Call global constructors.
    call    ctors

    # Call kernel entry point: start(multiboot, magic).
    push    %esi
    push    %edi
    call    start

stop:
    cli
    hlt
    jmp     stop


.section .bss
.align 4096
boot_page_directory:
    .space 4096
boot_page_table:
    .space 4096

.space 2*1024*1024; # 2 MiB
stack:
