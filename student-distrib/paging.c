#include "x86_desc.h"
#include "paging.h"
#include "lib.h"
#include "types.h"



#define progimg 0x20

#define useridx 0x1
#define uservideo 0x1000

#define viddiv 0xB8
#define video 0xB8000

#define readshift 1
#define useshift 2
#define writetshift 3
#define cacheshift 4
#define accessshift 5
#define dirtyshift 6
#define sizeshift 7
#define globalshift 8

//int pagedir[tablesize] __attribute__((aligned(kb))); //page directory
 dir_t dir[8] __attribute__((aligned(kb)));
int pagetable[tablesize] __attribute__((aligned (kb))); //page table
int newtable[tablesize] __attribute__((aligned (kb)));
int paddr = (int) dir[0].pagedir; //address of page directory
int saveflags; //for the purpose of saving flags
int kpage; //holds info for kernal page


uint32_t newpagedir;
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
dir[0].pagedir[i]=0;

}

//set flags to allow read/write,  not allowing user access, declaring present
tp=1;
tr=1;
tu=0;
tflags= (tp | (tr << readshift) | (tu << useshift) |(tw << writetshift) | (tc << cacheshift) | (ta <<accessshift) | (td <<dirtyshift) | (tg << globalshift));

//put video memory into table
pagetable[viddiv]= video;
pagetable[viddiv] |= tflags;


//allow user access to video

tp=1;
tr=1;
tu=1;
tflags= (tp | (tr << readshift) | (tu << useshift) |(tw << writetshift) | (tc << cacheshift) | (ta <<accessshift) | (td <<dirtyshift) | (tg << globalshift));

//put video memory into table
pagetable[useridx]= video;
pagetable[useridx] |= tflags;


//declare present, allow read/write, allow user
dp=1;
dr=1;
du=1;
dflags=(dp | (dr << readshift) | (du << useshift) |(dw << writetshift) | (dd << cacheshift) | (da <<accessshift) | (ds <<sizeshift) | (dg << globalshift));
//put page table into page directory
dir[0].pagedir[0]= (uint32_t)pagetable;
dir[0].pagedir[0] |= dflags;


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

dir[0].pagedir[1]=kpage;



}


int32_t newtask( uint8_t process){

uint32_t i;
if( process>=8)
	return -1;

int tg=0;
int td=0;
int ta=0;
int tc=0;
int tw=0;
int tu=0;
int tr=0;
int tp=0;
int tflags=0;


newpagedir=(uint32_t)(&dir[process]);
for(i=0; i<tablesize; i++){
	if(i==0)
		tp=0;
	else
		tp=1;

	tr=1;
	tu=1;
	tg=1;


	tflags= (tp | (tr << readshift) | (tu << useshift) |(tw << writetshift) | (tc << cacheshift) | (ta <<accessshift) | (td <<dirtyshift) | (tg << globalshift));

newtable[i]= i* kb;
newtable[i] |= tflags;
}
//printf("newtable addr:%x\n",newtable);
//printf("newtable entry:%x\n",newtable[2]);

int dg=0;
int ds=0;
int da=0;
int dd=0;
int dw=0;
int du=0;
int dr=0;
int dp=0;

int dflags=0;

dp=1;
dr=1;
du=1;

dflags=(dp | (dr << readshift) | (du << useshift) |(dw << writetshift) | (dd << cacheshift) | (da <<accessshift) | (ds <<sizeshift) | (dg << globalshift));

dir[process].pagedir[0]= (uint32_t)newtable;
dir[process].pagedir[0] |= dflags;
//printf("process, dir entry:%d, %x\n",process,dir[process].pagedir[0]);



dp=1;
dr=1;
du=0;
ds=1;
dg=1;

dflags=(dp | (dr << readshift) | (du << useshift) |(dw << writetshift) | (dd << cacheshift) | (da <<accessshift) | (ds <<sizeshift) | (dg << globalshift));


kpage= ksize;
kpage |= dflags;
dir[process].pagedir[1]=kpage;
//printf("kpage:%x\n",dir[process].pagedir[1]);


dp=1;
dr=1;
du=1;
ds=1;
dg=0;

dflags=(dp | (dr << readshift) | (du << useshift) |(dw << writetshift) | (dd << cacheshift) | (da <<accessshift) | (ds <<sizeshift) | (dg << globalshift));

dir[process].pagedir[progimg]= (process+2)*ksize;
dir[process].pagedir[progimg] |= dflags;

//printf("0x20: %x\n",dir[process].pagedir[0x20] );


asm volatile ("				\
		movl %0, %%cr3 \n\
		movl %%cr4, %%eax	\n\
		orl $0x90, %%eax	\n\
		movl %%eax, %%cr4	\n\
		movl %%cr0, %%eax \n\
		orl $0x80000000, %%eax	\n\
		movl %%eax, %%cr0"
		:
		:"r" (newpagedir)
		:"%eax"
		);

		return 0;
}

uint32_t getaddr(uint8_t process){
	return (uint32_t) &dir[process];
}



