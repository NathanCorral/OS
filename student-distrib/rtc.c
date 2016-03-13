#include "rtc.h"
volatile int count = 0;
void rtc_init(){
	// Turn on IRQ 8 for the periodic interrupts
	
	cli();  // Disables interrupts
	outb((RTC_REG_B|DISABLE_NMI), RTC_PORT); // Disable NMI and select register B
	char prev_b = inb(CMOS_PORT);  // Stores the current value of register B
	outb((RTC_REG_B|DISABLE_NMI),RTC_PORT); // Sets the index
	outb((prev_b | BIT_SIX), CMOS_PORT);  // Turns on bit 6 of register B
	

	// Sets the frequency of the rtc driver to 2 HZ
	//RATE &= BOT_FOUR; // Checks if rate is at least 2 and not above 15
	
	outb((RTC_REG_A|DISABLE_NMI), RTC_PORT); // Disable NMI and select register B
	char prev_a = inb(CMOS_PORT);  // Stores the current value of register B
	outb((RTC_REG_A|DISABLE_NMI),RTC_PORT); // Resets the index to B
	outb(((prev_a & TOP_FOUR) | RATE),CMOS_PORT); // Writes the rate to B
	sti(); // Enables interrupts again

}

void rtc_handle(){
	// NOTE::: delete cli/ sti after we make a common handler
	//cli();
	printf("%d in rtc \n", count++);
	send_eoi(8);
	outb(RTC_REG_C,RTC_PORT);	// select register C
	inb(CMOS_PORT);		// just throw away contents
	//sti();
}
