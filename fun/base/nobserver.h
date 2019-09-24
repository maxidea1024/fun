#pragma once

#include "fun/base/base.h"
#include "fun/base/observer_base.h"
#include "fun/base/mutex.h"

namespace fun {

//TODO dynamic_cast를 써야만할까??

/**
 * This template class implements an adapter that sits between
 * a NotificationCenter and an object receiving notifications
 * from it. It is quite similar in concept to the
 * RunnableAdapter, but provides some NotificationCenter
 * specific additional methods.
 * See the NotificationCenter class for information on how
 * to use this template class.
 * 
 * This class template is quite similar to the Observer class
 * template. The only difference is that the NObserver
 * expects the callback function to accept a const Ptr&
 * instead of a plain pointer as argument, thus simplifying memory
 * management.
 */
template <typename C, typename N>
class NObserver : public ObserverBase {
 public:
  NObserver(C& object, Callback method)
    : object_(object), method_(method) {}

  NObserver(const NObserver& rhs)
    : ObserverBase(rhs), object_(rhs.object_), method_(rhs.method_) {}

  NObserver& operator = (const NObserver& rhs) {
    if (FUN_LIKELY(&rhs != this)) {
      object_ = rhs.object_;
      method_ = rhs.method_;
    }
    return *this;
  }

  void Notify(Notification* noti) const {
    ScopedLock guard(mutex_);

    if (object_) {
      N* casted_noti = dynamic_cast<N*>(noti);
      if (cated_noti) {
        Notification::Ptr n(casted_noti, true);
        (object_->*method_)(n);
      }
    }
  }

  bool Equals(const ObserverBase& other) const {
    const NObserver* casted_other = dynamic_cast<const NObserver*>(&other);

    return  casted_other &&
            casted_other->object_ == object_ &&
            casted_other->method_ == method_;
  }

  bool Accepts(Notification* noti) const {
    return dynamic_cast<N*>(noti) != nullptr;
  }

  SharedPtr<ObserverBase> Clone() const {
    return new NObserver(*this);
  }

  void Disable() {
    ScopedLock guard(mutex_);
    object_ = nullptr;
  }

 private:
  NObserver();

  C* object_;
  Callback method_;
  Mutex mutex_;
};

} // namespace fun
