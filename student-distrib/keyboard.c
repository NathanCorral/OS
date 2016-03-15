

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
char buffer[128];
char readbuffer[128];
// static uint8_t altset=0; //for later
int i=0;


//this will all have to change to deal with the buffer



int32_t keyboardread(int32_t fd, char* buf, int32_t nbytes){
int j=0;
int bytesread=0;
while(j<nbytes && buffer[j] != '\0'){
buf[j]= buffer[j];
bytesread++;
j++;
}

while(j<nbytes){
	buf[j]='\0';
	j++;
}
for(j=0; j<nbytes; j++){
	putc(buf[j]);
}

int y=gety();
setcoords(0, y);
return bytesread;
}

void keyboard_handle(){
//printf("in keyboard\n");
cli();
int j;	
uint8_t key;

	key=inb(PORT); //get key
	 if(key== LEFTSHIFT || key== RIGHTSHIFT)
	 	shiftset=1;
	 else if (key== RELEASE(LEFTSHIFT) || key== RELEASE(RIGHTSHIFT))
	 	shiftset=0;
if (i>=80){ 
	i=0;
	for(j=0; j<128; j++)
	buffer[j]='\0';
	//putc('\n');
}

	 if (key==CAPS)
	 	capset = ~capset;

	 if(key==LEFTCTRL)
	 	ctrlset=1;
	 else if (key==RELEASE(LEFTCTRL))
	 	ctrlset=0;

	if(key < ALLRELEASE){
		if (key==ENTER){
			putc('\n');
			for(j=0; j<128; j++)
				
				buffer[j]='\0';
			i=0;
		}
		else if (ctrlset==1 && key==LKEY){
			for(j=0; j<128; j++)
				buffer[j]='\0';
			i=0;
			clear();
			setcoords(0,0);

		}
		else if(key==BACKSPACE){
			if (i>0)
			i--;
			buffer[i]=' ';
		}
		else if (shiftset || capset){
		buffer[i]=(keyshiftchar[key]);
		i++;
		//putc(keyshiftchar[key]);
	}
		else{	
		buffer[i]=(keychar[key]);
		i++;
		//putc(keychar[key]);
		}
		
	} //check if release 
	

for(j=0; j<128; j++){
	if(buffer[j] != '\0')
	putc(buffer[j]);
}
int y= gety();
setcoords(0,y);
//keyboardread(0, readbuffer, 80);
	send_eoi(1); //end interrupt
sti();
}
