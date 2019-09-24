#include "fun/base/tickable_timer.h"

namespace fun {

// DEFINE_LOG_CATEGORY(LogTickableTimer);

//
// TickableTimer
//

TickableTimer::TickableTimer(const String& name)
    : next_id_(1), timer_name_(name) {}

TickableTimer::~TickableTimer() { CancelAll(); }

const String& TickableTimer::GetTimerName() const { return timer_name_; }

void TickableTimer::SetTimerName(const String& name) { timer_name_ = name; }

TickableTimer::IdType TickableTimer::ExpireAt(const Timestamp& time,
                                              const Handler& handler,
                                              const String& tag) {
  return Expire(time, Timespan::Zero, handler, -1, tag);
}

TickableTimer::IdType TickableTimer::ExpireAfter(const Timespan& delay,
                                                 const Handler& handler,
                                                 const String& tag) {
  if (delay <= Timespan::Zero) {
    // 경고정도로 처리하고 말아야할까??
  }

  return Expire(Timestamp::Now() + delay, Timespan::Zero, handler, -1, tag);
}

TickableTimer::IdType TickableTimer::ExpireRepeatedly(const Timespan& period,
                                                      const Handler& handler,
                                                      int32 repeat_limit,
                                                      const String& tag) {
  if (period <= Timespan::Zero) {
    // 경고정도로 처리하고 말아야할까??
  }

  // 처음에는 수행하지 않음.  최소한 period 이후에...
  return Expire(Timestamp::Now() + period, period, handler, repeat_limit, tag);
}

TickableTimer::IdType TickableTimer::Expire(const Timestamp& first_time,
                                            const Timespan& period,
                                            const Handler& handler,
                                            int32 repeat_limit,
                                            const String& tag) {
  Entry* entry = new Entry();
  entry->id = next_id_++;
  entry->next_expire_time = first_time.ToUniversalTime();
  // entry->last_expired_time = nullptr;
  entry->period = period;
  entry->repeat_limit = repeat_limit;
  entry->tag = tag;
  entry->expired_count = 0;
  // entry->last_spent_time = nullptr;
  entry->handler = handler;
  return Schedule(entry, entries_);
}

bool TickableTimer::Cancel(const IdType id) {
  for (auto it = entries_.CreateIterator(); it; ++it) {
    if (it->id == id) {
      delete *it;
      entries_.Remove(it);
      return true;
    }
  }
  return false;
}

void TickableTimer::CancelAll() {
  for (auto it = entries_.CreateIterator(); it; ++it) {
    delete *it;
  }
  entries_.Clear();

  for (auto it = paused_entries_.CreateIterator(); it; ++it) {
    delete *it;
  }
  paused_entries_.Clear();
}

bool TickableTimer::IsScheduled(const IdType id) const {
  return FindEntry(id) != nullptr || FindPausedEntry(id) != nullptr;
}

bool TickableTimer::IsOneshot(const IdType id) const {
  auto entry = FindEntry(id);
  return entry && entry->period <= Timespan::Zero;
}

bool TickableTimer::GetPeriod(const IdType id, Timespan& out_period) const {
  if (auto entry = FindEntry(id)) {
    out_period = entry->period;
    return true;
  }
  return false;
}

bool TickableTimer::SetPeriod(const IdType id, const Timespan& period) {
  if (auto entry = FindEntry(id)) {
    entry->period = period;
    // NextExpireTime을 수정해주어야할까??
    return true;
  }
  return false;
}

bool TickableTimer::Pause(const IdType id) {
  if (auto entry = FindEntry(id)) {
    Pause(entry);
    return true;
  }
  return false;
}

bool TickableTimer::Resume(const IdType id) {
  if (auto entry = FindPausedEntry(id)) {
    Resume(entry);
    return true;
  }
  return false;
}

bool TickableTimer::IsPaused(const IdType id) const {
  return FindPausedEntry(id) != nullptr;
}

bool TickableTimer::GetTag(const IdType id, String& out_tag) const {
  if (auto entry = FindEntry(id)) {
    out_tag = entry->tag;
    return true;
  }
  return false;
}

bool TickableTimer::SetTag(const IdType id, const String& tag) {
  if (auto entry = FindEntry(id)) {
    entry->tag = tag;
    return true;
  }
  return false;
}

void TickableTimer::ClearCentralCallback() { central_callback_ = Handler(); }

void TickableTimer::SetCentralCallback(const Handler& handler) {
  central_callback_ = handler;
}

int32 TickableTimer::Tick() { return Tick(Timestamp::Now()); }

int32 TickableTimer::Tick(const Timestamp& abs_time) {
  const Timestamp utc_abs_time = abs_time.ToUniversalTime();

  last_ticked_abs_time_ = abs_time.ToUniversalTime();

  // TODO 여기서 다시 실행되는 부분이 있을 수 있으므로, 이에 대한 대응을
  // 해주어야함.
  // Period가 0이하로 설정되면 스케줄링에서 아예 제거되므로, 연달아 수행되는
  // 현상은 발생할 수 없으므로, 별다른 처리는 필요치 않음.
  //
  // 하지만,
  // Context.GetTimer()를 통해서 현재 가장 앞선 시간보다 이전으로 설정해버리면
  // 무한 반복의 위험이 있을 수 있으므로, 이에 대한 대응을 해주어야함.

  /*
  for (auto it = paused_entries_.CreateIterator(); it; ++it) {
    if (it->resume_requested && !it->elapsed_time_at_pause_requested.IsNull()) {
      it->next_expire_time = abs_time + it->elapsed_time_at_pause_requested;
      it->elapsed_time_at_pause_requested = nullptr;

      Reschedule(*it);

      paused_entries_.Remove(it);
    }
  }
  */

  /*
  //TODO paused된 것들은 실행큐에 옮기지 않는걸로??
  List<Entry*> execution_queue = MoveTemp(entries_);

  int32 expired_count = 0;
  while (!execution_queue.IsEmpty()) {
    auto front = execution_queue.CutFront();

    if (Front->next_expire_time > utc_abs_time) {
      break;
    }

    ExpireOne(front, utc_abs_time);

    //TODO 만약 내부에서 paused된 상태라면 어떻게 되는가??

    if (front->period <= Timespan::Zero) {
      delete front;
    }

    expired_count++;
  }
  */

  int32 expired_count = 0;

  if (!entries_.IsEmpty() &&
      entries_.Front()->next_expire_time <= utc_abs_time) {
    List<Entry*> repeated_entries;

    while (!entries_.IsEmpty() &&
           entries_.Front()->next_expire_time <= utc_abs_time) {
      auto front = entries_.CutFront();

      ExpireOne(front, utc_abs_time);

      if (front->period <= Timespan::Zero) {
        delete front;
      } else {
        front->next_expire_time = utc_abs_time + front->period;
        // Schedule(front, repeated_entries);
        repeated_entries.Append(front);
      }

      expired_count++;
    }

    // 목록을 교체함.
    // Swap(repeated_entries, entries_);

    for (auto& entry : repeated_entries) {
      Schedule(entry, entries_);
    }
  }

  return expired_count;
}

void TickableTimer::ExpireOne(Entry* entry, const Timestamp& abs_time) {
  // fun_log(LogTickableTimer,Info,TEXT(">> TickableTimer.%s"), *entry->tag);

  entry->expired_count++;
  // entry->last_expired_time = abs_time;

  // call handler function.
  Context context(entry, this);

  if (central_callback_) {
    central_callback_(context);
  }

  entry->handler(context);

  // TODO 로깅 및 트래킹...

  if (entry->period <= Timespan::Zero) {
    // oneshot, 호출부에서 엔트리 자체를 삭제할것이므로, 별다른 처리가 필요
    // 없음.
    return;
  }

  if (entry->repeat_limit > 0 && entry->expired_count >= entry->repeat_limit) {
    // 최대 반복 횟수를 초과했으므로, 엔트리에서 제거하도록 함.
    entry->period = Timespan::Zero;  // 호출부에서 엔트리를 제거하도록 표시함.
    return;
  }
}

TickableTimer::Entry* TickableTimer::FindEntry(const IdType id) {
  for (auto it = entries_.CreateIterator(); it; ++it) {
    if (it->id == id) {
      return *it;
    }
  }
  return nullptr;
}

const TickableTimer::Entry* TickableTimer::FindEntry(const IdType id) const {
  for (auto it = entries_.CreateConstIterator(); it; ++it) {
    if (it->id == id) {
      return *it;
    }
  }
  return nullptr;
}

TickableTimer::Entry* TickableTimer::FindPausedEntry(const IdType id) {
  for (auto it = paused_entries_.CreateIterator(); it; ++it) {
    if (it->id == id) {
      return *it;
    }
  }
  return nullptr;
}

const TickableTimer::Entry* TickableTimer::FindPausedEntry(
    const IdType id) const {
  for (auto it = paused_entries_.CreateConstIterator(); it; ++it) {
    if (it->id == id) {
      return *it;
    }
  }
  return nullptr;
}

TickableTimer::IdType TickableTimer::Schedule(Entry* entry,
                                              List<Entry*>& list) {
  bool is_inserted = false;
  for (auto it = list.CreateIterator(); it; ++it) {
    if (it->next_expire_time >= entry->next_expire_time) {
      list.InsertBefore(it, entry);
      is_inserted = true;
      break;
    }
  }

  if (!is_inserted) {
    list.Append(entry);
  }

  return entry->id;
}

void TickableTimer::Pause(Entry* entry) {
  ////이미 paused된지 확인.
  // if (entry->elapsed_time_at_pause_requested.IsNull()) {
  //  //액티브 목록에서 제거한 후 Paused 목록으로 옮겨주어야함.
  //}
}

void TickableTimer::Resume(Entry* entry) {
  // paused된 상태에서만 resume이 가능함.
  //
  // if (!entry->elapsed_time_at_pause_requested.IsNull()) {
  //  //재실행이 가능 하도록 rescheduling되어야함.
  //}
}

//
// TickableTimer::Context
//

TickableTimer::IdType TickableTimer::Context::GetId() const {
  return ((const Entry*)entry)->id;
}

bool TickableTimer::Context::IsOneshot() const {
  return ((const Entry*)entry)->period <= Timespan::Zero;
}

const Timespan& TickableTimer::Context::GetPeriod() const {
  return ((const Entry*)entry)->period;
}

void TickableTimer::Context::SetPeriod(const Timespan& period) {
  ((Entry*)entry)->period = period;
}

const String& TickableTimer::Context::GetTag() const {
  return ((const Entry*)entry)->tag;
}

void TickableTimer::Context::SetTag(const String& tag) {
  ((Entry*)entry)->tag = tag;
}

TickableTimer* TickableTimer::Context::GetTimer() const { return timer; }

int32 TickableTimer::Context::GetRepeatLimit() const {
  return ((const Entry*)entry)->repeat_limit;
}

int32 TickableTimer::Context::GetExpiredCount() const {
  return ((const Entry*)entry)->expired_count;
}

}  // namespace fun
