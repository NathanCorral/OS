#ifndef PIT_H
#define PIT_H


#include "types.h"
#include "lib.h"
#include "syscalls.h"
#include "i8259.h"

#define MODE3 0x36
#define CHANNEL0 0x40
#define CMRP 0x43
#define HZ20 59659
#define HZ33 36157
 extern void pit_handle();
 void pit_init();


#endif
