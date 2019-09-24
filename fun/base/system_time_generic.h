#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * 시스템 시각을 조회합니다.
 *
 * Seconds() - 시스템의 현재 시각을 초 단위로 반환합니다.
 * Milliseconds() - 시스템의 현재 시각을 밀리초 단위로 반환합니다.
 * Cycles() - 시스템의 현재 시각을 Cycle 단위로 반환합니다.
 */
class FUN_BASE_API SystemTimeGeneric {
 public:
#if FUN_PLATFORM_HAS_BSD_TIME
  // TODO 명시적으로 호출하지 않고 처리하는 방법이 없을까??
  static double InitTiming();

  FUN_ALWAYS_INLINE static double Seconds() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return ((double)tv.tv_sec) + (((double)tv.tv_usec) / 1000000.0);
  }

  FUN_ALWAYS_INLINE static uint32 Cycles() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return (uint32)((((uint64)tv.tv_sec) * 1000000ULL) +
                    (((uint64)tv.tv_usec)));
  }

  FUN_ALWAYS_INLINE static uint64 Cycles64() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return ((((uint64)tv.tv_sec) * 1000000ULL) + (((uint64)tv.tv_usec)));
  }

  static void GetSystemTime(int32& year, int32& month, int32& day_of_week,
                            int32& day, int32& hour, int32& min, int32& sec,
                            int32& msec);
  static void GetUtcTime(int32& year, int32& month, int32& day_of_week,
                         int32& day, int32& hour, int32& min, int32& sec,
                         int32& msec);
#endif  // FUN_PLATFORM_HAS_BSD_TIME

  // CHECK 32비트 값으로 처리가 가능할까?? overflow는 호출자 쪽에서 처리하는
  // 것으로 해야할까? 뭐 64비트로 한다고 해도 오버플로우가 없는건 아니지만...
  static uint32 Milliseconds();
  static uint64 Milliseconds64();

  static uint32 CycleToMilliseconds(const uint32 cycles) {
    return uint32(cycles * seconds_per_cycle_);
  }

  static double SecondsPerCycle() { return seconds_per_cycle_; }

 protected:
  static double seconds_per_cycle_;
  static double seconds_per_cycle64_;
};

}  // namespace fun
