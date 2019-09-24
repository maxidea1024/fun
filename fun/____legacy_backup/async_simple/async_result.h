#pragma once

#include "fun/base/base.h"
#include "fun/base/shared_ptr.h"

namespace fun {

/**
 * Template for asynchronous return values.
 */
template <typename ResultT>
class AsyncResult
{
 public:
  /**
   * Default constructor.
   */
  AsyncResult()
  {
  }

  /**
   * Creates and initializes a new instance with the given synchronous result value.
   */
  AsyncResult(const ResultT& result)
  {
    Promise<ResultT> promise;
    promise.SetValue(result);

    future_ = promise.GetFuture();
  }

  /**
   * Creates and initializes a new instance
   */
  AsyncResult(Future<ResultT>&& future,
              const SharedPtr<IAsyncProgress>& progress,
              const SharedPtr<IAsyncTask>& task)
    : future_(MoveTemp(future))
    , progress_(MoveTemp(progress))
    , task_(task)
  {
  }

  /**
   * Move constructor.
   */
  AsyncResult(AsyncResult&& rhs)
    : future_(MoveTemp(rhs.future_))
    , progress_(rhs.progress_)
    , task_(rhs.task_)
  {
  }

 public:
  /**
   * Move assignment operator.
   */
  AsyncResult& operator = (AsyncResult&& rhs)
  {
    if (FUN_LIKELY(&rhs != this)) {
      future_ = MoveTemp(rhs.future_);
      progress_ = rhs.progress_;
      task_ = rhs.task_;

      rhs.progress_.Reset();
      rhs.task_.Reset();
    }

    return *this;
  }

 public:
  /**
   * Gets the future that will hold the result.
   */
  const Future<ResultT>& GetFuture() const
  {
    return future_;
  }

  /**
   * Get an object that indicates the progress of the task that is computing the result.
   */
  SharedPtr<IAsyncProgress> GetProgress() const
  {
    return progress_;
  }

  /**
   * Get the asynchronous task that is computing the result.
   */
  SharedPtr<IAsyncTask> GetTask() const
  {
    return task_;
  }

 private:
  AsyncResult(const AsyncResult&);
  AsyncResult& operator = (const AsyncResult&);

 private:
  /**
   * The future.
   */
  Future<ResultT> future_;

  /**
   * The progress object.
   */
  SharedPtr<IAsyncProgress> progress_;

  /**
   * The task object.
   */
  SharedPtr<IAsyncTask> task_;
};

} // namespace fun
