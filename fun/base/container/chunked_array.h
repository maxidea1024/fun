#pragma once

#include "fun/base/base.h"
#include "fun/base/container/array.h"

namespace fun {

/**
 * An array that uses multiple allocations to avoid allocation failure due to
 * fragmentation.
 */
template <typename _ElementType, uint32 TargetBytesPerChunk = 16384>
class ChunkedArray {
 public:
  using ElementType = _ElementType;

  /**
   * Initialization constructor.
   */
  ChunkedArray(int32 count = 0) : count_(count) {
    // Compute the number of chunks needed.
    const int32 chunk_count =
        (count_ + ElementsPerChunk - 1) / ElementsPerChunk;

    // Allocate the chunks.
    chunks_.Clear(chunk_count);
    for (int32 i = 0; i < chunk_count; ++i) {
      new (chunks_) Chunk;
    }
  }

 private:
  template <typename ArrayType>
  FUN_ALWAYS_INLINE static typename EnableIf<
      ContainerTraits<ArrayType>::MoveWillEmptyContainer>::Type
  MoveOrCopy(ArrayType& to_array, ArrayType& from_array) {
    to_array.chunks_ = (ChunksType &&) from_array.chunks_;
    to_array.count_ = from_array.count_;
    from_array.count_ = 0;
  }

  template <typename ArrayType>
  FUN_ALWAYS_INLINE static typename EnableIf<
      !ContainerTraits<ArrayType>::MoveWillEmptyContainer>::Type
  MoveOrCopy(ArrayType& to_array, ArrayType& from_array) {
    to_array = from_array;
  }

 public:
  ChunkedArray(ChunkedArray&& other) { MoveOrCopy(*this, other); }

  ChunkedArray& operator=(ChunkedArray&& other) {
    if (FUN_LIKELY(&other != this)) {
      MoveOrCopy(*this, other);
    }

    return *this;
  }

  ChunkedArray(const ChunkedArray&) = default;
  ChunkedArray& operator=(const ChunkedArray&) = default;

  ElementType& operator[](int32 element_index) {
    const uint32 chunk_index = element_index / ElementsPerChunk;
    const uint32 chunk_element_index = element_index % ElementsPerChunk;
    return chunks_[chunk_index].elements[chunk_element_index];
  }

  const ElementType& operator[](int32 element_index) const {
    const int32 chunk_index = element_index / ElementsPerChunk;
    const int32 chunk_element_index = element_index % ElementsPerChunk;
    return chunks_[chunk_index].elements[chunk_element_index];
  }

  int32 Count() const { return count_; }

  uint32 GetAllocatedSize() const { return chunks_.GetAllocatedSize(); }

  /**
   * Tests if index is valid, i.e. greater than zero and less than number of
   * elements in array.
   *
   * \param index - index to test.
   *
   * \returns True if index is valid. False otherwise.
   */
  FUN_ALWAYS_INLINE bool IsValidIndex(int32 index) const {
    return index >= 0 && index < count_;
  }

  /**
   * Adds a new item to the end of the chunked array.
   *
   * \param item - The item to add
   *
   * \return index to the new item
   */
  int32 AddElement(const ElementType& item) {
    new (*this) ElementType(item);
    return this->count_ - 1;
  }

  /**
   * Appends the specified array to this array.
   * Cannot append to self.
   *
   * \param other - The array to append.
   */
  FUN_ALWAYS_INLINE ChunkedArray& operator+=(const Array<ElementType>& other) {
    if (FUN_LIKELY((uintptr_t*)this != (uintptr_t*)&other)) {
      for (const auto& it : other) {
        AddElement(it);
      }
    }

    return *this;
  }

  FUN_ALWAYS_INLINE ChunkedArray& operator+=(const ChunkedArray& other) {
    if (FUN_LIKELY((uintptr_t*)this != (uintptr_t*)&other)) {
      for (int32 i = 0; i < other.Count(); ++i) {
        AddElement(other[i]);
      }
    }

    return *this;
  }

  int32 Add(int32 count = 1) {
    fun_check(count >= 0);
    fun_check_dbg(count_ >= 0);

    const int32 old_count = count_;
    for (int32 i = 0; i < count; i++) {
      if (count_ % ElementsPerChunk == 0) {
        new (chunks_) Chunk;
      }
      count_++;
    }

    return old_count;
  }

  void Clear(int32 slack = 0) {
    // Compute the number of chunks needed.
    const int32 chunk_count = (slack + ElementsPerChunk - 1) / ElementsPerChunk;
    chunks_.Clear(chunk_count);
    count_ = 0;
  }

  /**
   * Reserves memory such that the array can contain at least Number elements.
   *
   * \param min_capacity - The number of elements that the array should be able
   * to contain after allocation.
   */
  void Reserve(int32 min_capacity) {
    // Compute the number of chunks needed.
    const int32 chunk_count =
        (min_capacity + ElementsPerChunk - 1) / ElementsPerChunk;
    chunks_.Reserve(chunk_count);
  }

  void Shrink() { chunks_.Shrink(); }

 protected:
  friend struct ContainerTraits<ChunkedArray<ElementType, TargetBytesPerChunk>>;

  enum { ElementsPerChunk = TargetBytesPerChunk / sizeof(ElementType) };

  /**
   * A chunk of the array's elements.
   */
  struct Chunk {
    /**
     * The elements in the chunk.
     */
    ElementType elements[ElementsPerChunk];
  };

  /**
   * The chunks of the array's elements.
   */
  typedef IndirectArray<Chunk> ChunksType;
  ChunksType chunks_;

  /**
   * The number of elements in the array.
   */
  int32 count_;
};

template <typename ElementType, uint32 TargetBytesPerChunk>
struct ContainerTraits<ChunkedArray<ElementType, TargetBytesPerChunk>>
    : public ContainerTraitsBase<
          ChunkedArray<ElementType, TargetBytesPerChunk>> {
  enum {
    MoveWillEmptyContainer = ContainerTraits<typename ChunkedArray<
        ElementType, TargetBytesPerChunk>::ChunksType>::MoveWillEmptyContainer
  };
};

}  // namespace fun

// NOTE: 'operator new' must declared in global namespace
template <typename T, fun::uint32 TargetBytesPerChunk>
void* operator new(fun::size_t size,
                   fun::ChunkedArray<T, TargetBytesPerChunk>& chuncked_array) {
  using namespace fun;
  fun_check(size == sizeof(T));
  const int32 index = chuncked_array.Add(1);
  return &chuncked_array[index];
}
