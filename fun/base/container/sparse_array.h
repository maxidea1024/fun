#pragma once

#include "fun/base/base.h"
#include "fun/base/container/array.h"
#include "fun/base/container/bit_array.h"
#include "fun/base/ftl/functional.h"
#include "fun/base/ftl/memory_ops.h"
#include "fun/base/serialization/archive.h"

namespace fun {

// Forward declarations.
template <typename _ElementType,
          typename Allocator = DefaultSparseArrayAllocator>
class SparseArray;

/**
 * The result of a sparse array allocation.
 */
struct SparseArrayAllocationInfo {
  int32 index;
  void* pointer;
};

/**
 * Allocated elements are overlapped with free element info in the element list.
 */
template <typename ElementType>
union SparseArrayElementOrFreeListLink {
  /** If the element is allocated, its value is stored here. */
  ElementType element_data;

  struct {
    /**
     * If the element isn't allocated, this is a link to
     * the previous element in the array's free list.
     */
    int32 prev_free_index;

    /**
     * If the element isn't allocated, this is a link to
     * the next element in the array's free list.
     */
    int32 next_free_index;
  };
};

class UntypedSparseArray;

/**
 * A dynamically sized array where element indices aren't
 * necessarily contiguous.
 *
 * Memory is allocated for all elements in the array's index range,
 * so it doesn't save memory; but it does allow O(1) element removal that
 * doesn't invalidate the indices of subsequent elements.
 *
 * it uses Array to store the elements, and a BitArray to store
 * whether each element index is allocated
 * (for fast iteration over allocated elements).
 */
template <typename _ElementType,
          typename Allocator /*= DefaultSparseArrayAllocator*/>
class SparseArray {
  friend struct ContainerTraits<SparseArray>;
  friend class UntypedSparseArray;

 public:
  using ElementType = _ElementType;

  /**
   * Destructor.
   */
  ~SparseArray() {
    // Destruct the elements in the array.
    Clear();
  }

  /**
   * Marks an index as allocated, and returns information about the allocation.
   */
  SparseArrayAllocationInfo AllocateIndex(int32 index) {
    fun_check(index >= 0);
    fun_check(index < GetMaxIndex());
    fun_check(!allocation_flags_[index]);

    // Flag the element as allocated.
    allocation_flags_[index] = true;

    // Set the allocation info.
    SparseArrayAllocationInfo result;
    result.index = index;
    result.pointer = &MutableData(result.index).element_data;
    return result;
  }

  /**
   * Allocates space for an element in the array.
   * The element is not initialized, and you must use
   * the corresponding placement new operator
   * to construct the element in the allocated memory.
   */
  SparseArrayAllocationInfo AddUninitialized() {
    int32 index;
    if (free_index_count_ > 0) {
      // Remove and use the first index from the list of free elements.
      index = first_free_index_;
      first_free_index_ = MutableData(first_free_index_).next_free_index;
      --free_index_count_;
      if (free_index_count_ > 0) {
        MutableData(first_free_index_).prev_free_index = -1;
      }
    } else {
      // Add a new element.
      index = data_.AddUninitialized(1);
      allocation_flags_.Add(false);
    }

    return AllocateIndex(index);
  }

  /**
   * Adds an element to the array.
   */
  int32 Add(typename TypeTraits<ElementType>::ConstInitType element) {
    SparseArrayAllocationInfo allocation = AddUninitialized();
    new (allocation) ElementType(element);
    return allocation.index;
  }

  /**
   * Allocates space for an element in the array at a given index.
   * The element is not initialized, and you must use the corresponding
   * placement new operator to construct the element in the allocated memory.
   */
  SparseArrayAllocationInfo InsertUninitialized(int32 index) {
    // Enlarge the array to include the given index.
    if (index >= data_.Count()) {
      data_.AddUninitialized(index + 1 - data_.Count());
      while (allocation_flags_.Count() < data_.Count()) {
        const int32 free_index = allocation_flags_.Count();
        MutableData(free_index).prev_free_index = -1;
        MutableData(free_index).next_free_index = first_free_index_;
        if (free_index_count_ > 0) {
          MutableData(first_free_index_).prev_free_index = free_index;
        }
        first_free_index_ = free_index;
        fun_verify(allocation_flags_.Add(false) == free_index);
        ++free_index_count_;
      };
    }

    // Verify that the specified index is free.
    fun_check(!allocation_flags_[index]);

    // Remove the index from the list of free elements.
    --free_index_count_;
    const int32 prev_free_index = ConstData(index).prev_free_index;
    const int32 next_free_index = ConstData(index).next_free_index;
    if (prev_free_index != -1) {
      MutableData(prev_free_index).next_free_index = next_free_index;
    } else {
      first_free_index_ = next_free_index;
    }
    if (next_free_index != -1) {
      MutableData(next_free_index).prev_free_index = prev_free_index;
    }

    return AllocateIndex(index);
  }

