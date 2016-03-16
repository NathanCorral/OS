#include "x86_desc.h"
#include "terminal.h"
#include "lib.h"
#include "spinlock.h"


int x, y, min_x, cursor, handle_inputs;

unsigned char print_dir;

//static spinlock_t lock = SPINLOCK_UNLOCKED;


void termain_init(  ){
	clear();
	x = 0;
	y = 0;
	cursor = 0;
	handle_inputs = 0;
	print_dir = 0;
	// open keyboard file
	// set starting directory
}

void terminal_shell(){
	int i;
	char c;
	//unsigned long flags;
	x = 0;
	y = 0;
	update_screen(x, y, cursor);
	printf("[root@no_directory]# ");
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
				printf("[root@no_directory]# ");
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


void terminal_ctr(char command){
	switch(command){
		case 'l' :
		case 'L' :
			clear();
			x = 0;
			y = 0;
			break;

		// Easy to add more commands for later
	}
}

void terminal_input(){
	// handle_inputs will be ignored during user programs and output
	// once we finish

	//spin_lock(lock);
	handle_inputs++;
	//spin_unlock(lock);	
}

void terminal_backspace(){
	//if(x > min_x)
		back_space();
}

void terminal_enter(){
	//print_dir = 1;
	// used for later
}

void update_terminal(screen_x, screen_y){
	x = screen_x;
	y = screen_y;
}

void terminal_scroll_up(){
	scroll_up();
}