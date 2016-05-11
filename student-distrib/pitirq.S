#define ASM     1




.global pit_wrapper
//.global saved
.align 4

pit_wrapper:
//movl %esp, saved
pushl %eax
pushl %ebx
pushl %ecx
pushl %edx
pushl %esp
pushl %ebp
pushl %esi
pushl %edi

call pit_handle

popl %edi
popl %esi
popl %ebp
popl %esp
popl %edx
popl %ecx
popl %ebx
popl %eax
iret