  /**
   * Inserts an element to the array.
   */
  void Insert(int32 index,
              typename TypeTraits<ElementType>::ConstInitType element) {
    new (InsertUninitialized(index)) ElementType(element);
  }

  /**
   * Removes Count elements from the array, starting from index.
   */
  void RemoveAt(int32 index, int32 count = 1) {
    if (!IsTriviallyDestructible<ElementType>::Value) {
      for (int32 it = index, it_count = count; it_count; ++it, --it_count) {
        ((ElementType&)MutableData(it).element_data).~ElementType();
      }
    }

    RemoveAtUninitialized(index, count);
  }

  /**
   * Removes count elements from the array, starting from index, without
   * destructing them.
   */
  void RemoveAtUninitialized(int32 index, int32 count = 1) {
    for (; count > 0; --count) {
      fun_check(allocation_flags_[index]);

      // Mark the element as free and add it to the free element list.
      if (free_index_count_ > 0) {
        MutableData(first_free_index_).prev_free_index = index;
      }

      auto& index_data = MutableData(index);
      index_data.prev_free_index = -1;
      index_data.next_free_index =
          free_index_count_ > 0 ? first_free_index_ : INVALID_INDEX;
      first_free_index_ = index;
      ++free_index_count_;
      allocation_flags_[index] = false;

      ++index;
    }
  }

  /**
   * Removes all elements from the array, potentially leaving space
   * allocated for an expected number of elements about to be added.
   *
   * \param count - The expected number of elements about to be added.
   */
  void Clear(int32 count = 0) {
    // Destruct the allocated elements.
    if (!IsTriviallyDestructible<ElementType>::Value) {
      for (Iterator it(*this); it; ++it) {
        ElementType& element = *it;
        element.~ElementType();
      }
    }

    // Free the allocated elements.
    data_.Clear(count);
    first_free_index_ = -1;
    free_index_count_ = 0;
    allocation_flags_.Clear(count);
  }

  /**
   * Clear the array, but keep its allocated memory as slack.
   */
  void Reset() {
    // Destruct the allocated elements.
    if (!IsTriviallyDestructible<ElementType>::Value) {
      for (Iterator it(*this); it; ++it) {
        ElementType& element = *it;
        element.~ElementType();
      }
    }

    // Free the allocated elements.
    data_.Reset();
    first_free_index_ = -1;
    free_index_count_ = 0;
    allocation_flags_.Reset();
  }

  /**
   * Preallocates enough memory to contain the specified number of elements.
   *
   * \param count - the total number of elements that the array will have
   */
  void Reserve(int32 count) {
    if (count > data_.Count()) {
      const int32 elements_to_add = count - data_.Count();

      // allocate memory in the array itself
      const int32 element_index = data_.AddUninitialized(elements_to_add);

      // now mark the new elements as free
      for (int32 free_index = element_index; free_index < count; ++free_index) {
        if (free_index_count_ > 0) {
          MutableData(first_free_index_).prev_free_index = free_index;
        }
        MutableData(free_index).prev_free_index = -1;
        MutableData(free_index).next_free_index =
            free_index_count_ > 0 ? first_free_index_ : INVALID_INDEX;
        first_free_index_ = free_index;
        ++free_index_count_;
      }

      //@fixme - this will have to do until BitArray has a Reserve method....
      for (int32 i = 0; i < elements_to_add; ++i) {
        allocation_flags_.Add(false);
      }
    }
  }

