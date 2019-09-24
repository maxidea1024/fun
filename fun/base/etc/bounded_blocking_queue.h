#pragma once

#include "fun/base/base.h"
#include "fun/base/container/list.h"
//#include "fun/base/container/circular_buffer.h"
#include "fun/base/mutex.h"
#include "fun/base/event.h"

namespace fun {

template <typename T>
class BoundedBlockingQueue : Noncopyable
{
 public:
  BoundedBlockingQueue(int32 max_size)
    : mutex_()
    , not_empty_()
    , not_full_()
    //TODO CircularBuffer<T> 지원!
    //, queue_(max_size)
    , queue_()
  {
  }

  void Enqueue(const T& item)
  {
    ScopedLock<FastMutex> guard(mutex_);

    while (queue_.IsFull()) {
      not_full_.Wait();
    }
    fun_check(not queue_.IsFull());

    queue_.PushBack(item);
    not_empty_.Set();
  }

  void Enqueue(T&& item)
  {
    ScopedLock<FastMutex> guard(mutex_);

    while (queue_.IsFull()) {
      not_full_.Wait();
    }
    fun_check(not queue_.IsFull());

    queue_.PushBack(MoveTemp(item));
    not_empty_.Set();
  }

  T Dequeue()
  {
    ScopedLock<FastMutex> guard(mutex_);
    while (queue_.IsEmpty()) {
      not_empty_.Wait();
    }
    fun_check(not queue_.IsEmpty());

    T front(MoveTemp(queue_.Front()));
    queue_.PopFront();

    not_full_.Set();

    return front;
  }

  //TODO timed-wait version?

  bool IsEmpty() const
  {
    ScopedLock<FastMutex> guard(mutex_);
    return queue_.IsEmpty();
  }

  int32 Count() const
  {
    ScopedLock<FastMutex> guard(mutex_);
    return queue_.Count();
  }

  int32 Capacity() const
  {
    ScopedLock<FastMutex> guard(mutex_);
    return queue_.Capacity();
  }

 private:
  mutable FastMutex mutex_;
  Event not_empty_;
  Event not_full_;
  //TODO CircularBuffer<T>로 대체해야함?
  //CircularBuffer<T> queue_;
  List<T> queue_;
};

} // namespace fun
