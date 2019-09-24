#pragma once

#include "fun/base/base.h"
#include "fun/base/container/array.h"

namespace fun {

/**
 * Template for fluent array builders.
 *
 * \param ElementType - The type of elements stored in the array.
 * \param Allocator - The allocator to use for elements.
 */
template <typename _ElementType, typename Allocator = DefaultAllocator>
class ArrayBuilder
{
 public:
  using ElementType = _ElementType;
  using ArrayType = Array<ElementType, Allocator>;

  /**
   * Default constructor.
   */
  ArrayBuilder() {}

  /**
   * Creates and initializes an array builder from an array of items.
   *
   * \param array - The array of items to copy.
   */
  template <typename OtherAllocator>
  ArrayBuilder(const Array<ElementType, OtherAllocator>& array)
    : array_(array)
  {}

  /**
   * Adds an item.
   *
   * \param item - The item to add.
   *
   * \return This instance (for method chaining).
   *
   * \see AddUnique
   */
  ArrayBuilder& Add(const ElementType& item)
  {
    array_.Add(item);
    return *this;
  }

  /**
   * Adds an unique item.
   *
   * \param item - The unique item to add.
   *
   * \return This instance (for method chaining).
   *
   * \see Add
   */
  ArrayBuilder& AddUnique(const ElementType& item)
  {
    array_.AddUnique(item);
    return *this;
  }

  /**
   * Appends an array of items.
   *
   * \param other_array - The array to append.
   *
   * \return This instance (for method chaining).
   */
  template <typename OtherAllocator>
  ArrayBuilder& Append(const ArrayType& other_array)
  {
    array_.Append(other_array);
    return *this;
  }

  /**
   * Builds the array as configured.
   *
   * \return A new array.
   */
  ArrayType Build() const
  {
    return array_;
  }

  /**
   * Implicit conversion operator to build the array as configured.
   *
   * \return A new array.
   */
  operator ArrayType() const
  {
    return Build();
  }

 private:
  /** Holds the array. */
  ArrayType array_;
};

} // namespace fun
