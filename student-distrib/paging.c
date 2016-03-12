#include "x86_desc.h"
#include "paging.h"
#include "lib.h"

#define tablesize 1024
#define kb 0x1000
#define viddiv 0xB8
#define ksize 0x400000

int pagedir[tablesize] __attribute__((aligned(kb)));
int pagetable[tablesize] __attribute__((aligned (kb)));
int paddr = (int) pagedir;
int saveflags;
int kpage;


void pageenable(){
cli_and_save(saveflags);
	asm volatile ("				\
		movl %0, %%cr3 \n\
		movl %%cr4, %%eax	\n\
		orl $0x10, %%eax	\n\
		movl %%eax, %%cr4	\n\
		movl %%cr0, %%eax \n\
		orl $0x80000000, %%eax	\n\
		movl %%eax, %%cr0"
		:
		:"r" (paddr)
		:"%eax"
		);
restore_flags(saveflags);
}



void pageinit(){
	int i;




for(i=0; i<tablesize; i++){
pagetable[i]=0;
pagedir[i]=0;

}


pagetable[viddiv]= 0x000B8000;
pagetable[viddiv] |= 0x07;


pagedir[0]= (uint32_t)pagetable;
pagedir[0] |= 0x07;

// printf("pagetable: %x\n", pagetable);

// printf("pagedir[0]: %x \n", pagedir[0]);

// printf("pagetable[0]: %x\n", pagetable[0]);
// printf("pagetable[b8]: %x\n", pagetable[0xb8]);

//4MB kernal page

kpage= 0x400000;
kpage |= 0x8B; //set supervisor, don't allow writes

pagedir[1]=kpage;
	// printf("pagedir[1]: %x \n" ,pagedir[1]);


}


