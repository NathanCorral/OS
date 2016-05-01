#include "keyboard.h"
#include "terminal.h"
#include "spinlock.h"
#include "i8259.h"
#include "syscalls.h"

#define RELEASE(key) (key |0x80)


#define buf_used(buf) ((buf.start) <= (buf.end) ? \
				  ((buf.end) - (buf.start))  \
				: (BUF_SIZE + (buf.end) - (buf.start)))

#define buf_room(buf) (BUF_SIZE - buf_used(buf) - 1)
#define buf_empty(buf) ((buf.start) == (buf.end))
#define buf_full(buf) ((((buf.end)+1)%BUF_SIZE) == (buf.start))
#define buf_incidx(idx) ((idx) = ((idx)+1) % BUF_SIZE)
#define buf_decidx(idx) ((idx) = ((idx)-1) % BUF_SIZE)


#define get_char(terminal, stdin) (stdin[terminal].buf[buf_incidx(stdin[terminal].start)])
#define write_char(terminal, stdin, char) ((stdin[terminal].buf[buf_incidx(stdin[terminal].end)]) = char)
buf_t stdin[3];

static uint8_t keychar [64]={
'\0', '\0', '1','2','3','4','5','6','7','8','9','0','-','=','\0','\0',
'q','w','e','r','t','y','u','i','o','p','[',']','\0','\0','a','s',
'd','f','g','h','j','k','l',';','\'', '`', '\0','\\', 'z','x','c','v',
'b','n','m',',','.','/', '\0','\0','\0', ' ', '\0','\0','\0','\0','\0','\0'
};
static uint8_t keyshiftchar [64]={
'\0', '\0', '!','@','#','$','%','^','&','*','(',')','_','+','\0','\0',
'Q','W','E','R','T','Y','U','I','O','P','{','}','\0','\0','A','S',
'D','F','G','H','J','K','L',':','\'', '~', '\0','|', 'Z','X','C','V',
'B','N','M','<','>','?', '\0','\0','\0', ' ', '\0','\0','\0','\0','\0','\0'
};



static spinlock_t lock = SPINLOCK_UNLOCKED;


static uint8_t ctrlset=0; 
static uint8_t shiftset=0;
static uint8_t capset=0;
static uint8_t altset = 0;

void print_keyboard_info() {
	printf(" 0:(%d, %d); 1:(%d, %d); 2:(%d, %d)", stdin[0].x,stdin[0].y,stdin[1].x,stdin[1].y,stdin[2].x,stdin[2].y);
}
int temp=1;
void set_temp() {
	temp = 1;
}
void reset_temp() {
	temp = 0;
}

void update_buffer(int term, int x, int y) {
	// if(temp) {
	// 	printf("UB from (%d,%d) to (%d,%d)",stdin[term].x, stdin[term].y, x,y);
	// }
	stdin[term].x = x;
	stdin[term].y = y;
}

void keyboard_init(){
	int i;
	for(i=0; i<3; i++) {
		stdin[i].start = 0;
		stdin[i].end = 0;
		stdin[i].x = 0;
		stdin[i].y = 0;
	}
	enable_irq(1);
}

// Get next char from input
char getc(){
	uint32_t active_terminal = getactiveterm();
	if(buf_empty(stdin[active_terminal]))
		return -1; // error check
	char c;
	c = get_char(active_terminal, stdin);
	return c;
}

