#pragma once

#include "fun/base/base.h"
#include "fun/base/runnable.h"

namespace fun {

/**
 * This adapter simplifies using ordinary methods as
 * targets for threads.
 * Usage:
 *    RunnableAdapter<MyClass> ra(myObject, &MyObject::DoSomething));
 *    Thread thr;
 *    thr.Start(ra);
 *
 * For using a freestanding or static member function as a thread
 * target, please see the ThreadTarget class.
 */
template <typename C>
class RunnableAdapter : public Runnable {
 public:
  typedef void (C::*Callback)();

  RunnableAdapter(C& object, Callback method)
      : object_(&object), method_(method) {}

  RunnableAdapter(const RunnableAdapter& rhs)
      : object_(rhs.object_), method_(rhs.method_) {}

  ~RunnableAdapter() {}

  RunnableAdapter& operator=(const RunnableAdapter& rhs) {
    if (FUN_LIKELY(&rhs != this)) {
      object_ = rhs.object_;
      method_ = rhs.method_;
    }
    return *this;
  }

  void Run() override { (object_->*method_)(); }

 private:
  RunnableAdapter();

  C* object_;
  Callback method_;
};

}  // namespace fun
