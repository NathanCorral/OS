#ifndef TERMINAL_H
#define TERMINAL_H

#include "types.h"

void terminal_init();

int32_t terminal_open();

void terminal_ctr( char command );
int32_t terminal_close();
int32_t terminal_read(const int8_t *fname, uint32_t offset, uint8_t * buf, uint32_t nbytes);
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes);

void set_c();
int get_c();

void terminal_input(char key_input);

void terminal_backspace();

void update_terminal(int screen_x, int screen_y);

void terminal_enter();

void save_this_terminal(uint32_t active_terminal, int viewed, void * stdin);

#endif

