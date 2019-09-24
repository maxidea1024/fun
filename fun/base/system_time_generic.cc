//#include "fun/base/system_time_generic.h"
#include "fun/base/system_time.h"

#if FUN_PLATFORM_HAS_BSD_TIME
#include <sched.h>
#include <sys/time.h>
#endif

namespace fun {

double SystemTimeGeneric::seconds_per_cycle_ = 1.0f / 1000000.0f;
double SystemTimeGeneric::seconds_per_cycle64_ = 1.0 / 1000000.0;

#if FUN_PLATFORM_HAS_BSD_TIME

double SystemTimeGeneric::InitTiming() {
  seconds_per_cycle_ = 1.0 / 1000000.0;
  seconds_per_cycle_ = 1.0f / 1000000.0f;
  return SystemTime::Seconds();
}

void SystemTimeGeneric::GetSystemTime(int32& year, int32& month,
                                      int32& day_of_week, int32& day,
                                      int32& hour, int32& min, int32& sec,
                                      int32& msec) {
  // query for calendar time
  struct timeval tv;
  gettimeofday(&tv, nullptr);

  // convert it to local time
  struct tm local_tm;
  localtime_r(&tv.tv_sec, &local_tm);

  // pull out data/time
  year = local_tm.tm_year + 1900;
  month = local_tm.tm_mon + 1;
  day_of_week = local_tm.tm_wday;
  day = local_tm.tm_mday;
  hour = local_tm.tm_hour;
  min = local_tm.tm_min;
  sec = local_tm.tm_sec;
  msec = tv.tv_usec / 1000;
}

void SystemTimeGeneric::GetUtcTime(int32& year, int32& month,
                                   int32& day_of_week, int32& day, int32& hour,
                                   int32& min, int32& sec, int32& msec) {
  // query for calendar time
  struct timeval tv;
  gettimeofday(&tv, nullptr);

  // convert it to UTC
  struct tm gm_tm;
  gmtime_r(&tv.tv_sec, &gm_tm);

  // pull out data/time
  year = gm_tm.tm_year + 1900;
  month = gm_tm.tm_mon + 1;
  day_of_week = gm_tm.tm_wday;
  day = gm_tm.tm_mday;
  hour = gm_tm.tm_hour;
  min = gm_tm.tm_min;
  sec = gm_tm.tm_sec;
  msec = tv.tv_usec / 1000;
}
#endif  // FUN_PLATFORM_HAS_BSD_TIME

uint32 SystemTimeGeneric::Milliseconds() {
  return uint32(SystemTime::Cycles() * seconds_per_cycle_ * 1000.0);
}

uint64 SystemTimeGeneric::Milliseconds64() {
  return uint64(SystemTime::Cycles64() * seconds_per_cycle64_ * 1000.0);
}

}  // namespace fun
