#include "fun/base/priority_notification_queue.h"
#include "fun/base/notification_center.h"
#include "fun/base/notification.h"
#include "fun/base/singleton.h"

namespace fun {

PriorityNotificationQueue::PriorityNotificationQueue() {
}

PriorityNotificationQueue::~PriorityNotificationQueue() {
  try {
    Clear();
  }
  catch (...) {
    fun_unexpected();
  }
}

void PriorityNotificationQueue::Enqueue(Notification::Ptr noti, int32 priority) {
  fun_check_ptr(noti);
  FastMutex::ScopedLock guard(mutex_);
  if (wait_queue_.empty()) {
    noti_queue_.insert(NfQueue::value_type(priority, noti));
  }
  else {
    fun_check_dbg(noti_queue_.empty());
    WaitInfo* wait_info = wait_queue_.front();
    wait_queue_.pop_front();
    wait_info->noti = noti;
    wait_info->available.Set();
  }
}

//TODO 이하 반환된 포인터의 카운팅에 문제가 있을 수 있음.
//RAW로 접근해서 카운터를 하나 올려주는 형태로 해야하나?
//받는쪽에서 RefCountedPtr로 받으면, 카운터가 하나 올라갈텐데..

Notification* PriorityNotificationQueue::Dequeue() {
  FastMutex::ScopedLock guard(mutex_);
  return DequeueOne().AddRef();
}

Notification* PriorityNotificationQueue::WaitDequeue() {
  Notification::Ptr noti;
  WaitInfo* wait_info = nullptr; {
    FastMutex::ScopedLock guard(mutex_);
    noti = DequeueOne();
    if (noti) {
      return noti.AddRef();
    }
    wait_info = new WaitInfo;
    wait_queue_.push_back(wait_info);
  }
  wait_info->available.Wait();
  noti = wait_info->noti;
  delete wait_info;
  return noti.AddRef();
}

Notification* PriorityNotificationQueue::WaitDequeue(int32 milliseconds) {
  Notification::Ptr noti;
  WaitInfo* wait_info = nullptr; {
    FastMutex::ScopedLock guard(mutex_);
    noti = DequeueOne();
    if (noti) {
      return noti.AddRef();
    }
    wait_info = new WaitInfo;
    wait_queue_.push_back(wait_info);
  }

  if (wait_info->available.TryWait(milliseconds)) {
    noti = wait_info->noti;
  } else {
    FastMutex::ScopedLock guard(mutex_);
    noti = wait_info->noti;
    for (WaitQueue::iterator it = wait_queue_.begin(); it != wait_queue_.end(); ++it) {
      if (*it == wait_info) {
        wait_queue_.erase(it);
        break;
      }
    }
  }
  delete wait_info;
  return noti.AddRef();
}

void PriorityNotificationQueue::Dispatch(NotificationCenter& notification_center) {
  FastMutex::ScopedLock guard(mutex_);
  Notification::Ptr noti = DequeueOne();
  while (noti) {
    notification_center.Post(noti);
    noti = DequeueOne();
  }
}

void PriorityNotificationQueue::WakeUpAll() {
  FastMutex::ScopedLock guard(mutex_);
  for (WaitQueue::iterator it = wait_queue_.begin(); it != wait_queue_.end(); ++it) {
    (*it)->available.Set();
  }
  wait_queue_.clear();
}

bool PriorityNotificationQueue::IsEmpty() const {
  FastMutex::ScopedLock guard(mutex_);
  return noti_queue_.empty();
}

int PriorityNotificationQueue::Count() const {
  FastMutex::ScopedLock guard(mutex_);
  return static_cast<int>(noti_queue_.size());
}

void PriorityNotificationQueue::Clear() {
  FastMutex::ScopedLock guard(mutex_);
  noti_queue_.clear();
}

bool PriorityNotificationQueue::HasIdleThreads() const {
  FastMutex::ScopedLock guard(mutex_);
  return !wait_queue_.empty();
}

Notification::Ptr PriorityNotificationQueue::DequeueOne() {
  Notification::Ptr noti;
  NfQueue::iterator it = noti_queue_.begin();
  if (it != noti_queue_.end()) {
    noti = it->second;
    noti_queue_.erase(it);
  }
  return noti;
}

PriorityNotificationQueue& PriorityNotificationQueue::DefaultQueue() {
  static Singleton<PriorityNotificationQueue>::Holder sh;
  return *sh.Get();
}

} // namespace fun
