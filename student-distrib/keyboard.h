#ifndef KEYBOARD_H
#define KEYBOARD_H


#include "types.h"


 extern int32_t keyboardwrite(const unsigned char*buf, int32_t nbytes);
 extern int32_t keyboardread(char* buf, int32_t nbytes);
 extern int32_t keyboardclose();
 extern void keyboardopen();
extern void keyboard_handle();
#endif
