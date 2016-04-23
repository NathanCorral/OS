
#include "pit.h"

 void pit_init(){
 	outb(MODE3, CMRP);
 	outb(HZ20 & 0xFF, CHANNEL0);
 	outb(HZ20>>8, CHANNEL0);
 	enable_irq(0);
 }

void pit_handle(){
	// cli();
	send_eoi(0);

	// uint8_t running=nowrunning();
	// uint8_t current=getcurrent();

	// uint8_t next= (current+1)%8;
	// uint8_t bitmask= 0x80;
	// int i;
	// uint32_t pdaddr;
	// pcb_t * pcb;

	// for(i=0; i<next; i++){
	// 	bitmask=bitmask >>1;
	// }

	// while(current != next){

	// 	if(bitmask &running){
	// 		pcb= (pcb_t *) (MB8-KB8*(next+1));

	// 		if(pcb->haschild==0)
	// 			break;
	// 	}
	// 	next= (next+1)%8;
	// 	bitmask= bitmask >>1;
	// 	if (bitmask==0)
	// 		bitmask= 0x80;
	// }

	// if(current== next)
	// 	return;
	// pcb= (pcb_t *) (MB8-KB8*(current+1));

	// asm volatile("movl %%esp, %0":"=g"(pcb->oldesp));
	// asm volatile("movl %%ebp, %0":"=g"(pcb->oldebp));

	// current=next;
	// setcurrent(current);
	// pdaddr= getaddr(pcb->process);
	// setpdaddr(pdaddr);

	// asm volatile ("				\
	// 	movl %0, %%cr3 \n\
	// 	movl %%cr4, %%eax	\n\
	// 	orl $0x90, %%eax	\n\
	// 	movl %%eax, %%cr4	\n\
	// 	movl %%cr0, %%eax \n\
	// 	orl $0x80000000, %%eax	\n\
	// 	movl %%eax, %%cr0"
	// 	:
	// 	:"r" (pdaddr)
	// 	:"%eax"
	// 	);

	// tss.esp0= MB8-KB8*(next+1)-4;
	// setkstack(tss.esp0);

	// pcb= (pcb_t *)(MB8-KB8*(next+1));

	// asm volatile ("movl %0, %%ebp     ;"
	// 	"movl %1, %%esp     ;"
	// 	"leave;"
	// 	"ret;"
	// 	::"g"(pcb->oldebp), "g"(pcb->oldesp));
	return;
}

