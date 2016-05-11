
#include "memory_map.h"
#include "paging.h"
#include "lib.h"

#define mm_idx(entry) ((entry >> 5) & 0x1F)
#define mm_off(entry) (entry & 0x1F)
#define exists(mem_map, entry) (mem_map->mm[mm_idx(entry)] & (1 << mm_off(entry)))

#define used(heap) (heap->heap_table.used)
#define map(heap) (&(heap->heap_table))

#define table_start(heap) (heap->heap_table.table->table[0])

#define ceiling(heap, nbytes) ((nbytes % heap->heap_table.size == 0) ? nbytes / heap->heap_table.size : ((nbytes / heap->heap_table.size) + 1))
// The reason this works to see if a heap is small is because
// large heaps always have a pointer to physical memory, which
// doesnt need a table. The small heap physical memory is the
// table of whatever big heap created it and therefore it has a
// table
#define small(heap) (heap->phys_mem->table != NULL)

#define get_entry_from_addr(heap, addr) ((addr - heap->heap_table.start)/heap->heap_table.size)

#define set_size(sizes, entry, size) (sizes[entry/6] |= ((size & 0x1F) << 5*(entry % 6)))
#define get_size(sizes, entry) ((sizes[entry/6] >> (5*(entry %6))) & 0x1F)

/*
Helper Functions 
*/
// Help set up the largest heap which hands out 4KB pages and has a total size of 4MB
// Used during heap_init and large_heap init
heap_t * heap_setup(mem_map_t * phys, uint32_t virt_addr, uint32_t alloc_flags);

// Initialize a new heap of size 4MB which hands out 4KB pages
// This is called when a heap has run out of 4 KB pages to hand out and needs to
// get a new 4MB page from physical memory.
// The new heap is linked to the next pointer in the heap that called it
int large_heap_init(heap_t * heap);

// Recusive function that sets up a smaller heap to keep track of Smaller memory
// For example, A large heap has size 4 MB and hands out 4 KB pages. The large heap
// has a pointer to smaller heaps which can keep track of 32 B and hand out chuncks with
// that size.  In order to do this the smaller heap will get (32 * 1024)/4KB pages from the
// larger heap and set them as used.  When malloc is called for an amount smaller than 4KB
// we go to the smaller heap and let it keep track of the smaller allocations
// This is done recursivly where the maximum size a heap will hand out is 32 * the heap
// page size.
heap_t * small_heap_init(uint32_t flags, mem_map_t * mem, uint32_t bigger_page_size);

// Create a new heap of the same size as the one it is called on
// If this fails then the larger heap has run out of memory and we need to call this
// function on the larger heap
// This will only fail on the 4 MB heap if we run out of physical memory
int new_heap(heap_t * heap);

// Get a new smallest heap. Will probably be phased out since it is only called for new
// memory maps which can be malloced
int new_smallest_heap(heap_t * heap);

// Returns the entry into the memory map.  Called from get.
uint32_t get_entry(mem_map_t * mem_map, uint32_t flags);

// When getting continous entries we first look at the 32 entry into the mm bitmap
// If the 32 entry is '0' Then there are 32 continous entries that we can return
// If every entry in 32 is '1' or amount > 32 then  we call this function to walk through
// the mm bitmap and try to find amount unused entries
uint32_t look_slow(mem_map_t * mem_map, uint32_t flags, int amount);

// When the memory map queue becomes empty we call this to add unused entries to the queue
int refill_queue(mem_map_t * mem_map);

/*
End of Helper Functions
*/

mem_map_t * cur_map = NULL;

// Set map_cur_page to null if the current page should not be mapped
// If it is not null treat page as 4 MB allocated for kernel data
// structures.
heap_t * heap_init(mem_map_t * phys, uint32_t virt_addr, uint32_t alloc_flags) {


	// printf("Starting Heap Init\n");
	heap_t * new_heap = heap_setup(phys, virt_addr, alloc_flags);
	uint32_t page_size = BIGGEST_HEAP / TABLESIZE;
	// printf("Starting Small Init\n");
	new_heap->small_heap = small_heap_init(new_heap->flags, map(new_heap), page_size);

	return new_heap;
}

