#ifndef TERMINAL_H
#define TERMINAL_H

#include "types.h"
#include "lib.h"
#include "spinlock.h"
#include "keyboard.h"
#include "x86_desc.h"


void terminal_init();

int32_t terminal_open();

void terminal_ctr( char command );
int32_t terminal_close();
int32_t terminal_read(const int8_t *fname, uint32_t offset, uint8_t * buf, uint32_t nbytes);
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes);

void terminal_input(char key_input);

void terminal_backspace();

void update_terminal(int screen_x, int screen_y);

void terminal_enter();

#endif
