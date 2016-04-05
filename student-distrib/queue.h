#ifndef QUEUE_H
#define QUEUE_H

// Copy and pasted from stack which is why inputs are "stack"

// Limit size of queue
#define QUEUE_MAX 20
#include "types.h"

// Always push/pop things of 4 or less bytes
typedef struct queue_type {
	int start;
	int end;
	uint32_t buf[QUEUE_MAX];

} queue_t;

// Use circular buffer for implementation
#define queue_used(stack) ((stack.start) <= (stack.end) ? \
				  ((stack.end) - (stack.start))  \
				: (QUEUE_MAX + (stack.end) - (stack.start)))

#define queue_room(stack) (QUEUE_MAX - queue_used(stack) - 1)
#define queue_empty(stack) ((stack.start) == (stack.end))
#define queue_full(stack) ((((stack.end)+1)%QUEUE_MAX) == (stack.start))
#define queue_incidx(idx) ((idx) = ((idx)+1) % QUEUE_MAX)
#define queue_decidx(idx) ((idx) = ((idx)-1) % QUEUE_MAX)

// Keep in mind the lack of error checking.  
// Pushing onto a full stack will override the smallest value
#define queue_push(stack, push) stack.buf[queue_incidx(stack.end)] = push

#define queue_pop(stack) stack.buf[queue_incidx(stack.start)]

#define queue_init(stack) stack.start = 0; stack.end = 0;

#endif
