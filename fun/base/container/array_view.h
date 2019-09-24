#pragma once

#include <initializer_list>
#include "fun/base/base.h"

// TODO Invoke로 처리해야할까??

namespace fun {

namespace ArrayView_internal {

/**
 * Trait testing whether a type is compatible with the view type
 */
template <typename T, typename ElementType>
struct IsCompatibleElementType {
  // NOTE:
  // The stars in the PointerIsConvertibleFromTo test are *IMPORTANT*
  // They prevent ArrayView<base>(Array<Derived>&) from compiling!
  enum { Value = PointerIsConvertibleFromTo<T*, ElementType* const>::Value };
};

}  // namespace ArrayView_internal

/**
 * Templated fixed-size view of another array
 *
 * A statically sized view of an array of typed elements.  Designed to allow
 * functions to take either a fixed C array or a Array with an arbitrary
 * allocator as an argument when the function neither adds nor removes elements
 *
 * e.g.:
 * int32 SumAll(ArrayView<const int32> array) {
 *   return algo::Accumulate(array);
 * }
 *
 * could be called as:
 *   SumAll(MyTArray);
 *   SumAll(MyCArray);
 *   SumAll({1, 2, 3});
 *   SumAll(MakeArrayView(ptr, count));
 *
 * Note:
 *   View classes are not const-propagating! If you want a view where the
 * elements are const, you need "ArrayView<const T>" not "const ArrayView<T>"!
 *
 * Caution:
 *   Treat a view like a *reference* to the elements in the array. DO NOT free
 * or reallocate the array while the view exists!
 */
template <typename _ElementType>
class ArrayView {
 public:
  using ElementType = _ElementType;

  /**
   * Default constructor.
   */
  ArrayView() : data_ptr_(nullptr), array_count_(0) {}

 private:
  template <typename T>
  using IsCompatibleElementType =
      ArrayView_internal::IsCompatibleElementType<T, ElementType>;

 public:
  /**
   * Copy constructor from another view
   *
   * \param other - The source array view to copy
   */
  template <typename OtherElementType,
            typename = typename EnableIf<
                IsCompatibleElementType<OtherElementType>::Value>::Type>
  FUN_ALWAYS_INLINE ArrayView(const ArrayView<OtherElementType>& other)
      : data_ptr_(other.MutableData()), array_count_(other.Count()) {}

  /**
   * Construct a view of a C array with a compatible element type
   *
   * \param other - The source array to view.
   */
  template <typename OtherElementType, size_t Size,
            typename = typename EnableIf<
                IsCompatibleElementType<OtherElementType>::Value>::Type>
  FUN_ALWAYS_INLINE ArrayView(OtherElementType (&other)[Size])
      : data_ptr_(other), array_count_((int32)Size) {
    static_assert(Size <= int32_MAX,
                  "array size too large! size is only int32 for compatibility "
                  "with Array!");
  }

  /**
   * Construct a view of a C array with a compatible element type
   *
   * \param other - The source array to view.
   */
  template <typename OtherElementType, size_t Size,
            typename = typename EnableIf<
                IsCompatibleElementType<const OtherElementType>::Value>::Type>
  FUN_ALWAYS_INLINE ArrayView(const OtherElementType (&other)[Size])
      : data_ptr_(other), array_count_((int32)Size) {
    static_assert(Size <= int32_MAX,
                  "array size too large! size is only int32 for compatibility "
                  "with Array!");
  }

  /**
   * Construct a view of a Array with a compatible element type
   *
   * \param other - The source array to view.
   */
  template <typename OtherElementType, typename OtherAllocator,
            typename = typename EnableIf<
                IsCompatibleElementType<OtherElementType>::Value>::Type>
  FUN_ALWAYS_INLINE ArrayView(Array<OtherElementType, OtherAllocator>& other)
      : data_ptr_(other.MutableData()), array_count_(other.Count()) {}

  template <typename OtherElementType, typename OtherAllocator,
            typename = typename EnableIf<
                IsCompatibleElementType<const OtherElementType>::Value>::Type>
  FUN_ALWAYS_INLINE ArrayView(
      const Array<OtherElementType, OtherAllocator>& other)
      : data_ptr_(other.MutableData()), array_count_(other.Count()) {}

