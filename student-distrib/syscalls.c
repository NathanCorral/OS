#include "syscalls.h"

uint32_t kstackbottom;
uint8_t running= 0;
uint8_t current=0;
uint32_t pdaddr;


int32_t emptyfunc(){
	return 0;
}

// uint32_t stdintable[4]={
// 			(uint32_t) (emptyfunc),
// 			 (uint32_t) (terminal_read),
// 			 (uint32_t) (emptyfunc),
// 			(uint32_t) (emptyfunc)};


// uint32_t stdouttable[4]={
// 			(uint32_t) (emptyfunc),
// 			 (uint32_t) (emptyfunc),
// 			(uint32_t) (terminal_write),
// 			 (uint32_t) (emptyfunc)};



// uint32_t rtctable[4]={
// 			(uint32_t) (rtc_open),
// 			 (uint32_t) (rtc_read),
// 			(uint32_t) (rtc_write),
// 			 (uint32_t) (rtc_close)};

// uint32_t filetable[4]={
// 			(uint32_t) (fileopen),
// 			(uint32_t) (fileread),
// 		(uint32_t) (filewrite),
// 			(uint32_t) (fileclose)};

// uint32_t dirtable[4]={
// 			 (uint32_t) (diropen),
// 			 (uint32_t) (dirread),
// 			(uint32_t) (dirwrite),
// 			 (uint32_t) (dirclose)};




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
	int8_t fname[32] = {[0 ... 31] = 0};
	uint32_t entrypoint;
	uint8_t exnumbers[4]= {0x7F, 0x45, 0x4C, 0x46};
	uint32_t spaceflag;
	uint32_t namelength;
	uint8_t argbuf[1024];
	int i;


int openprocess;

	entrypoint=0;
	spaceflag=0;
	namelength=0;


	// Checks if file is valid
	if(cmd == NULL){
		return -1;
	}

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
				return -1;
			}
			fname[i]= cmd[i];
		}

	}
//printf ("parsed\n");



	if(fsread(fname, 0, (uint8_t *)buf, 4) == -1){ // Checks for executable image
		return -1;
	}

	if(strncmp((int8_t *)buf, (int8_t *)exnumbers, 4))
		return -1;


//look for open spot in process
	uint8_t bitmask=0x80;
	for (i=0; i<8; i++){
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




	if(fsread(fname, ENTRYPT, (uint8_t *)buf, 4)==-1)
		return -1;

	for(i=0; i<4; i++){
		buf[i]= buf[i] & 0xFF;
		//printf("%x", buf[i]);
		//printf("\n");
		entrypoint |= (buf[i] << i*8);
		//printf("entrypoint: %x\n", entrypoint);
	}


	//printf("ready for paging\n");
	if( newtask(openprocess)==-1)
		return -1;

	//printf("openprocess: %d\n", openprocess);
//set up page directory for task
//allocate hands back pcb or process number



// 	uint32_t flags=0;
// 	uint32_t phys= 0x800000+0x400000;
// 	uint32_t virt= 0x8000000;
// 	flags |= (SET_P) | SET_R | SET_U |SET_S;

// allocate_big_page( flags, virt);

// 	printf("post allcocate\n");
	fstomem(fname, PROGADDR);
//printf("post copy\n");
	pcb_t * pcb= (pcb_t*)(0x800000-0x2000*(openprocess+1)); //openprocess- need this

	uint32_t esp;
	//asm volatile("movl %%esp, %0":"=g"(esp));
	pcb->kernel_sp=tss.esp0;

	uint32_t ebp;
	asm volatile("movl %%ebp, %0":"=g"(ebp));
	pcb->kernel_bp=ebp;
	pcb->kernel_ss=tss.ss0;


//check if 1st open process
	if( running ==0x80){
		pcb->parent_process= -1;
	}
	else{
		pcb->parent_process= ((pcb_t *)((uint32_t)&esp & PCBALIGN))->process;
	}

	pcb->process= openprocess; //openprocess- need this


	strcpy(pcb->argsave, (int8_t *)argbuf);

	for (i=0; i<8; i++){
		pcb->fdescs->position=0;
		pcb->fdescs->inuse=0;
		pcb->fdescs->inode=0;
	}

	tss.esp0= 0x800000-0x2000*openprocess-4; //need openprocess number
	tss.ss0=KERNEL_DS;
	kstackbottom=tss.esp0;

	//open stdin, stdout
	 open((uint8_t *) "stdin");
	 open((uint8_t *) "stdout");

//printf("going to end\n");
	// for(i=0; i<31; i++){
	// 	*((int *) entrypoint + i) = i;
	// }
	// for(i=0; i<31; i++){
	// 	printf("0x%#x ",*(uint8_t *) (entrypoint+i));
	// }
//printf("\n");
	 asm volatile("movl %%esp, %0":"=g"(pcb->espsave));
	gotouser(entrypoint);
asm volatile ("haltreturn:");
//printf("end\n");
	
	uint32_t temp;
	temp = ((pcb_t *)((uint32_t)&temp & PCBALIGN))->savestatus;
	return  temp;

}



