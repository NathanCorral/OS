#ifndef SPINLOCK_H
#define SPINLOCK_H

#include "lib.h"

typedef struct spinlock_type { 
	unsigned char lock;
} spinlock_t;



#define SPINLOCK_UNLOCKED (spinlock_t) { 0 }

#define SPINOCK_LOCKED (spinlock_t) { 1 }


#define spin_lock(lock)					\
do {									\
	asm volatile(						\
		"movb $1, %%al \n\t"			\
		"1: \n\t"						\
		"xchgb %%al, (%0) \n\t"			\
		"testb %%al, %%al \n\t"			\
		"jnz 1b"						\
		:								\
		: "r" (lock)					\
		: "cc", "%al", "memory"			\
		);								\
} while(0)

		
#define spin_unlock(lock)				\
do {									\
	asm volatile(						\
		"movb $0, (%%eax)"				\
		:								\
		: "a" (lock)					\
		: "memory"						\
		);								\
} while(0)

/*
My implemenation does not work for some reason.
Use cli_and_save to keep flags

#define spin_lock_irqsave(lock, flags)	\
do {									\
	asm volatile(						\
		"pushfl \n\t"					\
		"popl %0"						\
		"movb $1, %%al \n\t"			\
		"1: \n\t"						\
		"xchgb %%al, (%1) \n\t"			\
		"testb %%al, %%al \n\t"			\
		"jnz 1b"						\
		:	"=c" (flags)				\
		: "r" (lock)					\
		: "cc", "%al", "memory"			\
		);							\
} while(0)

#define spin_unlock_irqsave(lock, flags)\
do {									\
	asm volatile(						\
		"pushl %0 \n\t"					\
		"popfl \n\t"					\
		"movb $0, (%%eax) \n\t"			\
		:								\
		: "r" (flags), "a" (lock)		\
		: "cc", "memory"				\
		);								\
} while(0)


*/

#endif
