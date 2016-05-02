#include "syscalls.h"
#include "files.h"
#include "x86_desc.h"
#include "paging.h"
#include "types.h"
#include "terminal.h"
#include "rtc.h"
#include "syscallhandle.h"
#include "stack.h"



#define maxfd 7
#define MB8 0x800000
#define EMPTYMASK 0x7F
#define KB8 0x2000
#define MASK 0x80
uint32_t kstackbottom;
uint8_t running= 0;
uint8_t current=0;
uint32_t pdaddr;
int run[3];

uint32_t savestatus;
int setup=0;

stack_t open_processes;
pcb_t * running_process = NULL;

//this does nothing
int32_t emptyfunc(){
	return 0;
}

void programs_init() {
	int i=0;
	running_process = NULL;
	stack_init(open_processes);
	for(i=25; i >= 0; i--) {
		stack_push(open_processes, i);
	}
	for(i=0; i<3; i++) {
		run[i] = 0;
	}
	// cli();
	// setup = 1;
	// setactiveterm(1);
	// execute("shell");
	// setactiveterm(2);
	// execute("shell");
	// setup = 0;
	// switchterm(0);
	// while(1);
}


void switch_to(pcb_t * pcb) {
	
	if(running_process == NULL)
		return;

	// Save state of running program
	asm volatile("movl %%ebp, %0":"=g"(running_process->kernel_bp));
	asm volatile("movl %%esp, %0":"=g"(running_process->espsave));
	running_process->kernel_sp=tss.esp0;
	// running_process->kernel_ss=tss.ss0;
	// Remap user video memory
	int terminal = getactiveterm();
	if(running_process->term != terminal) {
		remap_user_video(running_process, get_vid_buf_addr(running_process->term));
	}

	// Get next pcb
	//cli();
	if(pcb == NULL) {
		pcb = running_process->next;
		while(pcb->child != NULL) {
			pcb = pcb->child;
		}
	}

	if(pcb == running_process)
		return;

	// Set video mem on new process
	if(pcb->term == terminal){
		remap_user_video(pcb, VIDEO);
	}
	
	// printf("\nSwitch from %s num %d stack 0x%#x base 0x%#x esp0 0x%#x\n", running_process->temp, running_process->process, running_process->espsave, running_process->kernel_bp, running_process->kernel_sp);
	// printf("To %s num %d stack 0x%#x base 0x%#x esp0 0x%#x\n", pcb->temp, pcb->process, pcb->espsave, pcb->kernel_bp, pcb->kernel_sp);
	// Set running process
	running_process = pcb;
	page_set(running_process->dir);
	// Set Saved State
	//sti();
	tss.esp0= running_process->kernel_sp; 
	// tss.ss0= running_process->kernel_ss;
	asm volatile ("movl %0, %%ebp     ;"
				"movl %1, %%esp     ;"
				"leave				;"
				"ret 				;"
				::"g"(running_process->kernel_bp), "g"(running_process->espsave));
	return;
}

pcb_t * get_prog(int term) {
	if(term == -1) {
		return running_process;
	}
	if((term < 0) || (term >= 3))
		return NULL;
	pcb_t * ret = running_process;
	if(ret == NULL)
		return ret;
	// Search for process with same terminal as the one we switch to
	while(ret->term != term) {
		if((ret = ret->next) == running_process)
			return NULL;
	}
	// Search for the smallest child of that process
	while(ret->child != NULL) {
		ret = ret->child;
	}
	return ret;
}

//all the fops tables
fops_t stdouttable = {
	.open = NULL,
	.close = NULL,
	.write = (void *)terminal_write,
	.read = NULL
};

fops_t stdintable = {
	.open = NULL,
	.close = NULL,
	.write = NULL,
	.read = (void *)terminal_read
};

fops_t rtctable = {
	.open = (void *) (rtc_open),
	.close = (void *) (rtc_close),
	.write = (void *) (rtc_write),
	.read = (void *) (rtc_read)
};