heap_t * heap_setup(mem_map_t * phys, uint32_t virt_addr, uint32_t alloc_flags) {
		// Set S untill we initialize heap table
	uint32_t heap_flags = (SET_P) | (SET_R) | (SET_S);
	int i;

	// Get physical location
	uint32_t phys_addr = get(phys, heap_flags);
	if(phys_addr == -1)
		return NULL;

	uint32_t virt;
	if(virt_addr == NULL)
		virt = get(cur_map, heap_flags);
	else{
		virt = virt_addr;
		//set_map(cur_map, virt, phys_addr, heap_flags);
	}

	set_map(cur_map, virt, phys_addr, heap_flags);
	// Initialize table
	table_t * tab = (table_t *) (virt + KILO4);
	for(i=0; i<TABLESIZE; i++) {
		tab->table[i] = (phys_addr + i*KILO4);
	}
	heap_flags = (SET_P) | (SET_R);
	tab->table[0] |= heap_flags;
	tab->table[1] |= heap_flags;

	// Remap page to table with user flag set
	heap_flags = (SET_P) | (SET_R) | (SET_U);
	// Remap to table without size flag
	set_map(cur_map, virt, (phys_addr + KILO4), heap_flags);
	heap_t * new_heap = (heap_t *) virt;
	new_heap->next = NULL;
	new_heap->phys_mem = phys;
	new_heap->flags = alloc_flags;

	uint32_t page_size = BIGGEST_HEAP / TABLESIZE;

	// We need to link the table so that our flags are not overwritten
	mm_init(map(new_heap), NULL, virt, page_size, 0, 0);
	set(map(new_heap), 0, heap_flags);
	set(map(new_heap), 1, heap_flags);
	new_heap->heap_table.table = tab;
	new_heap->small_heap = NULL;

	for(i = 0; i < (NUM_MAPS); i++) {
		//mm_init( &(new_heap->maps[i]), NULL, 0, 0 );
		queue_push(new_heap->open_maps, i);
	}
	for(i=0; i<171; i++) {
		new_heap->sizes[i] = 0;
	}

	return new_heap;
}

void set_cur_table(mem_map_t * mem_map) {
	cur_map = mem_map;
}

void * _malloc(heap_t * heap, uint32_t nbytes) {
	if(heap == NULL || nbytes == 0)
		return NULL;

	void * ret;
	if((nbytes < (heap->heap_table.size * HEAP_RATIO)) && (heap->small_heap != NULL)) {
		ret = _malloc(heap->small_heap, nbytes);
		if(ret == NULL) {
			if(new_heap(heap) < 0) {
				return NULL;
			}
			return _malloc(heap->next, nbytes);
		}
		else
			return ret;
	}

	uint32_t pages = ceiling(heap, nbytes);
	// printf("Num pages %d\n", (int) pages);
	uint32_t ret_addr;
	int cout = 0;
	// heap_t * start_heap = heap;
	while(heap->next != NULL) {
		// printf("While loop %d with heap 0x%#x with %d used and next 0x%#x\n", cout, heap, heap->heap_table.used, heap->next);
		cout++;
		if((ret_addr = get_continuous(map(heap), heap->flags, pages)) != -1) {
			// printf("Got addr 0x%#x\n", ret_addr);
			uint32_t entry = get_entry_from_addr(heap, ret_addr);
			// printf("Entry %d\n", entry);
			set_size(heap->sizes, entry, pages);
			//get_size(heap->sizes,entry)
			// printf("Size %d, Pages %d, Bytes %d\n",get_size(heap->sizes,entry), pages, nbytes);
			return (void *) ret_addr;
		}
		// printf("Checking next heap\n");
		heap = heap->next;
	}
	// printf("Here\n");
	if((ret_addr = get_continuous(map(heap), heap->flags, pages)) != -1) {
		// printf("Got addr 0x%#x\n", ret_addr);
		uint32_t entry = get_entry_from_addr(heap, ret_addr);
		// printf("Entry2 0x%#x\n", entry);
		set_size(heap->sizes, entry, pages);
		// printf("Size %d, Pages %d, Bytes %d\n",get_size(heap->sizes,entry), pages, nbytes);
		return (void *) ret_addr;
	}
	
	if(new_heap(heap) < 0) {
		//printf("Failed\n");
		return NULL;
	}
	// printf("Got new heap at 0x%#x\n", heap->next);

	return _malloc(heap->next, nbytes);
}