  /**
   * Shrinks the array's storage to avoid slack.
   */
  void Shrink() {
    // Determine the highest allocated index in the data array.
    int32 max_allocated_index = INVALID_INDEX;
    for (ConstSetBitIterator<typename Allocator::BitArrayAllocator>
             allocated_index_it(allocation_flags_);
         allocated_index_it; ++allocated_index_it) {
      max_allocated_index =
          MathBase::Max(max_allocated_index, allocated_index_it.GetIndex());
    }

    const int32 first_index_to_remove = max_allocated_index + 1;
    if (first_index_to_remove < data_.Count()) {
      if (free_index_count_ > 0) {
        // Look for elements in the free list that are in the memory to be
        // freed.
        int32 free_index = first_free_index_;
        while (free_index != INVALID_INDEX) {
          if (free_index >= first_index_to_remove) {
            const int32 prev_free_index = ConstData(free_index).prev_free_index;
            const int32 next_free_index = ConstData(free_index).next_free_index;

            if (next_free_index != INVALID_INDEX) {
              MutableData(next_free_index).prev_free_index = prev_free_index;
            }

            if (prev_free_index != -INVALID_INDEX) {
              MutableData(prev_free_index).next_free_index = next_free_index;
            } else {
              first_free_index_ = next_free_index;
            }
            --free_index_count_;

            free_index = next_free_index;
          } else {
            free_index = MutableData(free_index).next_free_index;
          }
        }
      }

      // Truncate unallocated elements at the end of the data array.
      data_.RemoveAt(first_index_to_remove,
                     data_.Count() - first_index_to_remove);
      allocation_flags_.RemoveAt(
          first_index_to_remove,
          allocation_flags_.Count() - first_index_to_remove);
    }

    // Shrink the data array.
    data_.Shrink();
  }

  /**
   * Compacts the allocated elements into a contiguous index range.
   * Returns true if any elements were relocated, false otherwise.
   */
  bool Compact() {
    int32 free_count = free_index_count_;
    if (free_count == 0) {
      return false;
    }

    bool result = false;

    ElementOrFreeListLink* element_data = data_.MutableData();

    int32 end_index = data_.Count();
    const int32 target_index = end_index - free_count;
    int32 free_index = first_free_index_;
    while (free_index != INVALID_INDEX) {
      const int32 next_free_index = ConstData(free_index).next_free_index;
      if (free_index < target_index) {
        // We need an element here
        do {
          --end_index;
        } while (!allocation_flags_[end_index]);

        RelocateConstructItems<ElementOrFreeListLink>(
            element_data + free_index, element_data + end_index, 1);
        allocation_flags_[free_index] = true;

        result = true;
      }

      free_index = next_free_index;
    }

    data_.RemoveAt(target_index, free_count);
    allocation_flags_.RemoveAt(target_index, free_count);

    free_index_count_ = 0;
    first_free_index_ = INVALID_INDEX;

    return result;
  }

  /**
   * Compacts the allocated elements into a contiguous index range.
   * Does not change the iteration order of the elements.
   *
   * Returns true if any elements were relocated, false otherwise.
   */
  bool CompactStable() {
    if (free_index_count_ == 0) {
      return false;
    }

    // Copy the existing elements to a new array.
    SparseArray<ElementType, Allocator> compacted_array;
    compacted_array.Clear(Count());
    for (ConstIterator it(*this); it; ++it) {
      new (compacted_array.AddUninitialized()) ElementType(*it);
    }

    // Replace this array with the compacted array.
    Swap(*this, compacted_array);

    return true;
  }

  /**
   * Sorts the elements using the provided comparison class.
   */
  template <typename Predicate>
  void Sort(const Predicate& pred) {
    if (Count() > 0) {
      // Compact the elements array so all the elements are contiguous.
      Compact();

      // Sort the elements according to the provided comparison class.
      fun::Sort(&MutableData(0), Count(), ElementCompareClass<Predicate>(pred));
    }
  }

  /**
   * Sorts the elements assuming < operator is defined for ElementType.
   */
  void Sort() { Sort(Less<ElementType>()); }

  /**
   * Helper function to return the amount of memory allocated by this container
   *
   * \return number of bytes allocated by this container
   */
  uint32 GetAllocatedSize() const {
    return (data_.Count() + data_.GetSlack()) * sizeof(ElementOrFreeListLink) +
           allocation_flags_.GetAllocatedSize();
  }

  /**
   * Tracks the container's memory use through an archive.
   */
  void CountBytes(Archive& ar) {
    data_.CountBytes(ar);

    allocation_flags_.CountBytes(ar);
  }

  /**
   * Serializer.
   */
  friend Archive& operator&(Archive& ar, SparseArray& array) {
    array.CountBytes(ar);

    if (ar.IsLoading()) {
      // Load array.
      int32 count = 0;
      ar& count;
      array.Clear(count);
      for (int32 element_index = 0; element_index < count; ++element_index) {
        ar&* ::new (array.AddUninitialized()) ElementType;
      }
    } else {
      // Save array.
      int32 count = array.Count();
      ar& count;
      for (Iterator it(array); it; ++it) {
        ar&* it;
      }
    }
    return ar;
  }

