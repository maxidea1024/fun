#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/memory_ops.h"
#include "fun/base/ftl/type_compatible_storage.h"
#include "fun/base/math/math_base.h"
#include "fun/base/math/numeric_limits.h"
#include "fun/base/memory_base.h"

namespace fun {

/**
 * branchless pointer selection
 * return a ? a : b;
 */
template <typename ReferencedType>
ReferencedType* IfAThenAElseB(ReferencedType* a, ReferencedType* b);

/**
 * branchless pointer selection based on predicate
 * return intptr_t(pred) ? a : b;
 */
template <typename Predicate, typename ReferencedType>
ReferencedType* IfPThenAElseB(Predicate pred, ReferencedType* a,
                              ReferencedType* b);

//*NEW*
#if TRACK_ARRAY_SLACK
FUN_BASE_API void TrackSlack(int32 element_count, int32 allocated_element_count,
                             size_t bytes_per_element, int32 rv);
#endif

FUN_ALWAYS_INLINE int32
DefaultCalculateSlackShrink(int32 element_count, int32 allocated_element_count,
                            size_t bytes_per_element, bool allow_quantize,
                            uint32 alignment = DEFAULT_ALIGNMENT) {
  int32 rv;
  fun_check_dbg(element_count < allocated_element_count);

  // If the container has too much slack, shrink it to exactly fit the number of
  // elements.
  const uint32 current_slack_elements = allocated_element_count - element_count;
  const size_t current_slack_bytes =
      (allocated_element_count - element_count) * bytes_per_element;
  const bool too_many_slack_bytes = current_slack_bytes >= 16384;
  const bool too_many_slack_elements =
      3 * element_count < 2 * allocated_element_count;
  if ((too_many_slack_bytes || too_many_slack_elements) &&
      (current_slack_elements > 64 || !element_count)) {  //  hard coded 64 :-(
    rv = element_count;
    if (rv > 0) {
      if (allow_quantize) {
        // TODO size_t / int32 어떤 타입으로 다루는게 좋을지...
        rv = (int32)(
            UnsafeMemory::QuantizeSize(rv * bytes_per_element, alignment) /
            bytes_per_element);
      }
    }
  } else {
    rv = allocated_element_count;
  }
#if TRACK_ARRAY_SLACK
  void TrackSlack(element_count, allocated_element_count, bytes_per_element,
                  rv);
#endif
  return rv;
}

FUN_ALWAYS_INLINE int32
DefaultCalculateSlackGrow(int32 element_count, int32 allocated_element_count,
                          size_t bytes_per_element, bool allow_quantize,
                          uint32 alignment = DEFAULT_ALIGNMENT) {
  int32 rv;
  fun_check_dbg(element_count > allocated_element_count && element_count > 0);

  size_t grow = 4;  // this is the amount for the first alloc
  if (allocated_element_count || size_t(element_count) > grow) {
    // Allocate slack for the array proportional to its size.
    grow = size_t(element_count) + 3 * size_t(element_count) / 8 + 16;
  }

  if (allow_quantize) {
    // TODO size_t / int32 어떤 타입으로 다루는게 좋을지...
    rv = (int32)(
        UnsafeMemory::QuantizeSize(grow * bytes_per_element, alignment) /
        bytes_per_element);
  } else {
    // TODO size_t / int32 어떤 타입으로 다루는게 좋을지...
    rv = (int32)grow;
  }

  // element_count and MaxElements are stored in 32 bit signed integers
  // so we must be careful not to overflow here.
  if (element_count > rv) {
    rv = int32_MAX;
  }

#if TRACK_ARRAY_SLACK
  void TrackSlack(element_count, allocated_element_count, bytes_per_element,
                  rv);
#endif

  return rv;
}

FUN_ALWAYS_INLINE int32 DefaultCalculateSlackReserve(
    int32 element_count, size_t bytes_per_element, bool allow_quantize,
    uint32 alignment = DEFAULT_ALIGNMENT) {
  int32 rv = element_count;
  fun_check_dbg(element_count > 0);
  if (allow_quantize) {
    // TODO size_t / int32 어떤 타입으로 다루는게 좋을지...
    rv = (int32)(UnsafeMemory::QuantizeSize(
                     size_t(rv) * size_t(bytes_per_element), alignment) /
                 bytes_per_element);
    // element_count and MaxElements are stored in 32 bit signed integers
    // so we must be careful not to overflow here.
    if (element_count > rv) {
      rv = int32_MAX;
    }
  }

#if TRACK_ARRAY_SLACK
  void TrackSlack(element_count, allocated_element_count, bytes_per_element,
                  rv);
#endif

  return rv;
}

/**
 * A type which is used to represent a script type
 * that is unknown at compile time.
 */
struct UntypedContainerElement {};

template <typename AllocatorType>
struct AllocatorTraitsBase {
  enum { SupportsMove = false };
  enum { IsZeroConstruct = false };
};

template <typename AllocatorType>
struct AllocatorTraits : AllocatorTraitsBase<AllocatorType> {};

/**
 * This is the allocation policy interface; it exists purely to document
 * the policy's interface, and should not be used.
 */
class ContainerAllocatorInterface {
 public:
  /**
   * Determines whether the user of the allocator
   * may use the ForAnyElementType inner class.
   */
  enum { NeedsElementType = true };
  enum { RequireRangeCheck = true };

