


#include "errors.h"
#include "syscalls.h"
#include "lib.h"
//print name of exception for each exception
void dividebyzero(){
	cli();
	printf("divide by zero error \n");
	halt(-1);
}
void debugger(){
	cli();
	printf("debugger \n");
	halt(-1);
}
void nmi(){
	cli();
	printf("nmi \n");
	halt(-1);
}
void breakpoint(){
	cli();
	printf("breakpoint \n");
	halt(-1);
}
void overflow(){
	cli();
	printf("overflow \n");
	halt(-1);
}
void bounds(){
	cli();
	printf("bounds \n");
	halt(-1);
}
void invalidopcode(){
	cli();
	printf("invalid opcode \n");
	halt(-1);
}
void coprocessornotavailable(){
	cli();
	printf("coprocessor not available \n");
	halt(-1);
}
void doublefault(){
	cli();
	printf("double fault \n");
	halt(-1);
}
void coprocessorsegoverrun(){
	cli();
	printf("coprocessor segment overrun \n");
	halt(-1);
}
void invalidtask(){
	cli();
	printf("invalid task state segment \n");
	halt(-1);
}
void segnotpresent(){
	cli();
	printf("segment not present \n");
	halt(-1);
}
void stackfault(){
	cli();
	printf("stack fault \n");
	halt(-1);
}
void genprotection(){
	cli();
	printf("gerneral protection fault \n");
	int var=0;

	asm volatile("movl %%cr2, %0  ;":"=r"(var));
	printf(" %x \n", var);
	halt(-1);
}
void pagefault(){
	cli();
	printf("page fault \n");
	int var=0;

	asm volatile("movl %%cr2, %0  ;":"=r"(var));
	printf(" %x \n", var);
	while(1);
	//halt(-1);
}
void reserved(){
	cli();
	printf("reserved \n");
	halt(-1);
}
void mathfault(){
	cli();
	printf("math fault \n");
	halt(-1);
}
void aligncheck(){
	cli();
	printf("alignment check \n");
	halt(-1);
}
void machinecheck(){
	cli();
	printf("machine check \n");
	halt(-1);
}
void simdfloat(){
	cli();
	printf("simd floating point exception \n");
	halt(-1);
}

void syscall(){
	cli();
	printf("system call \n");
	halt(-1);
}


