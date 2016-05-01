#include "pit.h"
#include "x86_desc.h"
#include "lib.h"

 void pit_init(){
 	outb(MODE3, CMRP);
 	outb(HZ20 & 0xFF, CHANNEL0);
 	outb(HZ20>>8, CHANNEL0);
 	enable_irq(0);
 }

// void pit_handle(){
// 	cli();
// 	send_eoi(0);

// //int old=active;
// active++;
// if (active==3)
// 	active=0;
// // if(newarray[active]==0){
// // 	save_this_terminal( old ,active, stdin);
// // 	switchterm(active, 0);}
// // //if(newarray[active])
// // else{
	
// // }
//  if(newarray[active]==1)
// switchterm(active, 1);
// }

void pit_handle(){
	cli();
	// if(temp != tss.eip){
	// 	temp = tss.eip;
	// 	printf("EIP 0x%#x\n", temp);
	// }
	// printf("EIP 0x%#x\n", tss.eip);


	send_eoi(0);
	// help_debug();
	switch_to(NULL);
	//sti();
	return;

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
	// 	if(bitmask &running ){
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

	// // printf("Current = 0x%#x,  next = %#x\n",current, next);

	// pcb= (pcb_t *) (MB8-KB8*(current+1));

	// asm volatile("movl %%esp, %0":"=g"(pcb->oldesp));
	// pcb->oldesp0=tss.esp0;
	// asm volatile("movl %%ebp, %0":"=g"(pcb->oldebp));
	// pcb->oldss0=tss.ss0;
	// current=next;
	// setcurrent(current);

	// pcb= (pcb_t *)(MB8-KB8*(next+1));

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
	// return;
	// tss.esp0= MB8-KB8*(next+1)-4;
	// tss.ss0=pcb->oldss0;
	// tss.esp0= pcb->oldesp0;
	// setkstack(tss.esp0);

	
	// asm volatile ("movl %0, %%ebp     ;"
	// 	"movl %1, %%esp     ;"
	// 	::"g"(pcb->oldebp), "g"(pcb->oldesp));
	// return;
}


