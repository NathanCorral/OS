/* lib.c - Some basic library functions (printf, strlen, etc.)
 * vim:ts=4 noexpandtab
 */

#include "lib.h"
#include "terminal.h"
#include "syscalls.h"
#include "syscallhandle.h"
#include "paging.h"
#include "keyboardirq.h"
#include "x86_desc.h"
 #include "keyboard.h"

 // 0xFA0 is 4000 Bytes, the size of video memory
#define VIDEO_SIZE 0x0FA0 - ((NUM_COLS)*(NUM_ROWS) << 1) // (4 kB - space to store one screen)
#define SCROLL_MAX VIDEO_SIZE
 // Right now scroll max is 0 since we only have enough video memory for 1 screen


static int screen_x;
static int screen_y;
static char* video_mem = (char *)VIDEO;
static unsigned int offset = 0;
static int scrolled;
static int activeterm=0;
//static int processingterm=0;

static char * vidbuff[3]={(char *) VIDBUF0, (char *) VIDBUF1, (char *) VIDBUF2};
 uint32_t saveesp[3];
 uint32_t saveebp[3];
 uint32_t savess0[3];
 uint32_t paddrsave[3];
 uint32_t saveesp0[3];
/*
* void clear(void);
*   Inputs: void
*   Return Value: none
*	Function: Clears video memory
*/

// Prints some debug info at the bottem of the screen
// Prints number of processes running
// prints name and number of active process running
// prints active terminal
void help_debug() {

	int x,y, num_process = 0;
	x = screen_x;
	y = screen_y;
	screen_y = 20;
	screen_x = 44;
	pcb_t * pcb = get_prog(-1);
	if(pcb == NULL)
		return;
	printf("Term %d Run %s num %d term %d\n", activeterm, pcb->temp, pcb->process, pcb->term);
	screen_y = 21;
	screen_x = 44;
	printf("Screen x %d,  Screen y %d\n",x,y);
	screen_y = 22;
	screen_x = 44;
	// update_buffer(activeterm, x, y);
	print_keyboard_info();
	screen_y = 23;
	screen_x = 44;
	num_process = 1;
	pcb_t * cur = pcb->next;
	while(cur != pcb) {
		num_process++;
		cur = cur->next;
	}
	screen_x = 47;
	printf("P: %d\n", num_process);
	screen_x = x;
	screen_y = y;
	// update_buffer(activeterm, x, y);
	updatecursor(0);
	//pcb_t * pcb = get_process(&num_process);
}

void init_mem() {
    int32_t i, term;
    scrolled = 0;
    screen_x = 0;
    screen_y = 0;
    clear();
    uint32_t addr;
    for(term=0; term<3; term++){
    	addr = get_vid_buf_addr(term);
    	for(i=0; i<NUM_ROWS*NUM_COLS; i++) {
	        *(uint8_t *)(addr + (i << 1)) = ' ';
	       *(uint8_t *)(addr + (i << 1) + 1) = ATTRIB;
	    }
    }
}

void
clear(void)
{
    int32_t i;
    scrolled = 0;
    screen_x = 0;
    screen_y = 0;
    // update_buffer(activeterm, screen_x, screen_y);

    for(i=0; i<NUM_ROWS*NUM_COLS; i++) {
        *(uint8_t *)(video_mem + (i << 1)) = ' ';
       *(uint8_t *)(video_mem + (i << 1) + 1) = ATTRIB;
    }
}

void
clear_re(void)
{
    int32_t i;
    scrolled = 0;
    screen_x = 0;
    screen_y = 0;

    for(i=0; i<NUM_ROWS*NUM_COLS; i++) {
        *(uint8_t *)(video_mem + (i << 1)) = ' ';
       *(uint8_t *)(video_mem + (i << 1) + 1) = ATTRIB;
    }
}

uint32_t get_vid_buf_addr(int term) {
	return (uint32_t) vidbuff[term];
}