void put_char_buff(char c, int term) {
	// printf("placing char in %d\n",term);
	// int x1,x2;
	// int y1,y2;
	// x1 = stdin[1].x;
	// y1 = stdin[1].y;
	// x2 = stdin[2].x;
	// y2 = stdin[2].y;

    if(c == '\n' || c == '\r') {
    	// printf("\nGot Enter\n");
    	if(stdin[term].y== NUM_ROWS-1){ //if last line, scroll
    		scroll_buff(term);
    	}
    	else{
    		stdin[term].y++;
       		stdin[term].x=0;
    	}
    } else {
    	uint32_t addr = get_vid_buf_addr(term);
		*(uint8_t *)(addr + ((NUM_COLS*(stdin[term].y) + stdin[term].x) << 1)) = c;
	    *(uint8_t *)(addr + ((NUM_COLS*stdin[term].y + stdin[term].x) << 1) + 1) = ATTRIB;
	    stdin[term].x++;
       
        // screen_x %= NUM_COLS;
        // screen_y = (screen_y + (screen_x / NUM_COLS)) % NUM_ROWS;
    }

    if(stdin[term].x==NUM_COLS){
    	stdin[term].x=0;
    	if(stdin[term].y==NUM_ROWS-1)
    		scroll_buff(term);
    	else
    		stdin[term].y++;
    }
    if(stdin[term].y==NUM_ROWS-1 && stdin[term].x==NUM_COLS){ //if on last line and get to end of line, scroll
    	scroll_buff(term);
    	stdin[term].x=0;
    }

 //    stdin[1].x = x1;
	// stdin[1].y = y1;
	// stdin[2].x = x2;
	// stdin[2].y = y2;

}

void scroll_buff(term) {
	int x, y;
	uint32_t addr = get_vid_buf_addr(term);
	for(y=0; y<NUM_ROWS-1; y++){
		for(x=0; x<NUM_COLS; x++){
		//move each row up
		*(uint8_t *)(addr+ ((NUM_COLS*y+x)<<1))=*(uint8_t *)(addr+ ((NUM_COLS*(y+1)+x)<<1));
		*(uint8_t *)(addr+ ((NUM_COLS*y+x)<<1) + 1)=*(uint8_t *)(addr+ ((NUM_COLS*(y+1)+x)<<1) + 1);
		}
	}
	//clear out last line
	for(x=0; x<NUM_COLS; x++){
		*(uint8_t *)(addr+ ((NUM_COLS*(NUM_ROWS-1)+x)<<1))= 0;
		*(uint8_t *)(addr+ ((NUM_COLS*(NUM_ROWS-1)+x)<<1) + 1)= ATTRIB;
	}
	stdin[term].x = 0;
	stdin[term].y = NUM_ROWS-1;
}

int32_t keyboard_open(){
	uint32_t active_terminal = getactiveterm();
	stdin[active_terminal].start = stdin[active_terminal].end;
	return 0;
}

int32_t keyboard_close(){
	return 0;
}

int32_t keyboard_read(void* buf, int32_t nbytes){
	int bytesread=0;
	char c;
	pcb_t * pcb = get_prog(-1);
	uint32_t active_terminal = pcb->term;

	if(buf == NULL || nbytes < 0)
		return -1;	

	// Keep writing until we fill up buffer or hit a new line
	while( bytesread < nbytes ){
		// Wait for new input
		while(buf_empty(stdin[active_terminal]));


		c = get_char(active_terminal, stdin);
		((char *) buf)[bytesread++] = c;
		if(c == '\n'){
			// stdin[active_terminal].start= stdin[active_terminal].end;
			break;	
		}
		// Get rid of backspace and input before
		else if(c == BACKSPACE){
			if(bytesread < 2) {
				bytesread--;
			}
			else{
				bytesread-=2;
				terminal_backspace();
			}
		}

		if(bytesread==126){
			if(buf_full(stdin[active_terminal]))
				buf_decidx(stdin[active_terminal].end);

			//stdin[active_terminal][buf_incidx(end)] = '\n';
			write_char(active_terminal, stdin, '\n');
			terminal_input('\n');
			terminal_enter();
		}
	}
	return bytesread;
}

int32_t keyboard_write(const void *buf, int32_t nbytes){
	// May want to call terminal write
	return terminal_write(8, buf, nbytes);
}