  /**
   * Construct a view from an initializer list with a compatible element type
   *
   * \param list - The initializer list to view.
   */
  template <typename OtherElementType,
            typename = typename EnableIf<
                IsCompatibleElementType<OtherElementType>::Value>::Type>
  FUN_ALWAYS_INLINE ArrayView(std::initializer_list<OtherElementType> list)
      : data_ptr_(&*list.begin()), array_count_(list.size()) {}

  /**
   * Construct a view of an arbitrary pointer
   *
   * \param data - The data to view
   * \param count - The number of elements
   */
  template <typename OtherElementType,
            typename = typename EnableIf<
                IsCompatibleElementType<OtherElementType>::Value>::Type>
  FUN_ALWAYS_INLINE ArrayView(OtherElementType* data, int32 count)
      : data_ptr_(data), array_count_(count) {
    fun_check(array_count_ >= 0);
  }

  /**
   * Assignment operator
   *
   * \param other - The source array view to assign from
   */
  template <typename OtherElementType,
            typename = typename EnableIf<
                IsCompatibleElementType<OtherElementType>::Value>::Type>
  FUN_ALWAYS_INLINE ArrayView& operator=(
      const ArrayView<OtherElementType>& other) {
    if (FUN_LIKELY(&other != this)) {
      data_ptr_ = other.data_ptr_;
      array_count_ = other.array_count_;
    }

    return *this;
  }

  FUN_ALWAYS_INLINE const ElementType* ConstData() const { return data_ptr_; }

  FUN_ALWAYS_INLINE ElementType* MutableData() const { return data_ptr_; }

  /**
   * Helper function returning the size of the inner type.
   *
   * \returns Size in bytes of array type.
   */
  FUN_ALWAYS_INLINE int32 GetTypeSize() const {
    return (int32)sizeof(ElementType);
  }

  /**
   * Checks array invariants: if array size is greater than zero and less
   * than maximum.
   */
  FUN_ALWAYS_INLINE void CheckInvariants() const {
    fun_check_dbg(array_count_ >= 0);
  }

  /**
   * Checks if index is in array range.
   *
   * \param index index to check.
   */
  FUN_ALWAYS_INLINE void RangeCheck(int32 index) const {
    CheckInvariants();
    fun_check_msg(index >= 0 & index < array_count_,
                  "array index out of bounds: %i from an array of size {0}",
                  index, array_count_);  // & for one branch
  }

  /**
   * Tests if index is valid, i.e. than or equal to zero, and less than the
   * number of elements in the array.
   *
   * \param index index to test.
   *
   * \returns True if index is valid. False otherwise.
   */
  FUN_ALWAYS_INLINE bool IsValidIndex(int32 index) const {
    return index >= 0 && index < array_count_;
  }

  /**
   * Returns number of elements in array.
   *
   * \returns Number of elements in array.
   */
  FUN_ALWAYS_INLINE int32 Count() const { return array_count_; }

  /**
   * array bracket operator. Returns reference to element at give index.
   *
   * \returns Reference to indexed element.
   */
  FUN_ALWAYS_INLINE ElementType& operator[](int32 index) const {
    RangeCheck(index);
    return MutableData()[index];
  }

  /**
   * Returns n-th last element from the array.
   *
   * \param index_from_the_end - (Optional) index from the end of array. Default
   * is 0.
   *
   * \returns Reference to n-th last element from the array.
   */
  FUN_ALWAYS_INLINE ElementType& Last(int32 index_from_the_end = 0) const {
    RangeCheck(array_count_ - index_from_the_end - 1);
    return MutableData()[array_count_ - index_from_the_end - 1];
  }

  /**
   * Returns a sliced view
   *
   * \param index - Starting index of the new view
   * \param count - Number of elements in the new view
   * \returns Sliced view
   */
  FUN_ALWAYS_INLINE ArrayView Slice(int32 index, int32 count) {
    fun_check(count > 0);
    fun_check(IsValidIndex(index));
    fun_check(IsValidIndex(index + count - 1));
    return ArrayView(data_ptr_ + index, count);
  }

  /**
   * Finds element within the array.
   *
   * \param item - item to look for.
   * \param out_index - Output parameter. found index.
   *
   * \returns True if found. False otherwise.
   */
  FUN_ALWAYS_INLINE bool Find(const ElementType& item, int32& out_index) const {
    out_index = this->Find(item);
    return out_index != INVALID_INDEX;
  }

