#include "syscalls.h"
uint32_t kstackbottom;

int32_t execute(const int8_t * cmd){
	int8_t buf[4];
	int8_t fname[32];
	uint32_t entrypoint;
	uint8_t exnumbers[4]= {0x7F, 0x45, 0x4C, 0x46};
	uint32_t spaceflag;
	uint32_t namelength;
	uint8_t argbuf[1024];
	int i;


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




	if(fsread(fname, 0, buf, 4) == -1){ // Checks for executable image
		return -1;
	}

	if(strncmp(buf, exnumbers, 4))
		return -1;


//look for open spot in process

	if(fsread(fname, ENTRYPT, buf, 4)==-1)
		return -1;

	for(i=0; i<4; i++){
		entrypoint |= (buf[i] << i*8);
	}

//set up page directory

	
	fstomem(fname, PROGADDR);

	pcb_t * pcb= (pcb_t*)(0x800000-0x2000*(openprocess+1)); //openprocess- need this

	uint32_t esp;
	asm volatile("movl %%esp, %0":"=g"(esp));
	pcb->kernel_sp=esp;

	uint32_t ebp;
	asm volatile("movl %%ebp, %0":"=g"(ebp));
	pcb->kernel_bp=ebp;


//check if 1st open process

	pcb->process= openprocess; //openprocess- need this


	strcpy(pcb->argsave, argbuf);

	for (i=0; i<8; i++){
		pcb->fdescs->position=0;
		pcb->fdescs->inuse=0;
		pcb->fdescs->inode=0;
	}

	tss.esp0= 0x800000-0x2000*openprocess-4; //need openprocess number
	kstackbottom=tss.esp0;

	//open stdin, stdout

	gotouser(entrypoint);

}



int32_t halt(int8_t done){
	return 0;
};