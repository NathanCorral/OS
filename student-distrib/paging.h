#ifndef PAGING_H
#define PAGING_H

#define KPriv 0
#define UPriv 3

#define PROG_VIRT_ADDR 0x08000000

// Defines for Linear Addr
#define DIRECTORY 0xFFC00000
#define TABLE 0x003FF000
#define OFFSET 0x00000FFF
#define OFFSET_E 0x003FFFFF
#define DIRECTORY_OFFSET 22
#define TABLE_OFFSET 12

#define USER_VIDEO_ENTRY 1000

#define SET_P 1
#define SET_R (1 << 1)
#define SET_U (1 << 2)
#define SET_W (1 << 3)
#define SET_CACHE (1 << 4)
#define SET_A (1 << 5)
#define SET_DIRTY (1 << 6)
#define SET_S (1 << 7)
#define SET_G (1 << 8)
#define PD_OFF 0xFFC00000
#define PT_OFF 0x003FF000
#define ADDR_ENTRY 0xFFFFF000

#define FLAG_ENTRY 0x00000FFF
#define tablesize 1024
#define TABLESIZE 1024

#define KERNEL 0x00400000
//#define KERNEL_VIRT 0x00400000
#define VIDDIV 0xB8
#define KERNELDIV 1
#define KERNEL_HEAP_VIRT 0x00800000
#define USER_HEAP_VIRT 0xA0000000
#define KB_FOUR 0x1000

//#define KERNEL_HEAP_VIRT 0x00800000

#define MAX_VIDEO_PAGES 4



#include "memory_map.h"
#include "syscalls.h"


typedef struct dir_t {
	int pagedir[tablesize];

} dir_t;


/*
pageinit:  Initialize Physical memory.  Set Video and Kernel Space as not available
			Initalize the first page table and video table that will be used before programs run
			Starts paging
			Initalize Kernel and User Heaps
	Inputs: 
	Returns:
*/
extern void pageinit();
// Test Function
extern void paging_test();

// Convert Linear address to the physical address on the current page table
// Returns the physical address
extern uint32_t lin_to_phys(uint32_t lin_addr);

/*
kmalloc:  Kernel Malloc Function.   Calls _malloc on the Kernel Heap
	Inputs: 
		nbytes:  Number of bytes to allocate
	Returns:
		A pointer to the memory allocated
*/
extern void * kmalloc(uint32_t nbytes);


/*
map_user_vid:  Get a table, Set the program page directory to point to it
			And map the table to video memory.
			Used to give user programs a user space to write to video memory
	Inputs: 
	Returns:
		A pointer to location mapped to VIDEO
*/
extern uint8_t * map_user_vid();


// Setup function for initalizing the video memory, kernel code, and kernel heap on
// a new page directory.  This is used to set up a new page directory for a new program
void kernel_setup(mem_map_t *dir);

/*
alloc_prog:  Allocates a new program.  
			Get a 4KB page for a new PCB and initalize the new PCB
			Get a 4KB page for a new Kernel Stack and set tss.esp0 to point to it
			Get a new page directory for the new Program, initalize it with kernel data
				structures, and set it as the current page directory
			Allocate a 4 MB page and map the PROG_VIRT_ADDR to point to it for the new 
				program	data
	Inputs: 
	Returns:
		A pointer to the PCB of the new program
*/
pcb_t * alloc_prog();

// Set the table in the input dir to the current page table
// Also used to start paging
void page_set(mem_map_t * dir);


/*
remap_user_video:  	Remap the programs video memory (gotten in map_user_vid) to addr
	Inputs: 
		pcb: Program that wants the remap.
		addr: video buffer that the program user video memory will be remapped to
	Returns:
*/
void remap_user_video(pcb_t * pcb, uint32_t addr);

// Returns table entry from linear address addr
uint32_t get_page_entry(uint32_t addr);

// Returns address to 4KB page allocated with flags
// Uses Kernel Heap to allocate
uint32_t allocate_small_page(uint32_t flags);

// Free the 4KB page allocated through allocate_small_page for re-allocation
void deallocate_small_page(uint32_t addr);

// Allocates a 4 MB page with flags and maps vir_addr to point to page
// If vir_addr is NULL we just get the virtual address from the current page directory
uint32_t allocate_big_page(uint32_t flags, uint32_t vir_addr);

// Free page allocated with allocate_big_page.  Lets 4 MB page be re-allocated
void deallocate_big_page(uint32_t addr);

// Maps a 4 MB page in the current page directory so vir_addr points to phys_addr
// This is called from allocate_big_page to perform virtual mapping
void map_epage(uint32_t flags, uint32_t phys_addr, uint32_t vir_addr);

// Prints info about the current directory
void print_dir();
// Prints info about the heap
void print_heap(heap_t * heap, int num);

/*
remap_video:  	Not used anymore but may be set up later since this is faster
			than writing to both video memory and the video buffer
			The idea is to remap what the program thinks as VIDEO to the video buffer
			for that terminal so the program can write as normal without affecting the 
			shown screen
			Must also set the screen_x and screen_y postions in lib.c
			Must also have a remap function
	Inputs: 
		pcb: Program that wants the remap.
		addr: video buffer that VIDEO is remapped to
	Returns:
*/
void remap_video(pcb_t * pcb, uint32_t addr);


#endif

