#pragma once

#include "fun/base/base.h"
#include "fun/base/container/list.h"
#include "fun/base/ftl/function.h"
#include "fun/base/nullable.h"
#include "fun/base/timespan.h"
#include "fun/base/timestamp.h"

namespace fun {

// FUN_BASE_API DECLARE_LOG_CATEGORY_EXTERN(LogTickableTimer, Info, All);

/**
 *
 *
 * 주의:
 * Thread Safe하지 않음. 외부에서 mutex로 보호해주어야함.
 */
class FUN_BASE_API TickableTimer {
 public:
  typedef uint32 IdType;

  struct Context {
    IdType GetId() const;

    bool IsOneshot() const;

    const Timespan& GetPeriod() const;
    void SetPeriod(const Timespan& period);

    const String& GetTag() const;
    void SetTag(const String& tag);

    TickableTimer* GetTimer() const;

    int32 GetRepeatLimit() const;
    int32 GetExpiredCount() const;

    // TODO
    // void Pause();
    // void Resume();

   private:
    friend class TickableTimer;

    void* entry;
    TickableTimer* timer;

    Context(void* entry, TickableTimer* timer) : entry(entry), timer(timer) {}
  };

  typedef TFunction<void(Context&)> Handler;

  TickableTimer(const String& name = String());
  ~TickableTimer();

  const String& GetTimerName() const;
  void SetTimerName(const String& name);

  void ClearCentralCallback();
  void SetCentralCallback(const Handler& handler);

  // oneshot
  IdType ExpireAt(const Timestamp& time, const Handler& handler,
                  const String& tag = String());

  // oneshot
  IdType ExpireAfter(const Timespan& delay, const Handler& handler,
                     const String& tag = String());

  // repeated
  IdType ExpireRepeatedly(const Timespan& period, const Handler& handler,
                          int32 repeat_limit = 0, const String& tag = String());

  IdType Expire(const Timestamp& first_time, const Timespan& period,
                const Handler& handler, int32 repeat_limit = -1,
                const String& tag = String());

  bool Cancel(const IdType id);
  void CancelAll();

  bool IsScheduled(const IdType id) const;

  bool IsOneshot(const IdType id) const;

  bool Pause(const IdType id);
  bool Resume(const IdType id);
  bool IsPaused(const IdType id) const;

  bool GetPeriod(const IdType id, Timespan& out_period) const;
  bool SetPeriod(const IdType id, const Timespan& period);

  bool GetTag(const IdType id, String& out_tag) const;
  bool SetTag(const IdType id, const String& tag);

  int32 Tick();
  int32 Tick(const Timestamp& abs_time);

  // TODO 통계관리

 private:
  struct Entry {
    IdType id;
    Timestamp next_expire_time;
    // Nullable<Timestamp> last_expired_time;
    Timespan period;
    int32 repeat_limit;
    int32 expired_count;
    // Nullable<Timespan> last_spent_time;
    Handler handler;
    String tag;

    // Nullable<Timespan> elapsed_time_at_pause_requested;
    // bool resume_requested;
  };
  List<Entry*> entries_;
  List<Entry*> paused_entries_;
  IdType next_id_;
  Timestamp last_ticked_abs_time_;
  String timer_name_;
  Handler central_callback_;

  Entry* FindEntry(const IdType id);
  const Entry* FindEntry(const IdType id) const;

  Entry* FindPausedEntry(const IdType id);
  const Entry* FindPausedEntry(const IdType id) const;

  void Pause(Entry* entry);
  void Resume(Entry* entry);

  void ExpireOne(Entry* entry, const Timestamp& abs_time);
  IdType Schedule(Entry* entry, List<Entry*>& list);
  void Reschedule(Entry* entry);
};

}  // namespace fun
