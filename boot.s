; boot.s - Multiboot bootloader for NASM

; Multiboot header constants
%define ALIGN   (1 << 0)
%define MEMINFO (1 << 1)
%define FLAGS   (ALIGN | MEMINFO)
%define MAGIC   0x1BADB002
%define CHECKSUM (-(MAGIC + FLAGS))

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

section .bss
align 16
stack_bottom:
    resb 16384          ; 16 KiB stack
stack_top:

section .text
global _start
extern kernel_main      ; defined in kernel.c

_start:
    mov esp, stack_top
    push eax
    push ebx
    call kernel_main
    cli
.hang:
    hlt
    jmp .hang
