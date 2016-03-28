#define ASM     1




.global keyboard_wrapper
.align 4

keyboard_wrapper:

pushl %eax
pushl %ebx
pushl %ecx
pushl %edx
pushl %esp
pushl %ebp
pushl %esi
pushl %edi


call keyboard_handle

popl %edi
popl %esi
popl %ebp
popl %esp
popl %edx
popl %ecx
popl %ebx
popl %eax
iret
