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

void rtc_init();
void rtc_handle();



#endif