  /**
   * Equality comparison operator.
   *
   * Checks that both arrays have the same elements and element indices;
   * that means that unallocated elements are signifigant!
   */
  friend bool operator==(const SparseArray& x, const SparseArray& y) {
    if (x.GetMaxIndex() != y.GetMaxIndex()) {
      return false;
    }

    for (int32 element_index = 0; element_index < x.GetMaxIndex();
         ++element_index) {
      const bool is_alloacated_x = x.IsAllocated(element_index);
      const bool is_alloacated_y = y.IsAllocated(element_index);

      if (is_alloacated_x != is_alloacated_y) {
        return false;
      } else if (is_alloacated_x) {
        if (!(x[element_index] == y[element_index])) {
          return false;
        }
      }
    }

    return true;
  }

  /**
   * Inequality comparison operator.
   *
   * Checks that both arrays have the same elements and element indices;
   * that means that unallocated elements are signifigant!
   */
  friend bool operator!=(const SparseArray& x, const SparseArray& y) {
    return !(x == y);
  }

  /**
   * Default constructor.
   */
  SparseArray() : first_free_index_(INVALID_INDEX), free_index_count_(0) {}

  /**
   * Move constructor.
   */
  SparseArray(SparseArray&& other) { MoveOrCopy(*this, other); }

  /**
   * Copy constructor.
   */
  SparseArray(const SparseArray& other)
      : first_free_index_(INVALID_INDEX), free_index_count_(0) {
    *this = other;
  }

  /**
   * Move assignment operator.
   */
  SparseArray& operator=(SparseArray&& other) {
    if (FUN_LIKELY(&other != this)) {
      MoveOrCopy(*this, other);
    }
    return *this;
  }

  /**
   * Copy assignment operator.
   */
  SparseArray& operator=(const SparseArray& other) {
    if (FUN_LIKELY(&other != this)) {
      // Reallocate the array.
      Clear(other.GetMaxIndex());
      data_.AddUninitialized(other.GetMaxIndex());

      // Copy the other array's element allocation state.
      first_free_index_ = other.first_free_index_;
      free_index_count_ = other.free_index_count_;
      allocation_flags_ = other.allocation_flags_;

      // Determine whether we need per element construction or bulk copy is fine
      if (TypeTraits<ElementType>::NeedsCopyConstructor) {
        ElementOrFreeListLink* src_data =
            (ElementOrFreeListLink*)data_.MutableData();
        const ElementOrFreeListLink* dest_data =
            (ElementOrFreeListLink*)other.data_.ConstData();

        // Use the inplace new to copy the element to an array element
        for (int32 index = 0; index < other.GetMaxIndex(); ++index) {
          ElementOrFreeListLink& dest_element = src_data[index];
          const ElementOrFreeListLink& src_element = dest_data[index];
          if (other.IsAllocated(index)) {
            ::new ((uint8*)&dest_element.element_data)
                ElementType(*(ElementType*)&src_element.element_data);
          }
          dest_element.prev_free_index = src_element.prev_free_index;
          dest_element.next_free_index = src_element.next_free_index;
        }
      } else {
        // Use the much faster path for types that allow it
        UnsafeMemory::Memcpy(
            data_.MutableData(), other.data_.ConstData(),
            sizeof(ElementOrFreeListLink) * other.GetMaxIndex());
      }
    }
    return *this;
  }

 private:
  template <typename SparseArrayType>
  FUN_ALWAYS_INLINE static typename EnableIf<
      ContainerTraits<SparseArrayType>::MoveWillEmptyContainer>::Type
  MoveOrCopy(SparseArrayType& to_array, SparseArrayType& from_array) {
    to_array.data_ = (DataType &&) from_array.data_;
    to_array.allocation_flags_ =
        (AllocationBitArrayType &&) from_array.allocation_flags_;

    to_array.first_free_index_ = from_array.first_free_index_;
    to_array.free_index_count_ = from_array.free_index_count_;
    from_array.first_free_index_ = INVALID_INDEX;
    from_array.free_index_count_ = 0;
  }

  template <typename SparseArrayType>
  FUN_ALWAYS_INLINE static typename EnableIf<
      !ContainerTraits<SparseArrayType>::MoveWillEmptyContainer>::Type
  MoveOrCopy(SparseArrayType& to_array, SparseArrayType& from_array) {
    to_array = from_array;
  }

