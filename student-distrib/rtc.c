#include "rtc.h"
volatile int count = 0;
volatile int interrupt = 0;  // Checks if an interrupt has occurred
void rtc_handle(){
	// NOTE::: delete cli/ sti after we make a common handler
	cli();
	//printf("%d in rtc \n", count++);
	send_eoi(8);
	interrupt++;
	outb(RTC_REG_C,RTC_PORT);	// select register C
	inb(CMOS_PORT);		// just throw away contents
	sti();
}

void rtc_init(){
	// Turn on IRQ 8 for the periodic interrupts
	
	cli();  // Disables interrupts
	enable_irq(8);
	outb((RTC_REG_B|DISABLE_NMI), RTC_PORT); // Disable NMI and select register B
	char prev_b = inb(CMOS_PORT);  // Stores the current value of register B
	outb((RTC_REG_B|DISABLE_NMI),RTC_PORT); // Sets the index
	outb((prev_b | BIT_SIX), CMOS_PORT);  // Turns on bit 6 of register B
	sti();

	// Sets the frequency of the rtc driver to 2 HZ
	set_freq(RATE);
}

int32_t rtc_open(){
	return 0;
}

int32_t rtc_close(int32_t fd){
	return 0; 
}
int32_t rtc_read(const int8_t *fname, uint32_t offset, uint8_t * buf, uint32_t nbytes){
	sti();
	interrupt = 0;
	while(interrupt == 0){ // Do nothing until the interrupt handler clears the flag
		/* Wait for an interrupt to occur */
	}
	// interrupt = 0; // Clears the flag
	return 0;
}

int32_t rtc_write(int32_t fd, const void* ptr, int32_t nbytes){
	int32_t freq;
	char rate;
	// Checks if the ptr is null
	if(ptr == NULL){
		return -1;  // Indicates failure
	}
	freq = *((int32_t *) ptr);
	// Checks if the rate is valid and 4 bytes long
	if(nbytes != 4){
		return -1; // Indicates failure
	} 

	// Sets the frequency of the rtc driver according to the input parameter and limit it to 1024 HZ
	switch(freq){
		case 0: rate = FREQ_0;
				break;
		case 2: rate = FREQ_2;
				break;
		case 4: rate = FREQ_4;
				break;
		case 8: rate = FREQ_8;
				break;
		case 16: rate = FREQ_16;
				break;
		case 32: rate = FREQ_32;
				break;
		case 64: rate = FREQ_64;
				break;
		case 128: rate = FREQ_128;
				break;
		case 256: rate = FREQ_256;
				break;
		case 512: rate = FREQ_512;
				break;
		case 1024: rate = FREQ_1024;
				break;
		default: return -1; // Invalid rate if it is anything higher than 1024
	} 

	// Sets new frequency rate to the rtc port
	set_freq(rate);

	return 0; // Writes new rate to rtc driver successfully
}

void set_freq(char rtc_rate){
	cli(); // Disables interrupts
	// Stores value of register A to prevent overwriting
	outb((RTC_REG_A|DISABLE_NMI), RTC_PORT); // Disable NMI and select register A
	char prev_a = inb(CMOS_PORT);  // Stores the current value of register A
	outb((RTC_REG_A|DISABLE_NMI),RTC_PORT); // Resets the index to A
	outb(((prev_a & TOP_FOUR) | rtc_rate),CMOS_PORT); // Writes the new rate to A
	sti(); // Enables interrupts again
}
