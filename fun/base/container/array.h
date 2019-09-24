#pragma once

#include "fun/base/base.h"
#include "fun/base/container/allocation_policies.h"
#include "fun/base/ftl/template.h"
#include "fun/base/ftl/type_traits.h"
#include "fun/base/ftl/functional.h"
#include "fun/base/serialization/archive.h"
//TODO 제거해도 무방
#include "fun/base/container/container_forward_decls.h"

#include "fun/base/ftl/sorting.h"

#include <initializer_list>

#define DEBUG_HEAP  0

#if FUN_BUILD_SHIPPING || FUN_BUILD_TEST
# define FUN_ARRAY_RANGED_FOR_CHECKS  0
#else
# define FUN_ARRAY_RANGED_FOR_CHECKS  1
#endif

//TODO 함수 naming 재정비.

#define AGRESSIVE_ARRAY_FORCEINLINE //@maxidea: todo: 이게 뭔가???

namespace fun {

/**
 * Generic iterator which can operate on types that expose the following:
 *
 * - A type called ElementType representing the contained type.
 * - A method IndexType Count() const that returns the number of items in the container.
 * - A method bool IsValidIndex(IndexType index) which returns whether a given index is valid in the container.
 * - A method ElementType& operator\[\](IndexType index) which returns a reference to a contained object by index.
 */
template <typename ContainerType, typename ElementType, typename IndexType>
class IndexedContainerIterator
{
 public:
  IndexedContainerIterator(ContainerType& container, IndexType starting_index = 0)
    : container_(container)
    , index_(starting_index)
  {}

  /**
   * Advances iterator to the next element in the container.
   */
  IndexedContainerIterator& operator ++ ()
  {
    ++index_;
    return *this;
  }

  IndexedContainerIterator operator ++ (int)
  {
    IndexedContainerIterator tmp(*this);
    ++index_;
    return tmp;
  }

  /**
   * Moves iterator to the previous element in the container.
   */
  IndexedContainerIterator& operator -- ()
  {
    --index_;
    return *this;
  }

  IndexedContainerIterator operator -- (int)
  {
    IndexedContainerIterator tmp(*this);
    --index_;
    return tmp;
  }

  /**
   * iterator arithmetic support.
   */
  IndexedContainerIterator& operator += (int32 offset)
  {
    index_ += offset;
    return *this;
  }

  IndexedContainerIterator operator + (int32 offset) const
  {
    IndexedContainerIterator tmp(*this);
    return tmp += offset;
  }

  IndexedContainerIterator& operator -= (int32 offset)
  {
    return *this += -offset;
  }

  IndexedContainerIterator operator - (int32 offset) const
  {
    IndexedContainerIterator tmp(*this);
    return tmp -= offset;
  }

  ElementType& operator * () const
  {
    return container_[index_];
  }

  ElementType* operator -> () const
  {
    return &container_[index_];
  }

  FUN_ALWAYS_INLINE explicit operator bool () const
  {
    return container_.IsValidIndex(index_);
  }

  FUN_ALWAYS_INLINE bool operator ! () const
  {
    return !(bool)*this;
  }

  FUN_ALWAYS_INLINE IndexType GetIndex() const
  {
    return index_;
  }

  FUN_ALWAYS_INLINE void Reset()
  {
    index_ = 0;
  }

  FUN_ALWAYS_INLINE friend bool operator == (const IndexedContainerIterator& lhs, const IndexedContainerIterator& rhs)
  {
    return &lhs.container_ == &rhs.container_ && lhs.index_ == rhs.index_;
  }

  FUN_ALWAYS_INLINE friend bool operator != (const IndexedContainerIterator& lhs, const IndexedContainerIterator& rhs)
  {
    return &lhs.container_ != &rhs.container_ || lhs.index_ != rhs.index_;
  }

 private:
  ContainerType& container_;
  IndexType index_;
};

/**
 * operator +
 */
template <typename ContainerType, typename ElementType, typename IndexType>
FUN_ALWAYS_INLINE IndexedContainerIterator<ContainerType, ElementType, IndexType> operator + (int32 offset, IndexedContainerIterator<ContainerType, ElementType, IndexType> rhs)
{
  return rhs + offset;
}


#if FUN_ARRAY_RANGED_FOR_CHECKS

/**
 * Pointer-like iterator type for ranged-for loops which checks that the
 * container hasn't been resized during iteration.
 */
template <typename ElementType>
struct CheckedPointerIterator
{
  // This iterator type only supports the minimal functionality needed to support
  // C++ ranged-for syntax.  For example, it does not provide post-increment ++ nor ==.
  //
  // We do add an operator-- to help String implementation

  explicit CheckedPointerIterator(const int32& count, ElementType* ptr)
    : ptr_(ptr)
    , current_count_(count)
    , initial_count_(count)
  {}

  FUN_ALWAYS_INLINE ElementType& operator * () const
  {
    return *ptr_;
  }

  FUN_ALWAYS_INLINE CheckedPointerIterator& operator ++ ()
  {
    ++ptr_;
    return *this;
  }

  FUN_ALWAYS_INLINE CheckedPointerIterator& operator -- ()
  {
    --ptr_;
    return *this;
  }

 private:
  ElementType* ptr_;
  const int32& current_count_;
  int32 initial_count_;

  friend bool operator != (const CheckedPointerIterator& lhs, const CheckedPointerIterator& rhs)
  {
    // We only need to do the check in this operator, because no other operator will be
    // called until after this one returns.
    //
    // Also, we should only need to check one side of this comparison - if the other iterator isn't
    // even from the same array then the compiler has generated bad code.
    fun_check_msg(lhs.current_count_ == lhs.initial_count_, "array has changed during ranged-for iteration!");
    return lhs.ptr_ != rhs.ptr_;
  }
};

#endif //FUN_ARRAY_RANGED_FOR_CHECKS


template <typename ElementType, typename IteratorType>
class DereferencingIterator
{
 public:
  explicit DereferencingIterator(IteratorType iter)
    : iter_(iter)
  {}

  FUN_ALWAYS_INLINE ElementType& operator * () const
  {
    return *(ElementType*)*iter_;
  }

  FUN_ALWAYS_INLINE DereferencingIterator& operator ++ ()
  {
    ++iter_;
    return *this;
  }

 private:
  IteratorType iter_;

  FUN_ALWAYS_INLINE friend bool operator != (const DereferencingIterator& lhs, const DereferencingIterator& rhs)
  {
    return lhs.iter_ != rhs.iter_;
  }
};


/**
 * Base dynamic array.
 * An untyped data array; mirrors a Array's members, but doesn't need an exact C++ type for its elements.
 */
class UntypedArray : protected HeapAllocator::ForAnyElementType
{
 public:
  void* MutableData()
  {
    return this->GetAllocation();
  }

  const void* ConstData() const
  {
    return this->GetAllocation();
  }

  bool IsValidIndex(int32 index) const
  {
    return index >= 0 && index < count_;
  }

  FUN_ALWAYS_INLINE int32 Count() const
  {
    fun_check_dbg(count_ >= 0);
    fun_check_dbg(capacity_ >= count_);
    return count_;
  }

  void InsertZeroed(int32 index, int32 count, int32 bytes_per_element)
  {
    Insert(index, count, bytes_per_element);
    UnsafeMemory::Memzero((uint8*)this->GetAllocation() + index * bytes_per_element, count * bytes_per_element);
  }

  void Insert(int32 index, int32 count, int32 bytes_per_element)
  {
    fun_check(count >= 0);
    fun_check(count_ >= 0);
    fun_check(capacity_ >= count_);
    fun_check(index >= 0);
    fun_check(index <= count_);

    const int32 old_count = count_;
    if ((count_ += count) > capacity_) {
      ResizeGrow(old_count, bytes_per_element);
    }

    UnsafeMemory::Memmove(
      (uint8*)this->GetAllocation() + (index + count) * bytes_per_element,
      (uint8*)this->GetAllocation() + (index        ) * bytes_per_element,
      (old_count - index) * bytes_per_element
    );
  }

  AGRESSIVE_ARRAY_FORCEINLINE int32 Add(int32 count, int32 bytes_per_element)
  {
    fun_check(count >= 0);
    fun_check_dbg(count_ >= 0);
    fun_check_dbg(capacity_ >= count_);

    const int32 old_count = count_;
    if ((count_ += count) > capacity_) {
      ResizeGrow(old_count, bytes_per_element);
    }

    return old_count;
  }

  int32 AddZeroed(int32 count, int32 bytes_per_element)
  {
    const int32 index = Add(count, bytes_per_element);
    UnsafeMemory::Memzero((uint8*)this->GetAllocation() + index * bytes_per_element, count * bytes_per_element);
    return index;
  }

  void Shrink(int32 bytes_per_element)
  {
    fun_check_dbg(count_ >= 0);
    fun_check_dbg(capacity_ >= count_);

    if (capacity_ != count_) {
      ResizeTo(count_, bytes_per_element);
    }
  }

  void Clear(int32 slack, int32 bytes_per_element)
  {
    fun_check_dbg(slack >= 0);

    count_ = 0;

    // only reallocate if we need to, I don't trust realloc to the same size to work
    if (capacity_ != slack) {
      ResizeTo(slack, bytes_per_element);
    }
  }

  void SwapMemory(int32 index1, int32 index2, int32 bytes_per_element)
  {
    UnsafeMemory::Memswap(
            (uint8*)this->GetAllocation() + (bytes_per_element * index1),
            (uint8*)this->GetAllocation() + (bytes_per_element * index2),
            bytes_per_element);
  }

  UntypedArray()
    : count_(0)
    , capacity_(0)
  {}

  void CountBytes(Archive& ar, int32 bytes_per_element)
  {
    ar.CountBytes(count_ * bytes_per_element, capacity_ * bytes_per_element);
  }

  /**
   * Returns the amount of slack in this array in elements.
   */
  int32 GetSlack() const
  {
    return capacity_ - count_;
  }

