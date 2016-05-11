#include "mp3.2_test.h"

void test_mp3_2(){
	paging_test();
	//test_rtc();
	//test_io();
	//test_filesystem();
	//test_keyboard();
	//test_terminal();
	// test_execute();
	while(1);
}


// void test_rtc(){

// 	int i;
// 	rtc_open();
// 	uint32_t test = 128;
// 	for(i = 0; i < 1000000000; i++);
// 	rtc_write(&test, 4);
// 	rtc_close();
// }

// void test_io(){
// 	//test_keyboard();
// 	//test_terminal();

// 	// Clear Keyboard buffer
// 	keyboard_open();
// 	// Clear screen
// 	// Allow echoing of characters to the screen
// 	terminal_open();
	
// 	while(1);

// 	keyboard_close();
// 	terminal_close();
// }


// void test_terminal(){
// 	char * string = "This Should Be Printed On the Terminal For a Short Period of time\n";
// 	int i;
// 	terminal_open();
// 	terminal_write(2,string, strlen(string));
// 	for(i = 0; i<100000000; i++);

// 	terminal_close();
// }

// void test_keyboard(){
// 	char buf[128];
// 	int32_t length = 1;
// 	int i, amount, flag = 1;
// 	keyboard_open();
// 	// Clear the screen since we will be printing to it
// 	clear_re();
// 	printf("Starting Test Keyboard \n");
// 	printf("Type input into the buffer\n");
// 	for(i=0; i<2000000000; i++);
// 	printf("We will now write to the screen what was on the buffer\n");
// 	// Press 'Q' to exit
// 	while(flag){
// 		amount = terminal_read(2,buf,length);
// 		for(i=0; i<amount; i++){
// 			if(buf[i] == 'Q'){
// 				flag = 0;
// 				break;
// 			}
// 			putc(buf[i]);
// 		}
// 	}
// 	printf("\n Finished Testing Keyboard \n");

// 	keyboard_close();

// }

// void test_filesystem(){
// 	// Inode number of program named "counter"
// 	// Modify these values to test other memory locations/files
// 	uint32_t inode = 22;
// 	uint32_t offset = 0;
// 	uint32_t length = 64;
// 	 char * string = "counter";
// 	// buf array should be same size or greater than length
// 	uint8_t buf[64];
// 	int i;
	
// 	print_directory();
// 	test_read_dentry_by_name((unsigned char *)string);
// 	test_read_dentry_by_index(5);
// 	read_data_test(inode,  offset, buf, length);
	
	
// 	printf("\n Wrapper data");
// 	//while(fsread(string, offset, buf, length) !=0){
// 	fsread(string, offset, buf, length);
// 	for(i=0; i<length; i++){
// 		if(i%16 == 0)
// 			printf("\n");
// 		printf("0x%x ", buf[i]);
// 		offset += length;
// 	//}
// 	}
// }

void test_execute(){
	programs_init();
	execute("shell");

}

