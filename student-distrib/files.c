#include "files.h"
#include "lib.h"
#include "x86_desc.h"

int openflag;
int bb; //boot block;

inode_t * inodes; //array of inodes
dentry_t * dentries; //array of dentries
datablocks_t * datablocks; // array of datablocks
boot_info info; //boot block info



unsigned int datastart; //addr of 1st data block
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

	if(openflag)
		return -1;


	info.start = (uint8_t *) start_addr;
	info.size = end_addr - start_addr;

	info.dentries = read_lit_endian(info.start); // first entry
	info.inodes = read_lit_endian(info.start + 4); // second entry
	info.datablocks = read_lit_endian(info.start + 8); // third entry

	dentries = (dentry_t *) (info.start + NEXT_ENTRY); //this is the directories given in boot block
	inodes = (inode_t *) (info.start + NEXT_BLOCK); //this is where inodes start (1 page after boot block)
	datablocks = (datablocks_t *) (info.start + NEXT_BLOCK * (info.inodes + 1)); //this is boot block + number of inodes * 1 page
	datastart = (unsigned int)(info.start + NEXT_BLOCK * (info.inodes + 1));

	reads=0; //nothing read yet
	openflag=1;
	return 0;
}

uint32_t read_lit_endian(uint8_t * addr){
	uint32_t ret = 0;
	ret = (*addr) | ((*(addr+1)) << 8) | ((*(addr+2)) << 16) | ((*(addr+3)) << 24);
	return ret;
}

int divide_ceiling(uint32_t num, uint32_t den){
	int ret = num/den;
	if(num % den != 0)
		ret++;
	return ret;
}

int divide_floor(uint32_t num, uint32_t den){
	int ret = num/den;
	return ret;
}

void test_read_dentry_by_name(const uint8_t *fname){
	dentry_t test_d;
	int err = read_dentry_by_name(fname, &test_d);
	if(err == 0){
		printf("Found file %s\n", test_d.fname);
		if(test_d.ftype == 2){
			printf("     type:  2\n");
			printf("     inode#:  %d\n", test_d.inode);
			printf("     size:   %d\n", inodes[test_d.inode].size);
		}
		else
			printf("     type:  %d\n", test_d.ftype);
	}
	else if(err == -1)
		printf("File \" %s \" does not exist\n", fname);
	else if(err == -2)
		printf("File Name \" %s \" is invalid\n", fname);
}

void test_read_dentry_by_index(uint32_t index){
	dentry_t test_d;
	int err = read_dentry_by_index(index, &test_d);
	if(err == 0){
		printf("Found file %s\n", test_d.fname);
		if(test_d.ftype == 2){
			printf("     type:  2\n");
			printf("     inode#:  %d\n", test_d.inode);
			printf("     size:   %d\n", inodes[test_d.inode].size);
		}
		else
			printf("     type:  %d\n", test_d.ftype);
	}
	else if(err == -1)
		printf("File Index \" %d \" does not exist\n", index);
}

void print_directory(){
	int i;
	printf("___File System Statistics___\n");
	printf("Dentries = %d\n", info.dentries);
	printf("Inodes = %d\n", info.inodes);
	printf("DataBlocks = %d\n", info.datablocks);
	printf("Directory List and Statistics:    \n");
	for(i=0; i<(info.dentries); i++){
		// Note for printing string we hand in address of string.
		printf("%s", dentries[i].fname);
		if((dentries+i)->ftype == 2){
			printf("     type:  2");
			printf("     inode#:  %d", dentries[i].inode);
			printf("     size:   %d\n", inodes[dentries[i].inode].size);
		}
		else
			printf("     type:  %d\n", dentries[i].ftype);
	}
}