  /**
   * A class that receives both the explicit allocation policy
   * template parameters specified by the user of the container,
   * but also the implicit ElementType template parameter
   * from the container type.
   */
  template <typename ElementType>
  struct ForElementType {
    /**
     * Moves the state of another allocator into this one.
     * Assumes that the allocator is currently empty, i.e. memory may be
     * allocated but any existing elements have already been destructed (if
     * necessary).
     *
     * \param other - The allocator to move the state from.  This allocator
     * should be left in a valid empty state.
     */
    void MoveToEmpty(ForElementType& other);

    /**
     * Accesses the container's current data.
     */
    ElementType* GetAllocation() const;

    /**
     * Resizes the container's allocation.
     *
     * \param previous_element_count - The number of elements that were stored
     * in the previous allocation. \param element_count - The number of elements
     * to allocate space for. \param bytes_per_element - The number of
     * bytes/element.
     */
    void ResizeAllocation(int32 previous_element_count, int32 element_count,
                          size_t bytes_per_element);

    /**
     * Calculates the amount of slack to allocate for an array that has just
     * grown or shrunk to a given number of elements.
     *
     * \param element_count - The number of elements to allocate space for.
     * \param current_slack_element_count - The current number of elements
     * allocated. \param bytes_per_element - The number of bytes/element.
     */
    int32 CalculateSlack(int32 element_count, int32 current_slack_element_count,
                         size_t bytes_per_element) const;

    /**
     * Calculates the amount of slack to allocate for an array that has just
     * shrunk to a given number of elements.
     *
     * \param element_count - The number of elements to allocate space for.
     * \param current_slack_element_count - The current number of elements
     * allocated. \param bytes_per_element - The number of bytes/element.
     */
    int32 CalculateSlackShrink(int32 element_count,
                               int32 current_slack_element_count,
                               size_t bytes_per_element) const;

    /**
     * Calculates the amount of slack to allocate for an array that has just
     * grown to a given number of elements.
     *
     * \param element_count - The number of elements to allocate space for.
     * \param current_slack_element_count - The current number of elements
     * allocated. \param bytes_per_element - The number of bytes/element.
     */
    int32 CalculateSlackGrow(int32 element_count,
                             int32 current_slack_element_count,
                             size_t bytes_per_element) const;

    size_t GetAllocatedSize(int32 allocated_element_count,
                            size_t bytes_per_element) const;
  };

  /**
   * A class that may be used when NeedsElementType=false is specified.
   * If NeedsElementType=true, then this must be present but will not be used,
   * and so can simply be a typedef to void
   */
  typedef ForElementType<UntypedContainerElement> ForAnyElementType;
};

/**
 * The indirect allocation policy always allocates the elements indirectly.
 */
template <uint32 Alignment = DEFAULT_ALIGNMENT>
class AlignedHeapAllocator {
 public:
  enum { NeedsElementType = false };
  enum { RequireRangeCheck = true };

  struct ForAnyElementType {
    /**
     * Default constructor.
     */
    ForAnyElementType() : data_(nullptr) {}

    /**
     * Moves the state of another allocator into this one.
     * Assumes that the allocator is currently empty, i.e. memory may be
     * allocated but any existing elements have already been destructed (if
     * necessary).
     *
     * \param other - The allocator to move the state from.  This allocator
     * should be left in a valid empty state.
     */
    FUN_ALWAYS_INLINE void MoveToEmpty(ForAnyElementType& other) {
      fun_check(&other != this);

      if (data_) {
#if FUN_PLATFORM_HAS_UMA
        UnsafeMemory::GPUFree(data_);
#else
        UnsafeMemory::Free(data_);
#endif
      }

      data_ = other.data_;
      other.data_ = nullptr;
    }