void _free(heap_t* heap, void* addr) {
	if(heap == NULL)
		return;
	uint32_t entry = get_entry_from_addr(heap, (uint32_t) addr);
	// printf("free %d\n", entry);
	if(((uint32_t) addr < heap->heap_table.start) || (entry >= 1024)) {
		_free(heap->next, addr);
		return;
	}
	// if(get_size(heap->sizes,entry) == 0) {
	// 	_free(heap->small_heap, addr);
	// }
	heap_t* temp;
	if(get_size(heap->sizes, entry) == 0){
		temp = heap->small_heap;
	}
	else{
		temp = heap;
	}

	while((entry = get_entry_from_addr(temp, (uint32_t) addr)) >= 1024) {
		temp = temp->next;
		if(temp == NULL) {
			temp = heap->small_heap;
			heap = temp;
			if(temp == NULL)
				return;
		}
	}
	heap = temp;

	int i, size = get_size(heap->sizes,entry);
	// printf("In Free Entry: %d, Size %d, from heap 0x%#x w/ map 0x%#x\n", entry, size, heap, map(heap));
	for(i=0; i<size; i++) {

		free(map(heap), entry+i);

	}
	// printf("Finished Free\n");
	// while(1);
}


// int large_count = 0;

int large_heap_init(heap_t * heap) {
	// printf("New Large Heap %d from heap 0x%#x w/ phys: 0x%#x\n", large_count, heap, heap->phys_mem);
	// large_count++;
	// print_mm(map(heap),0, 5);
	// while(1);
	heap_t * new_heap = heap_setup(heap->phys_mem, NULL, heap->flags);
	if(new_heap == NULL)
		return -1;

	uint32_t page_size = BIGGEST_HEAP / TABLESIZE;
	new_heap->small_heap = small_heap_init(new_heap->flags, map(new_heap), page_size);

	heap_t * temp1;
	heap_t * temp2;
	temp1 = heap->small_heap;
	temp2 = new_heap->small_heap;
	// int c1=0, c2=0;
	while((temp1 != NULL) && (temp2 != NULL)) {
		while(temp1->next != NULL) {
			temp1 = temp1->next;
		}
		// temp2->next = temp1->next;
		temp1->next = temp2;
		temp1 = temp1->small_heap;
		temp2 = temp2->small_heap;
	}

	new_heap->next = heap->next;
	heap->next = new_heap;
	// for(i=0; i<TABLESIZE; i++) {
	// 	if((heap->heap_table.table->table[i] & ADDR_ENTRY) != (i * heap->heap_table.size + heap->heap_table.start)) {
	// 		printf("Problem old table end %d\n", i);
	// 		while(1);
	// 	}
	// }
	return 0;
}

heap_t * small_heap_init(uint32_t flags, mem_map_t * mem, uint32_t bigger_page_size) {
	// printf("Small Heap init\n");
	int i;
	uint32_t heap_flags = (SET_P) | (SET_R);
	uint32_t map_size = (bigger_page_size * HEAP_RATIO) / HEAP_ALLOC_NUM;
	// printf("Map Size  0x%#x\n", map_size);
	if(map_size < SMALLEST_AMOUNT)
		return NULL;

	// Should get ceiling
	// Careful when changing defined numbers
	int num_pages = (map_size * KILO) / (mem->size);

	// Get page for heap
	// Reuse this variable after assigning the heap to it
	uint32_t virt_addr = get(mem, heap_flags);
	// printf("Small Heap Virt Addr 0x%#x\n", virt_addr);
	if(virt_addr == -1)
		return NULL;

	heap_t * small = (heap_t *) virt_addr;

	small->next = NULL;
	// Small heap gets space from map of big heap
	small->phys_mem = mem;
	small->flags = flags;
	// Set up area that small heap manages. Doesnt have a table
	if((virt_addr = get_continuous(mem, flags, num_pages)) == -1){
		printf("Small Heap Allocation Failed\n");
		return NULL;
	}
	int phys_entry = (virt_addr - mem->start)/mem->size;
	//printf("mm init\n");
	mm_init( map(small), NULL, virt_addr, map_size, (mem->table->table[phys_entry] & ADDR_ENTRY), map_size );
	// Initilize open maps
	for(i = 0; i < (NUM_MAPS); i++) {
		//mm_init( &(small->maps[i]), NULL, 0, 0 );
		queue_push(small->open_maps, i);
	}
	for(i=0; i<171; i++) {
		small->sizes[i] = 0;
	}

	// Recursive call ends when map_size < SMALLEST_AMOUNT
	// Use same physv memory for each heap
	// printf("Small 0x%#x\n", small);
	small->small_heap = small_heap_init(flags, mem, map_size);
	// small->small_heap =NULL;
	return small;
}