fops_t filetable = {
	.open = (void *) (fileopen),
	.close = (void *) (fileclose),
	.write = (void *) (filewrite),
	.read = (void *) (fileread)
};

fops_t dirtable = {
	.open = (void *) (diropen),
	.close = (void *) (dirclose),
	.write = (void *) (dirwrite),
	.read = (void *) (dirread)
};

//input command
//executes the executable file given in command
			 //thanks xi391

int32_t execute(const int8_t * cmd){
	// Save state of running program
	cli();
	if((running_process != NULL) && (!setup)) {
		asm volatile("movl %%ebp, %0":"=g"(running_process->kernel_bp));
		asm volatile("movl %%esp, %0":"=g"(running_process->espsave));
		running_process->kernel_sp=tss.esp0;
		running_process->kernel_ss=tss.ss0;
	}

	uint8_t buf[4];
	int8_t fname[32] = {[0 ... 31] = 0};
	uint32_t entrypoint;
	uint8_t exnumbers[4]= {0x7F, 0x45, 0x4C, 0x46};
	uint32_t spaceflag;
	uint32_t namelength;
	uint8_t argbuf[1024];
	int i;


	entrypoint=0;
	spaceflag=0;
	namelength=0;

// printf("Start execute\n");
	// Checks if file is valid
	if(cmd == NULL){
		return -1;
	}
//get name of file
	for (i=0; cmd[i] != '\0'; i++){
		if (cmd[i]== ' ' && spaceflag==0){
			namelength=i;
			fname[i]= '\0';
			spaceflag=1;
		}
		else if (spaceflag){
			argbuf[i-namelength-1]= cmd[i];
		}
		else{
			if (i>=32 && spaceflag==0){
				sti();
				return -1;
			}
			fname[i]= cmd[i];
		}

	}
	argbuf[i-namelength-1]= '\0';
// printf ("parsed\n");


	if(fsread(fname, 0, (uint8_t *)buf, 4) == -1){ // Checks for executable image
		sti();
		return -1;
	}
	if(strncmp((int8_t *)buf, (int8_t *)exnumbers, 4)){
		sti();
		return -1;
	}

	// Get process num
	if(stack_empty(open_processes)){
		sti();
		return -1;
	}
	int PID = stack_pop(open_processes);

//read to get entrypoint
	if(fsread(fname, ENTRYPT, (uint8_t *)buf, 4)==-1){
		sti();
		return -1;
	}

	for(i=0; i<4; i++){
		//buf[i]= buf[i] & 0xFF;
		
		entrypoint |= (buf[i] << i*(maxfd+1)); //set entrypoint
		
	}
	// printf("Execute Entrypoint 0x%#x\n", entrypoint);

//set up paging
	
	// if( newtask(openprocess)==-1)
	// 	return -1;
	

	// printf("Before Alloc prog\n");
	pcb_t * pcb =  alloc_prog(PID);
	if(pcb == NULL){
		sti();
		return -1;
	}

	i = 0;
	while(fname[i] != '\0') {
		pcb->temp[i] = fname[i];
		i++;
	}
	pcb->temp[i] = fname[i];

	int	terminal=getactiveterm();
	// Set running process
	if(run[terminal]<1) {
		pcb->parent_process = NULL;
		pcb->term = terminal;

	}
	else {
		if(running_process->term == terminal) {
			pcb->parent_process = running_process;
			running_process->child = pcb;
			pcb->term = terminal;
		}
		else {
			pcb->parent_process = NULL;
		}
	}

	// Add program to Circular Linked List
	if(running_process != NULL) {
		pcb->next = running_process->next;
		running_process->next = pcb;
	}
	else{
		pcb->next = pcb;
	}

	
	pcb->child = NULL;
	running_process = pcb;
	run[terminal]++;
	sti();

	strcpy(pcb->argsave, (int8_t *)argbuf);
	// printf("ALLOCATED \n");
	// while(1);
	fstomem(fname, PROGADDR);

//save ebp, esp, ss
	//uint32_t esp;
	
	//pcb->kernel_sp=tss.esp0;

	//uint32_t ebp;
	//asm volatile("movl %%ebp, %0":"=g"(ebp));
	//pcb->kernel_bp=ebp;
	//pcb->kernel_ss=tss.ss0;


//check if 0th open process
	// if( running ==MASK){
	// 	pcb->parent_process= -1;
	// }
	// else{
	// 	pcb->parent_process= ((pcb_t *)((uint32_t)&esp & PCBALIGN))->process;
	// }

	// pcb->process= openprocess; 


//clear file descs
	// for (i=0; i<maxfd+1; i++){
	// 	pcb->fdescs[i].position=0;
	// 	pcb->fdescs[i].inuse=0;
	// 	pcb->fdescs[i].inode=0;
	// }

	//set tss

	//tss.esp0= MB8-KB8*openprocess-4; //need openprocess number
	//tss.ss0=KERNEL_DS;
	//kstackbottom=tss.esp0;

	//open stdin, stdout
	 open((uint8_t *) "stdin");
	 open((uint8_t *) "stdout");
	 // set_c();

	// Set stack to new process
	// tss.esp0= running_process->kernel_sp; 
	// tss.ss0= running_process->kernel_ss;
	// asm volatile ("movl %0, %%ebp     ;"
	// 			"movl %1, %%esp     ;"
	// 			::"g"(running_process->kernel_bp), "g"(running_process->espsave));

	gotouser(entrypoint);
	asm volatile ("haltreturn: ");

// asm volatile ("haltreturn:		;"
// 				"movl %0, %%eax ;"
// 				"leave			;"
// 				"ret 			;"
// 				::"g"(savestatus) );

	// uint32_t temp;
	// temp = ((pcb_t *)((uint32_t)&temp & PCBALIGN))->savestatus;
	// return  temp;
	return savestatus;

}