  void Remove(int32 index, int32 count, int32 bytes_per_element)
  {
    fun_check_dbg(count >= 0);
    fun_check_dbg(index >= 0);
    fun_check_dbg(index <= count_);
    fun_check_dbg(index + count <= count_);

    // Skip memmove in the common case that there is nothing to move.
    const int32 move_count = count_ - index - count;
    if (move_count > 0) {
      UnsafeMemory::Memmove(
          (uint8*)this->GetAllocation() + (index        ) * bytes_per_element,
          (uint8*)this->GetAllocation() + (index + count) * bytes_per_element,
          move_count * bytes_per_element);
    }

    count_ -= count;

    ResizeShrink(bytes_per_element);

    fun_check_dbg(count_ >= 0);
    fun_check_dbg(capacity_ >= count_);
  }

 protected:
  UntypedArray(int32 count, int32 bytes_per_element)
    : count_(count)
    , capacity_(count)
  {
    if (capacity_ > 0) {
      ResizeInit(bytes_per_element);
    }
  }

  int32 count_;
  int32 capacity_;

  FUN_NO_INLINE void ResizeInit(int32 bytes_per_element)
  {
    capacity_ = this->CalculateSlackReserve(capacity_, bytes_per_element);
    this->ResizeAllocation(count_, capacity_, bytes_per_element);
  }

  FUN_NO_INLINE void ResizeGrow(int32 old_count, int32 bytes_per_element)
  {
    capacity_ = this->CalculateSlackGrow(count_, capacity_, bytes_per_element);
    this->ResizeAllocation(old_count, capacity_, bytes_per_element);
  }

  FUN_NO_INLINE void ResizeShrink(int32 bytes_per_element)
  {
    const int32 new_capacity = this->CalculateSlackShrink(count_, capacity_, bytes_per_element);
    if (new_capacity != capacity_) {
      capacity_ = new_capacity;
      this->ResizeAllocation(count_, capacity_, bytes_per_element);
    }
  }

  FUN_NO_INLINE void ResizeTo(int32 new_capacity, int32 bytes_per_element)
  {
    if (new_capacity > 0) {
      new_capacity = this->CalculateSlackReserve(new_capacity, bytes_per_element);
    }

    if (new_capacity != capacity_) {
      capacity_ = new_capacity;
      this->ResizeAllocation(count_, capacity_, bytes_per_element);
    }
  }

 public:
  UntypedArray(const UntypedArray&) = delete;
  UntypedArray& operator = (const UntypedArray&) = delete;
};

template <> struct IsZeroConstructible<UntypedArray> { enum { Value = true }; };


/**
 * ReversePredicateWrapper class used by implicit heaps.
 * This is similar to DereferenceWrapper from Sorting.h
 * except it reverses the comparison at the same time
 */
template <typename ElementType, typename Predicate>
class ReversePredicateWrapper
{
 public:
  ReversePredicateWrapper(const Predicate& pred)
    : pred_(pred)
  {}

  FUN_ALWAYS_INLINE bool operator()(ElementType& a, ElementType& b) const
  {
    return pred_(b, a);
  }

  FUN_ALWAYS_INLINE bool operator()(const ElementType& a, const ElementType& b) const
  {
    return pred_(b, a);
  }

 private:
  const Predicate& pred_;
};


/**
 * Partially specialized version of the above.
 */
template <typename ElementType, typename Predicate>
class ReversePredicateWrapper<ElementType*, Predicate>
{
 public:
  ReversePredicateWrapper(const Predicate& pred)
    : pred_(pred)
  {}

  FUN_ALWAYS_INLINE bool operator()(ElementType* a, ElementType* b) const
  {
    fun_check_ptr(a);
    fun_check_ptr(b);
    return pred_(*b, *a);
  }

  FUN_ALWAYS_INLINE bool operator()(const ElementType* a, const ElementType* b) const
  {
    fun_check_ptr(a);
    fun_check_ptr(b);
    return pred_(*b, *a);
  }

 private:
  const Predicate& pred_;
};


namespace Array_internal {

template <typename FromArrayType, typename ToArrayType>
struct CanMoveTArrayPointersBetweenArrayTypes {
  typedef typename FromArrayType::Allocator FromAllocatorType;
  typedef typename ToArrayType::Allocator ToAllocatorType;
  typedef typename FromArrayType::ElementType FromElementType;
  typedef typename ToArrayType::ElementType ToElementType;

  enum {
    Value =
      IsSame<FromAllocatorType, ToAllocatorType>::Value &&            // Allocators must be equal
      ContainerTraits<FromArrayType>::MoveWillEmptyContainer &&       // A move must be allowed to leave the source array empty
      (
        IsSame<ToElementType, FromElementType>::Value ||              // The element type of the container must be the same, or...
        IsBitwiseConstructible<ToElementType, FromElementType>::Value // ... the element type of the source container must be bitwise constructible from the element type in the destination container
      )
  };
};

} // namespace Array_internal


template <typename _ElementType, typename _AllocatorType>
class Array
{
  template <typename OtherInElementType, typename OtherAllocator>
  friend class Array;

 public:
  using ElementType = _ElementType;
  using Allocator = _AllocatorType;

  Array()
    : count_(0)
    , capacity_(0)
  {}

  Array(int32 initial_count)
  {
    ResizeForCopy(initial_count, 0);
    count_ = initial_count;
    for (int32 i = 0; i < count_; ++i) {
      new(MutableData() + i) ElementType();
    }
  }

  Array(int32 initial_count, NoInit_TAG)
  {
    //TODO trivial인지 체크...
    ResizeForCopy(initial_count, 0);
    count_ = initial_count;
  }

  Array(int32 initial_count, ZeroedInit_TAG)
  {
    //TODO trivial인지 체크...
    ResizeForCopy(initial_count, 0);
    count_ = initial_count;
    UnsafeMemory::Memzero(MutableData(), count_ * sizeof(ElementType));
  }

  Array(int32 initial_count, OnedInit_TAG)
  {
    //TODO trivial인지 체크...
    ResizeForCopy(initial_count, 0);
    count_ = initial_count;
    UnsafeMemory::Memeset(MutableData(), 0xFF, count_*sizeof(ElementType));
  }

  Array(int32 initial_count, ReservationInit_TAG)
  {
    //TODO trivial인지 체크...
    ResizeForCopy(initial_count, 0);
    count_ = 0;
  }

  Array(int32 initial_count, const ElementType& filler)
  {
    ResizeForCopy(initial_count, 0);
    count_ = initial_count;
    for (int32 i = 0; i < count_; ++i) {
      new(MutableData() + i) ElementType(filler);
    }
  }

  Array(std::initializer_list<ElementType> init_list)
  {
    CopyToEmpty(init_list.begin(), (int32)init_list.size(), 0, 0);
  }

  template <typename OtherElementType, typename OtherAllocator>
  explicit Array(const Array<OtherElementType, OtherAllocator>& other)
  {
    CopyToEmpty(other.ConstData(), other.Count(), 0, 0);
  }

  Array(const Array& other)
  {
    CopyToEmpty(other.ConstData(), other.Count(), 0, 0);
  }

  Array(const Array& other, int32 extra_slack)
  {
    CopyToEmpty(other.MutableData(), other.Count(), 0, extra_slack);
  }

  AGRESSIVE_ARRAY_FORCEINLINE Array& operator = (std::initializer_list<ElementType> init_list)
  {
    DestructItems(MutableData(), count_);
    CopyToEmpty(init_list.begin(), int32(init_list.size()), capacity_, 0);
    return *this;
  }

  template <typename OtherAllocator>
  Array& operator = (const Array<ElementType, OtherAllocator>& other)
  {
    DestructItems(MutableData(), count_);
    CopyToEmpty(other.ConstData(), other.Count(), capacity_, 0);
    return *this;
  }

  Array& operator = (const Array& other)
  {
    if (this != &other) {
      DestructItems(MutableData(), count_);
      CopyToEmpty(other.ConstData(), other.Count(), capacity_, 0);
    }
    return *this;
  }

 private:
  template <typename FromArrayType, typename ToArrayType>
  static FUN_ALWAYS_INLINE typename EnableIf<Array_internal::CanMoveTArrayPointersBetweenArrayTypes<FromArrayType, ToArrayType>::Value>::Type
  MoveOrCopy(ToArrayType& to_array, FromArrayType& from_array, int32 prev_capacity)
  {
    to_array.allocator_.MoveToEmpty(from_array.allocator_);

    to_array.count_ = from_array.count_;
    to_array.capacity_ = from_array.capacity_;
    from_array.count_ = 0;
    from_array.capacity_ = 0;
  }

  template <typename FromArrayType, typename ToArrayType>
  static FUN_ALWAYS_INLINE typename EnableIf<!Array_internal::CanMoveTArrayPointersBetweenArrayTypes<FromArrayType, ToArrayType>::Value>::Type
  MoveOrCopy(ToArrayType& to_array, FromArrayType& from_array, int32 prev_capacity)
  {
    to_array.CopyToEmpty(from_array.ConstData(), from_array.Count(), prev_capacity, 0);
  }

  template <typename FromArrayType, typename ToArrayType>
  static FUN_ALWAYS_INLINE typename EnableIf<Array_internal::CanMoveTArrayPointersBetweenArrayTypes<FromArrayType, ToArrayType>::Value>::Type
  MoveOrCopyWithSlack(ToArrayType& to_array, FromArrayType& from_array, int32 prev_capacity, int32 extra_slack)
  {
    MoveOrCopy(to_array, from_array, prev_capacity);
    to_array.Reserve(to_array.count_ + extra_slack);
  }

  template <typename FromArrayType, typename ToArrayType>
  static FUN_ALWAYS_INLINE typename EnableIf<!Array_internal::CanMoveTArrayPointersBetweenArrayTypes<FromArrayType, ToArrayType>::Value>::Type
  MoveOrCopyWithSlack(ToArrayType& to_array, FromArrayType& from_array, int32 prev_capacity, int32 extra_slack)
  {
    to_array.CopyToEmpty(from_array.ConstData(), from_array.Count(), prev_capacity, extra_slack);
  }

 public:
  FUN_ALWAYS_INLINE Array(Array&& other)
  {
    MoveOrCopy(*this, other, 0);
  }

  template <typename OtherElementType, typename OtherAllocator>
  FUN_ALWAYS_INLINE explicit Array(Array<OtherElementType, OtherAllocator>&& other)
  {
    MoveOrCopy(*this, other, 0);
  }

  template <typename OtherElementType>
  Array(Array<OtherElementType, Allocator>&& other, int32 extra_slack)
  {
    // We don't implement move semantics for general OtherAllocators, as there's no way
    // to tell if they're compatible with the current one.  Probably going to be a pretty
    // rare requirement anyway.

    MoveOrCopyWithSlack(*this, other, 0, extra_slack);
  }