int new_heap(heap_t * heap) {
	uint32_t heap_flags = (SET_P) | (SET_R);
	uint32_t addr;
	int i;
	// printf("New Heap size %d from 0x%#x\n", heap->heap_table.size, heap->phys_mem);
	// different allocations for small heaps and the biggest heap
	// since the biggest heap only requires one page for itself and
	// the memory it maps
	if(small(heap)) {
		// Page for heap
		// printf("New small heap \n");
		addr = get(heap->phys_mem, heap_flags);
		if(addr == -1){
			// printf("Heap S 0x%#x from 0x%#x failed\n", heap->heap_table.size, heap);
			heap_t* cur_heap = heap->next;
			while(cur_heap != NULL) {
				if(cur_heap->phys_mem != heap->phys_mem) {
					return new_heap(cur_heap);
				}
				cur_heap = cur_heap->next;
			}
			return -1;
		}
		// printf("Addr: 0x%#x\n", addr);

		heap_t * new = (heap_t *) addr;
		int num_pages = (heap->heap_table.size * KILO4) / (heap->phys_mem->size);
		if((addr = get_continuous(heap->phys_mem, heap->flags, num_pages)) == -1) {
			uint32_t entry = (((uint32_t) new) - heap->phys_mem->start)/ (heap->phys_mem->size); 
			free(heap->phys_mem, entry);
			// printf("New Heap Size 0x%#x from 0x%#x failed\n", heap->heap_table.size, heap);
			heap_t* cur_heap = heap->next;
			while(cur_heap != NULL) {
				if(cur_heap->phys_mem != heap->phys_mem) {
					return new_heap(cur_heap);
				}
				cur_heap = cur_heap->next;
			}
			return -1;
		}
		// printf("Addr: 0x%#x\n", addr);
		//uint32_t phys_entry = addr >> ENTRY_OFFSET; 

		mm_init(map(new), NULL, addr, heap->heap_table.size, 0,0);

		new->next = heap->next;
		new->small_heap = NULL;
		new->phys_mem = heap->phys_mem;
		new->flags = heap->flags;
		for(i = 0; i < (NUM_MAPS); i++) {
			//mm_init( &(small->maps[i]), NULL, 0, 0 );
			queue_push(new->open_maps, i);
		}
		for(i=0; i<171; i++) {
			new->sizes[i] = 0;
		}
		//print_mm(heap->phys_mem, 31, 1);

		heap->next = new;
		// printf("New heap %d at 0x%#x linked to 0x%#x\n", new_heap->heap_table.size, new_heap, heap);
		// printf("start %d size 0x%d used %d\n", new_heap->phys_mem->size, new_heap->phys_mem->start, new_heap->phys_mem->used);
		return 0;
	}
	// Allocate big heap
	if(large_heap_init(heap) == -1){
		printf("Out of ram\n");
		return -1;
	}
	return 0;
}

int new_smallest_heap(heap_t * heap) {
	// printf("New Smallest Heap\n");
	if(heap->small_heap != NULL) {
		if(new_smallest_heap(heap->small_heap) < 0) {
			if(!small(heap) && (heap->small_heap != NULL) && (heap->next != NULL)) {
				new_smallest_heap(heap->next);
			}
			return new_heap(heap);
		}
		return 0;
	}
	if(!small(heap)) {
		heap->small_heap = small_heap_init(heap->flags, map(heap), heap->heap_table.size);
	}
	return new_heap(heap);
}

