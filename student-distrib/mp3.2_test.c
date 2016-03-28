#include "mp3.2_test.h"

void test_mp3_2(){
	//test_rtc();
	//while(1);
	//test_io();
	//test_filesystem();
}


void test_rtc(){

	int i;
	uint32_t test = 128;
	for(i = 0; i < 1000000000; i++);
	rtc_write(&test, 4);
}

void test_io(){
	//test_keyboard();
	//test_terminal();

	// Clear Keyboard buffer
	keyboard_open();
	// Clear screen
	// Allow echoing of characters to the screen
	terminal_open();
	
	while(1);

	keyboard_close();
	terminal_close();
}


void test_terminal(){
	char * string = "This Should Be Printed On the Terminal For a Short Period of time\n";
	int i;
	terminal_open();
	terminal_write(string, strlen(string));
	for(i = 0; i<100000000; i++);
	terminal_close();
}

void test_keyboard(){
	char buf[128];
	int32_t length = 1;
	int i, amount, flag = 1;
	keyboard_open();
	// Clear the screen since we will be printing to it
	clear_re();
	printf("Starting Test Keyboard \n");
	printf("Type input into the buffer\n");
	for(i=0; i<2000000000; i++);
	printf("We will now write to the screen what was on the buffer\n");
	// Press 'Q' to exit
	while(flag){
		amount = keyboard_read(buf,length);
		for(i=0; i<amount; i++){
			if(buf[i] == 'Q'){
				flag = 0;
				break;
			}
			putc(buf[i]);
		}
	}
	printf("\n Finished Testing Keyboard \n");

	keyboard_close();

}

void test_filesystem(){
	// Inode number of program named "counter"
	// Modify these values to test other memory locations/files
	uint32_t inode = 22;
	uint32_t offset = 5600;
	uint32_t length = 64;
	unsigned char * string = "counter";
	// buf array should be same size or greater than length
	uint8_t buf[64];

	
	print_directory();
	test_read_dentry_by_name(string);
	test_read_dentry_by_index(5);
	read_data_test(inode,  offset, buf, length);
}
