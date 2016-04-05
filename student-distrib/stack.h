#ifndef STACK_H
#define STACK_H

// Limit size of stack
#define STACK_MAX 40
#include "types.h"

// Always push/pop things of 4 or less bytes
typedef struct stack_type {
	int start;
	int end;
	uint32_t buf[STACK_MAX];

} stack_t;

// Use circular buffer for implementation
#define stack_used(stack) ((stack.start) <= (stack.end) ? \
				  ((stack.end) - (stack.start))  \
				: (STACK_MAX + (stack.end) - (stack.start)))

#define stack_room(stack) (STACK_MAX - stack_used(stack) - 1)
#define stack_empty(stack) ((stack.start) == (stack.end))
#define stack_full(stack) ((((stack.end)+1)%STACK_MAX) == (stack.start))
#define stack_incidx(idx) ((idx) = ((idx)+1) % STACK_MAX)
#define stack_decidx(idx) ((idx) = ((idx)-1) % STACK_MAX)

// Keep in mind the lack of error checking.  
// Pushing onto a full stack will override the smallest value
#define stack_push(stack, push) stack.buf[stack_incidx(stack.end)] = push

#define stack_pop(stack) stack.buf[stack.end]; stack_decidx(stack.end)

#define stack_init(stack) stack.start = 0; stack.end = 0;

#endif