void back_space(){
	if(screen_x > 0){
		screen_x--;
		putc(' ');
		screen_x--;
	}
	else if(screen_y > 0){
		screen_y--;
		screen_x = NUM_COLS - 2;
		putc(' ');
		// Because it moves to the next line after putchar
		screen_y--;
		screen_x = NUM_COLS - 2;
	}
	
	update_terminal(screen_x, screen_y);
	updatecursor(0);
}

void update_screen(int x, int y){
	// Save updating for only when we need it
	screen_x = x;
	screen_y = y;
	updatecursor(0);
}

void setactiveterm(int term){
	if(term>=0 && term<3)
		activeterm=term;
}

void switchterm(int newterm){
	//save all
	// asm volatile("movl %%esp, %0":"=g"(saveesp[activeterm]));
	// saveesp0[activeterm]=tss.esp0;
	// asm volatile("movl %%ebp, %0":"=g"(saveebp[activeterm]));
	// savess0[activeterm]= tss.ss0;
	// asm volatile("movl %%cr3, %0":"=g"(paddrsave[activeterm]));
	


	if(newterm>=0 && newterm<3){

		//cli();
		// memcpy(vidbuff[activeterm], video_mem, KB4); //save video memory
		memcpy(video_mem, vidbuff[newterm], KB4); //show new memory
	
		setactiveterm(newterm);


		int run= getrunning(activeterm);
		// printf("active: %x\n", activeterm);
		// printf("running: %x\n", run);
		if(run==0){
			clear();
			sti();
			execute("shell");
			return;
		}
		pcb_t * pcb = get_prog(activeterm);
		//sti();
		switch_to(pcb);
		//sti();
	}
return;

	
}

// void switchterm(int newterm, int pk){
	
// 	//save all

// 	int now= nowrunning();
// 	if(now==0){
// 		return;
// 	}


// int temp;
// 	pcb_t * pcb;
// 	pcb = ((pcb_t *)((uint32_t)&temp & PCBALIGN));

// 	asm volatile("movl %%esp, %0":"=g"(pcb->oldesp));
// 	pcb->oldesp0=tss.esp0;
// 	asm volatile("movl %%ebp, %0":"=g"(pcb->oldebp));
// 	pcb->oldss0= tss.ss0;
// 	// asm volatile("movl %%cr3, %%eax;"
// 	// 	"movl %%eax, %0;":"=g"(pcb->oldpage)::"%eax");

	
// int run= getrunning(newterm);


// 	if((newterm>=0 && newterm<3)&&(pk>=0 && pk<=2)){
// //runningterm=newterm;
// 		if(pk==0 || pk==2){
// 		memcpy(vidbuff[activeterm], video_mem, KB4); //save video memory
// 		memcpy(video_mem, vidbuff[newterm], KB4); //show new memory

// 		//mapback(pcb->process);
		
// 		setactiveterm(newterm);
// 		// remap();

		

		
// 		// printf("active: %x\n", activeterm);
// 		// printf("running: %x\n", run);

// 		if(run==0){
			
// 			clear();
// 			//update_screen(0,0);
// 			//update_terminal(0,0);
// 			execute("shell");
// 		}



// 		}
	
// 	if(run==0)
// 		return;

// // 	uint8_t running=nowrunning();
// // 	uint8_t current=getcurrent();

// // 	uint8_t next= (current+1)%8;
// // 	uint8_t bitmask= 0x80;
// // 	int i;
// // 	pcb_t * pcbt;

// // 	for(i=0; i<next; i++){
// // 		bitmask=bitmask >>1;
// // 	}

// // 	while(current != next){

// // 		if(bitmask &running){
// // 			pcbt= (pcb_t *) (MB8-KB8*(next+1));

// // 			if(pcb->haschild==0)
// // 				break;
// // 		}
// // 		next= (next+1)%8;
// // 		bitmask= bitmask >>1;
// // 		if (bitmask==0)
// // 			bitmask= 0x80;
// // 	}

// // 	if(current== next)
// // 		return;