    /**
     * Destructor.
     */
    FUN_ALWAYS_INLINE ~ForAnyElementType() {
      if (data_) {
#if FUN_PLATFORM_HAS_UMA
        // The RHI should have taken ownership of this memory, so we don't free
        // it here
#else
        UnsafeMemory::Free(data_);
#endif
      }
    }

    // ContainerAllocatorInterface
    FUN_ALWAYS_INLINE UntypedContainerElement* GetAllocation() const {
      return data_;
    }

    void ResizeAllocation(int32 previous_element_count, int32 element_count,
                          size_t bytes_per_element) {
      // Avoid calling UnsafeMemory::Realloc(nullptr, 0) as ANSI C mandates
      // returning a valid pointer which is not what we want.
      if (data_ || element_count > 0) {
#if FUN_PLATFORM_HAS_UMA
        data_ = (UntypedContainerElement*)UnsafeMemory::GPURealloc(
            data_, element_count * bytes_per_element, Alignment);
#else
        // fun_check_dbg(((uint64)element_count*(uint64)ElementTypeInfo.GetSize()
        // < (uint64)INT_MAX));
        data_ = (UntypedContainerElement*)UnsafeMemory::Realloc(
            data_, element_count * bytes_per_element, Alignment);
#endif
      }
    }

    FUN_ALWAYS_INLINE int32
    CalculateSlackReserve(int32 element_count, int32 bytes_per_element) const {
      return DefaultCalculateSlackReserve(element_count, bytes_per_element,
                                          true, Alignment);
    }

    FUN_ALWAYS_INLINE int32
    CalculateSlackShrink(int32 element_count, int32 allocated_element_count,
                         int32 bytes_per_element) const {
      return DefaultCalculateSlackShrink(element_count, allocated_element_count,
                                         bytes_per_element, true, Alignment);
    }

    FUN_ALWAYS_INLINE int32 CalculateSlackGrow(int32 element_count,
                                               int32 allocated_element_count,
                                               int32 bytes_per_element) const {
      return DefaultCalculateSlackGrow(element_count, allocated_element_count,
                                       bytes_per_element, true, Alignment);
    }

    size_t GetAllocatedSize(int32 allocated_element_count,
                            size_t bytes_per_element) const {
      return allocated_element_count * bytes_per_element;
    }

   private:
    ForAnyElementType(const ForAnyElementType&) = delete;
    ForAnyElementType& operator=(const ForAnyElementType&) = delete;

    /**
     * A pointer to the container's elements.
     */
    UntypedContainerElement* data_;
  };

  template <typename ElementType>
  struct ForElementType : public ForAnyElementType {
    /**
     * Default constructor.
     */
    ForElementType() {}

    FUN_ALWAYS_INLINE ElementType* GetAllocation() const {
      return (ElementType*)ForAnyElementType::GetAllocation();
    }
  };
};

template <uint32 Alignment>
struct AllocatorTraits<AlignedHeapAllocator<Alignment>>
    : AllocatorTraitsBase<AlignedHeapAllocator<Alignment>> {
  enum { SupportsMove = true };
  enum { IsZeroConstruct = true };
};

/**
 * The indirect allocation policy always allocates the elements indirectly.
 */
class FUN_BASE_API HeapAllocator {
 public:
  enum { NeedsElementType = false };
  enum { RequireRangeCheck = true };

  struct FUN_BASE_API ForAnyElementType {
    /**
     * Default constructor.
     */
    ForAnyElementType() : data_(nullptr) {}

    /**
     * Moves the state of another allocator into this one.
     * Assumes that the allocator is currently empty, i.e. memory may be
     * allocated but any existing elements have already been destructed (if
     * necessary).
     *
     * \param other - The allocator to move the state from.  This allocator
     * should be left in a valid empty state.
     */
    FUN_ALWAYS_INLINE void MoveToEmpty(ForAnyElementType& other) {
      fun_check(&other != this);

      if (data_) {
        UnsafeMemory::Free(data_);
      }

      data_ = other.data_;
      other.data_ = nullptr;
    }

