#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "files.h"
#include "x86_desc.h"
#include "paging.h"
#include "types.h"
#include "terminal.h"
#include "rtc.h"
#include "syscallhandle.h"
#include "lib.h"


#define ENTRYPT 24
#define PROGADDR 0x08048000
#define PCBALIGN 0xFFFFE000
#define VIRT128  0x08000000
#define MB8 0x800000
#define KB8 0x2000


typedef struct fops {
	int32_t (*open)(const uint8_t* filename);
	int32_t (*close)(int32_t fd);
	int32_t (*read)(int8_t * filename, uint32_t offset, void* buf, int32_t nbytes );
	int32_t (*write)(int32_t fd, const void* buf, int32_t nbytes);
} fops_t;

typedef struct fdescriptor_t {
	fops_t *jumptable;
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
	uint32_t kernel_ss;
	uint32_t espsave;
	uint32_t user_sp;  // Points to the top of the user stack
	uint32_t user_bp; // Points to the base of the user stack
	int8_t process; // Indicates number of process currently being executed
	int8_t parent_process; 	
	int8_t argsave[1024];
	int32_t savestatus;
	uint32_t term;
	uint32_t haschild;
	uint32_t oldesp; // Points to the top of the kernel stack
	uint32_t oldebp;
	uint32_t oldesp0;
	uint32_t oldss0;
}pcb_t;

// function to execute user code
int32_t execute(const int8_t * cmd);
int32_t halt(int8_t status);
void stdinopen (int32_t fd);
void stdoutopen (int32_t fd);
int32_t open(const uint8_t * filename);
int32_t close( int32_t fd);
int32_t read(int32_t fd, void *buf, int32_t nbytes);
int32_t write(int32_t fd, void *buf, int32_t nbytes);
int startup();
int getrunning(int term);
int nowrunning();

uint8_t getcurrent();

void setcurrent(uint8_t set );

uint32_t getkstack();

void setkstack(uint32_t set);
void setpdaddr(uint32_t set);
#endif
