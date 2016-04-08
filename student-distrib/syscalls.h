#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "files.h"
#include "x86_desc.h"
#include "paging.h"
#include "types.h"
#include "terminal.h"
#include "rtc.h"
#include "syscallhandle.h"


#define ENTRYPT 24
#define PROGADDR 0x08048000
#define PCBALIGN 0xFFFFE000


typedef struct fdescriptor_t {
	uint32_t *jumptable;
	int32_t inode;
	int32_t position; //where in file
	int32_t inuse; //0 if not in use , 1 if in use

} fdescriptor_t;
// Define structure for pcb
typedef struct pcb_t{

	uint8_t names[8][32];
	fdescriptor_t fdescs[8];
	uint32_t kernel_sp; // Points to the top of the kernel stack
	uint32_t kernel_bp; // Points to the base of the kernel stack
	uint32_t user_sp;  // Points to the top of the user stack
	uint32_t user_bp; // Points to the base of the user stack
	uint8_t process; // Indicates number of process currently being executed
	uint8_t parent_process; 	
	int8_t argsave[1024];
}pcb_t;

// function to execute user code
int32_t execute(const int8_t * fname);
int32_t halt(int8_t done);
#endif