uint32_t get_page(heap_t * heap, uint32_t flags) {
	if(heap == NULL)
		return -1;
	uint32_t addr;
	// Check for falure
	// and search Linked list of heaps
	while((addr = get(map(heap), flags)) == -1) {
		if(heap->next == NULL) {
			if(new_heap(heap) < 0)
				return -1;
		}
		heap = heap->next;
	}
	return addr;
}

uint32_t get_mult_page(heap_t * heap, uint32_t flags, int amount) {
	if(heap == NULL || amount <= 0)
		return -1;
	if(amount == 1)
		return get_page(heap,flags);
	uint32_t addr;
	// Check for falure
	// and search Linked list of heaps
	while((addr = get_continuous(map(heap), flags, amount)) == -1) {
		if(heap->next == NULL) {
			if(new_heap(heap) == -1)
				return -1;
		}
		heap = heap->next;
	}
	
	return addr;
}

void free_page(heap_t * heap, uint32_t addr) {

	if(heap == NULL)
		return;

	uint32_t phys_page = (lin_to_phys(addr) >> ENTRY_OFFSET);


	if(phys_page == -1)
		return;


	while((phys_page < (table_start(heap) >> ENTRY_OFFSET)) || (phys_page > ((table_start(heap) >> ENTRY_OFFSET) + 4))) {
		if(heap->next == NULL)
			return;

		heap = heap->next;
	}

	free(map(heap), (addr - heap->heap_table.start)/(heap->heap_table.size));
}

/*
uint32_t allocate_mem(heap_t * heap, uint32_t nbytes) {
	if(heap == NULL)
		return -1;

	num_pages = (nbytes % KILO4 == 0) ? (nbytes/KILO4) : (nbytes/KILO4 + 1);
	if(get_continuous(&(heap->this_heap), num_pages) == -1) {
		heap->next = new_heap(heap);
		allocate_mem()
	}

}
*/
// int map_count = 0;
// Returns an uninitialized memory map
mem_map_t * get_map(heap_t * heap) {
	// Need to end recursion
	if(heap == NULL){
		//printf("Heap is NULL\n");
		return NULL;
	}

	heap_t * cur_heap = heap;
	while((queue_empty(cur_heap->open_maps))) {
		// printf("Queu Empty from heap at 0x%#x with size 0x%#x\n", heap, heap->heap_table.size);
		if(cur_heap->next == NULL) {
			// Call on smaller heap
			return get_map(heap->small_heap);
		}
		cur_heap = cur_heap->next;		
	}
	// map_count++;
	mem_map_t * ret = &(cur_heap->maps[queue_pop(cur_heap->open_maps)]);
	// printf("Found map num %d at 0x%#x\n", map_count, ret);
	return ret;
}

mem_map_t * get_table(heap_t * heap, uint32_t start, uint32_t size) {
	if(heap == NULL)
		return NULL;

	//printf("Got page for table\n");

	mem_map_t * ret = get_map(heap);
	// printf("Got map at 0x%#x\n", ret);
	if(ret == NULL) {
		// Create new smallest heap and recall for guarenteed success unless there
		// is no more physical memory
		if(new_smallest_heap(heap) < 0)
			return NULL;

		ret = get_map(heap);
	}

	uint32_t table_addr = get_page(heap, heap->flags);
	if(table_addr == -1)
		return NULL;
	// int temp = (table_addr - heap->heap_table.start) / heap->heap_table.size;
	// printf("Table entry %d with flags 0x%#x\n", temp, heap->heap_table.table->table[temp]);
	mm_init(ret, (table_t *) table_addr, start, size, start, size);
	//printf("Mapped table\n");
	//ret->table = (table_t *) table_addr;
	return ret;
}

// This will just free the map and not any table associated with it
void free_map(heap_t * heap, mem_map_t * mem_map) {
	// Need to end recursion
	if(heap == NULL)
		return;

	int i;
	heap_t * cur_heap = heap;
	while(cur_heap != NULL) {
		for(i=0; i<NUM_MAPS; i++) {
			if(&(heap->maps[i]) == mem_map) {
				// We found the map at entry i
				queue_push(heap->open_maps, i);
				// Make sure table is no longer associated to map
				mem_map->table = NULL;
				return;
			}
		}
		cur_heap = cur_heap->next;
	}
	// Since we did not find it in this heap we go to smaller heaps
	free_map(heap->small_heap, mem_map);
}

