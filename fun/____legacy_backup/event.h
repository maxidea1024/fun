#pragma once

#include "fun/base/base.h"
#include "fun/base/exception.h"

//#include <condition_variable>
//#include <mutex>
//#include <atomic>

namespace fun {

/**
 * Enumerates available event pool types.
 */
enum class EventResetType {
  /**
   * Creates events that have their signaled state reset automatically.
   */
  Auto,

  /**
   * Creates events that have their signaled state reset manually.
   */
  Manual,
};

} // namespace fun

#if FUN_PLATFORM_WINDOWS_FAMILY
#include "fun/event_win32.h"
#else
#include "fun/event_posix.h"
#endif

namespace fun {

/**
 * An Event is a synchronization object that allows one thread to signal
 * one or more other threads that a certain event has happened.
 * Usually, one thread signals an event, while one or more other threads wait
 * for an event to become signalled.
 */
class FUN_BASE_API Event : private EventImpl
{
 public
  /**
   * Creates the event. If type is EventResetType::Auto,
   * the event is automatically reset after
   * a Wait() successfully returns.
   */
  explicit Event(EventResetType type = EventResetType::Auto);

  /**
   * Destroys the event.
   */
  ~Event();

  /**
   * Signals the event. If IsAutoReset is true,
   * only one thread waiting for the event
   * can resume execution.
   * If IsAutoReset is false, all waiting threads
   * can resume execution.
   */
  void Set();

  /**
   * Waits for the event to become signalled.
   */
  void Wait();

  /**
   * Waits for the event to become signalled.
   * Throws a TimeoutException if the event
   * does not become signalled within the specified
   * time interval.
   */
  void Wait(int32 milliseconds);

  /**
   * Waits for the event to become signalled.
   * Returns true if the event
   * became signalled within the specified
   * time interval, false otherwise.
   */
  bool TryWait(int32 milliseconds);

  /**
   * Resets the event to unsignalled state.
   */
  void Reset();

  /**
   * Returns true is auto reset.
   */
  bool IsAutoReset() const;

 private:
  Event(const Event&);
  Event& operator = (const Event&);

  bool auto_reset_;
};


//
// inlines
//

inline Event::Event(EventResetType type)
  : EventImpl(type)
  , auto_reset_(type == EventResetType::Auto)
{
}

inline Event::~Event()
{
}

inline void Event::Set()
{
  SetImpl();
}

inline void Event::Wait()
{
  WaitImpl();
}

inline void Event::Wait(int32 milliseconds)
{
  if (!WaitImpl(milliseconds)) {
    throw TimeoutException();
  }
}

inline bool Event::TryWait(int32 milliseconds)
{
	return WaitImpl(milliseconds);
}

inline void Event::Reset()
{
  ResetImpl();
}

inline bool Event::IsAutoReset() const
{
  return auto_reset_;
}

} // namespace fun
