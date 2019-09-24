#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * Template for circular buffers.
 *
 * The size of the buffer is rounded up to the next power of two in order speed up indexing
 * operations using a simple bit mask instead of the commonly used modulus operator that may
 * be slow on some platforms.
 */
template <typename ElementType>
class CircularBuffer
{
 public:
  /**
   * Creates and initializes a new instance of the CircularBuffer class.
   *
   * \param capacity - The number of elements that the buffer can store (will be rounded up to the next power of 2).
   */
  CircularBuffer(uint32 capacity)
  {
    fun_check_dbg(capacity > 0);
    fun_check_dbg(capacity <= 0xFFFFFFFFU);

    elements_.AddZeroed(MathBase::RoundUpToPowerOfTwo(capacity));

    index_mask_ = elements_.Count() - 1;
  }

  /**
   * Creates and initializes a new instance of the CircularBuffer class.
   *
   * \param capacity - The number of elements that the buffer can store (will be rounded up to the next power of 2).
   * \param initial_value - The initial value for the buffer's elements.
   */
  CircularBuffer(uint32 capacity, const ElementType& initial_value)
  {
    fun_check_dbg(capacity <= 0xFFFFFFFFU);

    elements_.Init(initial_value, MathBase::RoundUpToPowerOfTwo(capacity));

    index_mask_ = elements_.Count() - 1;
  }

  /**
   * Returns the mutable element at the specified index.
   *
   * \param index - The index of the element to return.
   */
  FUN_ALWAYS_INLINE ElementType& operator[](uint32 index)
  {
    return elements_[index & index_mask_];
  }

  /**
   * Returns the immutable element at the specified index.
   *
   * \param index - The index of the element to return.
   */
  FUN_ALWAYS_INLINE const ElementType& operator[](uint32 index) const
  {
    return elements_[index & index_mask_];
  }

  /**
   * Returns the number of elements that the buffer can hold.
   *
   * \return Buffer capacity.
   */
  FUN_ALWAYS_INLINE uint32 Capacity() const
  {
    return elements_.Count();
  }

  /**
   * Calculates the index that follows the given index.
   *
   * \param current_index - The current index.
   *
   * \return The next index.
   */
  FUN_ALWAYS_INLINE uint32 GetNextIndex(uint32 current_index) const
  {
    return ((current_index + 1) & index_mask_);
  }

  /**
   * Calculates the index previous to the given index.
   *
   * \param current_index - The current index.
   *
   * \return The previous index.
   */
  FUN_ALWAYS_INLINE uint32 GetPreviousIndex(uint32 current_index) const
  {
    return ((current_index - 1) & index_mask_);
  }

 private:
  /** Holds the mask for indexing the buffer's elements. */
  uint32 index_mask_;

  /** Holds the buffer's elements. */
  Array<ElementType> elements_;
};

} // namespace fun
