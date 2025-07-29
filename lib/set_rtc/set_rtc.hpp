#ifndef SET_RTC_DATE_TIME_HPP_INCLUDED_
#define SET_RTC_DATE_TIME_HPP_INCLUDED_

// Framework libs
#include <STM32RTC.h>

// DateTime structure definition
struct DateTime {
    uint8_t day;
    uint8_t month;
    uint16_t year;
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
};

bool initRTC();

void setRTCToCompileTime();

void set_rtc_time(int day, int month, int year, int hour, int minute, int second);

DateTime get_rtc_datetime();

#endif /* SET_RTC_DATE_TIME_HPP_INCLUDED_ */