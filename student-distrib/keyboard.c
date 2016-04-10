
#include "keyboard.h"

#define RELEASE(key) (key |0x80)


#define BUF_SIZE 128

#define buf_used(start,end) ((start) <= (end) ? \
				  ((end) - (start))  \
				: (BUF_SIZE + (end) - (start)))

#define buf_room(start,end) (BUF_SIZE - buf_used(start,end) - 1)
#define buf_empty(start, end) ((start) == (end))
#define buf_full(start, end) ((((end)+1)%BUF_SIZE) == (start))
#define buf_incidx(idx) ((idx) = ((idx)+1) % BUF_SIZE)
#define buf_decidx(idx) ((idx) = ((idx)-1) % BUF_SIZE)

char stdin[BUF_SIZE];
int start, end;


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


void keyboard_init(){
	start = 0;
	end = 0;
	enable_irq(1);
}

// Get next char from input
char getc(){
	if(buf_empty(start,end))
		return -1; // error check
	char c;
	
	c = stdin[buf_incidx(start)];
	return c;
}

int32_t keyboard_open(){
	start = end;
	return 0;
}

int32_t keyboard_close(){
	return 0;
}

int32_t keyboard_read(void* buf, int32_t nbytes){
	int bytesread=0;
	char c;

	if(buf == NULL || nbytes < 0)
		return -1;	

	// Keep writing until we fill up buffer or hit a new line
	while( bytesread < nbytes ){
		// Wait for new input
		while(buf_empty(start,end));

		c = stdin[buf_incidx(start)];
		((char *) buf)[bytesread++] = c;
		if(c == '\n'){

start= end;
			break;	}	
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
			if(!buf_empty(start,end)){

				buf_decidx(end);
			}

			terminal_backspace();
			break;

		case ENTER :
			// Throw away last character if stdin is full and
			// replace it with new line
			if(buf_full(start,end))
				buf_decidx(end);

			stdin[buf_incidx(end)] = '\n';
			//terminal_enter();
			terminal_input('\n');
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
			

			else{
				if(buf_full(start, end) || key_input == '\0')
					break;
				
				stdin[buf_incidx(end)] = key_input;
		
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
//char readbuffer[128];
// static uint8_t altset=0; //for later
int i=0;
int cursor=0;

//this will all have to change to deal with the buffer

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

if (i>=128){ 
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
			 if(x==0 && y==0);
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

