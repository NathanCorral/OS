#include "x86_desc.h"
#include "paging.h"
#include "lib.h"

#define tablesize 1024
#define kb 0x1000
#define viddiv 0xB8
#define video 0xB8000
#define ksize 0x400000
#define readshift 1
#define useshift 2
#define writetshift 3
#define cacheshift 4
#define accessshift 5
#define dirtyshift 6
#define sizeshift 7
#define globalshift 8

int pagedir[tablesize] __attribute__((aligned(kb))); //page directory
int pagetable[tablesize] __attribute__((aligned (kb))); //page table
int paddr = (int) pagedir; //address of page directory
int saveflags; //for the purpose of saving flags
int kpage; //holds info for kernal page

//enable by setting pse bit in cr4, putting directory address into cr3
//enabled paging in cr0
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


//sets up intitial page directory and table
void pageinit(){
	int i;
//flag bits for pde and pte
int dg=0;
int ds=0;
int da=0;
int dd=0;
int dw=0;
int du=0;
int dr=0;
int dp=0;

int dflags=0;

int tg=0;
int td=0;
int ta=0;
int tc=0;
int tw=0;
int tu=0;
int tr=0;
int tp=0;

int tflags=0;

//initialize table to all 0s
for(i=0; i<tablesize; i++){
pagetable[i]=0;
pagedir[i]=0;

}

//set flags to allow read/write, allowing user access, declaring present
tp=1;
tr=1;
tu=1;
tflags= (tp | (tr << readshift) | (tu << useshift) |(tw << writetshift) | (tc << cacheshift) | (ta <<accessshift) | (td <<dirtyshift) | (tg << globalshift));

//put video memory into table
pagetable[viddiv]= video;
pagetable[viddiv] |= tflags;


//declare present, allow read/write, allow user
dp=1;
dr=1;
du=1;
dflags=(dp | (dr << readshift) | (du << useshift) |(dw << writetshift) | (dd << cacheshift) | (da <<accessshift) | (ds <<sizeshift) | (dg << globalshift));
//put page table into page directory
pagedir[0]= (uint32_t)pagetable;
pagedir[0] |= dflags;


//4MB kernal page
//set size for 4MB, enable write through, allow read/write, declare present
 dg=0;
 ds=1;
 da=0;
 dd=0;
 dw=1;
 du=0; //don't allow user access
 dr=1;
 dp=1;

dflags=(dp | (dr << readshift) | (du << useshift) |(dw << writetshift) | (dd << cacheshift) | (da <<accessshift) | (ds <<sizeshift) | (dg << globalshift));


//set kernal page in directory
kpage= ksize;
kpage |= dflags; 

pagedir[1]=kpage;



}


