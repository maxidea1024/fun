#pragma once

#include "fun/base/base.h"

#if FUN_PLATFORM_WINDOWS_FAMILY
#include "fun/base/named_event_win32.h"
#elif FUN_PLATFORM == FUN_PLATFORM_ANDROID
#include "fun/base/named_event_android.h"
#elif FUN_PLATFORM_UNIX_FAMILY
#include "fun/base/named_event_unix.h"
#endif

namespace fun {

/**
 * An NamedEvent is a global synchronization object
 * that allows one process or thread to signal an
 * other process or thread that a certain event
 * has happened.
 * 
 * Unlike an Event, which itself is the unit of synchronization,
 * a NamedEvent refers to a named operating system resource being the
 * unit of synchronization.
 * In other words, there can be multiple instances of NamedEvent referring
 * to the same actual synchronization object.
 * 
 * NamedEvents are always autoresetting.
 * 
 * There should not be more than one instance of NamedEvent for
 * a given name in a process. Otherwise, the instances may
 * interfere with each other.
 */
class FUN_BASE_API NamedEvent : public NamedEventImpl {
 public:
  /**
   * Creates the event.
   */
  NamedEvent(const String& name);

  /**
   * Destroys the event.
   */
  ~NamedEvent();

  /**
   * Signals the event.
   * The one thread or process waiting for the event
   * can resume execution.
   */
  void Set();

  /**
   * Waits for the event to become signalled.
   */
  void Wait();

  NamedEvent() = delete;
  NamedEvent(const NamedEvent&) = delete;
  NamedEvent& operator = (const NamedEvent&) = delete;
};


//
// inlines
//

FUN_ALWAYS_INLINE void NamedEvent::Set() {
  SetImpl();
}

FUN_ALWAYS_INLINE void NamedEvent::Wait() {
  WaitImpl();
}

} // namespace fun
