#ifndef FILES_H
#define FILES_H

#include "types.h"

#define MAX_FS_DENTRIES 63
#define MAX_NAMELENGTH 32


//for boot block
typedef struct {
	uint32_t dentries;
	uint32_t inodes;
	uint32_t datablocks;
	uint8_t reserved[52];

}boot_info;



typedef struct {
	char fname[MAX_NAMELENGTH];
	int ftype;
	int inode;
	uint8_t reserved[24]; //for jumptable later
}dentry_t;

typedef struct{
	uint32_t size;
	uint32_t blocks[1023];
}inode_t;


int fsopen(int start, int end);
int fsclose();
int fswrite();
int filewrite();
int diropen();
int dirclose();
int fileopen();
int fileclose();
int dirwrite();

int read_dentry_by_name( const uint8_t * fname, dentry_t * dentry);
int read_dentry_by_index( uint32_t index, dentry_t * dentry);
int read_data(uint32_t inode, uint32_t offset, uint8_t * buf, uint32_t length);

int fsread(const int8_t *fname, uint32_t offset, uint8_t * buf, uint32_t length);
int dirread(uint8_t * buf);
int fileread(uint8_t * buf, uint32_t length, const int8_t * fname, uint32_t offset);

int fstomem(const int8_t *fname, uint32_t address);

#endif
