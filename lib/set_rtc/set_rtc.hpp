#ifndef SET_RTC_DATE_TIME_HPP_INCLUDED_
#define SET_RTC_DATE_TIME_HPP_INCLUDED_

// Framework libs
#include <STM32RTC.h>

// Local Includes
#include <config.hpp>

bool initRTC();

void setRTCToCompileTime();

void set_rtc_time(int year, int month, int day, int hour, int minute, int second);

DateTime get_rtc_datetime();

#endif /* SET_RTC_DATE_TIME_HPP_INCLUDED_ */