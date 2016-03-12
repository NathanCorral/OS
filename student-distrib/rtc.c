#include "rtc.h"
volatile int count = 0;
void rtc_init(){
	// Turn on IRQ 8 for the periodic interrupts
	int rate = RATE;
	rate &= BOT_FOUR; // Checks if rate is at least 2 and not above 15
	cli();  // Disables interrupts

	outb(RTC_REG_B, RTC_PORT); // Disable NMI and select register B
	char prev_b = inb(CMOS_PORT);  // Stores the current value of register B
	outb(RTC_REG_B,RTC_PORT); // Sets the index
	outb((prev_b | BIT_SIX), CMOS_PORT);  // Turns on bit 6 of register B

	// Sets the frequency of the rtc driver to 2 HZ
	outb(RTC_REG_A, RTC_PORT); // Disable NMI and select register A
	char prev_a = inb(CMOS_PORT);  // Stores the current value of register A
	outb(RTC_REG_A,RTC_PORT); // Resets the index to A
	outb(((prev_a & TOP_FOUR) | 15),CMOS_PORT); // Writes the rate to A

	sti(); // Enables interrupts again

}

void rtc_handle(){
	// NOTE::: delete cli/ sti after we make a common handler
	//cli();
	printf("%d in rtc", count++);
	outb(0x0C,0x70);	// select register C
	inb(0x71);		// just throw away contents
	send_eoi(8);
	//sti();
}