// // pcbt= (pcb_t *)(MB8-KB8*(next+1));

// //need new pcb

// uint32_t paddr=getaddr(pcb->process);
// //mapforward(pcb->process);
// 		asm volatile ("				\
// 		movl %0, %%cr3 \n\
// 		movl %%cr4, %%eax	\n\
// 		orl $0x90, %%eax	\n\
// 		movl %%eax, %%cr4	\n\
// 		movl %%cr0, %%eax \n\
// 		orl $0x80000000, %%eax	\n\
// 		movl %%eax, %%cr0"
// 		:
// 		:"r" (paddr)
// 		:"%eax"
// 		);

// 		tss.ss0= pcb->oldss0;
// 		tss.esp0=pcb->oldesp0;
// 		setkstack(tss.esp0);
		
// 		asm volatile ("movl %0, %%ebp     ;"
// 		"movl %1, %%esp     ;"
// 		::"g"(pcb->oldebp), "g"(pcb->oldesp));

// 	}
// }

uint32_t getactiveterm(){
	return activeterm;
}

int32_t gety(){
return screen_y;
}
int32_t getx(){
return screen_x;
}

//sets coordinates within screen after checking if valid
void setcoords(int x, int y){
if(x<0){

	y--;
	x= NUM_COLS-1;
	
}
if(x>=NUM_COLS){
	x=0;
	y++;
}
if (y<0)
	y=0;
if (y>=NUM_ROWS)
	y=NUM_ROWS-1;
screen_x=x;
screen_y=y;

}
//scroll down when at the bottom of the terminal
//moves all printed text up a row and clears bottom row
void scroll(){
	int x, y;


	if(scrolled < SCROLL_MAX){
		
		offset += NUM_COLS;
		scrolled++;
	
		 outb(0x0D, 0x3D4); 
	     outb((unsigned char)(offset&0xFF), 0x3D5); 
	     outb(0x0C, 0x3D4); 
	     outb((unsigned char)((offset>>8)&0xFF), 0x3D5);

	    video_mem += (NUM_COLS << 1);
	    /*
		for(x=0; x<NUM_COLS; x++){
			*(uint8_t *)(video_mem+ ((NUM_COLS*(NUM_ROWS-1)+x)<<1))= 0;
		}
		*/
		updatecursor(0);		
	}

	else{
		// We have done all the scrolling in memory we can
		for(y=0; y<NUM_ROWS-1; y++){
			for(x=0; x<NUM_COLS; x++){
			//move each row up
			*(uint8_t *)(video_mem+ ((NUM_COLS*y+x)<<1))=*(uint8_t *)(video_mem+ ((NUM_COLS*(y+1)+x)<<1));
			}
		}
		//clear out last line
		for(x=0; x<NUM_COLS; x++){
			*(uint8_t *)(video_mem+ ((NUM_COLS*(NUM_ROWS-1)+x)<<1))= 0;
		}
		screen_x = 0;
		screen_y = NUM_ROWS-1;
		update_terminal(screen_x, screen_y);	
	}
}

void scroll_up(){
	
	if(scrolled > 0){
		scrolled--;
		offset -= NUM_COLS;
		outb(0x0D, 0x3D4); 
		outb((unsigned char)(offset&0xFF), 0x3D5); 
		outb(0x0C, 0x3D4); 
		outb((unsigned char)((offset>>8)&0xFF), 0x3D5);

		video_mem -= (NUM_COLS << 1);
		updatecursor(0);
	}
}

//print cursor to current screen position with offset x
void updatecursor(int x){

int position= (screen_y*NUM_COLS)+screen_x + x + offset;

 outb(0x0F, 0x3D4); 
     outb((unsigned char)(position&0xFF), 0x3D5); 
     /* cursor HIGH port to vga INDEX register */ 
     outb(0x0E, 0x3D4); 
     outb((unsigned char )((position>>8)&0xFF), 0x3D5); 


}

