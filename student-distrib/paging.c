#include "paging.h"
#include "lib.h"
#include "keyboard.h"
#include "x86_desc.h"
#include "files.h"
#include "errors.h"



#define tablesize 1024
#define kb 0x1000
#define viddiv 0xB8
#define ksize 0x00400000


// <<<<<<< .mine
// // Inputs:  
// //		flags:  flags of the page
// //		vir_addr:  virtual address of the page.  
// // Return the physical address to the 4MB allocated page
// uint32_t allocate_big_page(uint32_t flags, uint32_t vir_addr){
// =======
#define TEST_PAGING 1
// >>>>>>> .r15498

// <<<<<<< .mine
// 	uint32_t phys_addr;
// 	phys_addr = get_any_page();
// 	if(phys_addr < 0)
// 		return -1;

// 	// If we are handed NULL vir_addr then let the requester do the mapping
// 	if(vir_addr != NULL)
// 		map_epage(flags, phys_addr, vir_addr);
// =======


//uint32_t pagedir[tablesize] __attribute__((aligned(kb)));

// Video Pagetable
//uint32_t pagetable[tablesize] __attribute__((aligned (kb)));

// Map of 4GB physical memory in 4MB pages
mem_map_t phys_mem;

// mem_map_t * kernel_pagedir;
mem_map_t kernel_init;
table_t kernel_dir;
table_t kernel_vid;

heap_t * kernel_heap;
heap_t * user_heap;

mem_map_t * pagedir;
// Number of video pages for kernel
// Each program starts with there own video page
int kernel_video_pages = 1;

uint32_t getaddr(uint8_t process){
	return (uint32_t) pagedir->table;
//	return (uint32_t) &dir[process];
//>>>>>>> .r15498

	//uint32_t phys_addr = allocate_big_page(flags, PROG_VIRT_ADDR);


	//programs++;

}


void pageinit(){


	//printf("Starting Page Init\n");
	int i;

#ifdef TEST_PAGING
	int * test = (int *) 0x01800000;
	for(i=0; i<15; i++){
		test[i] = i*7;
	}
#endif

	uint32_t flags = 0;
	// Initialize physical memory
	// Set video, Kernel and kernel_heap as used
	mm_init(&phys_mem, NULL, 0, MEGA4, 0, 0);
	// Video Physical Space
	set(&phys_mem, 0, 0);	// Flags are not used
	// Kernel Physical Space
	uint32_t kernel_phys_idx = KERNEL >> DIRECTORY_OFFSET;
	set(&phys_mem, kernel_phys_idx, 0);
	//set(&phys_mem, 2, 0);

	// Use kernel memory map just to intialize video table
	flags = (SET_P) | (SET_R);
	table_init(&kernel_vid, 0, KILO4);
	kernel_vid.table[VIDDIV] = VIDEO | flags;
	kernel_vid.table[VIDDIV+1] = VIDBUF0 | flags;
	kernel_vid.table[VIDDIV+2] = VIDBUF1 | flags;
	kernel_vid.table[VIDDIV+3] = VIDBUF2 | flags;
	mm_init(&kernel_init, &kernel_dir, 0, MEGA4, 0, MEGA4);
	set_map(&kernel_init, 0, (uint32_t) &kernel_vid, flags);
	flags |= (SET_S);
	set_map(&kernel_init, KERNEL, KERNEL, flags);

	page_set(&kernel_init);
	
	// Initialize kernel heap
	flags = (SET_P) | (SET_R);
	kernel_heap = heap_init(&phys_mem, KERNEL_HEAP_VIRT, flags);
	flags = (SET_P) | (SET_R) | (SET_U);
	//user_heap = heap_init(&phys_mem, USER_HEAP_VIRT, flags);


	// kernel_pagedir = get_table(kernel_heap, 0, MEGA4);
	// printf("kernel dir addr 0x%#x\n", kernel_pagedir);
	// kernel_init(kernel_pagedir);
	// pagedir = kernel_pagedir;
	//printf("Ending Page Init\n");
}