    /**
     * Destructor.
     */
    FUN_ALWAYS_INLINE ~ForAnyElementType() {
      if (data_) {
        UnsafeMemory::Free(data_);
      }
    }

    // ContainerAllocatorInterface
    FUN_ALWAYS_INLINE UntypedContainerElement* GetAllocation() const {
      return data_;
    }

    void ResizeAllocation(int32 previous_element_count, int32 element_count,
                          size_t bytes_per_element) {
      // Avoid calling UnsafeMemory::Realloc(nullptr, 0) as ANSI C mandates
      // returning a valid pointer which is not what we want.
      if (data_ || element_count > 0) {
        // fun_check_dbg(((uint64)element_count*(uint64)ElementTypeInfo.GetSize()
        // < (uint64)INT_MAX));
        data_ = (UntypedContainerElement*)UnsafeMemory::Realloc(
            data_, element_count * bytes_per_element);
      }
    }

    FUN_ALWAYS_INLINE int32
    CalculateSlackReserve(int32 element_count, int32 bytes_per_element) const {
      return DefaultCalculateSlackReserve(element_count, bytes_per_element,
                                          true);
    }

    FUN_ALWAYS_INLINE int32
    CalculateSlackShrink(int32 element_count, int32 allocated_element_count,
                         int32 bytes_per_element) const {
      return DefaultCalculateSlackShrink(element_count, allocated_element_count,
                                         bytes_per_element, true);
    }

    FUN_ALWAYS_INLINE int32 CalculateSlackGrow(int32 element_count,
                                               int32 allocated_element_count,
                                               int32 bytes_per_element) const {
      return DefaultCalculateSlackGrow(element_count, allocated_element_count,
                                       bytes_per_element, true);
    }

    size_t GetAllocatedSize(int32 allocated_element_count,
                            size_t bytes_per_element) const {
      return allocated_element_count * bytes_per_element;
    }

   private:
    ForAnyElementType(const ForAnyElementType&) = delete;
    ForAnyElementType& operator=(const ForAnyElementType&) = delete;

    /**
     * A pointer to the container's elements.
     */
    UntypedContainerElement* data_;
  };

  template <typename ElementType>
  struct ForElementType : public ForAnyElementType {
    /**
     * Default constructor.
     */
    ForElementType() {}

    FUN_ALWAYS_INLINE ElementType* GetAllocation() const {
      return (ElementType*)ForAnyElementType::GetAllocation();
    }
  };
};

template <>
struct AllocatorTraits<HeapAllocator> : AllocatorTraitsBase<HeapAllocator> {
  enum { SupportsMove = true };
  enum { IsZeroConstruct = true };
};

class DefaultAllocator;

/**
 * The FUN_ALWAYS_INLINE allocation policy allocates up to a specified number of
 * elements in the same allocation as the container. Any allocation needed
 * beyond that causes all data to be moved into an indirect allocation. It
 * always uses DEFAULT_ALIGNMENT.
 */
template <uint32 NumInlineElements,
          typename SecondaryAllocator = DefaultAllocator>
class InlineAllocator {
 public:
  enum { NeedsElementType = true };
  enum { RequireRangeCheck = true };

  template <typename ElementType>
  struct ForElementType {
    /**
     * Default constructor.
     */
    ForElementType() {}

    /**
     * Moves the state of another allocator into this one.
     * Assumes that the allocator is currently empty, i.e. memory may be
     * allocated but any existing elements have already been destructed (if
     * necessary).
     *
     * \param other - The allocator to move the state from.  This allocator
     * should be left in a valid empty state.
     */
    FUN_ALWAYS_INLINE void MoveToEmpty(ForElementType& other) {
      fun_check(&other != this);

      if (!other.secondary_allocator_.GetAllocation()) {
        // Relocate objects from other FUN_ALWAYS_INLINE storage only if it was
        // stored FUN_ALWAYS_INLINE in other
        RelocateConstructItems<ElementType>(
            (void*)inline_data_, other.GetInlineElements(), NumInlineElements);
      }

      // Move secondary storage in any case.
      // This will move secondary storage if it exists but will also handle the
      // case where secondary storage is used in other but not in *this.
      secondary_allocator_.MoveToEmpty(other.secondary_allocator_);
    }

    // ContainerAllocatorInterface
    FUN_ALWAYS_INLINE ElementType* GetAllocation() const {
      return IfAThenAElseB<ElementType>(secondary_allocator_.GetAllocation(),
                                        GetInlineElements());
    }

