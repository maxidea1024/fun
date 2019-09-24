#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/function.h"
#include "fun/base/string/string.h"
#include "fun/base/timestamp.h"

namespace fun {

/** Timer ID type */
typedef uint32 TimerTaskId;

/** Timer task context. */
struct TimerTaskContext {
  class Timer* timer;
  TimerTaskId id;
  Timestamp fired_time;
  int32 fired_count;
  String tag;
};

/** Timer function type */
typedef TFunction<void(const TimerTaskContext& /*context*/)> TimerFunction;

class FUN_BASE_API Timer {
 public:
  /** Default constructor. */
  Timer();
  /** Construct with name. */
  Timer(const String& name);
  /** Destructor. */
  virtual ~Timer();

  /** Start timer. */
  void Start();
  /** Stop timer. */
  void Stop(bool drain = true);

  /** Perform the specified task at the specified time. */
  TimerTaskId Schedule(const Timestamp& time, const TimerFunction& func,
                       const String& tag = String());
  /** Perform the specified task after a certain delay. */
  TimerTaskId Schedule(const Timespan& delay, const TimerFunction& func,
                       const String& tag = String());
  /** Perform a task specified by a period from the specified time . */
  TimerTaskId Schedule(const Timestamp& first_time, const Timespan& period,
                       const TimerFunction& func, const String& tag = String());
  /** Perform a specified task at a fixed interval after a certain delay. */
  TimerTaskId Schedule(const Timespan& delay, const Timespan& period,
                       const TimerFunction& func, const String& tag = String());

  /** Perform a task specified by a period from the specified time. */
  TimerTaskId ScheduleAtFixedRate(const Timestamp& first_time,
                                  const Timespan& period,
                                  const TimerFunction& func,
                                  const String& tag = String());
  /** Perform a specified task at a fixed interval after a certain delay. */
  TimerTaskId ScheduleAtFixedRate(const Timespan& delay, const Timespan& period,
                                  const TimerFunction& func,
                                  const String& tag = String());

  /** Check if the specified timer task is valid. */
  bool IsValid(TimerTaskId timer_task_id);

  /** Cancel the specified task. */
  bool Cancel(TimerTaskId timer_task_id);
  /** Cancel all tasks. */
  void Cancel();

  /** tag the timer task. (Used for debugging and task classification) */
  bool SetTag(TimerTaskId timer_task_id, const String& tag);

 private:
  class Impl;
  Impl* impl_;
};

/**
 * Timer API functions
 */
namespace TimerService {

/** Perform the specified task at the specified time. */
FUN_BASE_API TimerTaskId Schedule(const Timestamp& time,
                                  const TimerFunction& func,
                                  const String& tag = String());
/** Perform the specified task after a certain delay. */
FUN_BASE_API TimerTaskId Schedule(const Timespan& delay,
                                  const TimerFunction& func,
                                  const String& tag = String());
/** Perform a task specified by a period from the specified time . */
FUN_BASE_API TimerTaskId Schedule(const Timestamp& first_time,
                                  const Timespan& period,
                                  const TimerFunction& func,
                                  const String& tag = String());
/** Perform a specified task at a fixed interval after a certain delay. */
FUN_BASE_API TimerTaskId Schedule(const Timespan& delay, const Timespan& period,
                                  const TimerFunction& func,
                                  const String& tag = String());

/** Perform a task specified by a period from the specified time. */
FUN_BASE_API TimerTaskId ScheduleAtFixedRate(const Timestamp& first_time,
                                             const Timespan& period,
                                             const TimerFunction& func,
                                             const String& tag = String());
/** Perform a specified task at a fixed interval after a certain delay. */
FUN_BASE_API TimerTaskId ScheduleAtFixedRate(const Timespan& delay,
                                             const Timespan& period,
                                             const TimerFunction& func,
                                             const String& tag = String());

/** Check if the specified timer task is valid. */
FUN_BASE_API bool IsValid(TimerTaskId timer_task_id);

/** Cancel the specified task. */
FUN_BASE_API bool Cancel(TimerTaskId timer_task_id);
/** Cancel all tasks. */
FUN_BASE_API void Cancel();

/** tag the timer task. (Used for debugging and task classification) */
FUN_BASE_API bool SetTag(TimerTaskId timer_task_id, const String& tag);

}  // end of namespace TimerService

}  // namespace fun
