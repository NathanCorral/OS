#include "syscalls.h"

uint32_t kstackbottom;


int32_t emptyfunc(){
	return 0;
}

uint32_t stdintable[4]={
			(uint32_t) (emptyfunc),
			 (uint32_t) (terminal_read),
			 (uint32_t) (emptyfunc),
			(uint32_t) (emptyfunc)};


uint32_t stdouttable[4]={
			(uint32_t) (emptyfunc),
			 (uint32_t) (emptyfunc),
			(uint32_t) (terminal_write),
			 (uint32_t) (emptyfunc)};



uint32_t rtctable[4]={
			(uint32_t) (rtc_open),
			 (uint32_t) (rtc_read),
			(uint32_t) (rtc_write),
			 (uint32_t) (rtc_close)};

uint32_t filetable[4]={
			(uint32_t) (fileopen),
			(uint32_t) (fileread),
		(uint32_t) (filewrite),
			(uint32_t) (fileclose)};

uint32_t dirtable[4]={
			 (uint32_t) (diropen),
			 (uint32_t) (dirread),
			(uint32_t) (dirwrite),
			 (uint32_t) (dirclose)};









int32_t execute(const int8_t * cmd){
	uint8_t buf[4];
	int8_t fname[32];
	uint32_t entrypoint;
	uint8_t exnumbers[4]= {0x7F, 0x45, 0x4C, 0x46};
	uint32_t spaceflag;
	uint32_t namelength;
	uint8_t argbuf[1024];
	int i;


int openprocess=0;

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
printf ("parsed\n");



	if(fsread(fname, 0, (uint8_t *)buf, 4) == -1){ // Checks for executable image
		return -1;
	}

	if(strncmp(buf, (int8_t *)exnumbers, 4))
		return -1;


//look for open spot in process

	if(fsread(fname, ENTRYPT, (uint8_t *)buf, 4)==-1)
		return -1;

	for(i=0; i<4; i++){
		buf[i]= buf[i] & 0xFF;
		printf("%x", buf[i]);
		printf("\n");
		entrypoint |= (buf[i] << i*8);
		printf("entrypoint: %x\n", entrypoint);
	}

//set up page directory for task
//allocate hands back pcb or process number

	printf("ready for paging\n");

	uint32_t flags=0;
	uint32_t phys= 0x800000+0x400000;
	uint32_t virt= 0x8000000;
	flags |= (SET_P) | SET_R | SET_U |SET_S;

allocate_big_page( flags, virt);

	printf("post allcocate\n");
	fstomem(fname, PROGADDR);
printf("post copy\n");
	pcb_t * pcb= (pcb_t*)(0x800000-0x2000*(openprocess+1)); //openprocess- need this

	uint32_t esp;
	asm volatile("movl %%esp, %0":"=g"(esp));
	pcb->kernel_sp=esp;

	uint32_t ebp;
	asm volatile("movl %%ebp, %0":"=g"(ebp));
	pcb->kernel_bp=ebp;


//check if 1st open process

	pcb->process= openprocess; //openprocess- need this


	strcpy(pcb->argsave, (int8_t *)argbuf);

	for (i=0; i<8; i++){
		pcb->fdescs->position=0;
		pcb->fdescs->inuse=0;
		pcb->fdescs->inode=0;
	}

	tss.esp0= 0x800000-0x2000*openprocess-4; //need openprocess number
	kstackbottom=tss.esp0;

	//open stdin, stdout
	// open((uint8_t *) "stdin");
	// open((uint8_t *) "stdout");

printf("going to end\n");
	// for(i=0; i<31; i++){
	// 	*((int *) entrypoint + i) = i;
	// }
	for(i=0; i<31; i++){
		printf("0x%#x ",*(uint8_t *) (entrypoint+i));
	}
printf("\n");
	gotouser(entrypoint);

printf("end\n");
	return  0;

}



int32_t halt(int8_t done){
	return 0;
}


