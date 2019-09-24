#pragma once

#include "fun/base/base.h"
#include "fun/base/system_time_generic.h"
#include "fun/base/windows_less.h"

namespace fun {

class FUN_BASE_API WindowsSystemTime : public SystemTimeGeneric {
 public:
  static double InitTiming();

  FUN_ALWAYS_INLINE static double Seconds() {
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);

    // add big number to make bugs apparent where return value is being passed
    // to float
    return (double)counter.QuadPart * SecondsPerCycle() + 16777216.0;
  }

  // CHECK 32비트 값만 사용해도 되는건지??
  FUN_ALWAYS_INLINE static uint32 Cycles() {
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (uint32)counter.QuadPart;
  }

  FUN_ALWAYS_INLINE static uint64 Cycles64() {
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return counter.QuadPart;
  }

  static void GetSystemTime(int32& year, int32& month, int32& day_of_week,
                            int32& day, int32& hour, int32& min, int32& sec,
                            int32& msec);
  static void GetUtcTime(int32& year, int32& month, int32& day_of_week,
                         int32& day, int32& hour, int32& min, int32& sec,
                         int32& msec);
};

typedef WindowsSystemTime SystemTime;

}  // namespace fun