void keyboard_handle(){

	cli();

	uint8_t key;
	uint8_t key_input;

	key = inb(PORT);
	uint32_t active_terminal = getactiveterm();

	spin_lock(lock);

	switch(key){
		case LEFTSHIFT :
		case RIGHTSHIFT :
			shiftset=1;
			break;

		case RELEASE(LEFTSHIFT) :
		case RELEASE(RIGHTSHIFT) :
			shiftset = 0;
			break;

		case LEFTCTRL :
			ctrlset=1;
			break;

		case RELEASE(LEFTCTRL) :
			ctrlset = 0;
			break;

		case CAPS :
			capset = ~capset;
			break;

		case LEFTALT :
			altset = 1;
			break;

		case RELEASE(LEFTALT) :
			altset = 0;
			break;

		case BACKSPACE :
		//stdin[buf_incidx(end)]= '\0';
			// if(buf_empty(stdin[active_terminal])) {
			// 	break;
			// }
			// if(!buf_empty(stdin[active_terminal])){

			// 	buf_decidx(stdin[active_terminal].end);
			// }

			//stdin[active_terminal][buf_incidx(end)] = BACKSPACE;
			write_char(active_terminal, stdin, BACKSPACE);
			// terminal_backspace();
			break;

		case ENTER :
			// Throw away last character if stdin is full and
			// replace it with new line
			if(buf_full(stdin[active_terminal]))
				buf_decidx(stdin[active_terminal].end);

			//stdin[active_terminal][buf_incidx(end)] = '\n';
			write_char(active_terminal, stdin, '\n');
			terminal_input('\n');
			//terminal_enter();
			//start = end;
			break;

		// Now that we have handled all special inputs
		default :


			if(key >= ALLRELEASE)
				break;  // Release key

			// Check if key is in array
			if(key >= 64)
				break;
				//key_input = key;
			// The key is valid so we find the input
			else if(shiftset || capset)
				key_input = keyshiftchar[key];
			else
				key_input = keychar[key];

			
			// Control keys are not displayed and are used for commands.
			if(ctrlset)
				terminal_ctr(key_input);

			else if (altset){
				int viewed;
				if(key==F1)
					viewed= 0;
				else if(key==F2)
					viewed=1;
				else if (key==F3)
					viewed=2;
				else if (key_input == 'c') {
					// viewed = active_terminal;
					spin_unlock(lock);
					send_eoi(1);
					sti();
					// printf("Switching\n");
					switch_to(NULL);
					return;
				}
				else
					break;
//printf("term: %d\n", viewed);
				if (active_terminal != viewed){
					//printf("Switching Terminal\n");
					save_this_terminal(active_terminal, viewed, stdin);
					spin_unlock(lock);
					send_eoi(1);
					
					switchterm(viewed);
					sti();
					return;
			//	return;
				//some kind of iret
			}
		
			}
			

			else{
				if(buf_full(stdin[active_terminal]) || key_input == '\0')
					break;
				
				//stdin[active_terminal][buf_incidx(end)] = key_input;
				write_char(active_terminal, stdin, key_input);
		
				terminal_input(key_input);
			}

	}

	

	spin_unlock(lock);
	send_eoi(1);
	sti();
}

