#ifndef MEMORY_MAP_H
#define MEMORY_MAP_H

#define KILO4 0x1000
#define KILO 0x400
#define MEGA4 0x400000
#define MEGA4_OFF 22;
#define ADDR_ENTRY 0xFFFFF000

//
#define ENTRY_OFFSET 22
// #define NUM_SMALL_HEAPS 1
#define BITMAP_WIDTH 32

// (33 + 5) * 4 + SIZEOF(Queue))
// 288 Bytes
#define MEM_MAP_SIZE 512

// floor(4 KB / MEM_MAP_SIZE)
// This is 14 but subtract one for the size of the heap type
#define NUM_MAPS 3
//#define AMOUNT 0
#define SMALL_HEAPS 1

#define FULL 0xFFFFFFFF

#define TABLESIZE 1024

// Number of 4 KB pages the small heap uses
// Allocations are of size, int Bytes : SMALL_HEAP_SIZE * 4 KB / 1 KB
// It takes 4KB to map SMALL_HEAP_SIZE 4KB pages
#define BIGGEST_HEAP MEGA4
// Ratio of what we allocate that may go unused
#define HEAP_RATIO 1
#define HEAP_ALLOC_NUM 32
// Set the smallest amount of bytes to keep track of
#define SMALLEST_AMOUNT 4

#include "queue.h"
#include "lib.h"

typedef struct heap_t heap_t;
typedef struct heap_info_t heap_info_t;

typedef struct table_type {
	uint32_t table[TABLESIZE];
} table_t __attribute__((aligned (KILO4)));


typedef struct mem_map_type {
	// Bitmap for 1024 entries
	// The 32 entry is used to find fully open groups of 32 entries for continuous allocation
	uint32_t mm[33];
	// Track how much we allocated
	int used;
	// Virtual address of where the memory is located
	uint32_t start;
	// Size of each allocation
	uint32_t size;
	// Quick_access queue 
	// Used when get is called to quickly return an unused entry
	queue_t quicklist;
	// Start point for when the queue runs out to refill the queue
	int recent;
	// Table that the mm bitmap keeps track of
	// Entries in the mm bitmap correspond to entries in this table
	// Can be null if not used ie we just want the map to keep track of memory
	table_t * table;

} mem_map_t;


// Treat heap as 4 MB blocks where we allocate 4 KB at a time
struct heap_t {
	// If we run out of room in this heap 
	heap_t * next;
	// Allow us to allocate another heap if we run out
	mem_map_t * phys_mem;

	// Use these flags when allocating memory
	uint32_t flags;
	// We allocate 4KB blocks using this memory map
	mem_map_t heap_table;
	// For allocating blocks less than HEAP_RATIO * (the size of blocks allocated) we go to smaller heap
	heap_t * small_heap;

	uint32_t sizes[171];

	// Array of open maps since we want to fill 4 KB for heap data structure.
	// Hand them out to whoever needs them.  Just extra space
	queue_t open_maps;
	mem_map_t maps[NUM_MAPS];

} __attribute__((aligned (KILO4)));


/*
_malloc:  Marks memory as used in heap and returns pointer to allocated memory
	Inputs: 
	heap: Heap where memory is allocated
	nbytes:  Number of bytes allocated
	Returns:
		A pointer to the allocated memory with the heap flags set on the table
*/
void * _malloc(heap_t * heap, uint32_t nbytes);

void _free(heap_t* heap, void* addr);

/*
heap_init:  Initialize A heap.
	Inputs: 
	phys: A pointer to the physical memory so that when the heap becomes
full a new heap can be spawned.  
	virt_addr:  The virtual address that will be handed out during new allocations
				This may be different from where the object is located in phys mem
	alloc_flags:	Flags that are used when allocating new memory.
		These flags can be different from the flags used to allocate the heap data
		structures which do not give user access to heap structure data
	Returns:
		An allocated heap that keeps track of available physical memory
*/
heap_t * heap_init(mem_map_t * phys, uint32_t virt_addr, uint32_t alloc_flags);


// Sets the current page directory in order to keep track of the virtual address space
void set_cur_table(mem_map_t * table);


/*
get_page:  Gets a 4 KB page from the largest heap
	Inputs: 
		heap: The heap we are allocating from
		flags:  We allow this function to allocate pages with custom flags and 
				not the flags used by the heap we are allocating from
	Returns:
		The address of the allocated page
*/
uint32_t get_page(heap_t * heap, uint32_t flags);

/*
get_mult_page:  Gets a set of 4 KB pages with continous addresses
	Inputs: 
		heap: The heap we are allocating from
		flags:  We allow this function to allocate pages with custom flags and 
				not the flags used by the heap we are allocating from
		amount: The number of 4KB pages we want
	Returns:
		The start address for the allocated pages
*/
uint32_t get_mult_page(heap_t * heap, uint32_t flags, int amount);

