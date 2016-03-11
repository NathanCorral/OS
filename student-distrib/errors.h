#ifndef _ERRORS_H
#define _ERRORS_H

#include "lib.h"

void dividebyzero();
void debugger();
void nmi();
void breakpoint();
void overflow();
void bounds();
void invalidopcode();
void coprocessornotavailable();
void doublefault();
void coprocessorsegoverrun();
void invalidtask();
void segnotpresent();
void stackfault();
void genprotection();
void pagefault();
void reserved();
void mathfault();
void aligncheck();
void machinecheck();
void simdfloat();
void syscall(); //temporary until full handler is done

#endif 