void kernel_setup(mem_map_t * dir) {
	uint32_t flags;
	// Mapping for kerenel page
	flags = SET_P | SET_R | SET_W | SET_S;
	// printf("seting up kernel map\n");
	set_map(dir, KERNEL, KERNEL, flags);
	// printf("done\n");
	// Mapping for kernel heap
	// It is linked list so we search through to map all 4 MB pages
	flags = SET_P | SET_R | SET_W | SET_U;
	heap_t * cur = kernel_heap;
	while( cur != NULL ) {
		// Set map for kernel heap table
		set_map( dir, cur->heap_table.start, (uint32_t) cur->heap_table.table, flags );
		cur = cur->next;
	}
	flags = SET_P | SET_R | SET_W | SET_U;
	// cur = user_heap;
	// while( cur != NULL ) {
	// 	// Set map for kernel heap table
	// 	set_map( dir, cur->heap_table.start, (uint32_t) cur->heap_table.table, flags );
	// 	cur = cur->next;
	// }

	// Get a new video table
	// printf("Getting vid table\n");
	mem_map_t * vidtab = get_table(kernel_heap, 0, KILO4);
	// Set video entry
	flags = SET_P | SET_R;
	set_map(vidtab, VIDEO, VIDEO, flags);
	set_map(vidtab, VIDBUF0, VIDBUF0, flags);
	set_map(vidtab, VIDBUF1, VIDBUF1, flags);
	set_map(vidtab, VIDBUF2, VIDBUF2, flags);
	// Set directory to point to table
	flags = SET_P | SET_R | SET_W;
	set_map( dir, 0, (uint32_t) vidtab->table, flags );
}

void pageenable(){
	page_set(pagedir);
}

uint8_t * map_user_vid() {
	table_t * vid_tab = (table_t *) kmalloc(KILO4);
	vid_tab->table[0] = VIDEO;
	vid_tab->table[0] |= SET_P | SET_U | SET_R;
	pagedir->table->table[USER_VIDEO_ENTRY] = (uint32_t) vid_tab;
	pagedir->table->table[USER_VIDEO_ENTRY] |= SET_P | SET_U | SET_R;
	//printf("dir entry 0x%#x page loc 0x%#x etnry 0x%#x\n", pagedir->table->table[USER_VIDEO_ENTRY], vid_tab, vid_tab->table[0]);
	return (uint8_t *) (USER_VIDEO_ENTRY << DIRECTORY_OFFSET);
}

void remap_user_video(pcb_t * pcb, uint32_t addr) {
	if(pcb->user_vid_mem != NULL) {
		table_t * vid_tab = (table_t *) (pcb->dir->table->table[USER_VIDEO_ENTRY] & ADDR_ENTRY);
		vid_tab->table[0] = addr;
		vid_tab->table[0] |= (SET_P) | (SET_R) | (SET_U);
	}
}

void remap_video(pcb_t * pcb, uint32_t addr) {
	table_t * vid_tab = (table_t *) (pcb->dir->table->table[0] & ADDR_ENTRY);
	vid_tab->table[VIDDIV] = addr;
	vid_tab->table[VIDDIV] |= (SET_P) | (SET_R);
	page_set(pcb->dir);
}


void page_set(mem_map_t * dir){
	// Set current table for this and mem map
	pagedir = dir;
	set_cur_table(dir);
	// Set cr3
	uint32_t saveflags;
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
		:"r" (dir->table)
		:"%eax"
		);
	restore_flags(saveflags);
}

void * kmalloc(uint32_t nbytes) {
	return _malloc(kernel_heap, nbytes);
}

// Get PID
// Set up parent
// handle node ll
// set current process

//save args	
//  strcpy(pcb->argsave, (int8_t *)argbuf);

