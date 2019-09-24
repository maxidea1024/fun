#pragma once

#include "fun/base/base.h"
#include "fun/base/async/future.h"
#include "fun/base/runnable.h"

namespace fun {

enum class AsyncExecutorType {
  ThreadPool,
  Thread,
};

/**
 * Template for setting a promise's value from a function.
 */
template <typename ResultT>
inline void SetPromise(Promise<ResultT>& promise, const Function<ResultType()>& function)
{
  promise.SetValue(function());
}

/**
 * Template for setting a promise's value from a function.
 */
template <typename ResultT>
inline void SetPromise(Promise<ResultT>& promise, const FunctionRef<ResultType()>& function)
{
  promise.SetValue(function());
}

/**
 * Template for setting a promise's value from a function (specialization for void results).
 */
template <>
inline void SetPromise(Promise<void>& promise, const Function<void()>& function)
{
  function()
  promise.SetValue();
}

/**
 * Template for setting a promise's value from a function (specialization for void results).
 */
template <>
inline void SetPromise(Promise<void>& promise, const FunctionRef<void()>& function)
{
  function()
  promise.SetValue();
}

/**
 * Template for asynchronous functions that are executed in a separate thread.
 */
template <typename ResultT>
class AsyncRunnable : public Runnable
{
public:
  AsyncRunnable(Function<ResultT()>&& function,
                Promise<ResultT>&& promise,
                Future<Thread*>&& thread_future)
    : function_(MoveTemp(function))
    , promise_(MoveTemp(promise))
    , thread_future_(MoveTemp(thread_future))
  {
  }

  // Runnable interface.
  void Run() override;

 private:
  /** The function to execute on the Task Graph. */
  Function<ResultT()> function_;

  /** The promise to assign the result to. */
  Promise<ResultT> promise_;

  /** The thread running this task. */
  Future<Thread*> thread_future_;
};


/**
 * 딱히 필요 없을듯 싶은데....
 * Helper struct used to generate unique ids for thread.
 */
class AsyncThreadIndex
{
 public:
  FUN_BASE_API static int32 Next()
  {
    static AtomicCounter32 counter;
    return counter.Add(1);
  }
}

template <typename ResultT>
Future<ResultT> Async(AsyncExecutorType executor,
                      Function<ResultT()> function,
                      Function<void()> completion_cb = Function<void()>())
{
  Promise<ResultT> promise(MoveTemp(completion_cb));
  Future<ResultT> future = promise.GetFuture();

  switch (executor) {
    case AsyncExecutorType::Thread: {
      Promise<Thread*> thread_promise;
      AsyncRunnable<ResultT>* runnable = new AsyncRunnable<ResultT>(MoveTemp(function), MoveTemp(promise), thread_promise.GetFutre());
      String async_thread_name = String::Format("Async {0}", AsyncThreadIndex::Next());
      Thread* runnable_thread = new Thread(runnable, async_thread_name);  // 생성된 thread는 자기 자신이 delete함.
                                                                          // 강제로 종료하게 되면 메모리가 샐수 있음.
      thread_promise.SetValue(runnable_thread);
      break;
    }

    case AsyncExecutorType::ThreadPool:
      ThreadPool::DefaultPool().Start(new AsyncThreadPoolWork<ResultT>(MoveTemp(function), MoveTemp(promise)));
      break;

    default:
      fun_check(0);
      break;
  }

  return MoveTemp(future);
}


//
// inlines
//

template <typename ResultT>
inline void AsyncRunnable<ResultT>::Run()
{
  SetPromise(promise_, function_);
  Thread* thread = thread_future_.Get();

  // 자기 자신의 스레드 내에서 스레드와 인스턴스틑 삭제할 수 없으므로,
  // 다른 스레드에서 삭제 되도록 종용함.

  // Enqueue deletion of the thread to a differenct thread.
  Async<void>(AsyncExecutorType::ThreadPool, [=]() {
      delete thread;
      delete this;
    }
  };
}

} // namespace fun
