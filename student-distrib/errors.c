


#include "errors.h"
#include "lib.h"
//print name of exception for each exception
void dividebyzero(){
	cli();
	printf("divide by zero error \n");
	while(1);
}
void debugger(){
	cli();
	printf("debugger \n");
	while(1);
}
void nmi(){
	cli();
	printf("nmi \n");
	while(1);
}
void breakpoint(){
	cli();
	printf("breakpoint \n");
	while(1);
}
void overflow(){
	cli();
	printf("overflow \n");
	while(1);
}
void bounds(){
	cli();
	printf("bounds \n");
	while(1);
}
void invalidopcode(){
	cli();
	printf("invalid opcode \n");
	while(1);
}
void coprocessornotavailable(){
	cli();
	printf("coprocessor not available \n");
	while(1);
}
void doublefault(){
	cli();
	printf("double fault \n");
	while(1);
}
void coprocessorsegoverrun(){
	cli();
	printf("coprocessor segment overrun \n");
	while(1);
}
void invalidtask(){
	cli();
	printf("invalid task state segment \n");
	while(1);
}
void segnotpresent(){
	cli();
	printf("segment not present \n");
	while(1);
}
void stackfault(){
	cli();
	printf("stack fault \n");
	while(1);
}
void genprotection(){
	cli();
	printf("gerneral protection fault \n");
	while(1);
}
void pagefault(){
	cli();
	printf("page fault \n");
	while(1);
}
void reserved(){
	cli();
	printf("reserved \n");
	while(1);
}
void mathfault(){
	cli();
	printf("math fault \n");
	while(1);
}
void aligncheck(){
	cli();
	printf("alignment check \n");
	while(1);
}
void machinecheck(){
	cli();
	printf("machine check \n");
	while(1);
}
void simdfloat(){
	cli();
	printf("simd floating point exception \n");
	while(1);
}

void syscall(){
	cli();
	printf("system call \n");
	while(1);
}