void mm_init(mem_map_t * mem_map, table_t * tab, uint32_t virt_start, uint32_t virt_size, uint32_t phys_start, uint32_t phys_size) {
	int i=0;
	mem_map->table = tab;
	queue_init(mem_map->quicklist);
	//printf("Mem Addr 0x%#x.  %#x Phys Start 0x%#x.  Size 0x%#x\n", mem_map, virt_start, phys_start, virt_size);
	//printf("Start %d.  End %d\n", mem_map->quicklist.start, mem_map->quicklist.end );
	while(!queue_full(mem_map->quicklist)){
		//printf("Start %d.  End %d\n", mem_map->quicklist.start, mem_map->quicklist.end );
		queue_push(mem_map->quicklist, i);
		i++;
	}
	mem_map->recent = i;
	for(i=0; i<33; i++)
		mem_map->mm[i] = 0;

	mem_map->used = 0;
	mem_map->start = virt_start;
	mem_map->size = virt_size;

	//  Initialize table
	table_init(mem_map->table, phys_start, phys_size);
	//printf("Finished mm init\n");
	
}

void table_init(table_t * tab, uint32_t phys_start, uint32_t phys_size) {
	if(tab != NULL){
		int i;
		for(i=0; i<TABLESIZE; i++) {
			tab->table[i] = i * phys_size + phys_start;
		}
	}
}

void set_map(mem_map_t * mem_map, uint32_t virt_addr, uint32_t phys_addr, uint32_t flags) {
	//printf("Set MAP MAP SET\n");
	if((mem_map == NULL) || (mem_map->table == NULL))
		return;
	//printf("AFTER\n");
	int entry = (virt_addr - mem_map->start)/ (mem_map->size);

	//printf("Map 0x%#x to 0x%#x at entry %d\n", phys_addr, virt_addr, entry);
	mem_map->table->table[entry] = phys_addr;
	set(mem_map, entry, flags);
}

void set(mem_map_t * mem_map, uint32_t entry, uint32_t flags) {
	if(!exists(mem_map, entry))
		mem_map->used++;
	// Mark entry as used
	mem_map->mm[mm_idx(entry)] |= (1 << mm_off(entry));
	// Set group as not completely open
	mem_map->mm[32] |= (1 << mm_idx(entry));

	if(mem_map->table != NULL)
		mem_map->table->table[entry] |= flags;
}

void free(mem_map_t * mem_map, uint32_t entry) {
	if(mem_map->used == 0)
		return;

	// printf("Free Page %d\n", entry);
	if(exists(mem_map, entry))
		mem_map->used--;
	// Mark entry as unused
	mem_map->mm[mm_idx(entry)] &= ~(1 << mm_off(entry));

	// Check if we should update the index for continuous allocation
	if(mem_map->mm[mm_idx(entry)] == 0)
		mem_map->mm[32] &= ~(1 << mm_idx(entry));

	if(queue_full(mem_map->quicklist))
		mem_map->recent = mem_map->recent > entry ? entry : mem_map->recent;
	else
		queue_push(mem_map->quicklist, entry); 

	if(mem_map->table != NULL) {
		// if(entry == 10){
		// 	printf("Erease table: 0x%#x\n", mem_map->table->table[entry]);
		// 	mem_map->table->table[entry] = 0x10063;
		// 	return;
		// }
		mem_map->table->table[entry] &= ADDR_ENTRY;
	}
}

uint32_t get_entry(mem_map_t * mem_map, uint32_t flags) {
	if(!queue_empty(mem_map->quicklist)){
		int entry = queue_pop(mem_map->quicklist);

		// Check if page is already allocated
		while(exists(mem_map, entry)) {
			if(queue_empty(mem_map->quicklist)) {
				if(refill_queue(mem_map) == -1)
					return -1;
			}
			entry = queue_pop(mem_map->quicklist);
		}

		// Mark entry as used
		mem_map->used++;
		mem_map->mm[mm_idx(entry)] |= (1 << mm_off(entry));
		mem_map->mm[32] |= (1 << mm_idx(entry));
		// If a table exists mark flags
		if(mem_map->table != NULL) {
			mem_map->table->table[entry] |= flags;
		}
		// Claculate the address and return it if a table doesnt exist
		return entry;
	}
	else{
		if(refill_queue(mem_map) == -1)
			return -1;
		// This is a non-recursive call since we refill the queue
		return get_entry(mem_map, flags);
	}
	return -1;
}

