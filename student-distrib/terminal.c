#include "x86_desc.h"
#include "terminal.h"
#include "lib.h"
#include "spinlock.h"
#include "keyboard.h"


int x, y, handle_inputs;

unsigned char echo_input;

//static spinlock_t lock = SPINLOCK_UNLOCKED;

void terminal_init(){
	// Wait for terminal to be opened
	handle_inputs = 0;
	echo_input = 0;
}


int32_t terminal_open(){
	int i;
	char c;
	clear();
	x = 0;
	y = 0;
	echo_input = 1;
	update_screen(x, y);
	// Output inputs taken before terminal was opened
	// but after it was initialized
	for(i=0; i<handle_inputs; i++){
		c = getc();
		if(c != -1)
			putc(c);
	}
	return 0;
}


int32_t terminal_close(){
	clear();
	x = 0;
	y = 0;
	echo_input = 0;
	update_screen(x,y);
	return 0;
}

int32_t terminal_read(void* buf, int32_t nbytes){
	return 0;
}

int32_t terminal_write(const void* buf, int32_t nbytes){
	int i;
	if(buf == NULL)
		return -1;
	for(i=0; i<nbytes; i++){
		putc(((char *) buf)[i]);
	}
	return 0;
}


// Unused.  May be repurposed for terminal
/*
void terminal_shell(){
	int i;
	char c;
	//unsigned long flags;
	x = 0;
	y = 0;
	clear();
	update_screen(x, y);
	//printf("[root@no_directory]# ");
	min_x = x;
	while(1){
		update_screen(x, y, cursor);
		// We dont need to update that often.
		
		if(handle_inputs > 0){
			// No more updates
			//spin_lock_irqsave(lock, flags);
			for(i=0; i<handle_inputs; i++){
				c = getc();
				if(c != -1)
					putc(c);
			}
			handle_inputs = 0;
			if(print_dir == 1){
				//printf("[root@no_directory]# ");
				min_x = x;
				print_dir = 0;
			}
			//spin_unlock_irqrestore(lock, flags);
		}
		// Not much else to do yet
		// Soon we will want to parse input for commands
		// and execute programs (add them to scheduler)
		// check for new program to run on scheduler
		// May want to move terminal shell to new file
	}
}
*/


void terminal_ctr(char command){
	switch(command){
		case 'l' :
		case 'L' :
			clear();
			x = 0;
			y = 0;
			break;

		case UPARROW:
			scroll_up();
			break;

		case DOWNARROW:
			scroll();
			break;
	}
}

void terminal_input(){
	// handle_inputs will be ignored during user programs and output
	// once we finish
	if(echo_input){
		char c;
		c = getc();
		if(c != -1)
			putc(c);
	}
	else
		handle_inputs++;
}

void terminal_backspace(){
	//if(x > min_x)
		back_space();
}

void terminal_enter(){
	// possible use for later
}

void update_terminal(screen_x, screen_y){
	x = screen_x;
	y = screen_y;
}