 public:
  // Accessors.
  ElementType& operator[](int32 index) {
    fun_check_dbg(index >= 0 && index < data_.Count() &&
                  index < allocation_flags_.Count());
    // fun_check_dbg(allocation_flags_[index]); // Disabled to improve loading
    // times -BZ
    return *(ElementType*)&MutableData(index).element_data;
  }

  const ElementType& operator[](int32 index) const {
    fun_check_dbg(index >= 0 && index < data_.Count() &&
                  index < allocation_flags_.Count());
    // fun_check_dbg(allocation_flags_[index]); // Disabled to improve loading
    // times -BZ
    return *(ElementType*)&ConstData(index).element_data;
  }

  bool IsAllocated(int32 index) const { return allocation_flags_[index]; }

  int32 GetMaxIndex() const { return data_.Count(); }

  int32 Count() const { return data_.Count() - free_index_count_; }

  bool IsEmpty() const { return Count() == 0; }

  /**
   * Checks that the specified address is not part of an element
   * within the container.
   * Used for implementations to check that reference arguments aren't
   * going to be invalidated by possible reallocation.
   *
   * \param addr - The address to check.
   */
  FUN_ALWAYS_INLINE void CheckAddress(const ElementType* addr) const {
    data_.CheckAddress(addr);
  }

 private:
  /**
   * The base class of sparse array iterators.
   */
  template <bool IsConst>
  class BaseIterator {
   public:
    using BitArrayItType =
        ConstSetBitIterator<typename Allocator::BitArrayAllocator>;

   private:
    using ArrayType =
        typename Conditional<IsConst, const SparseArray, SparseArray>::Result;
    using ItElementType =
        typename Conditional<IsConst, const ElementType, ElementType>::Result;

   public:
    explicit BaseIterator(ArrayType& array, const BitArrayItType& bit_array_it)
        : array_(array), bit_array_it_(bit_array_it) {}

    FUN_ALWAYS_INLINE BaseIterator& operator++() {
      // Iterate to the next set allocation flag.
      ++bit_array_it_;
      return *this;
    }

    FUN_ALWAYS_INLINE int32 GetIndex() const {
      return bit_array_it_.GetIndex();
    }

    FUN_ALWAYS_INLINE friend bool operator==(const BaseIterator& lhs,
                                             const BaseIterator& rhs) {
      return lhs.bit_array_it_ == rhs.bit_array_it_ &&
             &lhs.array_ == &rhs.array_;
    }

    FUN_ALWAYS_INLINE friend bool operator!=(const BaseIterator& lhs,
                                             const BaseIterator& rhs) {
      return lhs.bit_array_it_ != rhs.bit_array_it_ ||
             &lhs.array_ != &rhs.array_;
    }

    FUN_ALWAYS_INLINE explicit operator bool() const { return !!bit_array_it_; }

    FUN_ALWAYS_INLINE bool operator!() const { return !(bool)*this; }

    FUN_ALWAYS_INLINE ItElementType& operator*() const {
      return array_[GetIndex()];
    }

    FUN_ALWAYS_INLINE ItElementType* operator->() const {
      return &array_[GetIndex()];
    }

    FUN_ALWAYS_INLINE const RelativeBitReference& GetRelativeBitReference()
        const {
      return bit_array_it_;
    }

   protected:
    ArrayType& array_;
    BitArrayItType bit_array_it_;
  };

 public:
  /**
   * Iterates over all allocated elements in a sparse array.
   */
  class Iterator : public BaseIterator<false> {
   public:
    Iterator(SparseArray& array)
        : BaseIterator<false>(
              array, ConstSetBitIterator<typename Allocator::BitArrayAllocator>(
                         array.allocation_flags_)) {}

    Iterator(SparseArray& array,
             const typename BaseIterator<false>::BitArrayItType& bit_array_it)
        : BaseIterator<false>(array, bit_array_it) {}

    /**
     * Safely removes the current element from the array.
     */
    void RemoveCurrent() { this->array_.RemoveAt(this->GetIndex()); }
  };

  /**
   * Iterates over all allocated elements in a const sparse array.
   */
  class ConstIterator : public BaseIterator<true> {
   public:
    ConstIterator(const SparseArray& array)
        : BaseIterator<true>(
              array, ConstSetBitIterator<typename Allocator::BitArrayAllocator>(
                         array.allocation_flags_)) {}

