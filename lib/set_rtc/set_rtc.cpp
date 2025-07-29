// Local Includes
#include "set_rtc.hpp"
#include "config.hpp"

// Use the default STM32 RTC instance
STM32RTC &rtc = STM32RTC::getInstance();

int monthStrToNumber(const char *month)
{
    if (strcmp(month, "Jan") == 0)
        return 1;
    if (strcmp(month, "Feb") == 0)
        return 2;
    if (strcmp(month, "Mar") == 0)
        return 3;
    if (strcmp(month, "Apr") == 0)
        return 4;
    if (strcmp(month, "May") == 0)
        return 5;
    if (strcmp(month, "Jun") == 0)
        return 6;
    if (strcmp(month, "Jul") == 0)
        return 7;
    if (strcmp(month, "Aug") == 0)
        return 8;
    if (strcmp(month, "Sep") == 0)
        return 9;
    if (strcmp(month, "Oct") == 0)
        return 10;
    if (strcmp(month, "Nov") == 0)
        return 11;
    if (strcmp(month, "Dec") == 0)
        return 12;
    return 0;
}

bool initRTC()
{
    rtc.begin();
    if (!rtc.isTimeSet() || SET_RTC_BOOT)
    {
        setRTCToCompileTime();
    }
    return true;
}

void setRTCToCompileTime()
{
    // Data fixa: 29/07/2025 18:32:00
    set_rtc_time(29, 7, 2025, 18, 32, 0);
}

void set_rtc_time(int day, int month, int year, int hour, int minute, int second)
{
    rtc.setTime(hour, minute, second);
    rtc.setDate(1, day, month, year - 2000); // weekday dummy (1=Monday)
}

DateTime get_rtc_datetime()
{
    DateTime dt;
    dt.day = rtc.getDay();
    dt.month = rtc.getMonth();
    dt.year = rtc.getYear() + 2000; // Adjust for year format
    dt.hours = rtc.getHours();
    dt.minutes = rtc.getMinutes();
    dt.seconds = rtc.getSeconds();
    return dt;
}