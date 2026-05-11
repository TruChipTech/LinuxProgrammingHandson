; asm_add.s — Assembly function callable from C
; Demonstrates C-Assembly interoperability on x86_64 Linux
;
; Calling Convention (System V AMD64 ABI):
;   Arguments: RDI, RSI, RDX, RCX, R8, R9 (then stack)
;   Return value: RAX
;   Caller-saved: RAX, RCX, RDX, RSI, RDI, R8, R9, R10, R11
;   Callee-saved: RBX, RBP, R12-R15
;
; Assemble with: nasm -f elf64 asm_add.s -o asm_add.o

section .text
    global asm_add          ; Make visible to linker

; int asm_add(int a, int b)
; a is in EDI, b is in ESI (lower 32 bits of RDI, RSI)
asm_add:
    mov     eax, edi        ; eax = first argument (a)
    add     eax, esi        ; eax += second argument (b)
    ret                     ; return eax

; TODO Exercise: Implement asm_multiply(int a, int b)
; Uncomment and complete:
;
;   global asm_multiply
; asm_multiply:
;   mov     eax, edi
;   imul    eax, esi        ; signed multiply
;   ret
