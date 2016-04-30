#define ASM     1
 
#include "x86_desc.h"
 
 
 
 .global syscallhandle
 .global gotouser

 returnval:
 .long 0
 
syscallhandle:
	pushal

	pushl %edx #arg3
	pushl %ecx #arg2
	pushl %ebx #arg1

	cmpl $1, %eax  #check if valid syscall number
	jl invalid
	cmpl $10, %eax 
	jg invalid
	#cli

	movl sysjump(, %eax,4), %eax
	call *%eax
	jmp end 

invalid:
	movl $-1, %eax

end:
	#sti
	addl $12, %esp #pop args
	movl %eax, returnval

	popal
	movl returnval, %eax
	iret


sysjump:
	.long emptyfunc
	 .long halt 
	 .long execute
  	.long read
  	.long write
  	.long open
  	.long close
  	.long getargs
 	.long vidmap
 	.long set_handler
  	.long sigreturn 


gotouser:
	cli
	// pushl %ebp
 //    movl %esp, %ebp
	movl 4(%esp), %ebx # put eip into ebx

	//segment selectors
	movl $USER_DS, %eax
	
	movw %ax, %fs
	movw %ax, %es
	movw %ax, %ds
	movw %ax, %gs

	
	pushl %eax

	pushl $0x83FFFF0 #push user space esp  ??????????

	pushf #push flags
	popl %eax #flags in eax
	orl $0x4200, %eax # set flags
	pushl %eax
	pushl $USER_CS
	pushl %ebx

	iret
 //addl $8, %esp #pop args