/* Standard printf().
 * Only supports the following format strings:
 * %%  - print a literal '%' character
 * %x  - print a number in hexadecimal
 * %u  - print a number as an unsigned integer
 * %d  - print a number as a signed integer
 * %c  - print a character
 * %s  - print a string
 * %#x - print a number in 32-bit aligned hexadecimal, i.e.
 *       print 8 hexadecimal digits, zero-padded on the left.
 *       For example, the hex number "E" would be printed as
 *       "0000000E".
 *       Note: This is slightly different than the libc specification
 *       for the "#" modifier (this implementation doesn't add a "0x" at
 *       the beginning), but I think it's more flexible this way.
 *       Also note: %x is the only conversion specifier that can use
 *       the "#" modifier to alter output.
 * */

int32_t
printf(int8_t *format, ...)
{
	/* Pointer to the format string */
	int8_t* buf = format;
	/* Stack pointer for the other parameters */
	int32_t* esp = (void *)&format;
	esp++;

	while(*buf != '\0') {
		switch(*buf) {
			case '%':
				{
					int32_t alternate = 0;
					buf++;

format_char_switch:
					/* Conversion specifiers */
					switch(*buf) {
						/* Print a literal '%' character */
						case '%':
							putc('%');
							break;

						/* Use alternate formatting */
						case '#':
							alternate = 1;
							buf++;
							/* Yes, I know gotos are bad.  This is the
							 * most elegant and general way to do this,
							 * IMHO. */
							goto format_char_switch;

						/* Print a number in hexadecimal form */
						case 'x':
							{
								int8_t conv_buf[64];
								if(alternate == 0) {
									itoa(*((uint32_t *)esp), conv_buf, 16);
									puts(conv_buf);
								} else {
									int32_t starting_index;
									int32_t i;
									itoa(*((uint32_t *)esp), &conv_buf[8], 16);
									i = starting_index = strlen(&conv_buf[8]);
									while(i < 8) {
										conv_buf[i] = '0';
										i++;
									}
									puts(&conv_buf[starting_index]);
								}
								esp++;
							}
							break;

						/* Print a number in unsigned int form */
						case 'u':
							{
								int8_t conv_buf[36];
								itoa(*((uint32_t *)esp), conv_buf, 10);
								puts(conv_buf);
								esp++;
							}
							break;

						/* Print a number in signed int form */
						case 'd':
							{
								int8_t conv_buf[36];
								int32_t value = *((int32_t *)esp);
								if(value < 0) {
									conv_buf[0] = '-';
									itoa(-value, &conv_buf[1], 10);
								} else {
									itoa(value, conv_buf, 10);
								}
								puts(conv_buf);
								esp++;
							}
							break;

						/* Print a single character */
						case 'c':
							putc( (uint8_t) *((int32_t *)esp) );
							esp++;
							break;

						/* Print a NULL-terminated string */
						case 's':
							puts( *((int8_t **)esp) );
							esp++;
							break;

						default:
							break;
					}

				}
				break;

			default:
				putc(*buf);
				break;
		}
		buf++;
	}
	return (buf - format);
}

/*
* int32_t puts(int8_t* s);
*   Inputs: int_8* s = pointer to a string of characters
*   Return Value: Number of bytes written
*	Function: Output a string to the console 
*/

int32_t
puts(int8_t* s)
{
	register int32_t index = 0;
	while(s[index] != '\0') {
		putc(s[index]);
		index++;
	}

	return index;
}

/*
* void putc(uint8_t c);
*   Inputs: uint_8* c = character to print
*   Return Value: void
*	Function: Output a character to the console 
*/
// int key_write = -1;
// void set_key_write(int term) {
// 	key_write = term;
// }