    void ResizeAllocation(int32 previous_element_count, int32 element_count,
                          size_t bytes_per_element) {
      // Check if the new allocation will fit in the FUN_ALWAYS_INLINE data
      // area.
      if (element_count <= NumInlineElements) {
        // If the old allocation wasn't in the FUN_ALWAYS_INLINE data area,
        // relocate it into the FUN_ALWAYS_INLINE data area.
        if (secondary_allocator_.GetAllocation()) {
          RelocateConstructItems<ElementType>(
              (void*)inline_data_,
              (ElementType*)secondary_allocator_.GetAllocation(),
              previous_element_count);

          // Free the old indirect allocation.
          secondary_allocator_.ResizeAllocation(0, 0, bytes_per_element);
        }
      } else {
        if (secondary_allocator_.GetAllocation() == nullptr) {
          // Allocate new indirect memory for the data.
          secondary_allocator_.ResizeAllocation(0, element_count,
                                                bytes_per_element);

          // Move the data out of the FUN_ALWAYS_INLINE data area into the new
          // allocation.
          RelocateConstructItems<ElementType>(
              (void*)secondary_allocator_.GetAllocation(), GetInlineElements(),
              previous_element_count);
        } else {
          // Reallocate the indirect data for the new size.
          secondary_allocator_.ResizeAllocation(
              previous_element_count, element_count, bytes_per_element);
        }
      }
    }

    FUN_ALWAYS_INLINE int32
    CalculateSlackReserve(int32 element_count, int32 bytes_per_element) const {
      // If the elements use less space than the FUN_ALWAYS_INLINE allocation,
      // only use the FUN_ALWAYS_INLINE allocation as slack.
      return element_count <= NumInlineElements
                 ? NumInlineElements
                 : secondary_allocator_.CalculateSlackReserve(
                       element_count, bytes_per_element);
    }

    FUN_ALWAYS_INLINE int32
    CalculateSlackShrink(int32 element_count, int32 allocated_element_count,
                         int32 bytes_per_element) const {
      // If the elements use less space than the FUN_ALWAYS_INLINE allocation,
      // only use the FUN_ALWAYS_INLINE allocation as slack.
      return element_count <= NumInlineElements
                 ? NumInlineElements
                 : secondary_allocator_.CalculateSlackShrink(
                       element_count, allocated_element_count,
                       bytes_per_element);
    }

    FUN_ALWAYS_INLINE int32 CalculateSlackGrow(int32 element_count,
                                               int32 allocated_element_count,
                                               int32 bytes_per_element) const {
      // If the elements use less space than the FUN_ALWAYS_INLINE allocation,
      // only use the FUN_ALWAYS_INLINE allocation as slack.
      return element_count <= NumInlineElements
                 ? NumInlineElements
                 : secondary_allocator_.CalculateSlackGrow(
                       element_count, allocated_element_count,
                       bytes_per_element);
    }

    size_t GetAllocatedSize(int32 allocated_element_count,
                            int32 bytes_per_element) const {
      return secondary_allocator_.GetAllocatedSize(allocated_element_count,
                                                   bytes_per_element);
    }

   private:
    ForElementType(const ForElementType&) = delete;
    ForElementType& operator=(const ForElementType&) = delete;

    /**
     * The data is stored in this array if less than NumInlineElements is
     * needed.
     */
    TypeCompatibleStorage<ElementType> inline_data_[NumInlineElements];

    /**
     * The data is allocated through the indirect allocation policy if more than
     * NumInlineElements is needed.
     */
    typename SecondaryAllocator::template ForElementType<ElementType>
        secondary_allocator_;

    /**
     * Returns the base of the aligned FUN_ALWAYS_INLINE element data
     */
    ElementType* GetInlineElements() const {
      return (ElementType*)inline_data_;
    }
  };

  typedef void ForAnyElementType;
};

template <uint32 NumInlineElements, typename SecondaryAllocator>
struct AllocatorTraits<InlineAllocator<NumInlineElements, SecondaryAllocator>>
    : AllocatorTraitsBase<
          InlineAllocator<NumInlineElements, SecondaryAllocator>> {
  enum { SupportsMove = AllocatorTraits<SecondaryAllocator>::SupportsMove };
};

/**
 * The fixed allocation policy allocates up to a specified
 * number of elements in the same allocation as the container.
 *
 * It's like the FUN_ALWAYS_INLINE allocator, except it doesn't provide
 * secondary storage when the FUN_ALWAYS_INLINE storage has been filled.
 */
template <uint32 NumInlineElements>
class FixedAllocator {
 public:
  enum { NeedsElementType = true };
  enum { RequireRangeCheck = true };

