#include "files.h"
#include "lib.h"
#include "x86_desc.h"

int openflag;
int bb; //boot block;

inode_t * inodes; //array of inodes
dentry_t * dentries; //array of dentries
uint8_t * datablocks;
boot_info info; //boot block info



int datastart; //addr of 1st data block
int reads; //number of names read from system

//in discussion, suggested putting this index in dentry?

//something to start with, wasn't sure what was needed for this checkpoint vs what is needed in 
//checkpoint 3

//apparently if we don't clear the screen in kernal, that stuff they print at the beginning will 
//help us figure out where the file system is in memory- somewhere in the kernal space
//mod->mod_start??

//none of this has been tested- haven't tried to compile because still warnings from terminal stuff

//not sure if anything else is needed for copying program images into contiguous memory- maybe fstomem is enough?




//this opens the file system

int fsopen(uint32_t start_addr, uint32_t end_addr){
	int i;
	if(openflag)
		return -1;


	info.start = (uint8_t *) start_addr;
	info.size = end_addr - start_addr;

	info.dentries = LITTLE_TO_BIG(info.start); // first entry
	info.inodes = LITTLE_TO_BIG(info.start + 4); // second entry
	info.datablocks = LITTLE_TO_BIG(info.start + 8); // third entry

	dentries = (dentry_t *) (info.start + NEXT_ENTRY); //this is the directories given in boot block
	inodes = (inode_t *) (info.start + NEXT_BLOCK); //this is where inodes start (1 page after boot block)
	datablocks = (info.start + NEXT_BLOCK * (info.inodes + 1)); //this is boot block + number of inodes * 1 page

	// Print out directory names to prove we can
	for(i=0; i<(info.dentries); i++){
		printf("%s\n", (dentries+i));
	}

	reads=0; //nothing read yet
	openflag=1;
	return 0;
}

uint32_t LITTLE_TO_BIG(uint8_t * addr){
	uint32_t ret = 0;
	ret = (*addr) | ((*(addr+1)) << 8) | ((*(addr+2)) << 16) | ((*(addr+3)) << 24);
	return ret;
}

//check if open, then close
int fsclose(){
	if (!openflag)
		return -1;
	openflag=0;
	return 0;
}
//stub
int fswrite(){
	return -1;
}
//stub
int filewrite(){
	return -1;
}
//stub
int diropen(){
	return 0;
}
//stub
int dirclose(){
	return 0;
}
//stub
int fileopen(){
	return 0;
}
//stub
int fileclose(){
	return 0;
}
//stub
int dirwrite(){
	return -1;
}


//reads the dentry by the name of the file
//input is name and dentry
//retuns 0 on succes, -1 on fail
/*
int read_dentry_by_name( const uint8_t * fname, dentry_t * dentry){
int j=0;
int length= strlen((int8_t*) fname);
if( length <1 || length >MAX_NAMELENGTH) //test if valid name length
	return -1;

	for(j=0; j<MAX_FS_DENTRIES; j++){
		if( length== strlen(dentries[j].fname)){ //test if even same length
			if(strncmp(dentries[j].fname, (int8_t *)fname, length)==0){ //compare
				//found, get data and put it into the dentry
				strcpy(dentry->fname, dentries[j].fname,strlen(dentries[j].fname);
				dentry->inode= dentries[j].inode;
				dentry->ftype=dentries[j].ftype;
				return 0;
			}
		}
	}
	return -1; //not found
}

//searches by index
//input is index and dentry
//returns 0 on success, -1 on failure
int read_dentry_by_index( uint32_t index, dentry_t * dentry){
	if ( index>=MAX_FS_DENTRIES)
		return -1; //invalid
	strcpy(dentry->fname, dentries[index].fname,strlen(dentries[index].fname);
				dentry->inode= dentries[index].inode;
				dentry->ftype=dentries[index].ftype;
				return 0;
}


//reads the data from file
//input is inode of file, the offset into the file, the buffer to copy to, and the length of read
//output is -1 if fail, 0 if end of file, or number of bytes read otherwise
int read_data(uint32_t inode, uint32_t offset, uint8_t * buf, uint32_t length){

int success=0; //bytes read
int location; //where in block
int curblock; //data block
uint8_t * addr;


if(inode >= info.inodes) //invalid inode
	return -1;

curblock= offset/0x1000; //starting data block
if(inodes[inode].blocks[curblock] >= info.datablocks) 
	return -1;

if(offset >= inodes[inode].size)
	return 0; //end of file


location= offset % 0x1000; //starting spot in block


//this is the address to read from
addr= (uint8_t *)(datastart+(inodes[inode].blocks[curblock])*0x1000+ location);

while( success<length){
	if(location>=0x1000){ //if location is not in block
		location=0;
		curblock++; //go to beginning of next block
		if(inodes[inode].blocks[curblock] >= info.datablocks) 
			return -1;
		//start of next data block
		addr= (uint8_t *)(datastart+(inodes[inode].blocks[curblock])*0x1000+ location);
	}
	//see if end of the file has been reached
	if(success+offset>= inodes[inode].size)
		return success;

	//do the actual reading
	buf[success]= *addr;
	location++;
	success++;
	addr++;

}

return success;


}

//reads file
//input is file name, offset into file, buffer to copy to, length
//returns -1 if fail, results of read otherwise
int fsread(const int8_t *fname, uint32_t offset, uint8_t * buf, uint32_t length){

dentry_t mydentry;
int x;

if(fname==NULL || buf==NULL) //check if valid
	return -1;
x=read_dentry_by_name((uint8_t*)fname, &mydentry); //fill mydentry

if (x== -1) //check if name was there
return -1;


return read_data(mydentry.inode, offset, buf, length); //read
}

//reads a directory, like ls
//input is buffer to copy to
//output is 0 if end of directory length of name otherwise
int dirread(uint8_t * buf){

if(reads>= info.dentries){ //file system has been read all the way through, so reset
	reads=0;
	return 0;
}
//put name of file into buffer
strcpy((int8_t *)buf, (int8_t *)dentries[reads].fname, strlen(dentries[reads].fname));
reads++;
return strlen((int8_t *) buf);

}

//reads file, acts like fsread
int fileread(uint8_t * buf, uint32_t length, const int8_t * fname, uint32_t offset){
	return fsread(fname, offset, buf, length); //same as fs read
}


//puts data into memory from data blocks
//input is file name, address in memory
//returns -1 if fail, 0 if success
int fstomem(const int8_t *fname, uint32_t address){
	dentry_t mydentry;

	if(fname==NULL)
		return -1; //check if valid

	int x;
	x= read_dentry_by_name((uint8_t *) fname, &mydentry); //find file
	if (x==-1)
		return -1;

	if(read_data(mydentry.inode,0,(uint8_t*) address, inodes[mydentry.inode].size)) //read the data the inode points to
		return -1;

	return 0;

}
*/