void
putc(uint8_t c)
{
	// pcb_t * pcb = get_prog(-1);
	// if((pcb != NULL) && (pcb->term != activeterm)) {
	// 	put_char_buff(c, pcb->term);
 //    	return;
	// }
	// if(key_write != activeterm) {
	// 	return;
	// }

    if(c == '\n' || c == '\r') {
    	if(screen_y== NUM_ROWS-1){ //if last line, scroll
    		scroll();
   //  		screen_x = 0;
			// screen_y = NUM_ROWS-1;
    	}
    	else
        screen_y++;
        screen_x=0;
    } else {
		*(uint8_t *)(video_mem + ((NUM_COLS*screen_y + screen_x) << 1)) = c;
        *(uint8_t *)(video_mem + ((NUM_COLS*screen_y + screen_x) << 1) + 1) = ATTRIB;
        screen_x++;
       
        // screen_x %= NUM_COLS;
        // screen_y = (screen_y + (screen_x / NUM_COLS)) % NUM_ROWS;
    }

    if(screen_x==NUM_COLS){
    	screen_x=0;
    	if(screen_y==NUM_ROWS-1){
    		scroll();
   //  		screen_x = 0;
			// screen_y = NUM_ROWS-1;
    	}
    	else
    		screen_y++;
    }
    if(screen_y==NUM_ROWS-1 && screen_x==NUM_COLS){ //if on last line and get to end of line, scroll
    	scroll();
  //   	screen_x = 0;
		// screen_y = NUM_ROWS-1;
    }
    // put_char_buff(c, activeterm);
    update_terminal(screen_x, screen_y);
    // update_buffer(activeterm, screen_x, screen_y);
	updatecursor(0);
}

/*
* int8_t* itoa(uint32_t value, int8_t* buf, int32_t radix);
*   Inputs: uint32_t value = number to convert
*			int8_t* buf = allocated buffer to place string in
*			int32_t radix = base system. hex, oct, dec, etc.
*   Return Value: number of bytes written
*	Function: Convert a number to its ASCII representation, with base "radix"
*/

