#include "fun/base/system_time.h"
#include <iostream>

namespace fun {

double WindowsSystemTime::InitTiming() {
  LARGE_INTEGER freq;
  //fun_verify(QueryPerformanceFrequency(&freq));
  QueryPerformanceFrequency(&freq);

  SystemTime::seconds_per_cycle_ = 1.0f / (float)freq.QuadPart;
  SystemTime::seconds_per_cycle64_ = 1.0 / (double)freq.QuadPart;

  std::cout << "SystemTime::seconds_per_cycle_ => " << SystemTime::seconds_per_cycle_ << std::endl;
  std::cout << "SystemTime::seconds_per_cycle64_ => " << SystemTime::seconds_per_cycle64_ << std::endl;

  return SystemTime::Seconds();
}

void WindowsSystemTime::GetSystemTime(int32& year,
                                      int32& month,
                                      int32& day_of_week,
                                      int32& day,
                                      int32& hour,
                                      int32& min,
                                      int32& sec,
                                      int32& msec) {
  SYSTEMTIME sys_time;
  ::GetLocalTime(&sys_time);

  year = sys_time.wYear;
  month = sys_time.wMonth;
  day_of_week = sys_time.wDayOfWeek;
  day = sys_time.wDay;
  hour = sys_time.wHour;
  min = sys_time.wMinute;
  sec = sys_time.wSecond;
  msec = sys_time.wMilliseconds;
}

void WindowsSystemTime::GetUtcTime(int32& year, int32& month, int32& day_of_week, int32& day, int32& hour, int32& min, int32& sec, int32& msec) {
  SYSTEMTIME sys_time;
  ::GetSystemTime(&sys_time);

  year = sys_time.wYear;
  month = sys_time.wMonth;
  day_of_week = sys_time.wDayOfWeek;
  day = sys_time.wDay;
  hour = sys_time.wHour;
  min = sys_time.wMinute;
  sec = sys_time.wSecond;
  msec = sys_time.wMilliseconds;
}

} // namespace fun