    ConstIterator(
        const SparseArray& array,
        const typename BaseIterator<true>::BitArrayItType& bit_array_it)
        : BaseIterator<true>(array, bit_array_it) {}
  };

  /**
   * Creates an iterator for the contents of this array
   */
  Iterator CreateIterator() { return Iterator(*this); }

  /**
   * Creates a const iterator for the contents of this array
   */
  ConstIterator CreateConstIterator() const { return ConstIterator(*this); }

 private:
  // DO NOT USE DIRECTLY
  // STL-like iterators to enable range-based for loop support.
  FUN_ALWAYS_INLINE friend Iterator begin(SparseArray& array) {
    return Iterator(array,
                    ConstSetBitIterator<typename Allocator::BitArrayAllocator>(
                        array.allocation_flags_));
  }
  FUN_ALWAYS_INLINE friend ConstIterator begin(const SparseArray& array) {
    return ConstIterator(
        array, ConstSetBitIterator<typename Allocator::BitArrayAllocator>(
                   array.allocation_flags_));
  }
  FUN_ALWAYS_INLINE friend Iterator end(SparseArray& array) {
    return Iterator(
        array, ConstSetBitIterator<typename Allocator::BitArrayAllocator>(
                   array.allocation_flags_, array.allocation_flags_.Count()));
  }
  FUN_ALWAYS_INLINE friend ConstIterator end(const SparseArray& array) {
    return ConstIterator(
        array, ConstSetBitIterator<typename Allocator::BitArrayAllocator>(
                   array.allocation_flags_, array.allocation_flags_.Count()));
  }

 public:
  /**
   * An iterator which only iterates over the elements of the array which
   * correspond to set bits in a separate bit array.
   */
  template <typename SubsetAllocator = DefaultBitArrayAllocator>
  class ConstSubsetIterator {
   public:
    ConstSubsetIterator(const SparseArray& array,
                        const BitArray<SubsetAllocator>& bit_array)
        : array_(array), bit_array_it_(array.allocation_flags_, bit_array) {}

    FUN_ALWAYS_INLINE ConstSubsetIterator& operator++() {
      // Iterate to the next element which is both allocated and has its bit set
      // in the other bit array.
      ++bit_array_it_;
      return *this;
    }

    FUN_ALWAYS_INLINE int32 GetIndex() const {
      return bit_array_it_.GetIndex();
    }

    FUN_ALWAYS_INLINE explicit operator bool() const { return !!bit_array_it_; }

    FUN_ALWAYS_INLINE bool operator!() const { return !(bool)*this; }

    FUN_ALWAYS_INLINE const ElementType& operator*() const {
      return array_(GetIndex());
    }

    FUN_ALWAYS_INLINE const ElementType* operator->() const {
      return &array_(GetIndex());
    }

    FUN_ALWAYS_INLINE const RelativeBitReference& GetRelativeBitReference()
        const {
      return bit_array_it_;
    }

   private:
    const SparseArray& array_;
    ConstDualSetBitIterator<typename Allocator::BitArrayAllocator,
                            SubsetAllocator>
        bit_array_it_;
  };

  //
  // Concatenation operators
  //

  SparseArray& operator+=(const SparseArray& other_array) {
    this->Reserve(this->Count() + other_array.Count());

    for (typename SparseArray::ConstIterator it(other_array); it; ++it) {
      this->Add(*it);
    }

    return *this;
  }

  SparseArray& operator+=(const Array<ElementType>& other_array) {
    this->Reserve(this->Count() + other_array.Count());

    for (int32 i = 0; i < other_array.Count(); ++i) {
      this->Add(other_array[i]);
    }

    return *this;
  }

 private:
  /**
   * The element type stored is only indirectly related to
   * the element type requested, to avoid instantiating Array<T> redundantly for
   * compatible types.
   */
  using ElementOrFreeListLink = SparseArrayElementOrFreeListLink<
      AlignedStorage<sizeof(ElementType), alignof(ElementType)>>;

  /**
   * Extracts the element value from the array's element structure and
   * passes it to the user provided comparison class.
   */
  template <typename Predicate>
  class ElementCompareClass {
   public:
    ElementCompareClass(const Predicate& pred) : pred_(pred) {}

    bool operator()(const ElementOrFreeListLink& x,
                    const ElementOrFreeListLink& y) const {
      return pred_(*(ElementType*)&x.element_data,
                   *(ElementType*)&y.element_data);
    }

   private:
    const Predicate& pred_;
  };