// In progress
pcb_t * alloc_prog(int PID) {
	int i;
	mem_map_t * prog_dir = get_table(kernel_heap, 0, MEGA4);
	// printf("After getting new Table\n");
	// setup PCB
	uint32_t flags = SET_P | SET_R;
	//printf("alloc1\n");
	pcb_t * pcb = (pcb_t *) get_mult_page(kernel_heap, flags, PCB_PAGE_NUM);
	// printf("PCB at 0x%#x ends at 0x%#x\n", pcb, &(pcb->dir));
	//printf("alloc2\n");
	// pcb->kernel_sp=tss.esp0;
	// asm volatile("movl %%ebp, %0":"=g"(pcb->kernel_bp));
	// asm volatile("movl %%esp, %0":"=g"(pcb->espsave));
	// pcb->kernel_ss=tss.ss0;

	pcb->process=PID;
	for (i=0; i<NUM_FILES; i++){
		pcb->fdescs[i].position=0;
		pcb->fdescs[i].inuse=0;
		pcb->fdescs[i].inode=0;
	}
	//printf("alloc3\n");
	// Set TSS for 1 page kernel stack
	tss.esp0 = get_page(kernel_heap, flags);
	pcb->kernel_bp = tss.esp0;
	pcb->espsave = tss.esp0;
	// printf("After getting new K stack\n");
	tss.ss0 = KERNEL_DS;
	//printf("alloc4\n");

	// Set up program table
	
	kernel_setup(prog_dir);

	page_set(prog_dir);
	// printf("After setting new Table\n");
	// Set up program in new page directory
	uint32_t vir_addr = PROG_VIRT_ADDR;
	flags = SET_P | SET_R | SET_U | SET_S;
	if(allocate_big_page(flags, vir_addr) != PROG_VIRT_ADDR)
		return NULL;

	// printf("After Big alloc\n");
	pcb->dir = prog_dir;
	pcb->user_vid_mem = NULL;

	// print_dir();
	//while(1);
	return pcb;
}



// Translate a linear addres to a physical one
uint32_t lin_to_phys(uint32_t lin_addr) {
	//lin_addr_t lin_addr = (lin_addr_t) addr;
	uint32_t pde = pagedir->table->table[lin_addr >> DIRECTORY_OFFSET];
	//printf("PDE:   0x%#x\n", pde);
	if((pde & SET_P) == 0)
		return -1;

	// Small Page
	if((pde & SET_S) == 0){
		table_t * pte = (table_t *) (pde & ADDR_ENTRY);
		return (pte->table[(lin_addr & TABLE) >> TABLE_OFFSET] & ADDR_ENTRY) + (lin_addr & OFFSET);
	}

	return (pde & ADDR_ENTRY) + (lin_addr & OFFSET_E);
}

uint32_t get_page_entry(uint32_t addr) {
	uint32_t first = pagedir->table->table[addr >> DIRECTORY_OFFSET];
	// Small Page
	if((first & SET_S) == 0){
		table_t * pte = (table_t *) (first & ADDR_ENTRY);
		return pte->table[(addr & TABLE) >> TABLE_OFFSET];
	}

	return first;
}

// Returns physical address of 4KB allocated page
uint32_t allocate_small_page(uint32_t flags) {
	// Check if kernel page
	// if(flags & SET_U)
	// 	return -1;

	return get_page(kernel_heap, flags);
}


void deallocate_small_page(uint32_t addr) {

	//printf("Deallocationg addr:  0x%#x\n", addr);
	// if(get_page_entry(addr) & SET_U)
	// 	return;
	free_page(kernel_heap, addr);
}

// Inputs:  
//		flags:  flags of the page
//		vir_addr:  virtual address of the page.  
// Return the virtual address to the 4MB allocated page
uint32_t allocate_big_page(uint32_t flags, uint32_t vir_addr){

	uint32_t phys_addr = get(&phys_mem, flags);
	//printf("Allocated Phys Addr:  0x%#x\n", phys_addr);
	if(phys_addr == -1)
		return -1;

	uint32_t assigned_addr;

	if(vir_addr <= 0 ) 
		assigned_addr = get(pagedir, flags);
	
	else 
		assigned_addr = vir_addr;
		
	map_epage(flags, phys_addr, assigned_addr);

	return assigned_addr;
}

// Deallocate the memory area of virtual address
// Adds the memory space to the quicklist queue for reuse
void deallocate_big_page(uint32_t addr){
	uint32_t phys_idx = lin_to_phys(addr) >> DIRECTORY_OFFSET;
	free(&phys_mem, phys_idx);
	free(pagedir, (addr >> DIRECTORY_OFFSET));
}

// Map a 4MB page to a virtual address
void map_epage(uint32_t flags, uint32_t phys_addr, uint32_t vir_addr){
	set_map(pagedir, vir_addr, phys_addr, flags);
}

