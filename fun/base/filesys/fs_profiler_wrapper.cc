#include "containers/ticker.h"
#include "coreprivatepch.h"
#include "hal/iplatform_fsp_rofiler_wrapper.h"

namespace fun {

#if !FUN_BUILD_SHIPPING

bool g_suppress_profiled_file_log = false;

DEFINE_LOG_CATEGORY(LogProfiledFile);

// TODO stat 그룹을 string으로 처리하면 안되는건가??

DECLARE_STATS_GROUP(TEXT("File stats"), STATGROUP_FileStats, STATCAT_Advanced);
DECLARE_FLOAT_COUNTER_STAT(TEXT("Read Speed MB/s"), STAT_ReadSpeedMBs,
                           STATGROUP_FileStats);
DECLARE_DWORD_COUNTER_STAT(TEXT("Read Calls"), STAT_ReadIssued,
                           STATGROUP_FileStats);
DECLARE_FLOAT_COUNTER_STAT(TEXT("Read Size KB"), STAT_ReadSize,
                           STATGROUP_FileStats);
DECLARE_FLOAT_ACCUMULATOR_STAT(TEXT("Lifetime Average Read Size KB"),
                               STAT_LTAvgReadSize, STATGROUP_FileStats);
DECLARE_FLOAT_ACCUMULATOR_STAT(TEXT("Lifetime Average Read Speed MB/s"),
                               STAT_LTAvgReadSpeed, STATGROUP_FileStats);
DECLARE_FLOAT_ACCUMULATOR_STAT(TEXT("Total MBs Read"), STAT_TotalMBRead,
                               STATGROUP_FileStats);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Total File Read Calls"),
                               STAT_TotalReadCalls, STATGROUP_FileStats);

bool OsFSReadStatsHandle::Read(uint8* dst, int64 len_to_read) {
  double timer = SystemTime::Seconds();
  bool result = file->Read(dst, len_to_read);
  float delta = SystemTime::Seconds() - timer;
  if (delta > SMALL_NUMBER) {
    float bytes_per_sec = (len_to_read / 1024.0f) / delta;
    Atomics::Add(bytes_per_sec_counter_, (int32)(bytes_per_sec));
  }
  Atomics::Add(bytes_read_counter_, len_to_read);
  Atomics::Increment(reads_counter_);
  return result;
}

bool OsFSReadStats::Tick(float delta) {
  float real_delta = (float)(SystemTime::Seconds() - timer);
  uint32 bytes_per_sec = Atomics::Exchange(&bytes_per_sec_this_tick_, 0);
  uint32 bytes_read_tick = Atomics::Exchange(&bytes_read_this_tick_, 0);
  uint32 reads = Atomics::Exchange(&reads_this_tick_, 0);

  uint64 read_kbytes = 0;
  float read_size = 0.f;
  if (reads) {
    lifetime_read_calls_ += reads;
    read_kbytes = bytes_per_sec / reads;
    read_size = bytes_read_tick / (float)reads;
    lifetime_read_speed_ += bytes_per_sec;
    lifetime_read_size_ += bytes_read_tick / 1024.f;

    SET_FLOAT_STAT(STAT_LTAvgReadSize,
                   (lifetime_read_size_ / lifetime_read_calls_));
    SET_FLOAT_STAT(STAT_LTAvgReadSpeed,
                   (lifetime_read_speed_ / lifetime_read_calls_) / 1024.f);
  }

  SET_FLOAT_STAT(STAT_ReadSpeedMBs, read_kbytes / 1024.f);
  SET_FLOAT_STAT(STAT_ReadSize, read_size / 1024.f);
  SET_DWORD_STAT(STAT_ReadIssued, reads);
  INC_FLOAT_STAT_BY(STAT_TotalMBRead, (bytes_read_tick / (1024.f * 1024.f)));
  INC_DWORD_STAT_BY(STAT_TotalReadCalls, reads);

  timer = SystemTime::Seconds();
  return true;
}

// Wrapper function prevents Android Clang ICE (v3.3)
ALWAYS_NOINLINE void ExchangeNoInline(volatile int32* value) {
  Atomics::Exchange(value, 0);
}

bool OsFSReadStats::Initialize(IPlatformFS* inner, const char* cmdline) {
  // inner is required.
  fun_check_ptr(inner);
  lower_level_ = inner;
  Ticker::GetCoreTicker().AddTicker(
      TickerDelegate::CreateRaw(this, &OsFSReadStats::Tick), 0.f);

  lifetime_read_speed_ = 0;
  lifetime_read_size_ = 0;
  lifetime_read_calls_ = 0;
  timer = 0.f;
  ExchangeNoInline(&bytes_read_this_tick_);
  ExchangeNoInline(&reads_this_tick_);
  ExchangeNoInline(&bytes_per_sec_this_tick_);

  return !!lower_level_;
}

#endif  //! FUN_BUILD_SHIPPING

}  // namespace fun
