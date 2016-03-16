#ifndef KEYBOARD_H
#define KEYBOARD_H

extern char getc();

extern void keyboardopen();
//#include "types.h"


 extern int keyboardwrite(const unsigned char*buf, int nbytes);
 extern int keyboardread(char* buf, int nbytes);
//extern int32_t keyboardclose();
 extern void keyboardopen();
extern void keyboard_handle();



#endif
