#include "syscalls.h"


#define maxfd 7

#define EMPTYMASK 0x7F

#define MASK 0x80
#define NameSize 32
uint32_t kstackbottom;
uint8_t running= 0;
uint8_t current=0;
uint32_t pdaddr;
int run[3];

//this does nothing
int32_t emptyfunc(){
	return 0;
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
	uint8_t buf[4];
	int8_t fname[NameSize] = {[0 ... 31] = 0};
	uint32_t entrypoint;
	uint8_t exnumbers[4]= {0x7F, 0x45, 0x4C, 0x46};
	uint32_t spaceflag;
	uint32_t namelength;
	uint8_t argbuf[tablesize];
	int i;


int openprocess;

	entrypoint=0;
	spaceflag=0;
	namelength=0;


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
			if (i>=NameSize && spaceflag==0){
				return -1;
			}
			fname[i]= cmd[i];
		}

	}
	argbuf[i-namelength-1]= '\0';
//printf ("parsed\n");



	if(fsread(fname, 0, (uint8_t *)buf, 4) == -1){ // Checks for executable image
		return -1;
	}

	if(strncmp((int8_t *)buf, (int8_t *)exnumbers, 4))
		return -1;


//look for open spot in process
	uint8_t bitmask=MASK;
	for (i=0; i<maxfd+1; i++){
		if ((running & bitmask) ==0){
			openprocess=i;
			running |= bitmask;
			current=openprocess;
			break;
		}
		bitmask = bitmask >> 1;
		if (bitmask==0)
			return -1;
	}



//read to get entrypoint
	if(fsread(fname, ENTRYPT, (uint8_t *)buf, 4)==-1)
		return -1;

	for(i=0; i<4; i++){
		//buf[i]= buf[i] & 0xFF;
		
		entrypoint |= (buf[i] << i*(maxfd+1)); //set entrypoint
		
	}
int	terminal=getactiveterm();
//set up paging
	
	if( newtask(openprocess)==-1)
		return -1;

	fstomem(fname, PROGADDR);

	pcb_t * pcb= (pcb_t*)(MB8-KB8*(openprocess+1)); //get pcb


//save ebp, esp, ss
	uint32_t esp;
	
	pcb->kernel_sp=tss.esp0;

	uint32_t ebp;
	asm volatile("movl %%ebp, %0":"=g"(ebp));
	pcb->kernel_bp=ebp;
	pcb->kernel_ss=tss.ss0;


//check if 0th open process
	if( run[terminal]<1){
		pcb->parent_process= -1;
	
		pcb->term=terminal;
		pcb->haschild=0;
	}
	// else if(running== MASK<<1){
	// 	pcb->parent_process=0;
	// }
	else{
		pcb->parent_process= ((pcb_t *)((uint32_t)&esp & PCBALIGN))->process;
		((pcb_t *)((uint32_t)&esp & PCBALIGN))->haschild=1;
		pcb->term=((pcb_t *)((uint32_t)&esp & PCBALIGN))->term;
	}
run[terminal]++;
	pcb->process= openprocess; 

//save args
	strcpy(pcb->argsave, (int8_t *)argbuf);

//clear file descs
	for (i=0; i<maxfd+1; i++){
		pcb->fdescs->position=0;
		pcb->fdescs->inuse=0;
		pcb->fdescs->inode=0;
	}

	//set tss

	tss.esp0= MB8-KB8*openprocess-4; //need openprocess number
	tss.ss0=KERNEL_DS;
	kstackbottom=tss.esp0;

	//open stdin, stdout
	 open((uint8_t *) "stdin");
	 open((uint8_t *) "stdout");


	 asm volatile("movl %%esp, %0":"=g"(pcb->espsave));
// int flag=0;
	 if(run[1]==0)
	 	switchterm(1);
	 if(run[2]==0){
	 	switchterm(2);
	 	//switchterm(0);
	 	//flag=1;
	 }
	 // if(flag){
	 // 	flag=0;
	 // 	switchterm(0);
	 // }
	gotouser(entrypoint);
asm volatile ("haltreturn:");

	
	uint32_t temp;
	temp = ((pcb_t *)((uint32_t)&temp & PCBALIGN))->savestatus;
	return  temp;

}


