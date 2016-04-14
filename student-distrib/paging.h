#ifndef PAGING_H
#define PAGING_H

#define KPriv 0
#define UPriv 3

// Defines for Linear Addr
#define DIRECTORY 0xFFC00000
#define TABLE 0x003FF000
#define OFFSET 0x00000FFF
#define OFFSET_E 0x003FFFFF
#define DIRECTORY_OFFSET 22
#define TABLE_OFFSET 12
#define ksize 0x400000
#define kb 0x1000

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

#include "lib.h"
#include "x86_desc.h"
#include "files.h"
#include "errors.h"
#include "queue.h"


typedef struct dir_t {
	int pagedir[tablesize];

} dir_t;
extern uint32_t getaddr(uint8_t process);
extern void pageenable();
extern void pageinit();
extern void paging_test();
extern int32_t newtask( uint8_t process);
uint32_t lin_to_phys(uint32_t addr);

uint32_t allocate_big_page(uint32_t flags, uint32_t vir_addr);
uint32_t get_any_page();
void map_epage(uint32_t flags, uint32_t phys_addr, uint32_t vir_addr);
void alloc_prog();

void print_mm(int entries);
void deallocate_big_page(uint32_t phys_addr);


#endif
