#ifndef KEYBOARD_H
#define KEYBOARD_H

#define LEFTCTRL 0x1D
#define LEFTALT 0x38
#define LEFTSHIFT 0x2A
#define RIGHTSHIFT 0x36
#define CAPS 0x3A

#define LEFTARROW 0x4B
#define RIGHTARROW 0x4D
#define UPARROW 0x48
#define DOWNARROW 0x50

#define ENTER 0x1C 
#define BACKSPACE 0x0E
#define DELETE 0x53
#define SPACE 0x39
#define TAB 0x0F
#define ESCAPE 0x01
#define PORT 0x60
#define LKEY 0x26
#define ALLRELEASE 0x59	
#define F1 0x3B
#define F2 0x3C
#define F3 0x3D

#define BUF_SIZE 128

#include "types.h"

typedef struct buf_t buf_t;

struct buf_t {
	int start;
	int end;
	int x;
	int y;
	uint8_t buf[BUF_SIZE];
};
extern buf_t stdin[3];
extern char getc();

extern void keyboardopen();
//#include "types.h"
extern void keyboard_init();
extern int32_t keyboard_open();
extern int32_t keyboard_close();
void set_temp();
void reset_temp();

int32_t keyboard_read(void* buf, int32_t nbytes);
void print_keyboard_info();
void update_buffer(int term, int x, int y);
void put_char_buff(char c, int term);
void scroll_buff(term);

int32_t keyboard_write(const void *buf, int32_t nbytes);
//extern int32_t keyboardclose();
extern void keyboard_handle();



#endif