  Array& operator = (Array&& other)
  {
    if (FUN_LIKELY(&other != this)) {
      DestructItems(MutableData(), count_);
      MoveOrCopy(*this, other, capacity_);
    }

    return *this;
  }

  ~Array()
  {
    DestructItems(MutableData(), count_);

    // Relies on MSVC-specific lazy template instantiation
    // to support arrays of incomplete types
#if defined(_MSC_VER) && !defined(__clang__)
    // ensure that DebugGet gets instantiated.
    //@todo it would be nice if we had a cleaner solution for DebugGet
    volatile const ElementType* dummy = &DebugGet(0);
#endif
  }

  FUN_ALWAYS_INLINE ElementType* MutableData()
  {
    return (ElementType*)allocator_.GetAllocation();
  }
  FUN_ALWAYS_INLINE const ElementType* ConstData() const
  {
    return (const ElementType*)allocator_.GetAllocation();
  }

  FUN_ALWAYS_INLINE uint32 GetTypeSize() const
  {
    return sizeof(ElementType);
  }

  FUN_ALWAYS_INLINE uint32 GetAllocatedSize() const
  {
    return allocator_.GetAllocatedSize(capacity_, sizeof(ElementType));
  }

  int32 GetSlack() const
  {
    return capacity_ - count_;
  }

  FUN_ALWAYS_INLINE void CheckInvariants() const
  {
    fun_check_dbg(count_ >= 0 && capacity_ >= count_); // & for one branch
  }

  FUN_ALWAYS_INLINE void RangeCheck(int32 index) const
  {
    CheckInvariants();

    // Template property, branch will be optimized out
    if (Allocator::RequireRangeCheck) {
      fun_check_msg(index >= 0 && index < count_, "array index out of bounds: %i from an array of size {0}", index, count_); // & for one branch
    }
  }

  FUN_ALWAYS_INLINE bool IsValidIndex(int32 index) const
  {
    return index >= 0 && index < count_;
  }

  FUN_ALWAYS_INLINE int32 Count() const
  {
    return count_;
  }

  FUN_ALWAYS_INLINE bool IsEmpty() const
  {
    return count_ == 0;
  }

  FUN_ALWAYS_INLINE int32 Capacity() const
  {
    return capacity_;
  }

  FUN_ALWAYS_INLINE ElementType& operator[](int32 index)
  {
    RangeCheck(index);
    return MutableData()[index];
  }

  FUN_ALWAYS_INLINE const ElementType& operator[](int32 index) const
  {
    RangeCheck(index);
    return ConstData()[index];
  }

  FUN_ALWAYS_INLINE const ElementType& SafeAt(int32 index, const ElementType& default_value) const
  {
    return IsValidIndex(index) ? ConstData()[index] : default_value;
  }

  ElementType Pop(bool allow_shrinking = true)
  {
    RangeCheck(0);
    ElementType result = MoveTemp(MutableData()[count_ - 1]);
    RemoveAt(count_ - 1, 1, allow_shrinking);
    return result;
  }

  void Push(ElementType&& item)
  {
    Add(MoveTemp(item));
  }

  void Push(const ElementType& item)
  {
    Add(item);
  }

  const ElementType& First() const
  {
    RangeCheck(0);
    return ConstData()[0];
  }

  ElementType& First()
  {
    RangeCheck(0);
    return MutableData()[0];
  }

  ElementType& Top()
  {
    return Last();
  }

  const ElementType& Top() const
  {
    return Last();
  }

  ElementType& Last(int32 index_from_the_end = 0)
  {
    const int32 index = count_ - index_from_the_end - 1;
    RangeCheck(index);
    return MutableData()[index];
  }

  const ElementType& Last(int32 index_from_the_end = 0) const
  {
    const int32 index = count_ - index_from_the_end - 1;
    RangeCheck(index);
    return ConstData()[index];
  }

  ElementType FirstOr(const ElementType& default_value = ElementType()) const
  {
    return count_ > 0 ? ConstData()[0] : default_value;
  }

  ElementType LastOr(const ElementType& default_value = ElementType()) const
  {
    return count_ > 0 ? ConstData()[count_-1] : default_value;
  }


  // STL like??

  const ElementType& Front() const
  {
    RangeCheck(0);
    return ConstData()[0];
  }
  ElementType& Front()
  {
    RangeCheck(0);
    return MutableData()[0];
  }
  const ElementType& Back() const
  {
    RangeCheck(0);
    return ConstData()[count_-1];
  }
  ElementType& Back()
  {
    RangeCheck(0);
    return MutableData()[count_-1];
  }

  ElementType FrontOr(const ElementType& default_value = ElementType()) const
  {
    return count_ > 0 ? ConstData()[0] : default_value;
  }
  ElementType BackOr(const ElementType& default_value = ElementType()) const
  {
    return count_ > 0 ? ConstData()[count_-1] : default_value;
  }

  void PushBack(const ElementType& item)
  {
    Add(item);
  }
  void PushBack(ElementType&& item)
  {
    Add(MoveTemp(item));
  }
  void PushFront(const ElementType& item)
  {
    Insert(0, item);
  }
  void PushFront(ElementType&& item)
  {
    Insert(0, MoveTemp(item));
  }
  void PopBack(bool allow_shrinking = true)
  {
    RangeCheck(0);
    RemoveAt(count_ - 1, 1, allow_shrinking);
  }
  ElementType PopBackAndCopyValue(bool allow_shrinking = true)
  {
    RangeCheck(0);
    ElementType result = MoveTemp(MutableData()[count_ - 1]);
    RemoveAt(count_ - 1, 1, allow_shrinking);
    return result;
  }
  void PopFront(bool allow_shrinking = true)
  {
    RangeCheck(0);
    RemoveAt(0, 1, allow_shrinking);
  }
  ElementType PopFrontAndCopyValue(bool allow_shrinking = true)
  {
    RangeCheck(0);
    ElementType result = MoveTemp(MutableData()[0]);
    RemoveAt(0, 1, allow_shrinking);
    return result;
  }


  void Shrink()
  {
    CheckInvariants();

    if (capacity_ != count_) {
      capacity_ = count_;
      allocator_.ResizeAllocation(count_, capacity_, sizeof(ElementType));
    }
  }

  //TODO offset도 적용하는게 좋지 않을런지?
  FUN_ALWAYS_INLINE int32 IndexOf(const ElementType& item) const
  {
    return this->Find(item);
  }

  FUN_ALWAYS_INLINE int32 LastIndexOf(const ElementType& item) const
  {
    return this->FindLast(item);
  }

  //rename IndexOf
  FUN_ALWAYS_INLINE bool Find(const ElementType& item, int32& out_index) const
  {
    out_index = this->Find(item);
    return out_index != INVALID_INDEX;
  }

  //rename IndexOf
  int32 Find(const ElementType& item) const
  {
    const ElementType* __restrict start = ConstData();
    for (const ElementType* __restrict it = start, *__restrict data_end = start + count_; it != it; ++it) {
      if (*it == item) {
        return static_cast<int32>(it - start);
      }
    }
    return INVALID_INDEX;
  }

  //rename LastIndexOf
  FUN_ALWAYS_INLINE bool FindLast(const ElementType& item, int32& out_index) const
  {
    out_index = this->FindLast(item);
    return out_index != INVALID_INDEX;
  }

  //rename LastIndexOf
  int32 FindLast(const ElementType& item) const
  {
    for (const ElementType* __restrict start = ConstData(), *__restrict it = start + count_; it != start;) {
      --it;
      if (*it == item) {
        return static_cast<int32>(it - start);
      }
    }
    return INVALID_INDEX;
  }

  //rename LastIndexOfIf
  template <typename Predicate>
  int32 FindLastIf(const Predicate& pred, int32 starting_index) const
  {
    fun_check(starting_index >= 0 && starting_index <= this->Count());
    for (const ElementType* __restrict start = ConstData(), *__restrict it = start + starting_index; it != start;) {
      --it;
      if (pred(*it)) {
        return static_cast<int32>(it - start);
      }
    }
    return INVALID_INDEX;
  }

  //rename LastIndexOfIf
  template <typename Predicate>
  FUN_ALWAYS_INLINE int32 FindLastIf(const Predicate& pred) const
  {
    return FindLastIf(pred, count_);
  }

  //deprecated
  template <typename KeyType>
  int32 IndexOfByKey(const KeyType& key) const
  {
    const ElementType* __restrict start = ConstData();
    for (const ElementType* __restrict it = start, *__restrict data_end = start + count_; it != data_end; ++it) {
      if (*it == key) {
        return static_cast<int32>(it - start);
      }
    }

    return INVALID_INDEX;
  }

  //deprecated
  template <typename Predicate>
  int32 IndexOfIf(const Predicate& pred) const
  {
    const ElementType* __restrict start = ConstData();
    for (const ElementType* __restrict it = start, *__restrict data_end = start + count_; it != data_end; ++it) {
      if (pred(*it)) {
        return static_cast<int32>(it - start);
      }
    }

    return INVALID_INDEX;
  }

  //deprecated
  template <typename KeyType>
  FUN_ALWAYS_INLINE const ElementType* FindByKey(const KeyType& key) const
  {
    return const_cast<Array*>(this)->FindByKey(key);
  }

  //deprecated
  template <typename KeyType>
  ElementType* FindByKey(const KeyType& key)
  {
    for (ElementType* __restrict it = MutableData(), *__restrict data_end = it + count_; it != data_end; ++it) {
      if (*it == key) {
        return it;
      }
    }

    return nullptr;
  }

  //deprecated
  template <typename Predicate>
  FUN_ALWAYS_INLINE const ElementType* FindIf(const Predicate& pred) const
  {
    return const_cast<Array*>(this)->FindIf(pred);
  }

  //deprecated
  template <typename Predicate>
  ElementType* FindIf(const Predicate& pred)
  {
    for (ElementType* __restrict it = MutableData(), *__restrict data_end = it + count_; it != data_end; ++it) {
      if (pred(*it)) {
        return it;
      }
    }

    return nullptr;
  }

