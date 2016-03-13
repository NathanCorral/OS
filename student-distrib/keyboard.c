

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
#define ALLRELEASE 0x59	

#define RELEASE(key) (key |0x80)


static uint8_t keychar [64]={
'\0', '\0', '1','2','3','4','5','6','7','8','9','0','-','=','\0','\0',
'q','w','e','r','t','y','u','i','o','p','[',']','\0','\0','a','s',
'd','f','g','h','j','k','l',';','\'', '`', '\0','\\', 'z','x','c','v',
'b','n','m',',','.','/', '\0','\0','\0', ' ', '\0','\0','\0','\0','\0','\0'
};
// static uint8_t keyshiftchar [64]={
// '\0', '\0', '!','@','#','$','%','^','&','*','(',')','_','+','\0','\0',
// 'Q','W','E','R','T','Y','U','I','O','P','{','}','\0','\0','A','S',
// 'D','F','G','H','J','K','L',':','\'', '~', '\0','|', 'Z','X','C','V',
// 'B','N','M','<','>','?', '\0','\0','\0', ' ', '\0','\0','\0','\0','\0','\0'
// };

// static uint8_t ctrlset=0; //for later
// static uint8_t shiftset=0;
// static uint8_t altset=0; //for later

void keyboard_handle(){
//printf("in keyboard\n");
cli();
	uint8_t key;


	
	key=inb(PORT); //get key
	// if(key== LEFTSHIFT || key== RIGHTSHIFT)
	// 	shiftset=1;
	// else if (key== RELEASE(LEFTSHIFT) || key== RELEASE(RIGHTSHIFT))
	// 	shiftset=0;

	if(key < ALLRELEASE) //check if release 
	putc(keychar[key]);

	

	
	send_eoi(1); //end interrupt
sti();

	



}
