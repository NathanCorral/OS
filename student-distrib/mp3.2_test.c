#include "mp3.2_test.h"

void test_mp3_2(){
	test_rtc();
	test_terminal();
	test_keyboard();
	test_filesystem();
}


void test_rtc(){

}

void test_terminal(){

}

void test_keyboard(){

}

void test_filesystem(){
	// Inode number of program named "counter"
	// Modify these values to test other memory locations/programs
	uint32_t inode = 22;
	uint32_t offset = 5600;
	uint32_t length = 64;
	// buf array should be same size or greater than length
	uint8_t buf[64];

	print_directory();
	read_data_test(inode,  offset, buf, length);
}