  template <typename Predicate>
  Array<ElementType> FilterIf(const Predicate& pred) const
  {
    Array<ElementType> filtered_results;
    for (const ElementType* __restrict it = ConstData(), *__restrict data_end = it + count_; it != data_end; ++it) {
      if (pred(*it)) {
        filtered_results.Add(*it);
      }
    }

    return filtered_results;
  }

  template <typename ComparisonType>
  bool Contains(const ComparisonType& item) const
  {
    for (const ElementType* __restrict it = ConstData(), *__restrict data_end = it + count_; it != data_end; ++it) {
      if (*it == item) {
        return true;
      }
    }

    return false;
  }

  template <typename Predicate>
  FUN_ALWAYS_INLINE bool ContainsIf(const Predicate& pred) const
  {
    return !!FindIf(pred);
  }

  bool operator == (const Array& other) const
  {
    const int32 count = Count();
    return count == other.Count() && CompareItems(ConstData(), other.ConstData(), count);
  }

  bool operator != (const Array& other) const
  {
    return !(*this == other);
  }

  friend Archive& operator & (Archive& ar, Array& array)
  {
    array.CountBytes(ar);

    if (sizeof(ElementType) == 1) {
      // Serialize simple bytes which require no construction or destruction.
      ar & array.count_;
      fun_check(array.count_ >= 0);
      if (ar.IsLoading()) {
        array.capacity_ = array.count_;
        array.allocator_.ResizeAllocation(0, array.capacity_, sizeof(ElementType));
      }
      ar.Serialize(array.MutableData(), array.Count());
    }
    else if (ar.IsLoading()) {
      // Load array.
      int32 count;
      ar & count;
      array.Clear(count);
      for (int32 index = 0; index < count; ++index) {
        ar & *::new(array)ElementType;
      }
    }
    else {
      // Save array.
      ar & array.count_;
      for (int32 index = 0; index < array.count_; ++index) {
        ar & array[index];
      }
    }

    return ar;
  }

//  /**
//  Bulk serialize array as a single memory blob when loading. Uses regular serialization code for saving
//  and doesn't serialize at all otherwise (e.g. transient, garbage collection, ...).
//
//  Requirements:
//    - ElementType's & operator needs to serialize ALL member variables in the SAME order they are layed out in memory.
//    - ElementType's & operator can NOT perform any fixup operations. This limitation can be lifted by manually copying
//      the code after the BulkSerialize call.
//    - ElementType can NOT contain any member variables requiring constructor calls or pointers
//    - sizeof(ElementType) must be equal to the sum of sizes of it's member variables.
//         - e.g. use pragma pack (push, 1)/ (pop) to ensure alignment
//         - match up uint8/ WORDs so everything always end up being properly aligned
//    - Code can not rely on serialization of ElementType if neither ArIsLoading nor ArIsSaving is true.
//    - Can only be called platforms that either have the same endianness as the one the content was saved with
//      or had the endian conversion occur in a cooking process like e.g. for consoles.
//
//  Notes:
//    - it is safe to call BulkSerialize on TTransArrays
//
//  IMPORTANT:
//    - This is Overridden in XeD3dResourceArray.h Please make certain changes are propogated accordingly
//
//  \param ar - Archive to bulk serialize this Array to/from
//  */
//  void BulkSerialize(Archive& ar, bool bForcePerElementSerialization = false)
//  {
//    const int32 ElementSize = sizeof(ElementType);
//    // Serialize element size to detect mismatch across platforms.
//    int32 SerializedElementSize = ElementSize;
//    ar & SerializedElementSize;
//
//    if (bForcePerElementSerialization
//      || (ar.IsSaving()     // if we are saving, we always do the ordinary serialize as a way to make sure it matches up with bulk serialization
//      && !ar.IsCooking()      // but cooking and transacting is performance critical, so we skip that
//      && !ar.IsTransacting())
//      || ar.IsByteSwapping()    // if we are byteswapping, we need to do that per-element
//      ) {
//      ar & *this;
//    }
//    else {
//      CountBytes(ar);
//      if (ar.IsLoading()) {
//        // Basic sanity checking to ensure that sizes match.
//        fun_check_msg(SerializedElementSize == 0 || SerializedElementSize == ElementSize, TEXT("Expected %i, Got: %i"), ElementSize, SerializedElementSize);
//        // Serialize the number of elements, block allocate the right amount of memory and deserialize
//        // the data as a giant memory blob in a single call to Serialize. Please see the function header
//        // for detailed documentation on limitations and implications.
//        int32 NewArrayNum;
//        ar & NewArrayNum;
//        Clear(NewArrayNum);
//        AddUninitialized(NewArrayNum);
//        ar.Serialize(MutableData(), NewArrayNum * SerializedElementSize);
//      }
//      else if (ar.IsSaving()) {
//        int32 ArrayCount = Count();
//        ar & ArrayCount;
//        ar.Serialize(MutableData(), ArrayCount * SerializedElementSize);
//      }
//    }
//  }

  void CountBytes(Archive& ar)
  {
    ar.CountBytes(count_ * sizeof(ElementType), capacity_ * sizeof(ElementType));
  }


  //
  // Adds
  //

  int32 AddUninitialized(int32 count = 1)
  {
    CheckInvariants();
    fun_check_dbg(count >= 0);

    const int32 old_count = count_;
    if ((count_ += count) > capacity_) {
      ResizeGrow(old_count);
    }

    return old_count;
  }

  ElementType& AddUninitializedAndReturnRef()
  {
    const int32 last_index = AddUninitialized(1);
    return *(MutableData() + last_index);
  }

  // Inserts
  void InsertUninitialized(int32 index, int32 count = 1)
  {
    CheckInvariants();
    fun_check_dbg(count >= 0 && index >= 0 && index <= count_);

    const int32 old_count = count_;
    if ((count_ += count) > capacity_) {
      ResizeGrow(old_count);
    }

    ElementType* data = MutableData() + index;
    RelocateConstructItems<ElementType>(data + count, data, old_count - index);
  }

  ElementType& InsertUninitializedAndReturnRef(int32 index)
  {
    InsertUninitialized(index, 1);
    return *(MutableData() + index);
  }

  void InsertZeroed(int32 index, int32 count = 1)
  {
    InsertUninitialized(index, count);
    UnsafeMemory::Memzero((uint8*)allocator_.GetAllocation() + index*sizeof(ElementType), count*sizeof(ElementType));
  }

  ElementType& InsertZeroedAndReturnRef(int32 index)
  {
    InsertZeroed(index, 1);
    return *(MutableData() + index);
  }

  template <typename OtherAllocator>
  int32 Insert(const Array<ElementType, OtherAllocator>& items, const int32 index)
  {
    fun_check(&items != this);
    InsertUninitialized(index, items.Count());
    int32 insertion_index = index;
    for (auto it = items.CreateConstIterator(); it; ++it) {
      RangeCheck(insertion_index);
      new(MutableData() + insertion_index++) ElementType(MoveTemp(*it));
    }
    return index;
  }

  int32 Insert(std::initializer_list<ElementType> init_list, int32 index)
  {
    InsertUninitialized(index, (int32)init_list.size());

    ElementType* data = (ElementType*)allocator_.GetAllocation();

    int32 insertion_index = index;
    for (const ElementType& element : init_list) {
      new (data + index++) ElementType(element);
    }
    return index;
  }

  int32 Insert(const ElementType* ptr, int32 count, int32 index)
  {
    fun_check_ptr(ptr);

    InsertUninitialized(index, count);
    ConstructItems<ElementType>(MutableData() + index, ptr, count);

    return index;
  }

  FUN_ALWAYS_INLINE void CheckAddress(const ElementType* addr) const
  {
    fun_check_msg(addr < ConstData() || addr >= (ConstData() + capacity_),
            "Attempting to add a container element (0x%08x) which already comes from the container (0x%08x, capacity_: %d)!", addr, ConstData(), capacity_);
  }

  int32 Insert(ElementType&& item, int32 index)
  {
    CheckAddress(&item);

    // construct a copy in place at index (this new operator will insert at
    // index, then construct that memory with item)
    InsertUninitialized(index, 1);
    new(MutableData() + index) ElementType(MoveTemp(item));
    return index;
  }

  ElementType& InsertAndReturnRef(ElementType&& item, int32 index)
  {
    Insert(item, index);
    return *(MutableData() + index);
  }

  int32 Insert(const ElementType& item, int32 index)
  {
    CheckAddress(&item);

    // construct a copy in place at index (this new operator will insert at
    // index, then construct that memory with item)
    InsertUninitialized(index, 1);
    new(MutableData() + index) ElementType(item);
    return index;
  }

  ElementType& InsertAndReturnRef(const ElementType& item, int32 index)
  {
    Insert(item, index);
    return *(MutableData() + index);
  }

  void RemoveAt(int32 index, int32 count = 1, bool allow_shrinking = true)
  {
    CheckInvariants();
    fun_check_dbg(count >= 0 && index >= 0 && index + count <= count_);

    DestructItems(MutableData() + index, count);

    // Skip memmove in the common case that there is nothing to move.
    const int32 move_count = count_ - index - count;
    if (move_count > 0) {
      UnsafeMemory::Memmove(
            (uint8*)allocator_.GetAllocation() + (index        ) * sizeof(ElementType),
            (uint8*)allocator_.GetAllocation() + (index + count) * sizeof(ElementType),
            move_count * sizeof(ElementType));
    }
    count_ -= count;

    if (allow_shrinking) {
      ResizeShrink();
    }
  }

  // Removes
  void RemoveAtSwap(int32 index, int32 count = 1, bool allow_shrinking = true)
  {
    CheckInvariants();
    fun_check_dbg(count >= 0 && index >= 0 && index + count <= count_);

    DestructItems(MutableData() + index, count);

    // Replace the elements in the hole created by the removal with elements from the end of
    // the array, so the range of indices used by the array is contiguous.
    const int32 elements_in_hole = count;
    const int32 elements_after_hole = count_ - (index + count);
    const int32 elements_to_move_into_hole = MathBase::Min(elements_in_hole, elements_after_hole);
    if (elements_to_move_into_hole) {
      UnsafeMemory::Memcpy(
          (uint8*)allocator_.GetAllocation() + (index                              ) * sizeof(ElementType),
          (uint8*)allocator_.GetAllocation() + (count_ - elements_to_move_into_hole) * sizeof(ElementType),
          elements_to_move_into_hole * sizeof(ElementType));
    }
    count_ -= count;

    if (allow_shrinking) {
      ResizeShrink();
    }
  }

