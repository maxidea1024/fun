#pragma once

#include "fun/base/base.h"
#include "fun/base/event.h"
#include "fun/base/mutex.h"

namespace fun {

template <EventResetType reset_type>
class EventPool {
 public:
  FUN_NO_INLINE static EventPool& DefaultPool() {
    //TODO Singleton<T>로 해야할까??
    static EventPool default_instance;
    return default_instance;
  }

  FUN_NO_INLINE ~EventPool() {
    ScopedLock<FastMutex> guard(mutex_);

    try {
      while (!pool_.IsEmpty()) {
        Event* event = pool_.PopBackAndCopyValue();
        delete event;
      }
      fun_check(pool_.IsEmpty());
    } catch (...) {
      fun_unexpected();
    }
  }

  /**
   * Gets a single Event object from the event pool.
   *
   * If there is no object in the event pool, it is newly allocated
   * and returned.
   */
  Event* GetEventFromPool() {
    ScopedLock<FastMutex> guard(mutex_);

    if (pool_.IsEmpty()) {
      return new Event(reset_type);
    }

    return pool_.PopBackAndCopyValue();
  }

  /**
   * Returns an Event object in the event pool.
   */
  void ReturnEventToPool(Event* event) {
    //TODO tag를 하나 지정해서 이 풀에서 생성된 것인지 체크하는게 좋을듯도?
    fun_check_ptr(event);
    fun_check(event->IsAutoReset() == (reset_type == EventResetType::Auto));

    if (reset_type == EventResetType::Manual) {
      event->Reset();
    }

    ScopedLock<FastMutex> guard(mutex_);
    pool_.PushBack(event);
  }

 private:
  FastMutex mutex_;

  //TODO Lockfree list로 대체해야함!!
  Array<Event*> pool_;
};


class ScopedEvent {
 public:
  FUN_ALWAYS_INLINE ScopedEvent() {
    event_ = EventPool<EventResetType::Auto>::DefaultPool().GetEventFromPool();
  }

  FUN_ALWAYS_INLINE ~ScopedEvent() {
    event_->Wait();
    EventPool<EventResetType::Auto>::DefaultPool().ReturnEventToPool(event_);
    event_ = nullptr;
  }

  FUN_ALWAYS_INLINE void Set() {
    event_->Set();
  }

  FUN_ALWAYS_INLINE Event* Get() const {
    return event_;
  }

 private:
  Event* event_;
};

} // namespace fun