//intputs the status
//halts the program and goes back to previous process
int32_t halt(int8_t status){
	cli();
	int i;
	uint8_t buf[4];
	pcb_t * pcb = running_process;
	// if(pcb->parent_process != NULL)
	// 	printf("c:  %d  Halt %s num %d go to %s num %d\n",get_c(), pcb->temp, pcb->process, pcb->parent_process->temp, pcb->parent_process->process);

	//don't want to close final shell, so restart it just to be sure

	if (pcb->parent_process==NULL){
		// sti();
		// running_process = NULL;
		// execute("shell");
		uint32_t entrypoint = 0;
		if(fsread((const int8_t *)("shell"), ENTRYPT, (uint8_t *)buf, 4)==-1){
			return -1;
		}
		for(i=0; i<4; i++){
			entrypoint |= (buf[i] << i*(maxfd+1)); //set entrypoint
		}
		// printf("Halt Entrypoint 0x%#x\n", entrypoint);
		gotouser(entrypoint);
	}

	//mark that process is done and available
	// printf("Pushing value from 0x%#x on stack\n", pcb);
	stack_push(open_processes, pcb->process);
	// printf("Closing Files\n");
	for (i=0; i<NUM_FILES; i++){
		close(i);
	}
	run[pcb->term]--;


// 	pdaddr= getaddr(pcb->parent_process);
// //set paging to new page directory
// 	asm volatile ("				\
// 		movl %0, %%cr3 \n\
// 		movl %%cr4, %%eax	\n\
// 		orl $0x90, %%eax	\n\
// 		movl %%eax, %%cr4	\n\
// 		movl %%cr0, %%eax \n\
// 		orl $0x80000000, %%eax	\n\
// 		movl %%eax, %%cr0"
// 		:
// 		:"r" (pdaddr)
// 		:"%eax"
// 		);

//set tss
	//kstackbottom=tss.esp0;
	
	// Remove program From Linked List
	pcb_t * temp = pcb;
	while(temp->next != pcb)
		temp = temp->next;
	temp->next = pcb->next;


	//pcb->savestatus= status;
	savestatus = 0;
	savestatus |= status;
	// printf("Halt %d\n", savestatus);
	// switch_to(pcb->parent_process);
	// sti();
	// asm volatile ("jmp haltreturn ;");
	
	running_process = pcb->parent_process;
	running_process->child = NULL;
	page_set(running_process->dir);
	//sti();
	// Set Saved State
	tss.esp0= running_process->kernel_sp; 
	tss.ss0= running_process->kernel_ss;
	sti();
//go bacck to execute
	asm volatile ("movl %0, %%ebp     ;"
				"movl %1, %%esp     ;"
				"jmp haltreturn ;"
				::"g"(running_process->kernel_bp), "g"(running_process->espsave));
	return 0;

}