/*
#include "paging.h"


#define tablesize 1024
#define kb 0x1000
#define viddiv 0xB8
#define ksize 0x00400000
#define KERNEL 0x00400000
#define VIDEO 0x0B8000

#define TEST_PAGING 1



uint32_t pagedir[tablesize] __attribute__((aligned(kb)));

// Video Pagetable
uint32_t pagetable[tablesize] __attribute__((aligned (kb)));

// Memory bit map of full 4GB with the 32nd 4 bytes being a bit map for the [0:31] 4 bytes
uint32_t mm[33];

// Number of prgorams
int programs;

// Queue that keeps list of quickly accessable unused physical addresses
queue_t quicklist;



uint32_t paddr = (uint32_t) pagedir;
int saveflags;
int kpage;



void pageinit(){
	int i;

	queue_init(quicklist);

#ifdef TEST_PAGING
	int * test = (int *) 0x01800000;
	for(i=0; i<15; i++){
		test[i] = i*5;
	}
#endif

	// Initilize the number of programs
	programs = 0;

	for(i=0; i<tablesize; i++){
		pagedir[i] = 0;
		pagetable[i] = 0;
	}

	// Initialize memory map
	// All pages are available
	for(i=0; i<33; i++){
		mm[i] = 0;
	}

	pagetable[viddiv] |= (VIDEO & ADDR_ENTRY);
	pagetable[viddiv] |= SET_P | SET_R | SET_U;

	pagedir[0] |= ((uint32_t) pagetable & ADDR_ENTRY);
	pagedir[0] |= SET_P | SET_R | SET_U;
	// Set the first entry in the mm to in use
	mm[0] |= (1);

	pagedir[1] |= (KERNEL & ADDR_ENTRY);
	pagedir[1] |= SET_P | SET_R | SET_W | SET_S;
	// Set the second entry in the mm to in use
	mm[0] |= (1 << 1);

	//printf("We are here\n");
	//printf("video page dir:   0x%#x\n", pagedir[0]);
	//printf("video tab dir:   0x%#x\n", pagetable[viddiv]);
	//printf("kernel page dir:   0x%#x\n", pagedir[1]);
	//while(1);

	// Set up quicklist of available 4MB pages
	i = 2;
	while(!queue_full(quicklist)){
		queue_push(quicklist, i);
		i++;
	}


	pageenable();
}

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

// In progress
void alloc_prog(){

}



// Translate a linear addres to a physical one
uint32_t lin_to_phys(uint32_t lin_addr){
	//lin_addr_t lin_addr = (lin_addr_t) addr;
	uint32_t pde = pagedir[lin_addr >> DIRECTORY_OFFSET];
	if((pde & SET_P) == 0)
		return -1;

	// Large Page
	if((pde & SET_S) == 0){
		uint32_t * pte = (uint32_t *) (pde & ADDR_ENTRY);
		return (pte[(lin_addr & TABLE) >> TABLE_OFFSET] & ADDR_ENTRY) + (lin_addr & OFFSET);
	}

	return (pde & ADDR_ENTRY) + (lin_addr & OFFSET_E);
}



// Inputs:  
//		flags:  flags of the page
//		vir_addr:  virtual address of the page.  
// Return the physical address to the 4MB allocated page
uint32_t allocate_big_page(uint32_t flags, uint32_t vir_addr){
	if(vir_addr == 0)
		return -1;

	uint32_t phys_addr;
	uint32_t p2;
	phys_addr = get_any_page();
	if(phys_addr < 0)
		return -1;
	map_epage(flags, phys_addr, vir_addr);
	p2 = get_any_page();
	if(p2 < 0)
		return -1;
	map_epage(flags, p2, 0x0700000);
	
	return phys_addr;
}

// Deallocate the memory area of phys_addr
// Adds the memory space to the quicklist queue for reuse
void deallocate_big_page(uint32_t phys_addr){
	uint32_t phys_idx = phys_addr >> DIRECTORY_OFFSET;
	uint32_t mm_off_idx = phys_idx & (0x01F); // Last Five bits for 32 bit index
	uint32_t mm_idx = phys_idx >> 5;	// First 5 bits
	// Mark entry as unused
	mm[mm_idx] &= ~(1 << mm_off_idx);
	// Check if we should update the mm index
	// Idealy the last index is the last one allocated
	if(mm_off_idx == 31){
		mm[32] &= ~(1 << mm_idx);
	}

	if(!queue_full(quicklist))
		queue_push(quicklist, (int) phys_idx);
}

// Map a 4MB page to a virtual address
void map_epage(uint32_t flags, uint32_t phys_addr, uint32_t vir_addr){
	uint32_t page_dir = (vir_addr >> DIRECTORY_OFFSET);
	pagedir[page_dir] = (phys_addr & ADDR_ENTRY);
	pagedir[page_dir] |= (flags & FLAG_ENTRY);
}

// Returns the physical address of an unallocated area
uint32_t get_any_page(){
	if(!queue_empty(quicklist)){
		uint32_t phys_idx = (uint32_t) queue_pop(quicklist);
		uint32_t mm_off_idx = phys_idx & (0x01F); // Last Five bits for 32 bit index
		uint32_t mm_idx = phys_idx >> 5;	// First 5 bits
		// Check if page is already allocated somehow
		if((mm[mm_idx] & (1 << mm_off_idx))){
			printf("Address space already allocated \n");
			return -1;
		}
		// Mark entry as used
		mm[mm_idx] |= (1 << mm_off_idx);
		// Check if we should update the mm index
		// Idealy the last index is the last one allocated
		if(mm_off_idx == 31){
			mm[32] |= (1 << mm_idx);
		}
		// Align address to 4 MB
		return (phys_idx << DIRECTORY_OFFSET);
	}

	// Get pages slowly by looking through mm
	int mm_idx, mm_offset;
	uint32_t mask_off = 1, mask_idx = 1;
	for(mm_idx=0; mm_idx < 32; mm_idx++, mask_idx <<= 1){
		if(!(mm[32] & mask_idx))
			break;
	}
	// Check if no more memory is available
	if(mm_idx == 32)
		return -1;

	for(mm_offset=0; mm_offset < 32; mm_offset++, mask_off <<=1){
		if(!(mm[mm_idx] & mask_off)){
			mm[mm_idx] &= ~mask_off;
			break;
		}
	}
	if(mm_offset == 32)
		return -1;

	if(mm_offset == 31)
		mm[32] |= (1 << mm_idx);

	mm[mm_idx] |= (1 << mm_offset);
	return ((32*mm_idx + mm_offset) << DIRECTORY_OFFSET);
}


// Important
#ifdef TEST_PAGING
// These test are used to manipulate allocation and deallocation of memory and do not unallocate
// several allocated pages.
void paging_test(){
	int * test = (int *) 0x01800000;
	int i, pop;
	uint32_t first, second, third, fourth, fifth;
	// We are writing the the memory address that fifth will become
	// we then map this space to the vir_addr and read from it after
	// we turn paging on.
	
	uint32_t flags = 0;
	uint32_t vir_addr = 0x80000000;
	flags |= (SET_P) | SET_R | SET_U | SET_S;

	printf("__Paging Test__\n");
	printf("Cr3 = 0x%#x\n", pagedir);
	printf("Kernel page dir[1]:   0x%#x\n", pagedir[1]);
	printf("end of table = 0x%#x\n", &(pagedir[tablesize-1]));
	
	
	printf("Empty quicklist test\n");
	while(!queue_empty(quicklist)){
		pop = queue_pop(quicklist);
		printf("%d  ", pop);
	}
	printf("\n");
	*/