void print_dir() {
	int i,j, max;
	for(i=0; i<TABLESIZE; i++) {
		if(pagedir->table->table[i] & SET_P) {
			if(pagedir->table->table[i] & SET_S) {
				max = i;
				printf("Virtual Address 0x%#x mapped with flags to 0x%#x\n", (i << DIRECTORY_OFFSET), pagedir->table->table[i]);
			}
			else {
				table_t * pte = (table_t *) (pagedir->table->table[i] & ADDR_ENTRY);
				printf("Address 0x%#x mapped to table at 0x%#x\n",(i << DIRECTORY_OFFSET), pte );
				for(j=0; j<TABLESIZE; j++) {
					if(pte->table[j] & SET_P) {
						//uint32_t addr = (i << DIRECTORY_OFFSET) | (j << TABLE_OFFSET);
						//printf(" 0x%#x Mapped through table to 0x%#x\n", addr, pte->table[j]);
					}
				}
			}
		}
	}
	max = max%32 == 0 ? max/32 : max/32 + 1;
	print_mm(pagedir, 0,max);
}

void print_heap(heap_t * heap, int num) {
	if(heap == NULL)
		return;
	int count=1;
	heap_t* cur_heap = heap;
	while(cur_heap->next != NULL) {
		count++;
		cur_heap = cur_heap->next;
	}
	printf("Heap %d  count %d Start Location 0x%#x\n", num, count, heap);
	printf("Heap Map Start 0x%#x,  Size 0x%#x\n", heap->heap_table.start, heap->heap_table.size);
	print_mm(&(heap->heap_table),0, 3);
	print_heap(heap->small_heap, ++num);
}