  /**
   * Accessor for the element or free list data.
   */
  ElementOrFreeListLink& MutableData(int32 index) {
    return ((ElementOrFreeListLink*)data_.MutableData())[index];
  }

  /**
   * Accessor for the element or free list data.
   */
  const ElementOrFreeListLink& ConstData(int32 index) const {
    return ((ElementOrFreeListLink*)data_.ConstData())[index];
  }

  using DataType =
      Array<ElementOrFreeListLink, typename Allocator::ElementAllocator>;
  DataType data_;

  using AllocationBitArrayType =
      BitArray<typename Allocator::BitArrayAllocator>;
  AllocationBitArrayType allocation_flags_;

  /**
   * The index of an unallocated element in the array that currently
   * contains the head of the linked list of free elements.
   */
  int32 first_free_index_;

  /** The number of elements in the free list. */
  int32 free_index_count_;
};

template <typename ElementType, typename Allocator>
struct ContainerTraits<SparseArray<ElementType, Allocator>>
    : public ContainerTraitsBase<SparseArray<ElementType, Allocator>> {
  enum {
    MoveWillEmptyContainer =
        ContainerTraits<typename SparseArray<
            ElementType, Allocator>::DataType>::MoveWillEmptyContainer &&
        ContainerTraits<typename SparseArray<ElementType, Allocator>::
                            AllocationBitArrayType>::MoveWillEmptyContainer
  };
};

struct UntypedSparseArrayLayout {
  int32 element_offset;
  int32 alignment;
  int32 size;
};

/**
 * Untyped sparse array type for accessing SparseArray data,
 * like UntypedArray for Array.
 *
 * Must have the same memory representation as a Set.
 */
class UntypedSparseArray {
 public:
  static UntypedSparseArrayLayout GetUntypedLayout(int32 element_size,
                                                   int32 element_alignment) {
    UntypedSparseArrayLayout result;
    result.element_offset = 0;
    result.alignment =
        MathBase::Max(element_alignment, (int32)alignof(FreeListLink));
    result.size = MathBase::Max(element_size, (int32)sizeof(FreeListLink));
    return result;
  }

  UntypedSparseArray()
      : first_free_index_(INVALID_INDEX), free_index_count_(0) {}

  bool IsValidIndex(int32 index) const {
    return allocation_flags_.IsValidIndex(index) && allocation_flags_[index];
  }

  int32 Count() const { return data_.Count() - free_index_count_; }

  int32 GetMaxIndex() const { return data_.Count(); }

  void* MutableData(int32 index, const UntypedSparseArrayLayout& layout) {
    return (uint8*)data_.MutableData() + layout.size * index;
  }

  const void* ConstData(int32 index,
                        const UntypedSparseArrayLayout& layout) const {
    return (const uint8*)data_.ConstData() + layout.size * index;
  }

  void Clear(int32 slack, const UntypedSparseArrayLayout& layout) {
    // Free the allocated elements.
    data_.Clear(slack, layout.size);
    first_free_index_ = INVALID_INDEX;
    free_index_count_ = 0;
    allocation_flags_.Clear(slack);
  }

  /**
   * Adds an uninitialized object to the array.
   *
   * \return The index of the added element.
   */
  int32 AddUninitialized(const UntypedSparseArrayLayout& layout) {
    int32 index;
    if (free_index_count_ > 0) {
      // Remove and use the first index from the list of free elements.
      index = first_free_index_;
      first_free_index_ =
          GetFreeListLink(first_free_index_, layout)->next_free_index;
      --free_index_count_;
      if (free_index_count_ > 0) {
        GetFreeListLink(first_free_index_, layout)->prev_free_index =
            INVALID_INDEX;
      }
    } else {
      // Add a new element.
      index = data_.Add(1, layout.size);
      allocation_flags_.Add(false);
    }

    allocation_flags_[index] = true;

    return index;
  }

  /**
   * Removes Count elements from the array, starting from index,
   * without destructing them.
   */
  void RemoveAtUninitialized(const UntypedSparseArrayLayout& layout,
                             int32 index, int32 count = 1) {
    for (; count > 0; --count) {
      fun_check(allocation_flags_[index]);

      // Mark the element as free and add it to the free element list.
      if (free_index_count_ > 0) {
        GetFreeListLink(first_free_index_, layout)->prev_free_index = index;
      }

      auto* index_data = GetFreeListLink(index, layout);
      index_data->prev_free_index = INVALID_INDEX;
      index_data->next_free_index =
          free_index_count_ > 0 ? first_free_index_ : INVALID_INDEX;
      first_free_index_ = index;
      ++free_index_count_;
      allocation_flags_[index] = false;

      ++index;
    }
  }

