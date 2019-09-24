#pragma once

#include "fun/base/base.h"
#include "fun/base/runnable.h"

namespace fun {

/**
 * This adapter simplifies using static member functions as well as
 * standalone functions as targets for threads.
 *
 * Note that it is possible to pass those entities directly to Thread::Start().
 * This adapter is provided as a convenience for higher abstraction level
 * scenarios where Runnable abstract class is used.
 *
 * For using a non-static member function as a thread target, please
 * see the RunnableAdapter class.
 *
 * Usage:
 *   class MyObject {
 *     static void DoSomething() {}
 *   };
 *
 *   ThreadTarget ra(&MyObject::DoSomething);
 *   Thread thread;
 *   thread.Start(ra);
 *
 * or:
 *
 *   void DoSomething() {}
 *
 *   ThreadTarget ra(DoSomething);
 *   Thread thread;
 *   thread.Start(ra);
 */
class FUN_BASE_API ThreadTarget : public Runnable {
 public:
  typedef void (*Callback)();

  ThreadTarget(Callback method);
  ~ThreadTarget();

  ThreadTarget(const ThreadTarget& rhs);
  ThreadTarget& operator=(const ThreadTarget& rhs);

  void Run() override;

 private:
  ThreadTarget();

  Callback method_;
};

//
// inlines
//

FUN_ALWAYS_INLINE void ThreadTarget::Run() { method_(); }

}  // namespace fun
