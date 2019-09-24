#include "fun/base/timed_notification_queue.h"
#include <limits>
#include "fun/base/notification.h"

namespace fun {

TimedNotificationQueue::TimedNotificationQueue() {
  // NOOP
}

TimedNotificationQueue::~TimedNotificationQueue() {
  try {
    Clear();
  } catch (...) {
    fun_unexpected();
  }
}

void TimedNotificationQueue::Enqueue(Notification::Ptr noti,
                                     const Timestamp& timestamp) {
  fun_check_ptr(noti);

  Timestamp now(Timestamp::Now());
  Clock clock;
  Timestamp::TimeDiff diff = timestamp - now;
  clock += diff;

  FastMutex::ScopedLock guard(mutex_);
  noti_queue_.insert(NfQueue::value_type(clock, noti));
  noti_available_.Set();
}

void TimedNotificationQueue::Enqueue(Notification::Ptr noti,
                                     const Clock& clock) {
  fun_check_ptr(noti);

  FastMutex::ScopedLock guard(mutex_);
  noti_queue_.insert(NfQueue::value_type(clock, noti));
  noti_available_.Set();
}

Notification* TimedNotificationQueue::Dequeue() {
  FastMutex::ScopedLock guard(mutex_);

  NfQueue::iterator it = noti_queue_.begin();
  if (it != noti_queue_.end()) {
    Clock::DiffType sleep = -it->first.Elapsed();
    if (sleep <= 0) {
      Notification::Ptr noti = it->second;
      noti_queue_.erase(it);
      return noti.AddRef();
    }
  }
  return nullptr;
}

Notification* TimedNotificationQueue::WaitDequeue() {
  for (;;) {
    mutex_.Lock();
    NfQueue::iterator it = noti_queue_.begin();
    if (it != noti_queue_.end()) {
      mutex_.Unlock();
      Clock::DiffType sleep = -it->first.Elapsed();
      if (sleep <= 0) {
        return DequeueOne(it).AddRef();
      } else if (!Wait(sleep)) {
        return DequeueOne(it).AddRef();
      } else {
        continue;
      }
    } else {
      mutex_.Unlock();
    }
    noti_available_.Wait();
  }
}

Notification* TimedNotificationQueue::WaitDequeue(int32 milliseconds) {
  while (milliseconds >= 0) {
    mutex_.Lock();
    NfQueue::iterator it = noti_queue_.begin();
    if (it != noti_queue_.end()) {
      mutex_.Unlock();
      Clock now(Clock::Now());
      Clock::DiffType sleep = it->first - now;
      if (sleep <= 0) {
        return DequeueOne(it).AddRef();
      } else if (sleep <= 1000 * Clock::DiffType(milliseconds)) {
        if (!Wait(sleep)) {
          return DequeueOne(it).AddRef();
        } else {
          milliseconds -= static_cast<int32>((now.Elapsed() + 999) / 1000);
          continue;
        }
      }
    } else {
      mutex_.Unlock();
    }

    if (milliseconds > 0) {
      Clock now(Clock::Now());
      noti_available_.TryWait(milliseconds);
      milliseconds -= static_cast<int32>((now.Elapsed() + 999) / 1000);
    } else {
      return 0;
    }
  }

  return 0;
}

bool TimedNotificationQueue::Wait(Clock::DiffType interval) {
  const Clock::DiffType MAX_SLEEP =
      8 * 60 * 60 *
      Clock::DiffType(1000000);  // sleep at most 8 hours at a time
  while (interval > 0) {
    Clock now(Clock::Now());
    Clock::DiffType sleep = interval <= MAX_SLEEP ? interval : MAX_SLEEP;
    if (noti_available_.TryWait(static_cast<int32>((sleep + 999) / 1000))) {
      return true;
    }
    interval -= now.Elapsed();
  }
  return false;
}

bool TimedNotificationQueue::IsEmpty() const {
  FastMutex::ScopedLock guard(mutex_);
  return noti_queue_.empty();
}

int TimedNotificationQueue::Count() const {
  FastMutex::ScopedLock guard(mutex_);
  return static_cast<int>(noti_queue_.size());
}

void TimedNotificationQueue::Clear() {
  FastMutex::ScopedLock guard(mutex_);
  noti_queue_.clear();
}

Notification::Ptr TimedNotificationQueue::DequeueOne(NfQueue::iterator& it) {
  FastMutex::ScopedLock guard(mutex_);
  Notification::Ptr noti = it->second;
  noti_queue_.erase(it);
  return noti;
}

}  // namespace fun