//inputs: file descriptor
//opens the stdin for the file
void stdinopen (int32_t fd){

	pcb_t * pcb= (pcb_t *)(kstackbottom & PCBALIGN); //get pcb

	pcb->fdescs[fd].jumptable=stdintable; //get functions 

	pcb->fdescs[fd].inuse=1;
}


//inputs: file descriptor
//opens the stdout for the file
void stdoutopen (int32_t fd){

	pcb_t * pcb= (pcb_t *)(kstackbottom & PCBALIGN); //get pcb

	pcb->fdescs[fd].jumptable=stdouttable; //get functions

	pcb->fdescs[fd].inuse=1;
}

int32_t open(const uint8_t * filename){

	dentry_t mydentry;
	int i;

	pcb_t  * pcb= (pcb_t *) (kstackbottom & PCBALIGN);


	if( strncmp((const int8_t *)filename,(const int8_t *) "stdin", 5) ==0){
		stdinopen(0);
		return 0;
	}

	if( strncmp((const int8_t *)filename,(const int8_t *) "stdout", 5) ==0){
		stdoutopen(0);
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
					pcb->fdescs[i].jumptable= rtctable;
			}
		


		else if (mydentry.ftype==DIRTYPE){
			pcb->fdescs[i].jumptable= dirtable;
		}

		else if (mydentry.ftype==FILETYPE){
			pcb->fdescs[i].jumptable= filetable;
		}


		pcb->fdescs[i].inode=mydentry.inode;
		pcb->fdescs[i].inuse=1;
		strcpy((int8_t *)pcb->names[i], (const int8_t *)filename);
		return i;
	
	}
}
printf("Full File Descriptor Arrary\n");
return -1;

}

//close inputted file descriptor

int32_t close( int32_t fd){

	int value;

	pcb_t * pcb= (pcb_t *)(kstackbottom &PCBALIGN);


	if(fd >7 || fd <2 || pcb->fdescs[fd].inuse==0) //check if valid
		return -1;


//call close function
	asm volatile("call *%0 		;"
					:
					: "g" (pcb->fdescs[fd].jumptable[3]));

	asm volatile("movl %%eax, %0": "=g"(value));
//get return value


//reset descriptor info
	pcb->fdescs[fd].inode=0;
	pcb->fdescs[fd].position=0;
	pcb->fdescs[fd].inuse=0;
	pcb->fdescs[fd].jumptable=NULL;

	return value;

}


int32_t read(int32_t fd, void *buf, int32_t nbytes){

	sti();

	int successes;

	pcb_t * pcb= (pcb_t *)(kstackbottom &PCBALIGN);


	if(fd >7 || fd < 0 || pcb->fdescs[fd].inuse==0 ||buf==NULL) //check if valid
		return -1;

	uint8_t * filename= pcb->names[fd];
	uint32_t position= pcb->fdescs[fd].position;

	asm volatile ("pushl %0     ;"
					"pushl %1    ;"
					"pushl %2    ;"
					"pushl %3    ;"
					"call *%4    ;"
					:
					: "g" (position), "g" ((int32_t)filename), "g"(nbytes), "g" ((int32_t) buf), "g" (pcb->fdescs[fd].jumptable[1]));


	asm volatile ("movl %%eax, %0" : "=g"(successes));
	asm volatile ("addl $16, %esp    ;");


	pcb->fdescs[fd].position += successes;
	return successes;

}


int32_t write(int32_t fd, void *buf, int32_t nbytes){

	pcb_t * pcb= (pcb_t *)(kstackbottom &PCBALIGN);


	if(fd >7 || fd < 0 || pcb->fdescs[fd].inuse==0 ||buf==NULL) //check if valid
		return -1;

	asm volatile ("pushl %0     ;"
					"pushl %1    ;"
					"call *%2    ;"
					:
					: "g" (nbytes), "g" ((int32_t) buf), "g" (pcb->fdescs[fd].jumptable[2]));

	return 0;

}



