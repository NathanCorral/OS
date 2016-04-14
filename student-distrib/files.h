#ifndef FILES_H
#define FILES_H

#include "types.h"

#define MAX_FS_DENTRIES 63
#define MAX_NAMELENGTH 32
#define NEXT_ENTRY 64
#define NEXT_BLOCK 0x1000
#define RTCTYPE 0
#define DIRTYPE 1
#define FILETYPE 2



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

typedef struct{
	uint8_t data[NEXT_BLOCK];
}datablocks_t;


typedef struct {
	uint8_t * start;
	uint32_t size;

	uint32_t dentries;
	uint32_t inodes;
	uint32_t datablocks;

}boot_info;

uint32_t read_lit_endian(uint8_t * addr);
int divide_ceiling(uint32_t num, uint32_t den);
int divide_floor(uint32_t num, uint32_t den);

int fsopen(uint32_t start_addr, uint32_t end_addr);
int fsclose();
int fswrite(void);
int filewrite(void);
int diropen();
int dirclose();
int fileopen();
int fileclose();
int dirwrite(void);

// Test Functions for mp3.2
void print_directory();
void test_read_dentry_by_name(const uint8_t *fname);
void test_read_dentry_by_index(uint32_t index);
void read_data_test(uint32_t inode, uint32_t offset, uint8_t * buf, uint32_t length);

int read_dentry_by_name( const uint8_t * fname, dentry_t * dentry);
int read_dentry_by_index( uint32_t index, dentry_t * dentry);
int read_data(uint32_t inode, uint32_t offset, uint8_t * buf, uint32_t length);

int fsread(const int8_t *fname, uint32_t offset, uint8_t * buf, uint32_t length);

int dirread(const int8_t *fname, uint32_t offset, uint8_t * buf, uint32_t length);
int fileread( const int8_t * fname, uint32_t offset, uint8_t * buf,uint32_t length);

int fstomem(const int8_t *fname, uint32_t address);

#endif