//intputs the status
//halts the program and goes back to previous process
int32_t halt(int8_t status){
	int	terminal=getactiveterm();
	run[terminal]--;
	int i;
	uint8_t buf[4];
	pcb_t * pcb= (pcb_t *)(kstackbottom &PCBALIGN);

	//don't want to close final shell, so restart it just to be sure

	if (pcb->parent_process==-1){
		run[terminal]++;
		if(fsread((const int8_t *)("shell"), ENTRYPT, buf, 4)==-1)
			return -1;
		uint32_t entrypoint;
		for(i=0; i<4; i++){
		entrypoint |= (buf[i] << i*8);
		}
		gotouser(entrypoint);

	}

	//mark that process is done and available

	uint8_t bitmask=EMPTYMASK;
	for(i=0; i<pcb->process; i++){
		bitmask= (bitmask >>1)+MASK;
	}
	running &=bitmask;

	current= pcb->parent_process;

for (i=0; i<maxfd+1; i++){
		close(i);
	}

pcb_t * parentpcb = (pcb_t *)( MB8 - KB8*(pcb->parent_process + 1) );
	parentpcb->haschild = 0;

	pdaddr= getaddr(pcb->parent_process);
//set paging to new page directory
	asm volatile ("				\
		movl %0, %%cr3 \n\
		movl %%cr4, %%eax	\n\
		orl $0x90, %%eax	\n\
		movl %%eax, %%cr4	\n\
		movl %%cr0, %%eax \n\
		orl $0x80000000, %%eax	\n\
		movl %%eax, %%cr0"
		:
		:"r" (pdaddr)
		:"%eax"
		);

//set tss
tss.esp0= pcb->kernel_sp; 
tss.ss0= pcb->kernel_ss;
	kstackbottom=tss.esp0;


	pcb->savestatus= status;

	

//go bacck to execute
	asm volatile ("movl %0, %%ebp     ;"
				"movl %1, %%esp     ;"
				"jmp haltreturn  ;"
				::"g"(pcb->kernel_bp), "g"(pcb->espsave));
	return 0;

}


//inputs: file descriptor
//opens the stdin for the file
void stdinopen (int32_t fd){

	pcb_t * pcb= (pcb_t *)(kstackbottom & PCBALIGN); //get pcb

	pcb->fdescs[fd].jumptable=&stdintable; //get functions 

	pcb->fdescs[fd].inuse=1;
}


//inputs: file descriptor
//opens the stdout for the file
void stdoutopen (int32_t fd){

	pcb_t * pcb= (pcb_t *)(kstackbottom & PCBALIGN); //get pcb

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

	pcb_t  * pcb= (pcb_t *) (kstackbottom & PCBALIGN);


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

	pcb_t * pcb= (pcb_t *)(kstackbottom &PCBALIGN);


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

	pcb_t * pcb= (pcb_t *)(kstackbottom & PCBALIGN);

//error checking and saving
	if(fd >maxfd || fd < 0 || pcb->fdescs[fd].inuse==0 ||buf==NULL || fd==1) //check if valid
		return -1;

	uint8_t * filename= pcb->names[fd];
	uint32_t position= pcb->fdescs[fd].position; //save info 





//the actuall reading
successes += pcb->fdescs[fd].jumptable->read( (int8_t *) filename, position,buf, nbytes); //read
	




//more saving
	pcb->fdescs[fd].position += successes;

	return successes;

}

//writes to fd
//returns # of successes, -1 if fail
int32_t write(int32_t fd, void *buf, int32_t nbytes){
//printf("in write");
	pcb_t * pcb= (pcb_t *)(kstackbottom &PCBALIGN);

	//error checking

int successes=0;
	if(fd >maxfd || fd < 1 || pcb->fdescs[fd].inuse==0 ||buf==NULL) //check if valid
		return -1;

	


	successes +=pcb->fdescs[fd].jumptable->write(fd, buf, nbytes); //call write

	return successes;

}

int32_t getargs (uint8_t* buf, int32_t nbytes){

	if(nbytes==0 || buf==NULL)
		return -1;



	pcb_t * pcb= (pcb_t *)(kstackbottom &PCBALIGN);


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
	if((uint32_t) screen_start < VIRT128 || (uint32_t) screen_start > VIRT128+ksize )
		return -1;
pcb_t * pcb= (pcb_t *)(kstackbottom &PCBALIGN);
uint32_t theterm= pcb->term;

if (theterm==0)
 *screen_start= (uint8_t *) kb; //sets to user accessible video memory

else if (theterm==1)
	*screen_start= (uint8_t *) (3*kb); //0

else if (theterm==2)
	*screen_start= (uint8_t *) (5*kb);
else
	*screen_start= (uint8_t *) kb;
	return 0;
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
	return running;
}

uint8_t getcurrent(){
 return current;
}

void setcurrent(uint8_t set ){
current=set;
}

uint32_t getkstack(){
	return kstackbottom;
}

void setkstack(uint32_t set){
	kstackbottom=set;
}

void setpdaddr(uint32_t set){
	pdaddr=set;
}
