
#include "terminal.h"

int x, y, handle_inputs;

unsigned char echo_input;

//static spinlock_t lock = SPINLOCK_UNLOCKED;

void terminal_init(){
	// Wait for terminal to be opened
	handle_inputs = 0;
	echo_input = 0;
}


int32_t terminal_open(){
	x = 0;
	y = 0;
	clear();
	echo_input = 1;
	handle_inputs = 0;
	update_screen(x, y);
	keyboard_open();
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

int32_t terminal_read(const int8_t *fname, uint32_t offset, uint8_t * buf, uint32_t nbytes){
	return keyboard_read(buf, nbytes);
}

int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes){
	int i;
	if(buf == NULL)
		return -1;
	for(i=0; i<nbytes; i++){
		putc(((char *) buf)[i]);
	}
	return nbytes;
}


void terminal_ctr(char command){
	switch(command){
		case 'l' :
		case 'L' :
			clear();
			x = 0;
			y = 0;
			setcoords(x,y);
			update_screen(x,y);
			keyboard_open();
			break;

		case UPARROW:
			scroll_up();
			break;

		case DOWNARROW:
			scroll();
			break;
	}
}

void terminal_input(char key_input){
	// handle_inputs will be ignored during user programs and output
	// once we finish
	if(echo_input){
		putc(key_input);
	}
	// Save for later
	else
		handle_inputs++;
}

void terminal_backspace(){
	if(echo_input)
		back_space();
}

void terminal_enter(){
	// possible use for later
}

void update_terminal(screen_x, screen_y){
	x = screen_x;
	y = screen_y;
}

void save_this_terminal(uint32_t active_terminal, int viewed, void * stdin) {
	buf_t * st = (buf_t *) stdin;
	st[active_terminal].x = x;
	st[active_terminal].y = y;
	update_screen(st[viewed].x, st[viewed].y);
}

// Unused.  May be repurposed for shell
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


