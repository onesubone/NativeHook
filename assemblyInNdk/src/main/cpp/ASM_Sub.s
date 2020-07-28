; 这是一个汇编文件，for x86_64
; YASM的注释风格使用分号形式

global ASM_Sub

section .text

ASM_Sub:

    sub     edi, esi
    mov     eax, edi
    ret