void read_data_test(uint32_t inode, uint32_t offset, uint8_t * buf, uint32_t length){
	uint8_t *addr;
	int location; //where in block
	int curblock; //data block
	int i;
	int err;
	int success = 0;

	curblock = offset/NEXT_BLOCK;
	location = offset % NEXT_BLOCK;
	addr= (uint8_t *)(datastart+(inodes[inode].blocks[curblock])*0x1000+ location);
	printf("Data read from Memory:");
	for(i = 0; i < length; i++){
		if(location>=0x1000){ //if location is not in block
			location=0;
			curblock++; //go to beginning of next block
			if(inodes[inode].blocks[curblock] >= info.datablocks){
				printf("BAD BLOCK\n");
				break;
			}
			//start of next data block
			addr= (uint8_t *)(datastart+(inodes[inode].blocks[curblock])*0x1000+ location);
		}
		if(success+offset>= inodes[inode].size)
			break;
		if(i%16 == 0)
			printf("\n");
		printf("0x%x ", *addr);
		success++;
		location++;
		addr++;
	}


	err = read_data(inode, offset, buf, length);
	if(err != length){
		if(err == 0){
			length = inodes[inode].size - offset;
			printf("\nReached end of file.  Amount Read = %d", length);
		}
		else{
			printf("Error value Returned = %d\n", err);
			length = 0;
		}
	}
	printf("\nData read from buffer:");
	for(i=0; i<length; i++){
		if(i%16 == 0)
			printf("\n");
		printf("0x%x ", buf[i]);
	}
}

//check if open, then close
int fsclose(){
	if (!openflag)
		return -1;
	openflag=0;
	return 0;
}
//stub
int fswrite(void){
	return -1;
}
//stub
int filewrite(void){
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
int dirwrite(void){
	return -1;
}


//reads the dentry by the name of the file
//input is name and dentry
//retuns 0 on succes, -1 on fail

int read_dentry_by_name( const uint8_t * fname, dentry_t * dentry){
	int j=0;
	int length= strlen((int8_t*) fname);
	if( length <1 || length >MAX_NAMELENGTH) //test if valid name length
		return -2;

	for(j=0; j<(info.dentries); j++){

		if( length == strlen(dentries[j].fname) ){ //test if even same length
			if(strncmp(dentries[j].fname, (int8_t *)fname, length)==0){ //compare
				//found, get data and put it into the dentry
				strcpy(dentry->fname, dentries[j].fname);
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
	if ( index>=info.dentries || index < 0)
		return -1; //invalid
	strcpy(dentry->fname, dentries[index].fname);
	dentry->inode= dentries[index].inode;
	dentry->ftype=dentries[index].ftype;
	return 0;
}


//reads the data from file
//input is inode of file, the offset into the inode file in Bytes, the buffer to copy to, and the length of read
//output is -1 if fail, 0 if end of file, or number of bytes read otherwise
int read_data(uint32_t inode, uint32_t offset, uint8_t * buf, uint32_t length){

	int ret; //return value of Bytes read or zero if we reach the end of the file
	int success; // Bytes we will read
	int count = 0; // Location on buffer
	int location; //where in block
	int curblock_idx; // inode index to the current block
	int curblock; //data block

	if(inode >= info.inodes || inode < 0) //invalid inode
		return -1;

	// Other input error checks
	// Not sure if this is necessary and we can assume valid inputs
	if(offset < 0 || buf == NULL)
		return -1;

	if (offset== inodes[inode].size)
		return 0;
	// Read offset is greater than file size
	if(inodes[inode].size <= offset)
		return -1;

	curblock_idx = divide_floor(offset, NEXT_BLOCK);
	location = offset - (curblock_idx * NEXT_BLOCK);
	curblock = inodes[inode].blocks[curblock_idx];

	// Check that block is valid
	if(curblock >= info.datablocks)
		return -1;

	// We are going to read until the end of the file 
	// so we want to return 0 
	if(inodes[inode].size < (offset + length)){
		success = inodes[inode].size - offset;
		ret = 0;
	}
	// Otherwise we read all Bytes
	else{
		success = length;
		ret = length;
	}

	while(count < success ){
		// Write data
		buf[count] = datablocks[curblock].data[location];
		location++;
		// Check move to next block
		if(location >= NEXT_BLOCK){
			location = 0;
			curblock = inodes[inode].blocks[++curblock_idx];
			// Check that new block is valid
			if(curblock >= info.datablocks)
				return -1;
		}
		count++;
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
int dirread(const int8_t *fname, uint32_t offset, uint8_t * buf, uint32_t length){

if(reads>= info.dentries){ //file system has been read all the way through, so reset
	reads=0;
	return 0;
}
//put name of file into buffer
strcpy((int8_t *)buf, (int8_t *)dentries[reads].fname);
reads++;
return strlen((int8_t *) buf);

}


//reads file, acts like fsread
int fileread( const int8_t * fname, uint32_t offset, uint8_t * buf,uint32_t length){
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



