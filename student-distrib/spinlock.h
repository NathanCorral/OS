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
		"xchgb %%al, %0 \n\t"			\
		"testb %%al, %%al \n\t"			\
		"jnz 1b"						\
		: "=r" (lock)					\
		: "0" (lock)					\
		: "cc", "%al", "memory"			\
		);								\
} while(0)

		
#define spin_unlock(lock)				\
do {									\
	asm volatile(						\
		"movb $0, %0"  					\
		: "=r" (lock)					\
		: 								\
		: "cc"							\
		);								\
} while(0)

#define spin_lock_irqsave(lock, flags)	\
do {									\
	cli();								\
	asm volatile( "pushfl \n\t"			\
		"popl %0  \n\t"					\
		"movb $1, %%al \n\t"			\
		"1: \n\t"						\
		"xchgb %%al, %1 \n\t"			\
		"testb %%al, %%al \n\t"			\
		"jnz 1b"						\
		: "=r"(flags), "=r" (lock)		\
		: "1" (lock)					\
		: "cc", "%al", "memory"			\
		);								\
} while(0)


#define spin_unlock_irqrestore(lock, flags)	\
do {										\
	spin_unlock(lock);						\
	restore_flags(flags);					\
	sti();									\
} while(0)


/*
Does not compile.

#define spin_unlock_irqrestore(lock, flags)	\
do {										\
	asm volatile(  "movb $0, %0 \n\t"		\
		"pushl %1 \n\t"						\
		"popfl "							\
		:  "=r"(lock)	: "r"(flags)  : );  \						
} while(0)
*/



#endif