/*
//#include "lib.h"
#include "keyboard.h"
#include "i8259.h"
#include "lib.h"

#define LEFTCTRL 0x1D
#define LEFTALT 0x38
#define LEFTSHIFT 0x2A
#define RIGHTSHIFT 0x36
#define CAPS 0x3A

#define LEFTARROW 0x4B
#define RIGHTARROW 0x4D
#define UPARROW 0x48
#define DOWNARROW 0x50

#define ENTER 0x1C 
#define BACKSPACE 0x0E
#define DELETE 0x53
#define SPACE 0x39
#define TAB 0x0F
#define ESCAPE 0x01
#define PORT 0x60
#define LKEY 0x26
#define ALLRELEASE 0x59	

#define RELEASE(key) (key |0x80)


static uint8_t keychar [64]={
'\0', '\0', '1','2','3','4','5','6','7','8','9','0','-','=','\0','\0',
'q','w','e','r','t','y','u','i','o','p','[',']','\0','\0','a','s',
'd','f','g','h','j','k','l',';','\'', '`', '\0','\\', 'z','x','c','v',
'b','n','m',',','.','/', '\0','\0','\0', ' ', '\0','\0','\0','\0','\0','\0'
};
static uint8_t keyshiftchar [64]={
'\0', '\0', '!','@','#','$','%','^','&','*','(',')','_','+','\0','\0',
'Q','W','E','R','T','Y','U','I','O','P','{','}','\0','\0','A','S',
'D','F','G','H','J','K','L',':','\'', '~', '\0','|', 'Z','X','C','V',
'B','N','M','<','>','?', '\0','\0','\0', ' ', '\0','\0','\0','\0','\0','\0'
};

 static uint8_t ctrlset=0; //for later
 static uint8_t shiftset=0;
  static uint8_t capset=0;
  int savex=0;
  int savey=0;
char buffer[128];

// static uint8_t altset=0; //for later
int i=0;
int cursor=0;



void keyboardopen(){
	int j=0;
//clear buffer
	for (j=0; j<128; j++){
		buffer[j]='\0';
	}
	i=0;
	cursor=0;
	setcoords(0,0);
	updatecursor(cursor);
		enable_irq(1);

}

int32_t keyboardwrite(const char*buf, int32_t nbytes){
	int j=0;
	int successes=0;

	for(j=0; j<nbytes; j++){
		if(buf[j] != '\0'){
		putc(buf[j]);
		successes++;
	}
	}
return successes;
}


int32_t keyboardread( char* buf, int32_t nbytes){
int j=0;
int bytesread=0;
while(j<nbytes && buffer[j] != '\0' && buffer[j-1] !='\n'){
buf[j]= buffer[j];
bytesread++;
j++;
}

while(j<nbytes){
	buf[j]='\0';
	j++;
}
<<<<<<< .mine
// for(j=0; j<nbytes; j++){
// 	putc(buf[j]);
// }
=======
>>>>>>> .r15870

return bytesread;
}

int32_t keyboardclose(){
	return 0;
}







void keyboard_handle(){

cli();
int j;	
uint8_t key;
updatecursor(cursor);
	key=inb(PORT); //get key
	 if(key== LEFTSHIFT || key== RIGHTSHIFT)
	 	shiftset=1;
	 else if (key== RELEASE(LEFTSHIFT) || key== RELEASE(RIGHTSHIFT))
	 	shiftset=0;
	


	 if (key==CAPS) //check if capitalized or ctrl
	 	capset = ~capset;

	 if(key==LEFTCTRL)
	 	ctrlset=1;
	 else if (key==RELEASE(LEFTCTRL))
	 	ctrlset=0;

if (i>=127){ 
	if(key == ENTER || (ctrlset==1 && key== LKEY)){ //wait for enter

	i=0;
	for(j=0; j<128; j++)
	buffer[j]='\0';
}
else{
	send_eoi(1);
	return;
}

}



	if(key < ALLRELEASE){ //check if release
		if (key==ENTER){ //if enter
			putc('\n');

			for(j=0; j<128; j++) //clear buffer
				buffer[j]='\0';
					cursor=0; //reset cursor offset
			i=0;
	}
		else if (ctrlset==1 && key==LKEY){
			for(j=0; j<128; j++) //clear buffer
				buffer[j]='\0';
			i=0;
			clear(); //clear screen
			setcoords(0,0); //go to top of screen
			cursor=0;

	}
		else if(key==BACKSPACE){
			if (i>0) //go back in buffer to rewrite
			i--;
			buffer[i]=' ';
			
			 int x=getx(); //get coordinates
			 int y= gety();
			 if((x==0 && y==0) || (x==0 && i==0));
			 else
				x--; //go back

			setcoords(x, y);
			putc(' '); //print space
			setcoords(x,y); //go back to overwrite
	}
		else if (shiftset || capset){
		buffer[i]=(keyshiftchar[key]); //put in buffer
		i++;
		if (cursor <0) //make sure cursor is in correct spot
			cursor++;
		if(keyshiftchar[key] != '\0')
		putc(keyshiftchar[key]); //print
	}
		else{	
		buffer[i]=(keychar[key]);
		i++;
		if(cursor<0)
			cursor ++;
		if(keychar[key] != '\0')
		putc(keychar[key]);
		}
		char my[128];
keyboardread(my, 128);
keyboardwrite(my, 128);
	} 

	buffer[127]='\n';

updatecursor(cursor);


	send_eoi(1); //end interrupt
sti();
}

>>>>>>> .r13978
*/




