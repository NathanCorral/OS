#ifndef TERMINAL_H
#define TERMINAL_H

void termain_init();

void terminal_shell();

void terminal_ctr( char command );

void terminal_input();

void terminal_backspace();

void update_terminal(int screen_x, int screen_y);

void terminal_scroll_up();

#endif