//inputs: file descriptor
//opens the stdin for the file
void stdinopen (int32_t fd){

	pcb_t * pcb= running_process; //get pcb

	pcb->fdescs[fd].jumptable=&stdintable; //get functions 

	pcb->fdescs[fd].inuse=1;
}


//inputs: file descriptor
//opens the stdout for the file
void stdoutopen (int32_t fd){

	pcb_t * pcb= running_process; //get pcb

	pcb->fdescs[fd].jumptable=&stdouttable; //get functions

	pcb->fdescs[fd].inuse=1;
}

//opens file given by filename
//returns file descriptor or -1 if fail
int32_t open(const uint8_t * filename){

	dentry_t mydentry;
	int i;

	if(*filename=='\0' || filename==NULL) //check if valid
		return -1;

	pcb_t  * pcb= running_process;


	if( strncmp((const int8_t *)filename,(const int8_t *) "stdin", 5) ==0){ //check if stdin
		stdinopen(0);
		return 0;
	}

	if( strncmp((const int8_t *)filename,(const int8_t *) "stdout", 5) ==0){ //check if stdout
		stdoutopen(1);
		return 0;
	}

	if (read_dentry_by_name(filename, &mydentry)==-1) //read the file
		return -1;

	for( i=2; i<8; i++){ //set file desc

		if (pcb->fdescs[i].inuse==0){ //if not in use
			if(mydentry.ftype==RTCTYPE){ //set to rtc
				if(rtc_open()==-1)
					return -1;
				else
					pcb->fdescs[i].jumptable= &rtctable;
			}
		


		else if (mydentry.ftype==DIRTYPE){ //set to dir
			pcb->fdescs[i].jumptable= &dirtable;
		}

		else if (mydentry.ftype==FILETYPE){ //set to file
			pcb->fdescs[i].jumptable= &filetable;
		}


		pcb->fdescs[i].inode=mydentry.inode; //set inode, inuse, filename
		pcb->fdescs[i].inuse=1;
		strcpy((int8_t *)pcb->names[i], (const int8_t *)filename);
		return i;
	
	}
}
printf("Full File Descriptor Arrary\n"); //if fail
return -1;

}

//close inputted file descriptor
//returns based on individual close function
int32_t close( int32_t fd){

	int value;

	pcb_t * pcb= running_process;


	if(fd >maxfd || fd <2 || pcb->fdescs[fd].inuse==0) //check if valid
		return -1;




value=pcb->fdescs[fd].jumptable->close(fd);//call close
//reset descriptor info
	pcb->fdescs[fd].inode=0;
	pcb->fdescs[fd].position=0;
	pcb->fdescs[fd].inuse=0;
	pcb->fdescs[fd].jumptable=NULL;

	return value; //return if success or fail

}

//calls read on fd
//returns successes or -1 if fail
int32_t read(int32_t fd, void *buf, int32_t nbytes){

	sti();

	int successes=0;

	pcb_t * pcb= running_process;


	if(fd >maxfd || fd < 0 || pcb->fdescs[fd].inuse==0 ||buf==NULL || fd==1) //check if valid
		return -1;

	uint8_t * filename= pcb->names[fd];
	uint32_t position= pcb->fdescs[fd].position; //save info


successes += pcb->fdescs[fd].jumptable->read( (int8_t *) filename, position,buf, nbytes); //read
	pcb->fdescs[fd].position += successes;
//printf("%d \n", successes);

	//printf("buffer:");
// for (i=0; i<nbytes; i++){
 	//printf("%s", (uint8_t *) buf);
// }
//printf("%d \n", successes);
	return successes;

}

