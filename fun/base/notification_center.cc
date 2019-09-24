#include "fun/base/notification_center.h"
#include "fun/base/notification.h"
#include "fun/base/observer.h"
#include "fun/base/singleton.h"

namespace fun {

NotificationCenter::NotificationCenter() {}

NotificationCenter::~NotificationCenter() {}

void NotificationCenter::AddObserver(const ObserverBase& observer) {
  ScopedLock<Mutex> guard(mutex_);
  observers_.Add(SharedPtr<ObserverBase>(observer.Clone()));
}

void NotificationCenter::RemoveObserver(const ObserverBase& observer) {
  ScopedLock<Mutex> guard(mutex_);
  for (int32 i = 0; i < observers_.Count(); ++i) {
    if (observers_[i]->Equals(observer)) {
      observers_[i]->Disable();
      observers_.RemoveAt(i);
      return;
    }
  }
}

bool NotificationCenter::HasObserver(const ObserverBase& observer) const {
  ScopedLock<Mutex> guard(mutex_);
  for (int32 i = 0; i < observers_.Count(); ++i) {
    if (observers_[i]->Equals(observer)) {
      return true;
    }
  }

  return false;
}

void NotificationCenter::Post(Notification::Ptr noti) {
  ScopedLock<Mutex> guard(mutex_);
  for (int32 i = 0; i < observers_.Count(); ++i) {
    observers_[i]->Notify(noti);
  }
}

bool NotificationCenter::HasOservers() const {
  ScopedLock<Mutex> guard(mutex_);
  return !observers_.IsEmpty();
}

int32 NotificationCenter::GetObserverCount() const {
  ScopedLock<Mutex> guard(mutex_);
  return observers_.Count();
}

NotificationCenter& NotificationCenter::DefaultCenter() {
  Singleton<NotificationCenter>::Holder sh;
  return *sh.Get();
}

} // namespace fun