 private:
  UntypedArray data_;
  UntypedBitArray allocation_flags_;
  int32 first_free_index_;
  int32 free_index_count_;

  // This function isn't intended to be called, just to be compiled
  // to validate the correctness of the type.
  static void CheckConstraints() {
    typedef UntypedSparseArray RawType;
    typedef SparseArray<int32> RealType;

    // Check that the class footprint is the same
    static_assert(sizeof(RawType) == sizeof(RealType),
                  "UntypedSparseArray's size doesn't match SparseArray");
    static_assert(alignof(RawType) == alignof(RealType),
                  "UntypedSparseArray's alignment doesn't match SparseArray");

    // Check member sizes
    static_assert(
        sizeof(DeclVal<RawType>().data_) == sizeof(DeclVal<RealType>().data_),
        "UntypedSparseArray's data_ member size does not match SparseArray's");
    static_assert(sizeof(DeclVal<RawType>().allocation_flags_) ==
                      sizeof(DeclVal<RealType>().allocation_flags_),
                  "UntypedSparseArray's allocation_flags_ member size does not "
                  "match SparseArray's");
    static_assert(sizeof(DeclVal<RawType>().first_free_index_) ==
                      sizeof(DeclVal<RealType>().first_free_index_),
                  "UntypedSparseArray's first_free_index_ member size does not "
                  "match SparseArray's");
    static_assert(sizeof(DeclVal<RawType>().free_index_count_) ==
                      sizeof(DeclVal<RealType>().free_index_count_),
                  "UntypedSparseArray's free_index_count_ member size does not "
                  "match SparseArray's");

    // Check member offsets
    static_assert(offsetof(RawType, data_) == offsetof(RealType, data_),
                  "UntypedSparseArray's data_ member offset does not match "
                  "SparseArray's");
    static_assert(offsetof(RawType, allocation_flags_) ==
                      offsetof(RealType, allocation_flags_),
                  "UntypedSparseArray's allocation_flags_ member offset does "
                  "not match SparseArray's");
    static_assert(offsetof(RawType, first_free_index_) ==
                      offsetof(RealType, first_free_index_),
                  "UntypedSparseArray's first_free_index_ member offset does "
                  "not match SparseArray's");
    static_assert(offsetof(RawType, free_index_count_) ==
                      offsetof(RealType, free_index_count_),
                  "UntypedSparseArray's free_index_count_ member offset does "
                  "not match SparseArray's");

    // Check free index offsets
    static_assert(
        offsetof(RawType::FreeListLink, prev_free_index) ==
            offsetof(RealType::ElementOrFreeListLink, prev_free_index),
        "UntypedSparseArray's FreeListLink's prev_free_index member offset "
        "does not match SparseArray's");
    static_assert(
        offsetof(RawType::FreeListLink, next_free_index) ==
            offsetof(RealType::ElementOrFreeListLink, next_free_index),
        "UntypedSparseArray's FreeListLink's next_free_index member offset "
        "does not match SparseArray's");
  }

  struct FreeListLink {
    /**
     * If the element isn't allocated, this is a link to
     * the previous element in the array's free list.
     */
    int32 prev_free_index;

    /**
     * If the element isn't allocated, this is a link to
     * the next element in the array's free list.
     */
    int32 next_free_index;
  };

  /**
   * Accessor for the element or free list data.
   */
  FUN_ALWAYS_INLINE FreeListLink* GetFreeListLink(
      int32 index, const UntypedSparseArrayLayout& layout) {
    return (FreeListLink*)MutableData(index, layout);
  }

 public:
  // These should really be private, because they shouldn't be called,
  // but there's a bunch of code/ that needs to be fixed first.
  UntypedSparseArray(const UntypedSparseArray&) = delete;
  void operator=(const UntypedSparseArray&) = delete;
};

template <>
struct IsZeroConstructible<UntypedSparseArray> {
  enum { Value = true };
};

}  // namespace fun

// warning: 전역 namespace에 위치해야함.

/**
 * A placement new operator which constructs an element in
 * a sparse array allocation.
 */
FUN_ALWAYS_INLINE void* operator new(
    fun::size_t size, const fun::SparseArrayAllocationInfo& allocation) {
  fun_check_ptr(allocation.pointer);
  return allocation.pointer;
}