int32_t halt(int8_t status){
	//return 0;
	int i;
	uint8_t buf[4];
	pcb_t * pcb= (pcb_t *)(kstackbottom &PCBALIGN);

	//don't want to close final shell, so restart it just to be sure

	if (pcb->parent_process==-1){
		if(fsread((const int8_t *)("shell"), ENTRYPT, buf, 4)==-1)
			return -1;
		uint32_t entrypoint;
		for(i=0; i<4; i++){
		entrypoint |= (buf[i] << i*8);
		}
		gotouser(entrypoint);

	}

	//mark that process is done and available

	uint8_t bitmask=0x7F;
	for(i=0; i<pcb->process; i++){
		bitmask= (bitmask >>1)+0x80;
	}
	running &=bitmask;

	current= pcb->parent_process;


//pdaddr= (uint32_t)(&dir[pcb->parent_process]); //need to make pagedirs
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

tss.esp0= pcb->kernel_sp; 
tss.ss0= pcb->kernel_ss;
	kstackbottom=tss.esp0;


	pcb->savestatus= status;


	// asm volatile ("movl %0, %%esp   ;"
	// 	::"g"(pcb->kernel_sp));

	// asm volatile ( "movl %0, %%ebp     ;"
	// 				::"g"(pcb->kernel_bp));

	


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

int32_t open(const uint8_t * filename){

	dentry_t mydentry;
	int i;

	if(*filename=='\0' || filename==NULL)
		return -1;

	pcb_t  * pcb= (pcb_t *) (kstackbottom & PCBALIGN);


	if( strncmp((const int8_t *)filename,(const int8_t *) "stdin", 5) ==0){
		stdinopen(0);
		return 0;
	}

	if( strncmp((const int8_t *)filename,(const int8_t *) "stdout", 5) ==0){
		stdoutopen(1);
		return 0;
	}

	if (read_dentry_by_name(filename, &mydentry)==-1)
		return -1;

	for( i=2; i<8; i++){

		if (pcb->fdescs[i].inuse==0){
			if(mydentry.ftype==RTCTYPE){
				if(rtc_open()==-1)
					return -1;
				else
					pcb->fdescs[i].jumptable= &rtctable;
			}
		


		else if (mydentry.ftype==DIRTYPE){
			pcb->fdescs[i].jumptable= &dirtable;
		}

		else if (mydentry.ftype==FILETYPE){
			pcb->fdescs[i].jumptable= &filetable;
		}


		pcb->fdescs[i].inode=mydentry.inode;
		pcb->fdescs[i].inuse=1;
		strcpy((int8_t *)pcb->names[i], (const int8_t *)filename);
		return i;
	
	}
}
//printf("Full File Descriptor Arrary\n");
return -1;

}

//close inputted file descriptor

int32_t close( int32_t fd){

	int value;

	pcb_t * pcb= (pcb_t *)(kstackbottom &PCBALIGN);


	if(fd >7 || fd <2 || pcb->fdescs[fd].inuse==0) //check if valid
		return -1;


//call close function
	// asm volatile("call *%0 		;"
	// 				:
	// 				: "g" (pcb->fdescs[fd].jumptable[3]));

	// asm volatile("movl %%eax, %0": "=g"(value));
//get return value

pcb->fdescs[fd].jumptable->close(fd);
//reset descriptor info
	pcb->fdescs[fd].inode=0;
	pcb->fdescs[fd].position=0;
	pcb->fdescs[fd].inuse=0;
	pcb->fdescs[fd].jumptable=NULL;

	return value;

}


int32_t read(int32_t fd, void *buf, int32_t nbytes){

	sti();

	int successes=0;

	pcb_t * pcb= (pcb_t *)(kstackbottom & PCBALIGN);


	if(fd >7 || fd < 0 || pcb->fdescs[fd].inuse==0 ||buf==NULL) //check if valid
		return -1;

	uint8_t * filename= pcb->names[fd];
	uint32_t position= pcb->fdescs[fd].position;

	// asm volatile ("pushl %0     ;"
	// 				"pushl %1    ;"
	// 				"pushl %2    ;"
	// 				"pushl %3    ;"
	// 				"call *%4    ;"
	// 				:
	// 				: "g" (position), "g" ((int32_t)filename), "g"(nbytes), "g" ((int32_t) buf), "g" (pcb->fdescs[fd].jumptable[1]));


	// asm volatile ("movl %%eax, %0" : "=g"(successes));
	// asm volatile ("addl $16, %esp    ;");

successes += pcb->fdescs[fd].jumptable->read( (int8_t *) filename, position,buf, nbytes);
	pcb->fdescs[fd].position += successes;
	return successes;

}


int32_t write(int32_t fd, void *buf, int32_t nbytes){

	pcb_t * pcb= (pcb_t *)(kstackbottom &PCBALIGN);


	if(fd >7 || fd < 0 || pcb->fdescs[fd].inuse==0 ||buf==NULL) //check if valid
		return -1;

	// asm volatile ("pushl %0     ;"
	// 				"pushl %1    ;"
	// 				"call *%2    ;"
	// 				:
	// 				: "g" (nbytes), "g" ((int32_t) buf), "g" (pcb->fdescs[fd].jumptable[2]));
	pcb->fdescs[fd].jumptable->write(fd, buf, nbytes);

	return 0;

}