  /**
   * Finds element within the array.
   *
   * \param item - item to look for.
   *
   * \returns index of the found element. INVALID_INDEX otherwise.
   */
  int32 Find(const ElementType& item) const {
    const ElementType* __restrict start = ConstData();
    for (const ElementType* __restrict data = start, *__restrict data_end =
                                                         data + array_count_;
         data != data_end; ++data) {
      if (*data == item) {
        return static_cast<int32>(data - start);
      }
    }

    return INVALID_INDEX;
  }

  /**
   * Finds element within the array starting from the end.
   *
   * \param item - item to look for.
   * \param out_index - Output parameter. found index.
   *
   * \returns True if found. False otherwise.
   */
  FUN_ALWAYS_INLINE bool FindLast(const ElementType& item,
                                  int32& out_index) const {
    out_index = this->FindLast(item);
    return out_index != INVALID_INDEX;
  }

  /**
   * Finds element within the array starting from start_index and going
   * backwards. Uses predicate to match element.
   *
   * \param pred - Predicate taking array element and returns true if element
   * matches search criteria, false otherwise. \param start_index - index of
   * element from which to start searching.
   *
   * \returns index of the found element. INVALID_INDEX otherwise.
   */
  template <typename Predicate>
  int32 FindLastIf(const Predicate& pred, int32 start_index) const {
    fun_check(start_index >= 0 && start_index <= this->Count());
    for (const ElementType* __restrict start = ConstData(),
                                       *__restrict data = start + start_index;
         data != start;) {
      --data;
      if (pred(*data)) {
        return static_cast<int32>(data - start);
      }
    }

    return INVALID_INDEX;
  }

  /**
   * Finds element within the array starting from the end. Uses predicate to
   * match element.
   *
   * \param pred - Predicate taking array element and returns true if element
   * matches search criteria, false otherwise.
   *
   * \returns index of the found element. INVALID_INDEX otherwise.
   */
  template <typename Predicate>
  FUN_ALWAYS_INLINE int32 FindLastIf(const Predicate& pred) const {
    return FindLastIf(pred, array_count_);
  }

  /**
   * Finds an item by key (assuming the ElementType overloads operator== for
   * the comparison).
   *
   * \param key - The key to search by.
   *
   * \returns index to the first matching element, or INVALID_INDEX if none is
   * found.
   */
  template <typename KeyTy>
  int32 IndexOfByKey(const KeyTy& key) const {
    const ElementType* __restrict start = ConstData();
    for (const ElementType* __restrict data = start, *__restrict data_end =
                                                         start + array_count_;
         data != data_end; ++data) {
      if (*data == key) {
        return static_cast<int32>(data - start);
      }
    }

    return INVALID_INDEX;
  }

  /**
   * Finds an item by predicate.
   *
   * \param pred - The predicate to match.
   *
   * \returns index to the first matching element, or INVALID_INDEX if none is
   * found.
   */
  template <typename Predicate>
  int32 IndexOfIf(const Predicate& pred) const {
    const ElementType* __restrict start = ConstData();
    for (const ElementType* __restrict data = start, *__restrict data_end =
                                                         start + array_count_;
         data != data_end; ++data) {
      if (pred(*data)) {
        return static_cast<int32>(data - start);
      }
    }

    return INVALID_INDEX;
  }

  /**
   * Finds an item by key (assuming the ElementType overloads operator== for
   * the comparison).
   *
   * \param key - The key to search by.
   *
   * \returns Pointer to the first matching element, or nullptr if none is
   * found.
   */
  template <typename KeyTy>
  ElementType* FindByKey(const KeyTy& key) const {
    for (ElementType* __restrict data = MutableData(),
                                 *__restrict data_end = data + array_count_;
         data != data_end; ++data) {
      if (*data == key) {
        return data;
      }
    }

    return nullptr;
  }