// Important
#ifdef TEST_PAGING
// These test are used to manipulate allocation and deallocation of memory and do not unallocate
// several allocated pages.
void paging_test(){
	print_dir();
	print_heap(kernel_heap, 0);
	int i, j, malloc_size = 32;
	int amount = 7*KILO4;
	uint32_t  * test = (uint32_t *) kmalloc(amount * 4);
	for(i=0; i<amount; i++) {
		// test = (uint8_t *)  kmalloc(malloc_size);
		test[i] = get_page(kernel_heap, (SET_P) | (SET_R));
		printf("Malloc  ");
		// for(j=0; j<malloc_size; j++) {
		// 	*(test  + j) = 0x05;
		// }
		printf("%d, at addr 0x%#x, reads %d\n", i, test[i], 0);
		//get_table(kernel_heap, 2, KILO4);
	}
	print_heap(kernel_heap, 0);

	// int * test = (int *) 0x01800000;
	// int i, pop;
	// uint32_t first, second, third, fourth, fifth;
	// We are writing the the memory address that fifth will become
	// we then map this space to the vir_addr and read from it after
	// we turn paging on.
	
	// uint32_t flags = 0;
	// uint32_t vir_addr = 0;
	// flags |= (SET_P) | SET_R | SET_U | SET_S;

	// printf("__Paging Test__\n");
	// printf("Cr3 = 0x%#x\n", pagedir->table);
	// printf("Kernel page dir[1]:   0x%#x\n", pagedir->table->table[1]);
	// printf("end of table = 0x%#x\n", &(pagedir->table->table[tablesize-1]));
	
	// printf("Initializing:  0x%#x\n", pagedir->table);
	// printf("Heap Table: 0x%#x\n",  kernel_heap->heap_table.table);
	// for(i=0; i<4; i++) {
	// 	printf("Page Dir Entry %d:  0x%#x\n", i, pagedir->table->table[i]);
	// 	printf("Heap Page Table entry %d:  0x%#x\n", i, get_page_entry(KERNEL_HEAP + i*KILO4));
	// }
	// printf("Video Page Entry: 0x%#x\n", get_page_entry(VIDEO));
	//while(1);

	/*
	printf("Empty quicklist test\n");
	while(!queue_empty(quicklist)){
		pop = queue_pop(quicklist);
		printf("%d  ", pop);
	}
	printf("\n");
	*/
	 
	// printf("MM Test\n");
	// print_mm(&phys_mem,0, 2);
	// first = allocate_big_page(flags, vir_addr);
	// second = allocate_big_page(flags, vir_addr);
	// deallocate_big_page(first);
	// third = allocate_big_page(flags, vir_addr);
	// deallocate_big_page(second);
	// fourth = allocate_big_page(flags, vir_addr);
	// deallocate_big_page(fourth);
	// deallocate_big_page(third);
	// fifth = allocate_big_page(flags, vir_addr);

	// // first and third are the same
	// // second and fourth are the same
	// printf("First:   0x%#x\n", first);
	// printf("Sec:     0x%#x\n", second);
	// printf("Third:   0x%#x\n", third);
	// printf("Fourth:  0x%#x\n", fourth);
	// printf("Fifth:   0x%#x\n", fifth);
	// printf("NEW PAGE ADDR:  0x%#x\n", allocate_big_page(flags, vir_addr));
	// printf("NEW PAGE ADDR:  0x%#x\n", allocate_big_page(flags, vir_addr));
	// printf("NEW PAGE ADDR:  0x%#x\n", allocate_big_page(flags, vir_addr));
	// printf("NEW PAGE ADDR:  0x%#x\n", allocate_big_page(flags, vir_addr));
	// printf("NEW PAGE ADDR:  0x%#x\n", allocate_big_page(flags, vir_addr));
	// // Note how the dealocated physical addresses will be 0 on the mem map
	// print_mm(&phys_mem,0, 2);
	

	// printf("MM Small Test\n");
	//mem_map_t * mm = (mem_map_t *) (KERNEL_HEAP + KB_FOUR);
	//print_mm(&kernel_heap->heap_table,0, 3);
	/*
	first = allocate_small_page(flags);
	//while(1);
	second = allocate_small_page(flags);
	deallocate_small_page(first);
	third = allocate_small_page(flags);
	deallocate_small_page(second);
	fourth = allocate_small_page(flags);
	deallocate_small_page(fourth);
	deallocate_small_page(third);
	fifth = allocate_small_page(flags);

	// first and third are the same
	// second and fourth are the same
	printf("First:   0x%#x\n", first);
	printf("Sec:     0x%#x\n", second);
	printf("Third:   0x%#x\n", third);
	printf("Fourth:  0x%#x\n", fourth);
	printf("Fifth:   0x%#x\n", fifth);
	printf("NEW PAGE ADDR:  0x%#x\n", allocate_small_page(flags));
	printf("NEW PAGE ADDR:  0x%#x\n", allocate_small_page(flags));
	printf("NEW PAGE ADDR:  0x%#x\n", allocate_small_page(flags));
	printf("NEW PAGE ADDR:  0x%#x\n", allocate_small_page(flags));
	printf("NEW PAGE ADDR:  0x%#x\n", allocate_small_page(flags));
	printf("NEW PAGE ADDR:  0x%#x\n", allocate_small_page(flags));
	printf("NEW PAGE ADDR:  0x%#x\n", allocate_small_page(flags));
	printf("NEW PAGE ADDR:  0x%#x\n", allocate_small_page(flags));
	printf("NEW PAGE ADDR:  0x%#x\n", allocate_small_page(flags));
	printf("NEW PAGE ADDR:  0x%#x\n", allocate_small_page(flags));
	printf("NEW PAGE ADDR:  0x%#x\n", allocate_small_page(flags));


	// Note how the dealocated physical addresses will be 0 on the mem map
	print_mm(&kernel_heap->heap_table, 0,4);
	*/
	
	// uint32_t * test_write;
	// test_write = (uint32_t *) allocate_big_page(flags,0);
	// for(i=0; i<tablesize; i++) {
	// 	test_write[i] = i*5;
	// }
	// for(i=0; i<10; i++) {
	// 	printf("Write %d\n", test_write[i]);
	// }
	// // This test shows mapping of the physical address (0x01800000), writen to ealier, to
	// // the virtual address used with program execution (0x80000000).
	// map_epage(flags, test, vir_addr);
	// printf("Memory read test from phys addr 0x%#x\n", (uint32_t) test);
	// for(i=0; i<15; i++){
	// 	printf("0x%#x ", ((int *) vir_addr)[i]);
	// }
	
}

#endif