//writes to fd
//returns # of successes, -1 if fail
int32_t write(int32_t fd, void *buf, int32_t nbytes){
//printf("in write");
	pcb_t * pcb= running_process;

int successes=0;
	if(fd >maxfd || fd < 1 || pcb->fdescs[fd].inuse==0 ||buf==NULL) //check if valid
		return -1;

	
	successes +=pcb->fdescs[fd].jumptable->write(fd, buf, nbytes); //call write

	return successes;

}

int32_t getargs (uint8_t* buf, int32_t nbytes){

	if(nbytes==0 || buf==NULL)
		return -1;



	pcb_t * pcb= running_process;


// printf("pcb buffer:");
// for (i=0; i<nbytes; i++){
// 	printf("%c", pcb->argsave[i]);
// }
// printf("\n");

	if (strlen ((const int8_t*)pcb->argsave)>nbytes)
		return -1;

	strcpy((int8_t*) buf, (const int8_t *)pcb->argsave);

// printf("buffer:");
// for (i=0; i<nbytes; i++){
// 	printf("%c", buf[i]);
// }
// printf("\n");
	return 0;
}


int32_t vidmap (uint8_t ** screen_start){
	if((uint32_t) screen_start < VIRT128 || (uint32_t) screen_start > (VIRT128+0x400000) )
		return -1;
	running_process->user_vid_mem = map_user_vid();
	*screen_start = running_process->user_vid_mem;
	return 0;
// *screen_start= (uint8_t *) 0x100; //sets to user accessible video memory
// 	return 0;
}

