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
#define NUM_MAPS 6
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

typedef struct table_type {
	uint32_t table[TABLESIZE];
} table_t __attribute__((aligned (KILO4)));


typedef struct mem_map_type {
	// Bitmap for 1024 entries
	// The 32 entry is used to find fully open groups for continuous allocation
	uint32_t mm[33];
	// Track how much we allocated
	int used;
	// Virtual address of where the memory is located
	uint32_t start;
	// Size of each allocation
	uint32_t size;
	// Quick_access queue 
	queue_t quicklist;
	// Start point for when the queue runs out
	int recent;
	// Optinal way of tying memory map to table
	// Can be null if not used
	// Only maps with tables can be sorted
	table_t * table;

} mem_map_t;


typedef struct heap_info_type {
	uint32_t num_alloc;
	uint32_t num_free;
	// Abuse the fact that we alloc a lot of extra maps
	// This map uses the table to keep the location of each allocation
	mem_map_t * location;
	// This map uses the same entry as in loaction but the table keeps track of the size
	mem_map_t * size;

} heap_info_t;

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

	// Array of open maps since we want to fill 4 KB.
	// Hand them out to whoever needs them.  Just extra space
	queue_t open_maps;
	mem_map_t maps[NUM_MAPS];

} __attribute__((aligned (KILO4)));


void * _malloc(heap_t * heap, uint32_t nbytes);

heap_t * heap_init(mem_map_t * phys, uint32_t virt_addr, uint32_t alloc_flags);

heap_t * heap_setup(mem_map_t * phys, uint32_t virt_addr, uint32_t alloc_flags);

void set_cur_table(mem_map_t * table);

int large_heap_init(heap_t * heap);

heap_t * small_heap_init(uint32_t flags, mem_map_t * mem, uint32_t bigger_page_size) ;

int new_heap(heap_t * heap);

int new_smallest_heap(heap_t * heap);

uint32_t get_page(heap_t * heap, uint32_t flags);

uint32_t get_mult_page(heap_t * heap, uint32_t flags, int amount);

void free_page(heap_t * heap, uint32_t addr);

mem_map_t * get_map(heap_t * heap);

mem_map_t * get_table(heap_t * heap, uint32_t start, uint32_t size);

void free_map(heap_t * heap, mem_map_t * mem_map);

void mm_init(mem_map_t * mem_map, table_t * tab, uint32_t virt_start, uint32_t virt_size, uint32_t phys_start, uint32_t phys_size);

void table_init(table_t * tab, uint32_t phys_start, uint32_t phys_size);

// Entry is the table entry that will be mapped to phys_addr
void set_map(mem_map_t * mem_map, uint32_t virt_addr, uint32_t phys_addr, uint32_t flags);

void set(mem_map_t * mem_map, uint32_t entry, uint32_t flags);

void free(mem_map_t * mem_map, uint32_t entry);

// Returns address
uint32_t get(mem_map_t * mem_map, uint32_t flags);
// Returns table entry
uint32_t get_entry(mem_map_t * mem_map, uint32_t flags);

// Get amount number of pages in a row.
// Returns the value of the first entry but allocates amount number of entries
uint32_t get_continuous(mem_map_t * mem_map, uint32_t flags, int amount);

uint32_t look_slow(mem_map_t * mem_map, uint32_t flags, int amount);

int refill_queue(mem_map_t * mem_map);

void print_mm(mem_map_t * mem_map, int start, int entries);

#endif 
