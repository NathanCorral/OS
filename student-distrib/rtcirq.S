#define ASM     1




.global rtc_wrapper
.align 4

rtc_wrapper:

pushl %eax
pushl %ebx
pushl %ecx
pushl %edx
pushl %esp
pushl %ebp
pushl %esi
pushl %edi

cld
call rtc_handle



popl %edi
popl %esi
popl %ebp
popl %esp
popl %edx
popl %ecx
popl %ebx
popl %eax
iret
