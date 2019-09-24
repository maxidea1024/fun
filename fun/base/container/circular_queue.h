#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * Implements a lock-free first-in first-out queue using a circular array.
 *
 * This class is thread safe only in two-thread scenarios, where the first thread
 * always reads and the second thread always writes. The head and tail indices are
 * stored in volatile memory to prevent the compiler from reordering the instructions
 * that are meant to update the indices AFTER items have been inserted or removed.
 *
 * The number of items that can be enqueued is one less than the queue's capacity,
 * because one item will be used for detecting full and empty states.
 *
 * \param ElementType - The type of elements held in the queue.
 */
template <typename _ElementType>
class CircularQueue {
 public:
  using ElementType = _ElementType;

  /**
   * Default constructor.
   *
   * \param size The number of elements that the queue can hold
   *             (will be rounded up to the next power of 2).
   */
  CircularQueue(uint32 size)
    : buffer_(size), head_(0), tail_(0) {}

  /**
   * Virtual destructor.
   */
  virtual ~CircularQueue() {}

  /**
   * Gets the number of elements in the queue.
   *
   * \return Number of queued elements.
   */
  uint32 Count() const {
    int32 count = tail_ - head_;

    if (count < 0) {
      count += buffer_.Capacity();
    }

    return count;
  }

  /**
   * Removes an item from the front of the queue.
   *
   * \param out_item Will contain the element if the queue is not empty.
   *
   * \return true if an element has been returned, false if the queue was empty.
   */
  bool Dequeue(ElementType& out_item) {
    if (head_ != tail_) {
      out_item = buffer_[head_];
      head_ = buffer_.GetNextIndex(head_);
      return true;
    }

    return false;
  }

  /**
   * Empties the queue.
   *
   * \see IsEmpty
   */
  void Clear() {
    head_ = tail_;
  }

  /**
   * Adds an item to the end of the queue.
   *
   * \param item The element to add.
   *
   * \return true if the item was added, false if the queue was full.
   */
  bool Enqueue(const ElementType& item) {
    uint32 new_tail = buffer_.GetNextIndex(tail_);

    if (new_tail != head_) {
      buffer_[tail_] = item;
      tail_ = new_tail;
      return true;
    }

    return false;
  }

  /**
   * Checks whether the queue is empty.
   *
   * \return true if the queue is empty, false otherwise.
   *
   * \see Clear, IsFull
   */
  FUN_ALWAYS_INLINE bool IsEmpty() const {
    return head_ == tail_;
  }

  /**
   * Checks whether the queue is full.
   *
   * \return true if the queue is full, false otherwise.
   *
   * \see IsEmpty
   */
  bool IsFull() const {
    return buffer_.GetNextIndex(tail_) == head_;
  }

  /**
   * Returns the oldest item in the queue without removing it.
   *
   * \param out_item Will contain the item if the queue is not empty.
   *
   * \return true if an item has been returned, false if the queue was empty.
   */
  bool Peek(ElementType& out_item) {
    if (head_ != tail_) {
      out_item = buffer_[head_];
      return true;
    }

    return false;
  }

 private:
  /** Holds the buffer. */
  CircularBuffer<ElementType> buffer_;
  /** Holds the index to the first item in the buffer. */
  alignas(FUN_PLATFORM_CACHE_LINE_SIZE) volatile uint32 head_;
  /** Holds the index to the last item in the buffer. */
  alignas(FUN_PLATFORM_CACHE_LINE_SIZE) volatile uint32 tail_;
};

} // namespace fun