uint32_t get(mem_map_t * mem_map, uint32_t flags) {
	if(mem_map->used >= TABLESIZE)
		return -1;
	// int i, count=0;
	// for(i=0; i<TABLESIZE; i++) {
	// 	if(exists(mem_map,i))
	// 		count++;
	// }
	// if(count != mem_map->used) {
	// 	printf("Used: %d,  Actual: %d from 0x%#x\n",mem_map->used, count, mem_map);
	// }
	uint32_t entry = get_entry(mem_map, flags);
		

	// printf("Get Entry %d from 0x%#x with used %d\n", entry, mem_map, heap->heap_table.size);
	// printf("Start : %#x  Size:  %#x  Ret %#x\n",mem_map->start, mem_map->size, entry * mem_map->size + mem_map->start );
	return entry * mem_map->size + mem_map->start;
}

uint32_t get_continuous(mem_map_t * mem_map, uint32_t flags, int amount) {
	if(amount <= 0)
		return -1;
	if(amount == 1)
		return get(mem_map, flags);

	// printf("Get %d Pages from 0x%#x\n", amount, mem_map);

	if(mem_map->used >= (TABLESIZE - amount))
		return -1;

	if((mem_map->mm[32] == FULL) | (amount > BITMAP_WIDTH)){
		return look_slow(mem_map, flags, amount);
	}

	uint32_t start_idx, mask = 1;
	for(start_idx=0; start_idx < 32; start_idx++, mask <<= 1){
		if(!(mem_map->mm[32] & mask))
			break;
	}
	// Check if no more memory is available
	if(start_idx == 32)
		return -1;

	// Mark what we allocated
	start_idx = start_idx*32;
	int i;
	// printf("Start idx:  %d     Amoutn:  %d\n", start_idx, amount);
	for(i=0; i<amount; i++) {
		set(mem_map, i+start_idx, flags);
	}

	// printf("Get Entry %d from 0x%#x with used %d\n", start_idx, mem_map, heap->heap_table.size);

	return (start_idx*mem_map->size) + mem_map->start;
}

uint32_t look_slow(mem_map_t * mem_map, uint32_t flags, int amount) {
	int i, count=0;
	for(i=0; i<TABLESIZE; i++) {
		if(!exists(mem_map,i))
			count++;
		else
			count = 0;
		
		if(count == amount)
			break;
	}
	if(i == TABLESIZE)
		return -1;
	// printf("Found %d at start %d\n", amount, (i-amount));
	// print_mm(mem_map,0,5);
	int x;
	for(x=0; x<amount; x++) {
		set(mem_map, i+x - amount + 1, flags);
	}
	// printf("Start at 0x%#x\n",(i - amount)*mem_map->size + mem_map->start);
	// while(1);
	return (i - amount + 1)*mem_map->size + mem_map->start;
}

int refill_queue(mem_map_t * mem_map){
	int ret =  -1;
	while(!queue_full(mem_map->quicklist) && (mem_map->recent < TABLESIZE)) {
		if(exists(mem_map, mem_map->recent)){
			mem_map->recent++;
		}
		else{
			ret = 0;
			queue_push(mem_map->quicklist, mem_map->recent);
			mem_map->recent++;
		}
	}
	return ret;
}

void print_mm(mem_map_t * mem_map, int start, int entries){
	int i,j;
	uint32_t mask;
	for(i=start;i<(start + entries);i++){
		mask = 1;
		for(j=0; j<32; j++, mask<<=1){
			printf("%d ", ((mem_map->mm[i] & mask) >> j));
		}
		printf("\n");
	}
	printf("DIR:  ");
	mask = 1;
	for(j=0; j<32; j++, mask<<=1){
		printf("%d ", ((mem_map->mm[32] & mask) >> j));
	}
	printf("\n");
}