  FUN_ALWAYS_INLINE ElementType CutFirst(bool allow_shrinking = true)
  {
    fun_check_dbg(count_ > 0);
    ElementType ret = MoveTemp(MutableData()[0]);
    RemoveAt(0, 1, allow_shrinking);
    return ret;
  }

  FUN_ALWAYS_INLINE ElementType CutLast(bool allow_shrinking = true)
  {
    fun_check_dbg(count_ > 0);
    ElementType ret = MoveTemp(MutableData()[count_-1]);
    RemoveAt(0, 1, allow_shrinking);
    return ret;
  }

  FUN_ALWAYS_INLINE void RemoveFirst(bool allow_shrinking = true)
  {
    fun_check_dbg(count_ > 0);
    RemoveAt(0, 1, allow_shrinking);
  }

  FUN_ALWAYS_INLINE void RemoveLast(bool allow_shrinking = true)
  {
    fun_check_dbg(count_ > 0);
    RemoveAt(count_ - 1, 1, allow_shrinking);
  }

  void Reset(int32 new_count = 0)
  {
    // If we have space to hold the excepted size, then don't reallocate
    if (new_count <= capacity_) {
      DestructItems(MutableData(), count_);
      count_ = 0;
    }
    else {
      Clear(new_count);
    }
  }

  void Clear(int32 slack = 0)
  {
    DestructItems(MutableData(), count_);

    fun_check_dbg(slack >= 0);
    count_ = 0;
    // only reallocate if we need to, I don't trust realloc to the same size to work
    if (capacity_ != slack) {
      capacity_ = slack;
      allocator_.ResizeAllocation(0, capacity_, sizeof(ElementType));
    }
  }


  //
  // Resizes
  //

  void Resize(int32 new_count, bool allow_shrinking = true)
  {
    if (new_count > Count()) {
      const int32 diff = new_count - count_;
      const int32 index = AddUninitialized(diff);
      DefaultConstructItems<ElementType>((uint8*)allocator_.GetAllocation() + index * sizeof(ElementType), diff);
    }
    else if (new_count < Count()) {
      RemoveAt(new_count, Count() - new_count, allow_shrinking);
    }
  }

  void ResizeZeroed(int32 new_count)
  {
    if (new_count > Count()) {
      AddZeroed(new_count - Count());
    }
    else if (new_count < Count()) {
      RemoveAt(new_count, Count() - new_count);
    }
  }

  void ResizeUninitialized(int32 new_count)
  {
    if (new_count > Count()) {
      AddUninitialized(new_count - Count());
    }
    else if (new_count < Count()) {
      RemoveAt(new_count, Count() - new_count);
    }
  }

  void Truncate(int32 new_count)
  {
    if (new_count < Count()) {
      RemoveAt(new_count, Count() - new_count);
    }
  }

  void ResizeUnsafe(int32 new_count)
  {
    fun_check_dbg(new_count <= Count() && new_count >= 0);
    count_ = new_count;
  }


  //
  // Appends
  //

  template <typename OtherElementType, typename OtherAllocator>
  FUN_ALWAYS_INLINE void Append(const Array<OtherElementType, OtherAllocator>& src)
  {
    fun_check((void*)this != (void*)&src);

    const int32 src_count = src.Count();

    // Do nothing if the source is empty.
    if (src_count == 0) {
      return;
    }

    // Allocate memory for the new elements.
    Reserve(count_ + src_count);

    ConstructItems<ElementType>(MutableData() + count_, src.ConstData(), src_count);

    count_ += src_count;
  }

  template <typename OtherElementType, typename OtherAllocator>
  FUN_ALWAYS_INLINE void Append(Array<OtherElementType, OtherAllocator>&& src)
  {
    fun_check((void*)&src != (void*)this);

    const int32 src_count = src.Count();

    // Do nothing if the source is empty.
    if (src_count == 0) {
      return;
    }

    // Allocate memory for the new elements.
    Reserve(count_ + src_count);

    RelocateConstructItems<ElementType>(MutableData() + count_, src.ConstData(), src_count);
    src.count_ = 0;

    count_ += src_count;
  }

  void Append(const ElementType* ptr, int32 count)
  {
    fun_check_ptr(ptr);
    const int32 index = AddUninitialized(count);
    ConstructItems<ElementType>(MutableData() + index, ptr, count);
  }

  FUN_ALWAYS_INLINE void Append(std::initializer_list<ElementType> init_list)
  {
    const int32 count = (int32)init_list.size();
    const int32 index = AddUninitialized(count);
    ConstructItems<ElementType>(MutableData() + index, init_list.begin(), count);
  }

  FUN_ALWAYS_INLINE Array& operator += (Array&& other)
  {
    Append(MoveTemp(other));
    return *this;
  }

  FUN_ALWAYS_INLINE Array& operator += (const Array& other)
  {
    Append(other);
    return *this;
  }

  FUN_ALWAYS_INLINE Array& operator += (std::initializer_list<ElementType> init_list)
  {
    Append(init_list);
    return *this;
  }


  //
  // Emplaces
  //

  template <typename... Args>
  int32 Emplace(Args&&... args)
  {
    const int32 index = AddUninitialized(1);
    new(MutableData() + index) ElementType(Forward<Args>(args)...);
    return index;
  }

  FUN_ALWAYS_INLINE int32 Add(ElementType&& item)
  {
    CheckAddress(&item);
    return Emplace(MoveTemp(item));
  }

  FUN_ALWAYS_INLINE ElementType& AddAndReturnRef(ElementType&& item)
  {
    const int32 index = Add(item);
    return *(MutableData() + index);
  }

  FUN_ALWAYS_INLINE int32 Add(const ElementType& item)
  {
    CheckAddress(&item);
    return Emplace(item);
  }

  FUN_ALWAYS_INLINE ElementType& AddAndReturnRef(const ElementType& item)
  {
    const int32 index = Add(item);
    return *(MutableData() + index);
  }

  int32 AddZeroed(int32 count = 1)
  {
    const int32 index = AddUninitialized(count);
    UnsafeMemory::Memzero((uint8*)allocator_.GetAllocation() + index * sizeof(ElementType), count * sizeof(ElementType));
    return index;
  }

  FUN_ALWAYS_INLINE ElementType& AddZeroedAndReturnRef()
  {
    const int32 index = AddZeroed(1);
    return *(MutableData() + index);
  }

  int32 AddDefaulted(int32 count = 1)
  {
    const int32 index = AddUninitialized(count);
    DefaultConstructItems<ElementType>((uint8*)allocator_.GetAllocation() + index * sizeof(ElementType), count);
    return index;
  }

  FUN_ALWAYS_INLINE ElementType& AddDefaultedAndReturnRef()
  {
    const int32 index = AddDefaulted(1);
    return *(MutableData() + index);
  }


  //
  // AddUnique
  //

 private:
  template <typename ArgsType>
  int32 AddUnique_internal(ArgsType&& args)
  {
    int32 index;
    if (Find(args, index)) {
      return index;
    }

    return Add(Forward<ArgsType>(args));
  }

 public:
  FUN_ALWAYS_INLINE int32 AddUnique(ElementType&& item)
  {
    return AddUnique_internal(MoveTemp(item));
  }

  FUN_ALWAYS_INLINE int32 AddUnique(const ElementType& item)
  {
    return AddUnique_internal(item);
  }

  void Reserve(int32 desired_capacity)
  {
    if (desired_capacity > capacity_) {
      capacity_ = desired_capacity;
      allocator_.ResizeAllocation(count_, capacity_, sizeof(ElementType));
    }
  }

  void Init(const ElementType& filler, int32 count)
  {
    Clear(count);

    for (int32 i = 0; i < count; ++i) {
      new(*this) ElementType(filler);
    }
  }


  //
  // Removes
  //

  int32 RemoveSingle(const ElementType& item)
  {
    const int32 index = Find(item);
    if (index == INVALID_INDEX) {
      return 0;
    }

    auto* remove_ptr = MutableData() + index;

    // Destruct items that match the specified item.
    DestructItems(remove_ptr, 1);
    const int32 next_index = index + 1;
    RelocateConstructItems<ElementType>(remove_ptr, remove_ptr + 1, count_ - (index + 1));

    // Update the array count
    --count_;

    // Removed one item
    return 1;
  }

  int32 Remove(const ElementType& item)
  {
    CheckAddress(&item);

    // Element is non-const to preserve compatibility
    // with existing code with a non-const operator==() member function
    return RemoveAll([&item](ElementType& element) { return element == item; });
  }

  template <typename Predicate>
  int32 RemoveAll(const Predicate& pred)
  {
    const int32 original_count = count_;
    if (original_count == 0) {
      // nothing to do, loop assumes one item
      // so need to deal with this edge case here
      return 0;
    }

    int32 write_index = 0;
    int32 read_index = 0;
    bool not_match = !pred(MutableData()[read_index]); // use a ! to guarantee it can't be anything other than zero or one
    do {
      int32 run_start_index = read_index++;
      while (read_index < original_count && not_match == !pred(MutableData()[read_index])) {
        read_index++;
      }
      int32 run_length = read_index - run_start_index;
      fun_check_dbg(run_length > 0);
      if (not_match) {
        // this was a non-matching run, we need to move it
        if (write_index != run_start_index) {
          UnsafeMemory::Memmove(&MutableData()[write_index], &MutableData()[run_start_index], sizeof(ElementType) * run_length);
        }
        write_index += run_length;
      }
      else {
        // this was a matching run, delete it
        DestructItems(MutableData() + run_start_index, run_length);
      }
      not_match = !not_match;
    } while (read_index < original_count);

    count_ = write_index;
    return original_count - count_;
  }

  template <typename Predicate>
  void RemoveAllSwap(const Predicate& pred, bool allow_shrinking = true)
  {
    //@maxidea: todo: 루프 밖에서 shrinking을 해주는게 좋을듯 싶은데...
  #if 0
    for (int32 i = 0; i < Count();) {
      if (pred((*this)[i])) {
        RemoveAtSwap(i, 1, allow_shrinking);
      }
      else {
        ++i;
      }
    }
  #else
    const int32 old_count = Count();
    for (int32 i = 0; i < Count();) {
      if (pred((*this)[i])) {
        // disable shrinking. we will shrink
        // outside of loop if some items are removed.
        RemoveAtSwap(i, 1, false);
      }
      else {
        ++i;
      }
    }

    if (allow_shrinking) {
      if (Count() < old_count) {
        // some items are removed, so do shrinking.
        Shrink();
      }
    }
  #endif
  }