  template <typename ElementType>
  struct ForElementType {
    /**
     * Default constructor.
     */
    ForElementType() {}

    /**
     * Moves the state of another allocator into this one.
     * Assumes that the allocator is currently empty, i.e. memory may be
     * allocated but any existing elements have already
     * been destructed (if necessary).
     *
     * \param other - The allocator to move the state from.  This allocator
     * should be left in a valid empty state.
     */
    FUN_ALWAYS_INLINE void MoveToEmpty(ForElementType& other) {
      fun_check(&other != this);

      // Relocate objects from other FUN_ALWAYS_INLINE storage
      RelocateConstructItems<ElementType>(
          (void*)inline_data_, other.GetInlineElements(), NumInlineElements);
    }

    // ContainerAllocatorInterface
    FUN_ALWAYS_INLINE ElementType* GetAllocation() const {
      return GetInlineElements();
    }

    void ResizeAllocation(int32 previous_element_count, int32 element_count,
                          size_t bytes_per_element) {
      // Ensure the requested allocation will fit in the FUN_ALWAYS_INLINE data
      // area.
      fun_check(element_count <= NumInlineElements);
    }

    FUN_ALWAYS_INLINE int32
    CalculateSlackReserve(int32 element_count, size_t bytes_per_element) const {
      // Ensure the requested allocation will fit in the FUN_ALWAYS_INLINE data
      // area.
      fun_check(element_count <= NumInlineElements);
      return NumInlineElements;
    }

    FUN_ALWAYS_INLINE int32
    CalculateSlackShrink(int32 element_count, int32 allocated_element_count,
                         int32 bytes_per_element) const {
      // Ensure the requested allocation will fit in the FUN_ALWAYS_INLINE data
      // area.
      fun_check(allocated_element_count <= NumInlineElements);
      return NumInlineElements;
    }

    FUN_ALWAYS_INLINE int32 CalculateSlackGrow(int32 element_count,
                                               int32 allocated_element_count,
                                               int32 bytes_per_element) const {
      // Ensure the requested allocation will fit in the FUN_ALWAYS_INLINE data
      // area.
      fun_check(element_count <= NumInlineElements);
      return NumInlineElements;
    }

    size_t GetAllocatedSize(int32 allocated_element_count,
                            size_t bytes_per_element) const {
      return 0;
    }

   private:
    ForElementType(const ForElementType&) = delete;
    ForElementType& operator=(const ForElementType&) = delete;

    /**
     * The data is stored in this array if less than NumInlineElements is
     * needed.
     */
    TypeCompatibleStorage<ElementType> inline_data_[NumInlineElements];

    /**
     * Returns the base of the aligned FUN_ALWAYS_INLINE element data
     */
    ElementType* GetInlineElements() const {
      return (ElementType*)inline_data_;
    }
  };

