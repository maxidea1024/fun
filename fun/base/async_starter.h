#pragma once

#include "fun/base/async_runnable.h"
#include "fun/base/base.h"
#include "fun/base/thread_pool.h"

namespace fun {

/**
 * The default implementation of the StarterType policy for ActiveMethod.
 * It starts the method in its own thread, obtained from the default
 * thread pool.
 */
template <typename OwnerType>
class AsyncStarter {
 public:
  static void Start(OwnerType*, AsyncRunnableBase::Ptr runnable) {
    fun_check_ptr(runnable);

    // Enqueue to default thread pool.
    ThreadPool::DefaultPool().Start(*runnable);

    // The runnable will release it self.
    runnable->AddRef();
  }
};

//각각 다른 starter타입을 정의하고 사용하면 독립스레드를 할당하거나(좀 오래
//걸리는 작업) 그냥 단순하게 특정 스레드 풀에 요청할수도 있을것이다.

}  // namespace fun
