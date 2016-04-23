
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "debug.h"

#include "errors.h"
#include "keyboard.h"
#include "keyboardirq.h"
#include "rtc.h"
#include "rtcirq.h"
#include "syscalls.h"
#include "syscallhandle.h"
#include "pitirq.h"

void interruptinit(){
	int i;

// Set idt for exceptions
for(i=0; i<0x20; i++){
	
	idt[i].present = 1;
    idt[i].dpl = 0;
    idt[i].reserved0 = 0;
    idt[i].size = 1;
    idt[i].reserved1 = 1;
    idt[i].reserved2 = 1;
    idt[i].reserved3 = 1;
    idt[i].reserved4 = 0;
    idt[i].seg_selector = KERNEL_CS;
}

// Set idt for interrupts
for(i=0x20; i<0x2F; i++){
	idt[i].present = 1;
    idt[i].dpl = 0;
    idt[i].reserved0 = 0;
    idt[i].size = 1;
    idt[i].reserved1 = 1;
    idt[i].reserved2 = 1;
    idt[i].reserved3 = 0;
    idt[i].reserved4 = 0;
    idt[i].seg_selector = KERNEL_CS;
}

	// Link exception handlers
	SET_IDT_ENTRY(idt[0], dividebyzero);
	SET_IDT_ENTRY(idt[1], debugger);
	SET_IDT_ENTRY(idt[2], nmi);
	SET_IDT_ENTRY(idt[3], breakpoint);
	SET_IDT_ENTRY(idt[4], overflow);
	SET_IDT_ENTRY(idt[5], bounds);
	SET_IDT_ENTRY(idt[6], invalidopcode);
	SET_IDT_ENTRY(idt[7], coprocessornotavailable);
	SET_IDT_ENTRY(idt[8], doublefault);
	SET_IDT_ENTRY(idt[9], coprocessorsegoverrun);
	SET_IDT_ENTRY(idt[10], invalidtask);
	SET_IDT_ENTRY(idt[11], segnotpresent);
	SET_IDT_ENTRY(idt[12], stackfault);
	SET_IDT_ENTRY(idt[13], genprotection);
	SET_IDT_ENTRY(idt[14], pagefault);
	SET_IDT_ENTRY(idt[15], reserved);
	SET_IDT_ENTRY(idt[16], mathfault);
	SET_IDT_ENTRY(idt[17], aligncheck);
	SET_IDT_ENTRY(idt[18], machinecheck);
	SET_IDT_ENTRY(idt[19], simdfloat);

	//Fill the rest with reserved
	for(i=20; i <= 31; i++) {
        SET_IDT_ENTRY(idt[i], reserved);		
    }

    // Set up system call basics
	i=0x80;
    idt[i].present = 1;
    idt[i].dpl = 3;
    idt[i].reserved0 = 0;
    idt[i].size = 1;
    idt[i].reserved1 = 1;
    idt[i].reserved2 = 1;
    idt[i].reserved3 = 1;
    idt[i].reserved4 = 0;
    idt[i].seg_selector = KERNEL_CS; 
	//SET_IDT_ENTRY(idt[0x80], syscall);

	// Link interupt handlers.
	SET_IDT_ENTRY(idt[0x20],pit_wrapper);
	SET_IDT_ENTRY(idt[33],keyboard_wrapper);
	SET_IDT_ENTRY(idt[40],rtc_wrapper);
	SET_IDT_ENTRY(idt[0x80], syscallhandle);

	}