  typedef void ForAnyElementType;
};

template <uint32 NumInlineElements>
struct AllocatorTraits<FixedAllocator<NumInlineElements>>
    : AllocatorTraitsBase<FixedAllocator<NumInlineElements>> {
  enum { SupportsMove = true };
};

// template <uint32 NumInlineElements>
// class ExternalBufferAllocator {
// public:
//  enum { NeedsElementType = true };
//  enum { RequireRangeCheck = true };
//
//  template <typename ElementType>
//  struct ForElementType {
//    ForElementType() : ExternalBuffer(nullptr), ExternalBufferLength(0) {}
//
//    void Setup(ElementType* ExternalBuffer, int32 ExternalBufferLength) {
//      this->ExternalBuffer = ExternalBuffer;
//      this->ExternalBufferLength = ExternalBufferLength;
//    }
//
//    FUN_ALWAYS_INLINE void MoveToEmpty(ForElementType& other) {
//      fun_check(this != &other);
//
//      // Relocate objects from other FUN_ALWAYS_INLINE storage
//      RelocateConstructItems<ElementType>((void*)ExternalBuffer,
//      other.ExternalBuffer, ExternalBufferLength);
//    }
//
//    // ContainerAllocatorInterface
//    FUN_ALWAYS_INLINE ElementType* GetAllocation() const {
//      return ExternalBuffer;
//    }
//
//    void ResizeAllocation(int32 previous_element_count, int32 element_count,
//    size_t bytes_per_element) {
//      // Ensure the requested allocation will fit in the FUN_ALWAYS_INLINE
//      data area. fun_check(element_count <= ExternalBufferLength);
//    }
//
//    FUN_ALWAYS_INLINE int32 CalculateSlackReserve(int32 element_count, size_t
//    bytes_per_element) const
//    {
//      // Ensure the requested allocation will fit in the FUN_ALWAYS_INLINE
//      data area. fun_check_dbg(element_count <= ExternalBufferLength); return
//      ExternalBufferLength;
//    }
//
//    FUN_ALWAYS_INLINE int32 CalculateSlackShrink(int32 element_count, int32
//    allocated_element_count, int32 bytes_per_element) const
//    {
//      // Ensure the requested allocation will fit in the FUN_ALWAYS_INLINE
//      data area. fun_check_dbg(allocated_element_count <=
//      ExternalBufferLength); return ExternalBufferLength;
//    }
//
//    FUN_ALWAYS_INLINE int32 CalculateSlackGrow(int32 element_count, int32
//    allocated_element_count, int32 bytes_per_element) const
//    {
//      // Ensure the requested allocation will fit in the FUN_ALWAYS_INLINE
//      data area. fun_check_dbg(element_count <= ExternalBufferLength); return
//      ExternalBufferLength;
//    }
//
//    size_t GetAllocatedSize(int32 allocated_element_count, size_t
//    bytes_per_element) const
//    {
//      return 0;
//    }
//
//  private:
//    ForElementType(const ForElementType&) = delete;
//    ForElementType& operator = (const ForElementType&) = delete;
//
//    /// The data is stored in this array if less than NumInlineElements is
//    needed. TypeCompatibleStorage<ElementType>
//    inline_data_[NumInlineElements];
//
//    /// \return the base of the aligned FUN_ALWAYS_INLINE element data
//    ElementType* GetInlineElements() const
//    {
//      return (ElementType*)inline_data_;
//    }
//  };
//
//  typedef void ForAnyElementType;
//};
//
// struct AllocatorTraits<ExternalBufferAllocator> :
// AllocatorTraitsBase<ExternalBufferAllocator>
//{
//  enum { SupportsMove = true };
//};

// We want these to be correctly typed as int32, but we don't want them to have
// linkage, so we make them macros
#define NumBitsPerDWORD 32
#define NumBitsPerDWORDLogTwo 5

//
// Sparse array allocation definitions
//

class DefaultAllocator;
class DefaultBitArrayAllocator;

/**
 * Encapsulates the allocators used by a sparse array in a single type.
 */
template <typename _ElementAllocator = DefaultAllocator,
          typename _BitArrayAllocator = DefaultBitArrayAllocator>
class SparseArrayAllocator {
 public:
  typedef _ElementAllocator ElementAllocator;
  typedef _BitArrayAllocator BitArrayAllocator;
};

/**
 * An FUN_ALWAYS_INLINE sparse array allocator that allows sizing of the
 * FUN_ALWAYS_INLINE allocations for a set number of elements.
 */
template <uint32 NumInlineElements,
          typename SecondaryAllocator =
              SparseArrayAllocator<DefaultAllocator, DefaultAllocator>>
class InlineSparseArrayAllocator {
 private:
  /**
   * The size to allocate FUN_ALWAYS_INLINE for the bit array.
   */
  enum {
    InlineBitArrayDWORDs =
        (NumInlineElements + NumBitsPerDWORD - 1) / NumBitsPerDWORD
  };