int8_t*
itoa(uint32_t value, int8_t* buf, int32_t radix)
{
	static int8_t lookup[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	int8_t *newbuf = buf;
	int32_t i;
	uint32_t newval = value;

	/* Special case for zero */
	if(value == 0) {
		buf[0]='0';
		buf[1]='\0';
		return buf;
	}

	/* Go through the number one place value at a time, and add the
	 * correct digit to "newbuf".  We actually add characters to the
	 * ASCII string from lowest place value to highest, which is the
	 * opposite of how the number should be printed.  We'll reverse the
	 * characters later. */
	while(newval > 0) {
		i = newval % radix;
		*newbuf = lookup[i];
		newbuf++;
		newval /= radix;
	}

	/* Add a terminating NULL */
	*newbuf = '\0';

	/* Reverse the string and return */
	return strrev(buf);
}

/*
* int8_t* strrev(int8_t* s);
*   Inputs: int8_t* s = string to reverse
*   Return Value: reversed string
*	Function: reverses a string s
*/

int8_t*
strrev(int8_t* s)
{
	register int8_t tmp;
	register int32_t beg=0;
	register int32_t end=strlen(s) - 1;

	while(beg < end) {
		tmp = s[end];
		s[end] = s[beg];
		s[beg] = tmp;
		beg++;
		end--;
	}

	return s;
}

/*
* uint32_t strlen(const int8_t* s);
*   Inputs: const int8_t* s = string to take length of
*   Return Value: length of string s
*	Function: return length of string s
*/

uint32_t
strlen(const int8_t* s)
{
	register uint32_t len = 0;
	while(s[len] != '\0')
		len++;

	return len;
}

/*
* void* memset(void* s, int32_t c, uint32_t n);
*   Inputs: void* s = pointer to memory
*			int32_t c = value to set memory to
*			uint32_t n = number of bytes to set
*   Return Value: new string
*	Function: set n consecutive bytes of pointer s to value c
*/

void*
memset(void* s, int32_t c, uint32_t n)
{
	c &= 0xFF;
	asm volatile("                  \n\
			.memset_top:            \n\
			testl   %%ecx, %%ecx    \n\
			jz      .memset_done    \n\
			testl   $0x3, %%edi     \n\
			jz      .memset_aligned \n\
			movb    %%al, (%%edi)   \n\
			addl    $1, %%edi       \n\
			subl    $1, %%ecx       \n\
			jmp     .memset_top     \n\
			.memset_aligned:        \n\
			movw    %%ds, %%dx      \n\
			movw    %%dx, %%es      \n\
			movl    %%ecx, %%edx    \n\
			shrl    $2, %%ecx       \n\
			andl    $0x3, %%edx     \n\
			cld                     \n\
			rep     stosl           \n\
			.memset_bottom:         \n\
			testl   %%edx, %%edx    \n\
			jz      .memset_done    \n\
			movb    %%al, (%%edi)   \n\
			addl    $1, %%edi       \n\
			subl    $1, %%edx       \n\
			jmp     .memset_bottom  \n\
			.memset_done:           \n\
			"
			:
			: "a"(c << 24 | c << 16 | c << 8 | c), "D"(s), "c"(n)
			: "edx", "memory", "cc"
			);

	return s;
}

/*
* void* memset_word(void* s, int32_t c, uint32_t n);
*   Inputs: void* s = pointer to memory
*			int32_t c = value to set memory to
*			uint32_t n = number of bytes to set
*   Return Value: new string
*	Function: set lower 16 bits of n consecutive memory locations of pointer s to value c
*/

/* Optimized memset_word */
void*
memset_word(void* s, int32_t c, uint32_t n)
{
	asm volatile("                  \n\
			movw    %%ds, %%dx      \n\
			movw    %%dx, %%es      \n\
			cld                     \n\
			rep     stosw           \n\
			"
			:
			: "a"(c), "D"(s), "c"(n)
			: "edx", "memory", "cc"
			);

	return s;
}

/*
* void* memset_dword(void* s, int32_t c, uint32_t n);
*   Inputs: void* s = pointer to memory
*			int32_t c = value to set memory to
*			uint32_t n = number of bytes to set
*   Return Value: new string
*	Function: set n consecutive memory locations of pointer s to value c
*/

void*
memset_dword(void* s, int32_t c, uint32_t n)
{
	asm volatile("                  \n\
			movw    %%ds, %%dx      \n\
			movw    %%dx, %%es      \n\
			cld                     \n\
			rep     stosl           \n\
			"
			:
			: "a"(c), "D"(s), "c"(n)
			: "edx", "memory", "cc"
			);

	return s;
}

/*
* void* memcpy(void* dest, const void* src, uint32_t n);
*   Inputs: void* dest = destination of copy
*			const void* src = source of copy
*			uint32_t n = number of byets to copy
*   Return Value: pointer to dest
*	Function: copy n bytes of src to dest
*/

void*
memcpy(void* dest, const void* src, uint32_t n)
{
	asm volatile("                  \n\
			.memcpy_top:            \n\
			testl   %%ecx, %%ecx    \n\
			jz      .memcpy_done    \n\
			testl   $0x3, %%edi     \n\
			jz      .memcpy_aligned \n\
			movb    (%%esi), %%al   \n\
			movb    %%al, (%%edi)   \n\
			addl    $1, %%edi       \n\
			addl    $1, %%esi       \n\
			subl    $1, %%ecx       \n\
			jmp     .memcpy_top     \n\
			.memcpy_aligned:        \n\
			movw    %%ds, %%dx      \n\
			movw    %%dx, %%es      \n\
			movl    %%ecx, %%edx    \n\
			shrl    $2, %%ecx       \n\
			andl    $0x3, %%edx     \n\
			cld                     \n\
			rep     movsl           \n\
			.memcpy_bottom:         \n\
			testl   %%edx, %%edx    \n\
			jz      .memcpy_done    \n\
			movb    (%%esi), %%al   \n\
			movb    %%al, (%%edi)   \n\
			addl    $1, %%edi       \n\
			addl    $1, %%esi       \n\
			subl    $1, %%edx       \n\
			jmp     .memcpy_bottom  \n\
			.memcpy_done:           \n\
			"
			:
			: "S"(src), "D"(dest), "c"(n)
			: "eax", "edx", "memory", "cc"
			);

	return dest;
}

/*
* void* memmove(void* dest, const void* src, uint32_t n);
*   Inputs: void* dest = destination of move
*			const void* src = source of move
*			uint32_t n = number of byets to move
*   Return Value: pointer to dest
*	Function: move n bytes of src to dest
*/

/* Optimized memmove (used for overlapping memory areas) */
void*
memmove(void* dest, const void* src, uint32_t n)
{
	asm volatile("                  \n\
			movw    %%ds, %%dx      \n\
			movw    %%dx, %%es      \n\
			cld                     \n\
			cmp     %%edi, %%esi    \n\
			jae     .memmove_go     \n\
			leal    -1(%%esi, %%ecx), %%esi    \n\
			leal    -1(%%edi, %%ecx), %%edi    \n\
			std                     \n\
			.memmove_go:            \n\
			rep     movsb           \n\
			"
			:
			: "D"(dest), "S"(src), "c"(n)
			: "edx", "memory", "cc"
			);

	return dest;
}

/*
* int32_t strncmp(const int8_t* s1, const int8_t* s2, uint32_t n)
*   Inputs: const int8_t* s1 = first string to compare
*			const int8_t* s2 = second string to compare
*			uint32_t n = number of bytes to compare
*	Return Value: A zero value indicates that the characters compared 
*					in both strings form the same string.
*				A value greater than zero indicates that the first 
*					character that does not match has a greater value 
*					in str1 than in str2; And a value less than zero 
*					indicates the opposite.
*	Function: compares string 1 and string 2 for equality
*/

int32_t
strncmp(const int8_t* s1, const int8_t* s2, uint32_t n)
{
	int32_t i;
	for(i=0; i<n; i++) {
		if( (s1[i] != s2[i]) ||
				(s1[i] == '\0') /* || s2[i] == '\0' */ ) {

			/* The s2[i] == '\0' is unnecessary because of the short-circuit
			 * semantics of 'if' expressions in C.  If the first expression
			 * (s1[i] != s2[i]) evaluates to false, that is, if s1[i] ==
			 * s2[i], then we only need to test either s1[i] or s2[i] for
			 * '\0', since we know they are equal. */

			return s1[i] - s2[i];
		}
	}
	return 0;
}

/*
* int8_t* strcpy(int8_t* dest, const int8_t* src)
*   Inputs: int8_t* dest = destination string of copy
*			const int8_t* src = source string of copy
*   Return Value: pointer to dest
*	Function: copy the source string into the destination string
*/

int8_t*
strcpy(int8_t* dest, const int8_t* src)
{
	int32_t i=0;
	while(src[i] != '\0') {
		dest[i] = src[i];
		i++;
	}

	dest[i] = '\0';
	return dest;
}

/*
* int8_t* strcpy(int8_t* dest, const int8_t* src, uint32_t n)
*   Inputs: int8_t* dest = destination string of copy
*			const int8_t* src = source string of copy
*			uint32_t n = number of bytes to copy
*   Return Value: pointer to dest
*	Function: copy n bytes of the source string into the destination string
*/

int8_t*
strncpy(int8_t* dest, const int8_t* src, uint32_t n)
{
	int32_t i=0;
	while(src[i] != '\0' && i < n) {
		dest[i] = src[i];
		i++;
	}

	while(i < n) {
		dest[i] = '\0';
		i++;
	}

	return dest;
}

/*
* void test_interrupts(void)
*   Inputs: void
*   Return Value: void
*	Function: increments video memory. To be used to test rtc
*/

void
test_interrupts(void)
{
	int32_t i;
	for (i=0; i < NUM_ROWS*NUM_COLS; i++) {
		video_mem[i<<1]++;
	}
}