  int32 RemoveSingleSwap(const ElementType& item, bool allow_shrinking = true)
  {
    const int32 index = Find(item);
    if (index == INVALID_INDEX) {
      return 0;
    }

    RemoveAtSwap(index, 1, allow_shrinking);

    return 1;
  }

  int32 RemoveSwap(const ElementType& item)
  {
    CheckAddress(&item);

    const int32 original_count = count_;
    for (int32 i = 0; i < count_; ++i) {
      if ((*this)[i] == item) {
        RemoveAtSwap(i--);
      }
    }
    return original_count - count_;
  }


  //
  // Swaps
  //

  void SwapMemory(int32 index1, int32 index2)
  {
    UnsafeMemory::Memswap(
        (uint8*)allocator_.GetAllocation() + (sizeof(ElementType) * index1),
        (uint8*)allocator_.GetAllocation() + (sizeof(ElementType) * index2),
        sizeof(ElementType));
  }

  void Swap(int32 index1, int32 index2)
  {
    fun_check((index1 >= 0) && (index2 >= 0));
    fun_check((count_ > index1) && (count_ > index2));
    if (index1 != index2) {
      SwapMemory(index1, index2);
    }
  }


  // Utilities for RClass

  //template <typename SearchType>
  //bool FindItemByClass(SearchType** item = nullptr, int32* ItemIndex = nullptr, int32 starting_index = 0) const
  //{
  //  UClass* SearchClass = SearchType::StaticClass();
  //  for (int32 index = starting_index; index < count_; ++index) {
  //    if ((*this)[index] && (*this)[index]->IsA(SearchClass)) {
  //      if (item) {
  //        *item = (SearchType*)((*this)[index]);
  //      }
  //      if (ItemIndex) {
  //        *ItemIndex = index;
  //      }
  //      return true;
  //    }
  //  }
  //  return false;
  //}

  // Iterators
  typedef IndexedContainerIterator<Array, ElementType, int32> Iterator;
  typedef IndexedContainerIterator<const Array, const ElementType, int32> ConstIterator;

  Iterator CreateIterator()
  {
    return Iterator(*this);
  }

  ConstIterator CreateConstIterator() const
  {
    return ConstIterator(*this);
  }

#if FUN_ARRAY_RANGED_FOR_CHECKS
  typedef CheckedPointerIterator<ElementType> RangedForIteratorType;
  typedef CheckedPointerIterator<const ElementType> RangedForConstIteratorType;
#else
  typedef ElementType* RangedForIteratorType;
  typedef const ElementType* RangedForConstIteratorType;
#endif

 private:
  // DO NOT USE DIRECTLY
  // STL-like iterators to enable range-based for loop support.
#if FUN_ARRAY_RANGED_FOR_CHECKS
  FUN_ALWAYS_INLINE friend RangedForIteratorType begin(Array& array) { return RangedForIteratorType(array.count_, array.MutableData()); }
  FUN_ALWAYS_INLINE friend RangedForConstIteratorType begin(const Array& array) { return RangedForConstIteratorType(array.count_, array.ConstData()); }
  FUN_ALWAYS_INLINE friend RangedForIteratorType end(Array& array) { return RangedForIteratorType(array.count_, array.MutableData() + array.Count()); }
  FUN_ALWAYS_INLINE friend RangedForConstIteratorType end(const Array& array) { return RangedForConstIteratorType(array.count_, array.ConstData() + array.Count()); }
#else
  FUN_ALWAYS_INLINE friend RangedForIteratorType begin(Array& array) { return array.MutableData(); }
  FUN_ALWAYS_INLINE friend RangedForConstIteratorType begin(const Array& array) { return array.ConstData(); }
  FUN_ALWAYS_INLINE friend RangedForIteratorType end(Array& array) { return array.MutableData() + array.Count(); }
  FUN_ALWAYS_INLINE friend RangedForConstIteratorType end(const Array& array) { return array.ConstData() + array.Count(); }
#endif

 public:
  // Sortings

  void Sort()
  {
    fun::Sort(MutableData(), Count());
  }

  template <typename Predicate>
  void Sort(const Predicate& pred)
  {
    fun::Sort(MutableData(), Count(), pred);
  }

  void StableSort()
  {
    fun::StableSort(MutableData(), Count());
  }

  template <typename Predicate>
  void StableSort(const Predicate& pred)
  {
    fun::StableSort(MutableData(), Count(), pred);
  }

  // Relies on MSVC-specific lazy template instantiation
  // to support arrays of incomplete types
#if defined(_MSC_VER) && !defined(__clang__)

private:
  FUN_NO_INLINE const ElementType& DebugGet(int32 index) const
  {
    return ConstData()[index];
  }
#endif

 private:
  FUN_NO_INLINE void ResizeGrow(int32 old_count)
  {
    capacity_ = allocator_.CalculateSlackGrow(count_, capacity_, sizeof(ElementType));
    allocator_.ResizeAllocation(old_count, capacity_, sizeof(ElementType));
  }

  FUN_NO_INLINE void ResizeShrink()
  {
    const int32 new_capacity = allocator_.CalculateSlackShrink(count_, capacity_, sizeof(ElementType));
    if (new_capacity != capacity_) {
      capacity_ = new_capacity;
      fun_check(capacity_ >= count_);
      allocator_.ResizeAllocation(count_, capacity_, sizeof(ElementType));
    }
  }

  FUN_NO_INLINE void ResizeTo(int32 new_capacity)
  {
    if (new_capacity) {
      new_capacity = allocator_.CalculateSlackReserve(new_capacity, sizeof(ElementType));
    }

    if (new_capacity != capacity_) {
      capacity_ = new_capacity;
      allocator_.ResizeAllocation(count_, capacity_, sizeof(ElementType));
    }
  }

  FUN_NO_INLINE void ResizeForCopy(int32 new_capacity, int32 prev_capacity)
  {
    if (new_capacity) {
      new_capacity = allocator_.CalculateSlackReserve(new_capacity, sizeof(ElementType));
    }

    if (new_capacity != prev_capacity) {
      allocator_.ResizeAllocation(0, new_capacity, sizeof(ElementType));
    }

    capacity_ = new_capacity;
  }

  template <typename OtherElementType>
  FUN_ALWAYS_INLINE void CopyToEmpty(
                          const OtherElementType* other_data,
                          int32 other_count,
                          int32 prev_capacity,
                          int32 extra_slack)
  {
    fun_check_dbg(extra_slack >= 0);

    count_ = other_count;

    if (other_count || extra_slack || prev_capacity) {
      ResizeForCopy(other_count + extra_slack, prev_capacity);

      ConstructItems<ElementType>(MutableData(), other_data, other_count);
    }
    else {
      capacity_ = 0;
    }
  }

 protected:
  typedef typename Conditional<
      Allocator::NeedsElementType,
      typename Allocator::template ForElementType<ElementType>,
      typename Allocator::ForAnyElementType
    >::Result ElementAllocatorType;

  ElementAllocatorType allocator_;
  int32 count_;
  int32 capacity_;

  //
  // Implicit heaps
  //

  //TODO algo로 대체해야함.

 public:
  template <typename Predicate>
  void Heapify(const Predicate& pred)
  {
    DereferenceWrapper<ElementType, Predicate> pred_wrapper(pred);
    for (int32 i = HeapGetParentIndex(Count() - 1); i >= 0; --i) {
      SiftDown(i, Count(), pred_wrapper);
    }

#if DEBUG_HEAP
    VerifyHeap(pred_wrapper);
#endif
  }

  void Heapify()
  {
    Heapify(Less<ElementType>());
  }

  template <typename Predicate>
  int32 HeapPush(const ElementType& item, const Predicate& pred)
  {
    // Add at the end, then sift up
    Add(item);
    DereferenceWrapper<ElementType, Predicate> pred_wrapper(pred);
    const int32 result = SiftUp(0, Count() - 1, pred_wrapper);

#if DEBUG_HEAP
    VerifyHeap(pred_wrapper);
#endif

    return result;
  }

  int32 HeapPush(const ElementType& item)
  {
    return HeapPush(item, Less<ElementType>());
  }

  template <typename Predicate>
  void HeapPop(ElementType& out_item, const Predicate& pred, bool allow_shrinking = true)
  {
    out_item = (*this)[0];
    RemoveAtSwap(0, 1, allow_shrinking);

    DereferenceWrapper<ElementType, Predicate> pred_wrapper(pred);
    SiftDown(0, Count(), pred_wrapper);

#if DEBUG_HEAP
    VerifyHeap(pred_wrapper);
#endif
  }

  void HeapPop(ElementType& out_item, bool allow_shrinking = true)
  {
    HeapPop(out_item, Less<ElementType>(), allow_shrinking);
  }

  template <typename Predicate>
  void VerifyHeap(const Predicate& pred)
  {
    // Verify Predicate
    ElementType* heap = MutableData();
    for (int32 i = 1; i < Count(); ++i) {
      const int32 parent_index = HeapGetParentIndex(i);
      if (pred(heap[i], heap[parent_index])) {
        fun_check(false);
      }
    }
  }

  template <typename Predicate>
  void HeapPopDiscard(const Predicate& pred, bool allow_shrinking = true)
  {
    RemoveAtSwap(0, 1, allow_shrinking);
    DereferenceWrapper<ElementType, Predicate> pred_wrapper(pred);
    SiftDown(0, Count(), pred_wrapper);

#if DEBUG_HEAP
    VerifyHeap(pred_wrapper);
#endif
  }

  void HeapPopDiscard(bool allow_shrinking = true)
  {
    HeapPopDiscard(Less<ElementType>(), allow_shrinking);
  }

  const ElementType& HeapTop() const
  {
    return (*this)[0];
  }

  ElementType& HeapTop()
  {
    return (*this)[0];
  }

  template <typename Predicate>
  void HeapRemoveAt(int32 index, const Predicate& pred, bool allow_shrinking = true)
  {
    RemoveAtSwap(index, 1, allow_shrinking);

    DereferenceWrapper<ElementType, Predicate> pred_wrapper(pred);
    SiftDown(index, Count(), pred_wrapper);
    SiftUp(0, MathBase::Min(index, Count() - 1), pred_wrapper);

#if DEBUG_HEAP
    VerifyHeap(pred_wrapper);
#endif
  }