 public:
  typedef InlineAllocator<NumInlineElements,
                          typename SecondaryAllocator::ElementAllocator>
      ElementAllocator;
  typedef InlineAllocator<InlineBitArrayDWORDs,
                          typename SecondaryAllocator::BitArrayAllocator>
      BitArrayAllocator;
};

//
// Set allocation definitions.
//

#define DEFAULT_NUMBER_OF_ELEMENTS_PER_HASH_BUCKET 2
#define DEFAULT_BASE_NUMBER_OF_HASH_BUCKETS 8
#define DEFAULT_MIN_NUMBER_OF_HASHED_ELEMENTS 4

/**
 * Encapsulates the allocators used by a set in a single type.
 */
template <typename InSparseArrayAllocator = SparseArrayAllocator<>,
          typename InHashAllocator = InlineAllocator<1, DefaultAllocator>,
          uint32 AverageNumberOfElementsPerHashBucket =
              DEFAULT_NUMBER_OF_ELEMENTS_PER_HASH_BUCKET,
          uint32 BaseNumberOfHashBuckets = DEFAULT_BASE_NUMBER_OF_HASH_BUCKETS,
          uint32 MinNumberOfHashedElements =
              DEFAULT_MIN_NUMBER_OF_HASHED_ELEMENTS>
class SetAllocator {
 public:
  /**
   * Computes the number of hash buckets to use for a given number of elements.
   */
  static FUN_ALWAYS_INLINE uint32
  GetNumberOfHashBuckets(uint32 num_hashed_elements) {
    if (num_hashed_elements >= MinNumberOfHashedElements) {
      return MathBase::RoundUpToPowerOfTwo(
          num_hashed_elements / AverageNumberOfElementsPerHashBucket +
          BaseNumberOfHashBuckets);
    }

    return 1;
  }

  typedef InSparseArrayAllocator SparseArrayAllocator;
  typedef InHashAllocator HashAllocator;
};

class DefaultAllocator;

/**
 * An FUN_ALWAYS_INLINE set allocator that allows sizing of the
 * FUN_ALWAYS_INLINE allocations for a set number of elements.
 */
template <
    uint32 NumInlineElements,
    typename SecondaryAllocator =
        SetAllocator<SparseArrayAllocator<DefaultAllocator, DefaultAllocator>,
                     DefaultAllocator>,
    uint32 AverageNumberOfElementsPerHashBucket =
        DEFAULT_NUMBER_OF_ELEMENTS_PER_HASH_BUCKET,
    uint32 MinNumberOfHashedElements = DEFAULT_MIN_NUMBER_OF_HASHED_ELEMENTS>
class InlineSetAllocator {
 private:
  enum {
    NumInlineHashBuckets =
        (NumInlineElements + AverageNumberOfElementsPerHashBucket - 1) /
        AverageNumberOfElementsPerHashBucket
  };

 public:
  /**
   * Computes the number of hash buckets to use for a given number of elements.
   */
  static FUN_ALWAYS_INLINE uint32
  GetNumberOfHashBuckets(uint32 num_hashed_elements) {
    const uint32 NumDesiredHashBuckets = MathBase::RoundUpToPowerOfTwo(
        num_hashed_elements / AverageNumberOfElementsPerHashBucket);
    if (NumDesiredHashBuckets < NumInlineHashBuckets) {
      return NumInlineHashBuckets;
    }

    if (num_hashed_elements < MinNumberOfHashedElements) {
      return NumInlineHashBuckets;
    }

    return NumDesiredHashBuckets;
  }

  typedef InlineSparseArrayAllocator<
      NumInlineElements, typename SecondaryAllocator::SparseArrayAllocator>
      SparseArrayAllocator;
  typedef InlineAllocator<NumInlineHashBuckets,
                          typename SecondaryAllocator::HashAllocator>
      HashAllocator;
};

/**
 * 'typedefs' for various allocator defaults.
 *
 * These should be replaced with actual typedefs when Core.h include order is
 * sorted out, as then we won't need to 'forward' these AllocatorTraits
 * specializations below.
 */
class DefaultAllocator : public HeapAllocator {
 public:
  typedef HeapAllocator Typedef;
};
class DefaultSetAllocator : public SetAllocator<> {
 public:
  typedef SetAllocator<> Typedef;
};
class DefaultBitArrayAllocator : public InlineAllocator<4> {
 public:
  typedef InlineAllocator<4> Typedef;
};
class DefaultSparseArrayAllocator : public SparseArrayAllocator<> {
 public:
  typedef SparseArrayAllocator<> Typedef;
};

template <>
struct AllocatorTraits<DefaultAllocator>
    : AllocatorTraits<typename DefaultAllocator::Typedef> {};
template <>
struct AllocatorTraits<DefaultSetAllocator>
    : AllocatorTraits<typename DefaultSetAllocator::Typedef> {};
template <>
struct AllocatorTraits<DefaultBitArrayAllocator>
    : AllocatorTraits<typename DefaultBitArrayAllocator::Typedef> {};
template <>
struct AllocatorTraits<DefaultSparseArrayAllocator>
    : AllocatorTraits<typename DefaultSparseArrayAllocator::Typedef> {};

}  // namespace fun
