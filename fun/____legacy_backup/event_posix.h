#pragma once

#include "fun/base/base.h"
#include "fun/base/exception.h"

#include <pthread.h>
#include <errno.h>

namespace fun {

class FUN_BASE_API EventImpl
{
 public
  explicit EventImpl(EventResetType type = EventResetType::Auto);
  ~EventImpl();

  void SetImpl();
  void WaitImpl();
  bool WaitImpl(int32 milliseconds);
  void ResetImpl();

 private:
  bool auto_reset_;
  volatile bool state_;
  pthread_mutex_t mutex_;
  pthread_cond_t cond_;
};


//
// inlines
//

inline void EventImpl::SetImpl()
{
  if (pthread_mutex_lock(&mutex_)) {
    throw SystemException("cannot signal event (lock)");
  }

  state_ = true;

  if (pthread_cond_broadcast(&cond_)) {
    pthread_mutex_unlock(&mutex_);
    throw SystemException("cannot signal event");
  }

  pthread_mutex_unlock(&mutex_);
}

inline void EventImpl::ResetImpl()
{
  if (pthread_mutex_lock(&mutex_)) {
    throw SystemException("cannot reset event");
  }

  state_ = false;
  pthread_mutex_unlock(&mutex_);
}

} // namespace fun
