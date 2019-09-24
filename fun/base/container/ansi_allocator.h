#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * Allocator that allocates memory using standard library functions.
 */
class FUN_BASE_API AnsiAllocator {
 public:
  enum { NeedsElementType = false };
  enum { RequireRangeCheck = true };

  using ElementAllocator = AnsiAllocator;
  using BitArrayAllocator = AnsiAllocator;

  struct FUN_BASE_API ForAnyElementType   {
    ForAnyElementType() : data_(nullptr) {}

    FUN_ALWAYS_INLINE void MoveToEmpty(ForAnyElementType& other) {
      fun_check(&other != this);

      if (data_) {
        ::free(data_);
      }

      data_ = other.data_;
      other.data_ = nullptr;
    }

    FUN_ALWAYS_INLINE ~ForAnyElementType() {
      if (data_) {
        ::free(data_);
      }
    }

    FUN_ALWAYS_INLINE UntypedContainerElement* GetAllocation() const  { return data_; }

    void ResizeAllocation(int32 previous_element_count, int32 element_count, size_t bytes_per_element) {
      // Avoid calling UnsafeMemory::Realloc( nullptr, 0 ) as ANSI C mandates returning a valid pointer which is not what we want.
      if (element_count > 0) {
        //CHECK_SLOW(((uint64)element_count*(uint64)ElementTypeInfo.GetSize() < (uint64)INT_MAX));
        void* new_alloc = ::realloc(data_, element_count * bytes_per_element);
        data_ = (UntypedContainerElement*)new_alloc;
      }
      else {
        ::free(data_);
        data_ = nullptr;
      }
    }

    int32 CalculateSlackReserve(int32 element_count, int32 bytes_per_element) const {
      return DefaultCalculateSlackReserve(element_count, bytes_per_element, false);
    }

    int32 CalculateSlackShrink(int32 element_count, int32 allocated_element_count, int32 bytes_per_element) const {
      return DefaultCalculateSlackShrink(element_count, allocated_element_count, bytes_per_element, false);
    }

    int32 CalculateSlackGrow(int32 element_count, int32 allocated_element_count, int32 bytes_per_element) const {
      return DefaultCalculateSlackGrow(element_count, allocated_element_count, bytes_per_element, false);
    }

    size_t GetAllocatedSize(int32 allocated_element_count, size_t bytes_per_element) const {
      return allocated_element_count * bytes_per_element;
    }

    bool HasAllocation() {
      return data_ != nullptr;
    }

    ForAnyElementType(const ForAnyElementType&) = delete;
    ForAnyElementType& operator = (const ForAnyElementType&) = delete;

   private:
    /** A pointer to the container's elements. */
    UntypedContainerElement* data_;
  };

  template <typename ElementType>
  struct ForElementType : public ForAnyElementType {
    /** Default constructor. */
    ForElementType() {}

    FUN_ALWAYS_INLINE ElementType* GetAllocation() const {
      return (ElementType*)ForAnyElementType::GetAllocation();
    }
  };
};

template <>
struct AllocatorTraits<AnsiAllocator> : AllocatorTraitsBase<AnsiAllocator> {
  enum { SupportsMove = true };
  enum { IsZeroConstruct = true };
};


/**
 * ANSI allocator that can be used with a Set.
 */
class AnsiSetAllocator : public SetAllocator<AnsiAllocator, InlineAllocator<1, AnsiAllocator>> {
};

} // namespace fun
