#ifndef SYSCALLS_H
#define SYSCALLS_H


#include "memory_map.h"


#define ENTRYPT 24
#define PROGADDR 0x08048000
#define PCBALIGN 0xFFFFE000
#define VIRT128  0x08000000

#define PCB_PAGE_NUM 1

#define NUM_FILES 8

#define NUM_PROGRAMS 25

void programs_init();


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

typedef struct pcb_list_t pcb_list_t;
typedef struct pcb_t pcb_t;

// Circular linked list
struct pcb_list_t {
	int8_t process;
	pcb_t * next;
};

// Define structure for pcb
struct pcb_t{

	uint8_t names[NUM_FILES][32];
	fdescriptor_t fdescs[NUM_FILES];
	uint32_t kernel_sp; // Points to the top of the kernel stack
	uint32_t kernel_bp; // Points to the base of the kernel stack
	uint32_t malloc_bp_save;
	uint32_t kernel_ss;
	uint32_t espsave;
	uint32_t user_sp;  // Points to the top of the user stack
	uint32_t user_bp; // Points to the base of the user stack
	int process; // Indicates number of process currently being executed
	int term;	
	int8_t argsave[1024];
	char temp[32];
	//int32_t savestatus;
	// Use active pcb to search for all others
	pcb_t * next;
	pcb_t * parent_process;
	pcb_t * child;
	mem_map_t * dir;
	uint8_t * user_vid_mem;
	heap_t * heap;
};	



// function to execute user code
/*
switch_to:  Save state of running program and switch to next program
			pcb is NULL switch to next program on LL
			Remap user video space so the old program doesnt write to the screen
			set page directory of new program
			set kernel stack and tss for the new process
			return to wherever the process was switched from
	Inputs: 
		pcb: Program to switch to
	Returns:
*/
void switch_to(pcb_t * pcb);

// Get pogram running on term or the running program if term == -1
pcb_t * get_prog(int term);

/*
execute:  Execute the cmd.
			Save state of running program
			Parse through cmd to get arguments and name of new program to run
			Get new PID and allocate new pogram
			Set program parent and running program child.  Add Program to LL
			Copy program data from filesystem
			Open stdin and stdout
			Push artifical iret onto stack and iret
			Set up return position for halt
	Inputs: 
		cmd: Command to be execute
	Returns:
		The halt status or -1 if executing program fails
*/
int32_t execute(const int8_t * cmd);


/*
halt:  halt the running process.  Start the parent process
			If process doesnt have parent re-execute shell
			Delete process from LL and add PID to queue
			Set page and stack for parent and jump to haltreturn in execute to return 
				the status that the child halted with
	Inputs: 
		status: Status of halt
	Returns:
		Doesnt really return,  Just jumps to execute return
*/
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

