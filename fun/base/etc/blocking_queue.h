#pragma once

#include "fun/base/base.h"
#include "fun/base/list.h"
#include "fun/base/mutex.h"
#include "fun/base/event.h"

namespace fun {

template <typename T>
class BlockingQueue : Noncopyable
{
 public:
  BlockingQueue()
    : mutex_()
    , not_empty_()
    , queue_()
  {
  }

  void Enqueue(const T& item)
  {
    ScopedLock<FastMutex> guard(mutex_);
    queue_.PushBack(item);
    not_empty_.Set();
  }

  void Enqueue(T&& item)
  {
    ScopedLock<FastMutex> guard(mutex_);
    queue_.PushBack(MoveTemp(item));
    not_empty_.Set();
  }

  T Dequeue()
  {
    ScopedLock<FastMutex> guard(mutex_);
    while (queue_.IsEmpty()) {
      not_empty_.Wait();
    }
    fun_check(queue_.IsEmpty() == false);

    T front(MoveTemp(queue_.Front()));
    queue_.PopFront();

    return front;
  }

  //TODO timed-wait version?

  int32 Count() const
  {
    ScopedLock<FastMutex> guard(mutex_);
    return queue_.Count();
  }

 private:
  mutable FastMutex mutex_;
  Event not_empty_;
  List<T> queue_;
};

} // namespace fun