/*
free_page:  Free a 4KB page allocated through get_page
	Inputs: 
		heap: The heap we allocated from
		addr:  The address of the allocation (Returned from get_page)
	Returns:
*/
void free_page(heap_t * heap, uint32_t addr);

/*
get_map:  Returns an uninitialized memory map
	Inputs: 
		heap: The heap we get the map from
	Returns:
		An uninitalized memory map
*/
mem_map_t * get_map(heap_t * heap);

/*
get_table:  Retrns an initalized memory map with a table
			The table is initialized to have the virtual address point to the same 
				physical address
	Inputs: 
		heap: The heap we get the map from
		start: Vitual and physical start address
		size: Size of the memory map and table
	Returns:
		An memory map with the table initalized so the first value points to start
		and the second points to start+size and so on.
*/
mem_map_t * get_table(heap_t * heap, uint32_t start, uint32_t size);


/*
free_map:  Free the map gotten through get_map.  Allows the map to be reallocated
	Inputs: 
		heap: The heap we got the map from
		mem_map: The pointer to the map location
	Returns:
*/
void free_map(heap_t * heap, mem_map_t * mem_map);

/*
mm_init:  Initialize A Memory map with or without a table.
	Inputs: 
	mem_map: A pointer to memory map to be initalized
	virt_start:  The virtual address that will be handed out when get is called on this memory
				map.  This can be different from the physical address the map keeps track of
	virt_size:  The virtual size of the map ie for a page directory this would be 4 MB
	phys_start:  The physical address that the memory map keeps track of.  During allocations
			the virtual address that is returned directs to this physical address
			Only used if there is a table to be initializecd.
	phys_size:  The gaps between physical memory location.  Almost always the same as virt_size
	Returns:
		None since we call the function with a memory map pointer that we make changes to
*/
void mm_init(mem_map_t * mem_map, table_t * tab, uint32_t virt_start, uint32_t virt_size, uint32_t phys_start, uint32_t phys_size);

/*
table_init:  Initalize a table to point to phys_start with phys_size gaps between allocations
	Inputs: 
		tab: Pointer to table that is initalized
		phys_start: Physical start addr that the table redirects to
		phys_size: Size of the jumps in physical memory that the table points to
	Returns:
		None
*/
void table_init(table_t * tab, uint32_t phys_start, uint32_t phys_size);

/*
set_map: Maps a virtual address to physical address on the map it is called on
		The map must have a table associated to it
	Inputs: 
		mem_map: Pointer to memory map that has a table where we preform the map
		virt_addr: Virtual address (Spot in memory map table that will point to phys_addr)
		phys_addr: Physical address that the virt_addr redirects to
		flags:  Flags used when performing the map
	Returns:
		None
*/
void set_map(mem_map_t * mem_map, uint32_t virt_addr, uint32_t phys_addr, uint32_t flags);


/*
set: Marks entry( range from 0 : 1023) as in use
	Inputs: 
		mem_map: Pointer to memory map that will be marked
		entry: Entry (Range from 0 : 1023) that will be set as in use
		flags:  Flags used if there is a table that must be marked when seting the entry
	Returns:
		None
*/
void set(mem_map_t * mem_map, uint32_t entry, uint32_t flags);


/*
free: Need name change.  Free the entry that was marked with set.  Add the entry to the queue
		If the map has a table then the (PRESENT) bit will be un-set
	Inputs: 
		mem_map: Pointer to memory map where the entry will be freed
		entry: Entry (Range from 0 : 1023) that will be set as not in use and added to queue
	Returns:
		None
*/
void free(mem_map_t * mem_map, uint32_t entry);

/*
get: Get an available entry from the memory map and marks the entry as in use
	returns -1 if the map is full
	Inputs: 
		mem_map: Pointer to memory map where the entry will be freed
		flags: If there is a table, Sets the flags for the table
	Returns:
		Virtual address of the available entry we got from the memory map
*/
uint32_t get(mem_map_t * mem_map, uint32_t flags);

/*
get_continuous: Similar to get but for multiple continuous entries
			Marks all the entries as in use
	returns -1 if the map does not have enough continous entries
	Inputs: 
		mem_map: Pointer to memory map where the entry will be freed
		flags: If there is a table, Sets the flags for the table
		amount:  The number of entries we want
	Returns:
		Virtual address of the start of the available entry we got from the memory map
*/
uint32_t get_continuous(mem_map_t * mem_map, uint32_t flags, int amount);


// Helper function to print the entries into the mm bitmask
// Print mm[start] and increment start entries number of times
// Also prints mm[32] or the directory for the mm bitmask
void print_mm(mem_map_t * mem_map, int start, int entries);

#endif 