  /**
   * Finds an element which matches a predicate functor.
   *
   * \param pred - The functor to apply to each element.
   *
   * \return Pointer to the first element for which the predicate returns
   *         true, or nullptr if none is found.
   */
  template <typename Predicate>
  ElementType* FindIf(const Predicate& pred) const {
    for (ElementType* __restrict data = MutableData(),
                                 *__restrict data_end = data + array_count_;
         data != data_end; ++data) {
      if (pred(*data)) {
        return data;
      }
    }

    return nullptr;
  }

  /**
   * Filters the elements in the array based on a predicate functor.
   *
   * \param pred - The functor to apply to each element.
   *
   * \returns Array with the same type as this object which contains
   *          the subset of elements for which the functor returns true.
   */
  template <typename Predicate>
  Array<typename RemoveConst<ElementType>::Type> FilterIf(
      const Predicate& pred) const {
    Array<typename RemoveConst<ElementType>::Type> filtered_results;
    for (const ElementType* __restrict data = ConstData(),
                                       *__restrict data_end =
                                           data + array_count_;
         data != data_end; ++data) {
      if (pred(*data)) {
        filtered_results.Add(*data);
      }
    }

    return filtered_results;
  }

  /**
   * Checks if this array contains the element.
   *
   * \returns True if found. False otherwise.
   */
  template <typename ComparisonTy>
  bool Contains(const ComparisonTy& item) const {
    for (const ElementType* __restrict data = ConstData(),
                                       *__restrict data_end =
                                           data + array_count_;
         data != data_end; ++data) {
      if (*data == item) {
        return true;
      }
    }

    return false;
  }

  /**
   * Checks if this array contains element for which the predicate is true.
   *
   * \param pred - Predicate to use
   *
   * \returns True if found. False otherwise.
   */
  template <typename Predicate>
  FUN_ALWAYS_INLINE bool ContainsIf(const Predicate& pred) const {
    return !!FindIf(pred);
  }

 private:
  // DO NOT USE DIRECTLY
  // STL-like iterators to enable range-based for loop support.
  FUN_ALWAYS_INLINE friend ElementType* begin(const ArrayView& array) {
    return array.MutableData();
  }
  FUN_ALWAYS_INLINE friend ElementType* end(const ArrayView& array) {
    return array.MutableData() + array.Count();
  }

 public:
  /**
   * Sorts the array assuming < operator is defined for the item type.
   */
  void Sort() { fun::Sort(MutableData(), Count()); }

  /**
   * Sorts the array using user define predicate class.
   *
   * \param pred - Predicate class instance.
   */
  template <typename Predicate>
  void Sort(const Predicate& pred) {
    fun::Sort(MutableData(), Count(), pred);
  }

  /**
   * Stable sorts the array assuming < operator is defined for the item type.
   *
   * Stable sort is slower than non-stable algorithm.
   */
  void StableSort() { fun::StableSort(MutableData(), Count()); }

  /**
   * Stable sorts the array using user defined predicate class.
   *
   * Stable sort is slower than non-stable algorithm.
   *
   * \param pred - Predicate class instance
   */
  template <typename Predicate>
  void StableSort(const Predicate& pred) {
    fun::StableSort(MutableData(), Count(), pred);
  }

 private:
  ElementType* data_ptr_;
  int32 array_count_;
};

template <typename ElementType>
struct IsZeroConstructible<ArrayView<ElementType>> {
  enum { Value = true };
};

template <typename ElementType, size_t N>
ArrayView<ElementType> MakeArrayView(ElementType (&other)[N]) {
  return ArrayView<ElementType>(other);
}

template <typename ElementType, size_t N>
ArrayView<const ElementType> MakeArrayView(const ElementType (&other)[N]) {
  return ArrayView<ElementType>(other);
}

template <typename ElementType, typename Allocator>
ArrayView<ElementType> MakeArrayView(Array<ElementType, Allocator>& other) {
  return ArrayView<ElementType>(other);
}

template <typename ElementType, typename Allocator>
ArrayView<const ElementType> MakeArrayView(
    const Array<ElementType, Allocator>& other) {
  return ArrayView<const ElementType>(other);
}

template <typename ElementType>
ArrayView<ElementType> MakeArrayView(std::initializer_list<ElementType> list) {
  return ArrayView<ElementType>(list);
}

template <typename ElementType>
ArrayView<ElementType> MakeArrayView(ElementType* ptr, int32 count) {
  return ArrayView<ElementType>(ptr, count);
}

}  // namespace fun
