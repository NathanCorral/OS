#ifndef RTC_H
#define RTC_H
#include "i8259.h"
#include "lib.h"

#define RTC_REG_A 0x0A
#define RTC_REG_B 0x0B
#define RTC_REG_C 0x0C
#define RTC_REG_D 0x0D
#define DISABLE_NMI 0x80
#define RTC_PORT 0x70
#define CMOS_PORT 0x71
#define BIT_SIX 0x40
#define TOP_FOUR 0xF0
#define BOT_FOUR 0x0F
#define RATE 15  // Default rate is calculated using this equation: frequency =  32768 >> (rate-1)
#define FREQ_0 0x00
#define FREQ_2 0x0F
#define FREQ_4 0x0E
#define FREQ_8 0x0D
#define FREQ_16 0x0C
#define FREQ_32 0x0B
#define FREQ_64 0x0A
#define FREQ_128 0x09
#define FREQ_256 0x08
#define FREQ_512 0x07
#define FREQ_1024 0x06

void rtc_handle();
int32_t rtc_read(void * buf, int32_t nbytes); // Blocks until interrupt
int32_t rtc_write(const void * ptr, int32_t nbytes);  // Writes new freq to rtc
int32_t rtc_open(); // set to default freq of 2hz and set the rtc flag
int32_t rtc_close(); // turns off rtc
void rtc_init();
void set_freq(char rate);

#endif