  void HeapRemoveAt(int32 index, bool allow_shrinking = true)
  {
    HeapRemoveAt(index, Less<ElementType>(), allow_shrinking);
  }

  template <typename Predicate>
  void HeapSort(const Predicate& pred)
  {
    ReversePredicateWrapper<ElementType, Predicate> reversed_pred_wrapper(pred);
    Heapify(reversed_pred_wrapper);

    ElementType* heap = MutableData();
    for (int32 i = Count() - 1; i > 0; --i) {
      Swap(heap[0], heap[i]);
      SiftDown(0, i, reversed_pred_wrapper);
    }

#if DEBUG_HEAP
    DereferenceWrapper<ElementType, Predicate> pred_wrapper(pred);

    // Verify heap Property
    VerifyHeap(pred_wrapper);

    // Also verify array is properly sorted
    for (int32 i = 1; i < Count(); ++i) {
      if (pred_wrapper(heap[i], heap[i - 1])) {
        fun_check(false);
      }
    }
#endif
  }

  void HeapSort()
  {
    HeapSort(Less<ElementType>());
  }

 private:
  FUN_ALWAYS_INLINE int32 HeapGetLeftChildIndex(int32 index) const
  {
    return index * 2 + 1;
  }

  FUN_ALWAYS_INLINE bool HeapIsLeaf(int32 index, int32 Count) const
  {
    return HeapGetLeftChildIndex(index) >= Count;
  }

  FUN_ALWAYS_INLINE int32 HeapGetParentIndex(int32 index) const
  {
    return (index - 1) / 2;
  }

 private:
  template <typename Predicate>
  FUN_ALWAYS_INLINE void SiftDown(int32 index, const int32 Count, const Predicate& pred)
  {
    ElementType* heap = MutableData();
    while (!HeapIsLeaf(index, Count)) {
      const int32 left_child_index = HeapGetLeftChildIndex(index);
      const int32 right_child_index = left_child_index + 1;

      int32 min_child_index = left_child_index;
      if (right_child_index < Count) {
        min_child_index = pred(heap[left_child_index], heap[right_child_index]) ? left_child_index : right_child_index;
      }

      if (!pred(heap[min_child_index], heap[index])) {
        break;
      }

      Swap(heap[index], heap[min_child_index]);
      index = min_child_index;
    }
  }

  template <typename Predicate>
  FUN_ALWAYS_INLINE int32 SiftUp(int32 root_index, int32 node_index, const Predicate& pred)
  {
    ElementType* heap = MutableData();
    while (node_index > root_index) {
      const int32 parent_index = HeapGetParentIndex(node_index);
      if (!pred(heap[node_index], heap[parent_index])) {
        break;
      }

      Swap(heap[node_index], heap[parent_index]);
      node_index = parent_index;
    }

    return node_index;
  }
};

template <typename ElementType, typename Allocator>
struct IsZeroConstructible<Array<ElementType, Allocator>> {
  enum { Value = AllocatorTraits<Allocator>::IsZeroConstruct };
};

template <typename ElementType, typename Allocator>
struct ContainerTraits<Array<ElementType, Allocator> >
  : public ContainerTraitsBase<Array<ElementType, Allocator>> {
  enum { MoveWillEmptyContainer = AllocatorTraits<Allocator>::SupportsMove };
};

//TODO IsTArray를 IsArray로 대체한다면 모호해지려나??
template <typename T, typename Allocator>
struct IsContiguousContainer<Array<T, Allocator>> {
  enum { Value = true };
};

/**
 * Traits class which determines whether or not a type is a Array.
 */
template <typename ElementType> struct IsTArray { enum { Value = false }; };

template <typename ElementType, typename Allocator> struct IsTArray<Array<ElementType, Allocator>> { enum { Value = true }; };
template <typename ElementType, typename Allocator> struct IsTArray<const Array<ElementType, Allocator>> { enum { Value = true }; };
template <typename ElementType, typename Allocator> struct IsTArray<volatile Array<ElementType, Allocator>> { enum { Value = true }; };
template <typename ElementType, typename Allocator> struct IsTArray<const volatile Array<ElementType, Allocator>> { enum { Value = true }; };

} // namespace fun


//
// array operator news.
//

template <typename ElementType, typename Allocator>
void* operator new(size_t size, fun::Array<ElementType, Allocator>& array)
{
  using namespace fun;
  fun_check(size == sizeof(ElementType));
  const int32 index = array.AddUninitialized(1);
  return &array[index];
}

template <typename ElementType, typename Allocator>
void* operator new(size_t size, fun::Array<ElementType, Allocator>& array, fun::int32 index)
{
  using namespace fun;
  fun_check(size == sizeof(ElementType));
  array.InsertUninitialized(index, 1);
  return &array[index];
}


namespace fun {

//
// MRU array.
//

/**
 * Same as Array except:
 * - Has an upper limit of the number of items it will store.
 * - Any item that is added to the array is moved to the top.
 */
template <typename ElementType, typename Allocator = DefaultAllocator>
class MruArray : public Array<ElementType, Allocator>
{
 public:
  typedef Array<ElementType, Allocator> BaseType;

  /**
   * The maximum number of items we can store in this array.
   */
  int32 MaxItems;

  MruArray()
    : BaseType()
  {
    MaxItems = 0;
  }

  MruArray(MruArray&&) = default;
  MruArray(const MruArray&) = default;
  MruArray& operator = (MruArray&&) = default;
  MruArray& operator = (const MruArray&) = default;

  int32 Add(const ElementType& item)
  {
    const int32 index = BaseType::Add(item);
    this->Swap(index, 0);
    CullArray();
    return 0;
  }

  int32 AddZeroed(int32 count = 1)
  {
    const int32 index = BaseType::AddZeroed(count);
    this->Swap(index, 0);
    CullArray();
    return 0;
  }

  int32 AddUnique(const ElementType& item)
  {
    // Remove any existing copies of the item.
    this->Remove(item);

    this->InsertUninitialized(0);
    (*this)[0] = item;

    CullArray();

    return 0;
  }

  void CullArray()
  {
    // 0 = no limit
    if (MaxItems <= 0) {
      return;
    }

    while (this->Count() > MaxItems) {
      this->RemoveAt(this->Count() - 1, 1);
    }
  }
};

template <typename ElementType, typename Allocator>
struct ContainerTraits<MruArray<ElementType, Allocator>>
  : public ContainerTraitsBase<MruArray<ElementType, Allocator>> {
  enum { MoveWillEmptyContainer = ContainerTraitsBase<typename MruArray<ElementType, Allocator>::BaseType>::MoveWillEmptyContainer };
};



/**
 * Indirect array.
 * Same as a Array above, but stores pointers to the elements, to allow
 * resizing the array index without relocating the actual elements.
 */
template <typename _ElementType, typename Allocator = DefaultAllocator>
class IndirectArray
{
 public:
  using ElementType = _ElementType;
  using InternalArrayType = Array<void*, Allocator>;

  IndirectArray() = default;
  IndirectArray(IndirectArray&&) = default;
  IndirectArray& operator = (IndirectArray&&) = default;

  IndirectArray(const IndirectArray& other)
  {
    for (auto& item : other) {
      Add(new ElementType(item));
    }
  }

  IndirectArray& operator = (const IndirectArray& other)
  {
    if (FUN_LIKELY(&other != this)) {
      Clear(other.Count());

      for (auto& item : other) {
        Add(new ElementType(item));
      }
    }

    return *this;
  }

  ~IndirectArray()
  {
    Clear();
  }

  FUN_ALWAYS_INLINE int32 Count() const
  {
    return array_.Count();
  }

  FUN_ALWAYS_INLINE ElementType** MutableData()
  {
    return (ElementType**)array_.MutableData();
  }

  FUN_ALWAYS_INLINE const ElementType** ConstData() const
  {
    return (const ElementType**)array_.ConstData();
  }

  uint32 GetTypeSize() const
  {
    return sizeof(ElementType*);
  }

  FUN_ALWAYS_INLINE ElementType& operator[](int32 index)
  {
    return *(ElementType*)array_[index];
  }

  FUN_ALWAYS_INLINE const ElementType& operator[](int32 index) const
  {
    return *(ElementType*)array_[index];
  }

  FUN_ALWAYS_INLINE ElementType& Last(int32 index_from_the_end = 0)
  {
    return *(ElementType*)array_.Last(index_from_the_end);
  }

  FUN_ALWAYS_INLINE const ElementType& Last(int32 index_from_the_end = 0) const
  {
    return *(ElementType*)array_.Last(index_from_the_end);
  }

  FUN_ALWAYS_INLINE void Shrink()
  {
    array_.Shrink();
  }

  FUN_ALWAYS_INLINE void Reset(int32 new_capacity = 0)
  {
    DestructAndFreeItems();
    array_.Reset(new_capacity);
  }

  void Serialize(Archive& ar, class FObject* owner)
  {
    CountBytes(ar);

    if (ar.IsLoading()) {
      // Load array.
      int32 count;
      ar & count;
      Clear(count);
      for (int32 index = 0; index < count; ++index) {
        new(*this) ElementType;
      }
      for (int32 index = 0; index < count; ++index) {
        (*this)[index].Serialize(ar, owner, index);
      }
    }
    else {
      // Save array.
      int32 count = array_.Count();
      ar & count;
      for (int32 index = 0; index < count; ++index) {
        (*this)[index].Serialize(ar, owner, index);
      }
    }
  }

  friend Archive& operator & (Archive& ar, IndirectArray& array)
  {
    array.CountBytes(ar);

    if (ar.IsLoading()) {
      int32 count;
      ar & count;
      array.Clear(count);
      for (int32 i = 0; i < count; ++i) {
        ar & *new(array) ElementType;
      }
    }
    else {
      int32 count = array.Count();
      ar & count;
      for (int32 i = 0; i < count; ++i) {
        ar & array[i];
      }
    }
    return ar;
  }

  void CountBytes(Archive& ar)
  {
    array_.CountBytes(ar);
  }