/*
	printf("MM Test\n");
	print_mm(2);
	first = allocate_big_page(flags, vir_addr);
	second = allocate_big_page(flags, vir_addr);
	deallocate_big_page(first);
	third = allocate_big_page(flags, vir_addr);
	deallocate_big_page(second);
	fourth = allocate_big_page(flags, vir_addr);
	deallocate_big_page(fourth);
	deallocate_big_page(third);
	fifth = allocate_big_page(flags, vir_addr);

	// first and third are the same
	// second and fourth are the same
	printf("First:   0x%#x\n", first);
	printf("Sec:     0x%#x\n", second);
	printf("Third:   0x%#x\n", third);
	printf("Fourth:  0x%#x\n", fourth);
	printf("Fifth:   0x%#x\n", fifth);
	printf("NEW PAGE ADDR:  0x%#x\n", allocate_big_page(flags, vir_addr));
	printf("NEW PAGE ADDR:  0x%#x\n", allocate_big_page(flags, vir_addr));
	printf("NEW PAGE ADDR:  0x%#x\n", allocate_big_page(flags, vir_addr));
	printf("NEW PAGE ADDR:  0x%#x\n", allocate_big_page(flags, vir_addr));
	printf("NEW PAGE ADDR:  0x%#x\n", allocate_big_page(flags, vir_addr));
	// Note how the dealocated physical addresses will be 0 on the mem map
	print_mm(2);

	// This test shows mapping of the physical address (0x01800000), writen to ealier, to
	// the virtual address used with program execution (0x80000000).
	map_epage(flags, test, vir_addr);
	printf("Memory read test from phys addr 0x%#x\n", (uint32_t) test);
	for(i=0; i<15; i++){
		printf("0x%#x ", ((int *) vir_addr)[i]);
	}	
}

#endif


// print out entry number of indexs into the mm
// also prints out the directory for the indexs
void print_mm(int entries){
	int i,j;
	uint32_t mask;
	for(i=0;i<entries;i++){
		mask = 1;
		for(j=0; j<32; j++, mask<<=1){
			printf("%d ", ((mm[i] & mask) >> j));
		}
		printf("\n");
	}
	mask = 1;
	printf("Directory:\n");
	for(j=0; j<32; j++, mask<<=1){
		printf("%d ", ((mm[32] & mask) >> j));
	}
	printf("\n");
}
*/