int startup(){

// 	uint8_t buf[4];
// 	uint32_t i;
// 	uint32_t j;
// 	uint32_t entrypoint;
	
	
// 	pcb_t * pcb;

// if(fsread((const int8_t *)("shell"), ENTRYPT, (uint8_t *)buf, 4)==-1)
// 		return -1;

// 	for(i=0; i<4; i++){
// 		//buf[i]= buf[i] & 0xFF;
		
// 		entrypoint |= (buf[i] << i*(maxfd+1)); //set entrypoint
		
// 	}
// printf("reached 1\n");
//     for(i=3; i>0; i--) {
// 	if( newtask(i)==-1)
// 		return -1;

// 	fstomem((const int8_t *)("shell"), PROGADDR);

// 	pcb_t * pcb= (pcb_t*)(MB8-KB8*(i+1)); //get pcb


// //save ebp, esp, ss
// 	uint32_t esp;
	
// 	pcb->kernel_sp=tss.esp0;

// 	uint32_t ebp;
// 	asm volatile("movl %%ebp, %0":"=g"(ebp));
// 	pcb->kernel_bp=ebp;
// 	pcb->kernel_ss=tss.ss0;

// 	pcb->parent_process= -1;
// 		pcb->term=i-1;
// 		pcb->process=i;
// 		pcb->haschild=0;
// 		for (j=0; j<maxfd+1; j++){
// 		pcb->fdescs->position=0;
// 		pcb->fdescs->inuse=0;
// 		pcb->fdescs->inode=0;
// 	}
// printf("reached 2\n");


// tss.esp0= MB8-KB8*i-4; //need openprocess number
// 	tss.ss0=KERNEL_DS;
// 	kstackbottom=tss.esp0;


// 	 open((uint8_t *) "stdin");
// 	 open((uint8_t *) "stdout");
// 	}
// 	printf("reached 3\n");


// memcpy((char *) VIDBUF0, (char *)VIDEO, KB4);
// 	memcpy((char *) VIDBUF1, (char *)VIDEO, KB4);
// 	memcpy((char *) VIDBUF2, (char *)VIDEO, KB4);
// running |= 0x70;
// printf("reached 4\n");
// 	 asm volatile("movl %%esp, %0":"=g"(pcb->espsave));
// 	 sti();
// 	gotouser(entrypoint);
// 	uint32_t temp;
// 	temp = ((pcb_t *)((uint32_t)&temp & PCBALIGN))->savestatus;
// 	return  temp;
// 	return 0;
	execute("shell");
	// switchterm(1);
	// execute("shell");
	// switchterm(2);
	// execute("shell");
	// switchterm(0);
	return 0;

// 	uint8_t buf[4];

// 	uint32_t entrypoint;
// 	uint8_t exnumbers[4]= {0x7F, 0x45, 0x4C, 0x46};

// 	uint8_t argbuf[tablesize];
// 	int i;



// pcb_t * pcb;
// 	entrypoint=0;
// 	int k;


	
// //printf ("parsed\n");



// 	if(fsread("shell", 0, (uint8_t *)buf, 4) == -1){ // Checks for executable image
// 		return -1;
// 	}

// 	if(strncmp((int8_t *)buf, (int8_t *)exnumbers, 4))
// 		return -1;


// //look for open spot in process




// //read to get entrypoint
// 	if(fsread("shell", ENTRYPT, (uint8_t *)buf, 4)==-1)
// 		return -1;

// 	for(i=0; i<4; i++){
// 		//buf[i]= buf[i] & 0xFF;
		
// 		entrypoint |= (buf[i] << i*(maxfd+1)); //set entrypoint
		
// 	}

// //set up paging
// 	for(k=3; k>0; k--){
// 	if( newtask(k)==-1)
// 		return -1;

// 	fstomem("shell", PROGADDR);

// 	pcb= (pcb_t*)(MB8-KB8*(k+1)); //get pcb


// //save ebp, esp, ss
// 	uint32_t esp;
	
// 	pcb->kernel_sp=tss.esp0;

// 	uint32_t ebp;
// 	asm volatile("movl %%ebp, %0":"=g"(ebp));
// 	pcb->kernel_bp=ebp;
// 	pcb->kernel_ss=tss.ss0;


// //check if 0th open process

// 		pcb->parent_process= -1;
// 		pcb->haschild=0;
// 		pcb->term=k;
	
// 	// else if(running== MASK<<1){
// 	// 	pcb->parent_process=0;
// 	// }
	

// 	pcb->process= k; 

// //save args
// 	strcpy(pcb->argsave, (int8_t *)argbuf);

// //clear file descs
// 	for (i=0; i<maxfd+1; i++){
// 		pcb->fdescs->position=0;
// 		pcb->fdescs->inuse=0;
// 		pcb->fdescs->inode=0;
// 	}

// 	//set tss

// 	tss.esp0= MB8-KB8*k-4; //need openprocess number
// 	tss.ss0=KERNEL_DS;
// 	kstackbottom=tss.esp0;

// 	//open stdin, stdout
// 	 open((uint8_t *) "stdin");
// 	 open((uint8_t *) "stdout");

// }
// 	 asm volatile("movl %%esp, %0":"=g"(pcb->espsave));
// 	gotouser(entrypoint);


	
// 	uint32_t temp;
// 	temp = ((pcb_t *)((uint32_t)&temp & PCBALIGN))->savestatus;
// 	return  temp;
	

}

int32_t set_handler (int32_t signum, void * handler_address){
	return 0;
}

int32_t sigreturn (void){
	return 0;
}

int getrunning(int term){
	return run[term];
}

int nowrunning(){
	return running_process->process;
}

uint8_t getcurrent(){
 return running_process->process;
}

void setcurrent(uint8_t set ){
	//current=set;
}

uint32_t getkstack(){
	return 0;
}

void setkstack(uint32_t set){
	//kstackbottom=set;
}

void setpdaddr(uint32_t set){
	//pdaddr=set;
}


