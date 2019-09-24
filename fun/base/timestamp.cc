//TODO WINCE 지원은 제거하도록 하자.

#include "fun/base/timestamp.h"
#include "fun/base/timespan.h"
#include "fun/base/exception.h"

#if FUN_PLATFORM_WINDOWS_FAMILY
#include "fun/base/windows_less.h"
#if defined(_WIN32_WCE)
#include <cmath>
#endif
#endif
#include <algorithm>
#undef min
#undef max
#include <limits>
#if FUN_PLATFORM_UNIX_FAMILY
#include <time.h>
#include <unistd.h>
#if FUN_PLATFORM == FUN_PLATFORM_VXWORKS
#include <timers.h>
#else
#include <sys/time.h>
#include <sys/times.h>
#endif
#endif

#ifndef FUN_HAVE_CLOCK_GETTIME
  #if (defined(_POSIX_TIMERS) && defined(CLOCK_REALTIME)) || FUN_PLATFORM_VXWORKS || defined(__QNX__)
    #ifndef __APPLE__ // See GitHub issue #1453 - not available before Mac OS 10.12/iOS 10
      #define FUN_HAVE_CLOCK_GETTIME
    #endif
  #endif
#endif


//TODO WINCE는 지원 계획이 없으므로, 제거하는게 좋을듯...
#if defined(_WIN32_WCE) && defined(FUN_WINCE_TIMESTAMP_HACK)

//
// See <http://community.opennetcf.com/articles/cf/archive/2007/11/20/getting-a-millisecond-Resolution-datetime-under-windows-ce.aspx>
// for an explanation of the following code.
//
// In short: Windows CE system time in most cases only has a Resolution of one second.
// But we want millisecond Resolution.
//

namespace {

class TickOffset {
 public:
  TickOffset() {
    SYSTEMTIME st1, st2;
    std::memset(&st1, 0, sizeof(SYSTEMTIME));
    std::memset(&st2, 0, sizeof(SYSTEMTIME));
    GetSystemTime(&st1);
    while (true) {
      GetSystemTime(&st2);

      // wait for a rollover
      if (st1.wSecond != st2.wSecond) {
        offset_ = GetTickCount() % 1000;
        break;
      }
    }
  }

  void Calibrate(int seconds) {
    SYSTEMTIME st1, st2;
    SystemTime(&st1);

    WORD s = st1.wSecond;
    int sum = 0;
    int remaining = seconds;
    while (remaining > 0) {
      SystemTime(&st2);
      WORD s2 = st2.wSecond;

      if (s != s2) {
        remaining--;
        // store the offset from zero
        sum += (st2.wMilliseconds > 500) ? (st2.wMilliseconds - 1000) : st2.wMilliseconds;
        s = st2.wSecond;
      }
    }

    // adjust the offset by the average deviation from zero (round to the integer farthest from zero)
    if (sum < 0) {
      offset_ += (int) std::floor(sum / (float)seconds);
    } else {
      offset_ += (int) std::ceil(sum / (float)seconds);
    }
  }

  void SystemTime(SYSTEMTIME* st) {
    std::memset(st, 0, sizeof(SYSTEMTIME));

    WORD tick = GetTickCount() % 1000;
    GetSystemTime(st);
    WORD ms = (tick >= offset_) ? (tick - offset_) : (1000 - (offset_ - tick));
    st->wMilliseconds = ms;
  }

  void SystemTimeAsFileTime(FILETIME* ft) {
    SYSTEMTIME st;
    SystemTime(&st);
    SystemTimeToFileTime(&st, ft);
  }

 private:
  WORD offset_;
};


static TickOffset offset;

void GetSystemTimeAsFileTimeWithMillisecondResolution(FILETIME* ft) {
  offset.SystemTimeAsFileTime(ft);
}

} // namespace


#endif // defined(_WIN32_WCE) && defined(FUN_WINCE_TIMESTAMP_HACK)

namespace fun {

const Timestamp::TimeVal Timestamp::TIMEVAL_MIN = std::numeric_limits<Timestamp::TimeVal>::min();
const Timestamp::TimeVal Timestamp::TIMEVAL_MAX = std::numeric_limits<Timestamp::TimeVal>::max();

Timestamp Timestamp::FromEpochTime(std::time_t t) {
  return Timestamp(TimeVal(t) * Resolution());
}

Timestamp Timestamp::FromUtcTime(UtcTimeVal val) {
  val -= (TimeDiff(0x01b21dd2) << 32) + 0x13814000;
  val /= 10;
  return Timestamp(val);
}

void Timestamp::Update() {
#if FUN_PLATFORM_WINDOWS_FAMILY

  FILETIME ft;
#if defined(_WIN32_WCE) && defined(FUN_WINCE_TIMESTAMP_HACK)
  GetSystemTimeAsFileTimeWithMillisecondResolution(&ft);
#else
  GetSystemTimeAsFileTime(&ft);
#endif

  ULARGE_INTEGER epoch; // UNIX epoch (1970-01-01 00:00:00) expressed in Windows NT FILETIME
  epoch.LowPart  = 0xD53E8000;
  epoch.HighPart = 0x019DB1DE;

  ULARGE_INTEGER ts;
  ts.LowPart  = ft.dwLowDateTime;
  ts.HighPart = ft.dwHighDateTime;
  ts.QuadPart -= epoch.QuadPart;
  value_ = ts.QuadPart/10;

#elif defined(FUN_HAVE_CLOCK_GETTIME)

  struct timespec ts;
  if (clock_gettime(CLOCK_REALTIME, &ts)) {
    throw SystemException("cannot get time of day");
  }
  value_ = TimeVal(ts.tv_sec)*Resolution() + ts.tv_nsec/1000;

#else

  struct timeval tv;
  if (gettimeofday(&tv, NULL)) {
    throw SystemException("cannot get time of day");
  }
  value_ = TimeVal(tv.tv_sec)*Resolution() + tv.tv_usec;

#endif
}

Timestamp Timestamp::operator + (const Timespan& span) const {
  return *this + span.TotalMicroseconds();
}

Timestamp Timestamp::operator - (const Timespan& span) const {
  return *this - span.TotalMicroseconds();
}

Timestamp& Timestamp::operator += (const Timespan& span) {
  return *this += span.TotalMicroseconds();
}

Timestamp& Timestamp::operator -= (const Timespan& span) {
  return *this -= span.TotalMicroseconds();
}

#if defined(_WIN32)

Timestamp Timestamp::FromFileTimeNP(uint32 file_time_low, uint32 file_time_high) {
  ULARGE_INTEGER epoch; // UNIX epoch (1970-01-01 00:00:00) expressed in Windows NT FILETIME
  epoch.LowPart  = 0xD53E8000;
  epoch.HighPart = 0x019DB1DE;

  ULARGE_INTEGER ts;
  ts.LowPart  = file_time_low;
  ts.HighPart = file_time_high;
  ts.QuadPart -= epoch.QuadPart;

  return Timestamp(ts.QuadPart/10);
}

void Timestamp::ToFileTimeNP(uint32& file_time_low, uint32& file_time_high) const {
  ULARGE_INTEGER epoch; // UNIX epoch (1970-01-01 00:00:00) expressed in Windows NT FILETIME
  epoch.LowPart  = 0xD53E8000;
  epoch.HighPart = 0x019DB1DE;

  ULARGE_INTEGER ts;
  ts.QuadPart  = value_*10;
  ts.QuadPart += epoch.QuadPart;
  file_time_low  = ts.LowPart;
  file_time_high = ts.HighPart;
}

#endif //(_WIN32)

} // namespace fun
