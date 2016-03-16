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

extern char getc();

extern void keyboardopen();
//#include "types.h"


 extern int keyboardwrite(const unsigned char*buf, int nbytes);
 extern int keyboardread(char* buf, int nbytes);
//extern int32_t keyboardclose();
 extern void keyboardopen();
extern void keyboard_handle();



#endif
