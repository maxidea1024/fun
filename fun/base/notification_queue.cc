#include "fun/base/notification_queue.h"
#include "fun/base/notification.h"
#include "fun/base/notification_center.h"
#include "fun/base/singleton.h"

namespace fun {

NotificationQueue::NotificationQueue() {}

NotificationQueue::~NotificationQueue() {
  try {
    Clear();
  } catch (...) {
    fun_unexpected();
  }
}

void NotificationQueue::Enqueue(Notification::Ptr noti) {
  fun_check_ptr(noti);

  ScopedLock<FastMutex> guard(mutex_);
  if (wait_queue_.IsEmpty()) {
    queue_.PushBack(noti);
  } else {
    WaitInfo* wait = wait_queue_.Dequeue();
    wait->noti = noti;
    wait->available.Set();
  }
}

void NotificationQueue::EnqueueUrgent(Notification::Ptr noti) {
  fun_check_ptr(noti);

  ScopedLock<FastMutex> guard(mutex_);
  if (wait_queue_.IsEmpty()) {
    queue_.PushFront(noti);
  } else {
    WaitInfo* wait = wait_queue_.Dequeue();
    wait->noti = noti;
    wait->available.Set();
  }
}

Notification::Ptr NotificationQueue::Dequeue() {
  ScopedLock<FastMutex> guard(mutex_);
  return DequeueOne();
}

Notification::Ptr NotificationQueue::WaitDequeue() {
  Notification::Ptr noti;
  WaitInfo* wait = nullptr;
  {
    ScopedLock<FastMutex> guard(mutex_);
    noti = DequeueOne();
    if (noti) {
      return noti;
    }
    wait = new WaitInfo();
    wait_queue_.Enqueue(wait);
  }
  wait->available.Wait();
  noti = wait->noti;
  delete wait;
  return noti;
}

Notification::Ptr NotificationQueue::WaitDequeue(int32 milliseconds) {
  Notification::Ptr noti;
  WaitInfo* wait = nullptr;
  {
    ScopedLock<FastMutex> guard(mutex_);
    noti = DequeueOne();
    if (noti) {
      return noti;
    }
    wait = new WaitInfo();
    wait_queue_.Enqueue(wait);
  }

  if (wait->available.TryWait(milliseconds)) {
    noti = wait->noti;
  } else {
    // Timedout, cancel.
    ScopedLock<FastMutex> guard(mutex_);
    noti = wait->noti;
    wait_queue_.Remove(wait);
  }
  delete wait;
  return noti;
}

Notification::Ptr NotificationQueue::DequeueOne() {
  Notification::Ptr noti;
  if (!queue_.IsEmpty()) {
    noti = queue_.Dequeue();
  }
  return noti;
}

void NotificationQueue::Dispatch(NotificationCenter& notification_center) {
  ScopedLock<FastMutex> guard(mutex_);
  Notification::Ptr noti = DequeueOne();
  while (noti) {
    notification_center.Post(noti);
    noti = DequeueOne();
  }
}

void NotificationQueue::WakeUpAll() {
  ScopedLock<FastMutex> guard(mutex_);
  for (auto& waiter : wait_queue_) {
    waiter->available.Set();
  }
  wait_queue_.Clear();
}

bool NotificationQueue::IsEmpty() const {
  ScopedLock<FastMutex> guard(mutex_);
  return queue_.IsEmpty();
}

int32 NotificationQueue::Count() const {
  ScopedLock<FastMutex> guard(mutex_);
  return queue_.Count();
}

void NotificationQueue::Clear() {
  ScopedLock<FastMutex> guard(mutex_);
  queue_.Clear();
}

bool NotificationQueue::Remove(Notification::Ptr noti) {
  ScopedLock<FastMutex> guard(mutex_);
  return queue_.Remove(noti.Get()) > 0;
}

bool NotificationQueue::HasIdleThread() const {
  ScopedLock<FastMutex> guard(mutex_);
  return !wait_queue_.IsEmpty();
}

NotificationQueue& NotificationQueue::DefaultQueue() {
  static Singleton<NotificationQueue>::Holder h;
  return *h.Get();
}

}  // namespace fun
