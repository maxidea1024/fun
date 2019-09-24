#pragma once

#include "fun/base/base.h"
#include "fun/base/exception.h"

#include <atomic>
#include <condition_variable>
#include <mutex>

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

/**
 * An Event is a synchronization object that allows one thread to signal
 * one or more other threads that a certain event has happened.
 * Usually, one thread signals an event, while one or more other threads wait
 * for an event to become signalled.
 */
class FUN_BASE_API Event {
 public:
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
   * Signals the event. If autoReset is true, only one thread waiting for
   * the event can resume execution.
   *
   * If autoReset is false, all waiting threads can resume execution.
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
   * @doc
   */
  bool IsAutoReset() const;

  // Disable copy and assignment.
  Event(const Event&) = delete;
  Event& operator=(const Event&) = delete;

 private:
  bool WaitImpl(int32 milliseconds);

  std::atomic<bool> state_;
  bool auto_reset_;
  mutable std::mutex mutex_;
  std::condition_variable cond_;
};

//
// inlines
//

inline void Event::Wait(int32 milliseconds) {
  if (!WaitImpl(milliseconds)) {
    throw TimeoutException();
  }
}

inline bool Event::TryWait(int32 milliseconds) {
  return WaitImpl(milliseconds);
}

inline bool Event::IsAutoReset() const { return auto_reset_; }

}  // namespace fun