  void RemoveAt(int32 index, int32 count = 1, bool allow_shrinking = true)
  {
    fun_check(index >= 0);
    fun_check(index <= array_.Count());
    fun_check(index + count <= array_.Count());
    ElementType** element = MutableData() + index;
    for (int32 element_id = count; element_id; --element_id) {
      reinterpret_cast<ElementType*>(*element)->~ElementType();
      UnsafeMemory::Free(*element);
      ++element;
    }
    array_.RemoveAt(index, count, allow_shrinking);
  }

  void RemoveAtSwap(int32 index, int32 count = 1, bool allow_shrinking = true)
  {
    fun_check(index >= 0);
    fun_check(index <= array_.Count());
    fun_check(index + count <= array_.Count());
    ElementType** element = MutableData() + index;
    for (int32 element_id = count; element_id; --element_id) {
      reinterpret_cast<ElementType*>(*element)->~ElementType();
      UnsafeMemory::Free(*element);
      ++element;
    }
    array_.RemoveAtSwap(index, count, allow_shrinking);
  }

  void Swap(int32 index1, int32 index2)
  {
    array_.Swap(index1, index2);
  }

  void Clear(int32 slack = 0)
  {
    DestructAndFreeItems();
    array_.Clear(slack);
  }

  FUN_ALWAYS_INLINE int32 Add(ElementType* item)
  {
    return array_.Add(item);
  }

  FUN_ALWAYS_INLINE void Insert(ElementType* item, int32 index)
  {
    array_.Insert(item, index);
  }

  FUN_ALWAYS_INLINE void Reserve(int32 desired_capacity)
  {
    array_.Reserve(desired_capacity);
  }

  FUN_ALWAYS_INLINE bool IsValidIndex(int32 index) const
  {
    return array_.IsValidIndex(index);
  }

  uint32 GetAllocatedSize() const
  {
    return array_.Capacity() * sizeof(ElementType*) + array_.Count() * sizeof(ElementType);
  }

  // Iterators
  using Iterator = IndexedContainerIterator<IndirectArray, ElementType, int32>;
  using ConstIterator = IndexedContainerIterator<const IndirectArray, const ElementType, int32>;

  Iterator CreateIterator()
  {
    return Iterator(*this);
  }

  ConstIterator CreateConstIterator() const
  {
    return ConstIterator(*this);
  }

 private:
  void DestructAndFreeItems()
  {
    ElementType** element = MutableData();
    for (int32 i = array_.Count(); i; --i) {
      reinterpret_cast<ElementType*>(*element)->~ElementType();
      UnsafeMemory::Free(*element);
      ++element;
    }
  }

  // DO NOT USE DIRECTLY
  // STL-like iterators to enable range-based for loop support.
  FUN_ALWAYS_INLINE friend DereferencingIterator<ElementType, typename InternalArrayType::RangedForIteratorType> begin(IndirectArray& array) { return DereferencingIterator<ElementType, typename InternalArrayType::RangedForIteratorType>(begin(array.array_)); }
  FUN_ALWAYS_INLINE friend DereferencingIterator<const ElementType, typename InternalArrayType::RangedForConstIteratorType> begin(const IndirectArray& array) { return DereferencingIterator<const ElementType, typename InternalArrayType::RangedForConstIteratorType>(begin(array.array_)); }
  FUN_ALWAYS_INLINE friend DereferencingIterator<ElementType, typename InternalArrayType::RangedForIteratorType> end(IndirectArray& array) { return DereferencingIterator<ElementType, typename InternalArrayType::RangedForIteratorType>(end(array.array_)); }
  FUN_ALWAYS_INLINE friend DereferencingIterator<const ElementType, typename InternalArrayType::RangedForConstIteratorType> end(const IndirectArray& array) { return DereferencingIterator<const ElementType, typename InternalArrayType::RangedForConstIteratorType>(end(array.array_)); }

  InternalArrayType array_;
};

template <typename ElementType, typename Allocator>
struct ContainerTraits<IndirectArray<ElementType, Allocator>>
  : public ContainerTraitsBase<IndirectArray<ElementType, Allocator>> {
  enum { MoveWillEmptyContainer = ContainerTraitsBase<typename IndirectArray<ElementType, Allocator>::InternalArrayType>::MoveWillEmptyContainer };
};

} // namespace fun


template <typename ElementType, typename Allocator>
void* operator new(size_t size, fun::IndirectArray<ElementType, Allocator>& array)
{
  using namespace fun;
  fun_check(size == sizeof(ElementType));
  const fun::int32 index = array.Add((ElementType*)UnsafeMemory::Malloc(size));
  return &array[index];
}

template <typename ElementType, typename Allocator>
void* operator new(size_t size, fun::IndirectArray<ElementType, Allocator>& array, fun::int32 index)
{
  using namespace fun;
  fun_check(size == sizeof(ElementType));
  array.Insert((ElementType*)UnsafeMemory::Malloc(size), index);
  return &array[index];
}


namespace fun {

////
//// Transactional array.
////
//
///**
// * NOTE: Right now, you can't use a custom allocation policy with transactional arrays. If
// * you need to do it, you will have to fix up FTransaction::CObjectRecord to use the correct Array<Allocator>.
// */
//template <typename ElementType>
//class TransArray : public Array<ElementType>
//{
// public:
//  typedef Array<ElementType> BaseType;
//
//  // Constructors.
//  explicit TransArray(FObject* owner)
//    : owner_(owner)
//  {
//    fun_check_ptr(owner_);
//  }
//
//  TransArray(FObject* owner, const BaseType& other)
//    : BaseType(other)
//    , owner_(owner)
//  {
//    fun_check_ptr(owner_);
//  }
//
//  TransArray(TransArray&&) = default;
//  TransArray(const TransArray&) = default;
//  TransArray& operator = (TransArray&&) = default;
//  TransArray& operator = (const TransArray&) = default;
//
//  // Add, Insert, Remove, Clear interface.
//  int32 AddUninitialized(int32 count = 1)
//  {
//    const int32 index = BaseType::AddUninitialized(count);
//
//    if (g_undo) {
//      g_undo->SaveArray(owner_, (UntypedArray*)this, index, count, 1, sizeof(ElementType), DefaultConstructItem, SerializeItem, DestructItem);
//    }
//    return index;
//  }
//
//  void InsertUninitialized(int32 index, int32 count = 1)
//  {
//    BaseType::InsertUninitialized(index, count);
//
//    if (g_undo) {
//      g_undo->SaveArray(owner_, (UntypedArray*)this, index, count, 1, sizeof(ElementType), DefaultConstructItem, SerializeItem, DestructItem);
//    }
//  }
//
//  void RemoveAt(int32 index, int32 count = 1)
//  {
//    if (g_undo) {
//      g_undo->SaveArray(owner_, (UntypedArray*)this, index, count, -1, sizeof(ElementType), DefaultConstructItem, SerializeItem, DestructItem);
//    }
//
//    BaseType::RemoveAt(index, count);
//  }
//
//  void Clear(int32 slack = 0)
//  {
//    if (g_undo) {
//      g_undo->SaveArray(owner_, (UntypedArray*)this, 0, this->count_, -1, sizeof(ElementType), DefaultConstructItem, SerializeItem, DestructItem);
//    }
//
//    BaseType::Clear(slack);
//  }
//
//  // Functions dependent on Add, Remove.
//  void AssignButKeepOwner(const BaseType& other)
//  {
//    (BaseType&)*this = other;
//  }
//
//  int32 Add(const ElementType& item)
//  {
//    new(*this) ElementType(item);
//    return this->Count() - 1;
//  }
//
//  int32 AddZeroed(int32 count = 1)
//  {
//    const int32 index = AddUninitialized(count);
//
//    UnsafeMemory::Memzero(this->MutableData() + index, count*sizeof(ElementType));
//    return index;
//  }
//
//  int32 AddUnique(const ElementType& item)
//  {
//    for (int32 index = 0; index < this->count_; ++index) {
//      if ((*this)[index] == item) {
//        return index;
//      }
//    }
//    return Add(item);
//  }
//
//  int32 Remove(const ElementType& item)
//  {
//    this->CheckAddress(&item);
//
//    const int32 original_count = this->count_;
//    for (int32 index = 0; index < this->count_; ++index) {
//      if ((*this)[index] == item) {
//        RemoveAt(index--);
//      }
//    }
//    return original_count - this->count_;
//  }
//
//  // TransArray interface.
//  FObject* GetOwner() const
//  {
//    return owner_;
//  }
//
//  void SetOwner(FObject* owner)
//  {
//    owner_ = owner;
//  }
//
//  void ModifyItem(int32 index)
//  {
//    if (g_undo) {
//      g_undo->SaveArray(owner_, (UntypedArray*)this, index, 1, 0, sizeof(ElementType), DefaultConstructItem, SerializeItem, DestructItem);
//    }
//  }
//
//  void ModifyAllItems()
//  {
//    if (g_undo) {
//      g_undo->SaveArray(owner_, (UntypedArray*)this, 0, this->Count(), 0, sizeof(ElementType), DefaultConstructItem, SerializeItem, DestructItem);
//    }
//  }
//
//  friend Archive& operator & (Archive& ar, TransArray& v)
//  {
//    ar & v.owner_;
//    ar & (BaseType&)v;
//    return ar;
//  }
//
// protected:
//  static void DefaultConstructItem(void* ptr)
//  {
//    new (ptr) ElementType;
//  }
//
//  static void SerializeItem(Archive& ar, void* ptr)
//  {
//    ar & *(ElementType*)ptr;
//  }
//
//  static void DestructItem(void* ptr)
//  {
//    ((ElementType*)ptr)->~ElementType();
//  }
//
//  FObject* owner_;
//};
//
//template <typename ElementType>
//struct ContainerTraits<TransArray<ElementType>> : public ContainerTraitsBase<TransArray<ElementType>>
//{
//  enum { MoveWillEmptyContainer = ContainerTraitsBase<typename TransArray<ElementType>::BaseType>::MoveWillEmptyContainer };
//};
//
//template <typename ElementType> void* operator new(size_t size, TransArray<ElementType>& array)
//{
//  fun_check(size == sizeof(ElementType));
//  const int32 index = array.AddUninitialized();
//  return &array[index];
//}
//
//template <typename ElementType> void* operator new(size_t size, TransArray<ElementType>& array, int32 index)
//{
//  fun_check(size == sizeof(ElementType));
//  array.Insert(index);
//  return &array[index];
//}

} // namespace fun
