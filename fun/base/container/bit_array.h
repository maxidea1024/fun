#pragma once

#include "fun/base/base.h"
#include "fun/base/math/math_base.h"
#include "fun/base/atomics.h"

namespace fun {

// Functions for manipulating bit sets.
struct BitSet {
  /**
   * Clears the next set bit in the mask and returns its index.
   */
  static FUN_ALWAYS_INLINE uint32 GetAndClearNextBit(uint32& mask) {
    const uint32 lowest_bit_mask = mask & (-(int32)mask);
    const uint32 bit_index = MathBase::FloorLog2(lowest_bit_mask);
    mask ^= lowest_bit_mask;
    return bit_index;
  }
};


// Forward declaration.
template <typename Allocator = DefaultBitArrayAllocator>
class BitArray;

template <typename Allocator = DefaultBitArrayAllocator>
class ConstSetBitIterator;

template <typename Allocator = DefaultBitArrayAllocator, typename OtherAllocator = DefaultBitArrayAllocator>
class ConstDualSetBitIterator;

class UntypedBitArray;


/**
 * Serializer (predefined for no friend injection in gcc 411)
 */
template <typename Allocator>
Archive& operator & (Archive& ar, BitArray<Allocator>& bit_array);


/**
 * Used to read/write a bit in the array as a bool.
 */
class BitReference {
 public:
  FUN_ALWAYS_INLINE BitReference(uint32& data, uint32 mask)
    : data_(data)
    , mask_(mask) {}

  FUN_ALWAYS_INLINE explicit operator bool () const {
     return (data_ & mask_) != 0;
  }

  FUN_ALWAYS_INLINE void operator = (const bool bit) {
    if (bit) {
      data_ |= mask_;
    }
    else {
      data_ &= ~mask_;
    }
  }

  FUN_ALWAYS_INLINE void AtomicSet(const bool bit) {
    if (bit) {
      if (!(data_ & mask_)) {
        while (1) {
          uint32 current = data_;
          uint32 desired = current | mask_;
          if (current == desired || Atomics::CompareExchange((volatile int32*)&data_, (int32)desired, (int32)current) == (int32)current) {
            return;
          }
        }
      }
    } else {
      if (data_ & mask_) {
        while (1) {
          uint32 current = data_;
          uint32 desired = current & ~mask_;
          if (current == desired || Atomics::CompareExchange((volatile int32*)&data_, (int32)desired, (int32)current) == (int32)current) {
            return;
          }
        }
      }
    }
  }

  FUN_ALWAYS_INLINE BitReference& operator = (const BitReference& rhs) {
    // As this is emulating a reference, assignment should not rebind,
    // it should write to the referenced bit.
    *this = (bool)rhs;
    return *this;
  }

 private:
  uint32& data_;
  uint32 mask_;
};


/**
 * Used to read a bit in the array as a bool.
 */
class ConstBitReference {
 public:
  FUN_ALWAYS_INLINE ConstBitReference(const uint32& data, uint32 mask)
    : data_(data)
    , mask_(mask) {}

  FUN_ALWAYS_INLINE explicit operator bool () const {
     return (data_ & mask_) != 0;
  }

 private:
  const uint32& data_;
  uint32 mask_;
};


/**
 * Used to reference a bit in an unspecified bit array.
 */
class RelativeBitReference {
 public:
  FUN_ALWAYS_INLINE explicit RelativeBitReference(int32 bit_index)
    : dword_index_(bit_index >> NumBitsPerDWORDLogTwo)
    , mask_(1 << (bit_index & (32 - 1))) {}

  int32 dword_index_;
  uint32 mask_;
};


/**
 * A dynamically sized bit array.
 * An array of Booleans.  They stored in one bit/Boolean.
 * There are iterators that efficiently iterate over only set bits.
 */
template <typename Allocator /*= DefaultBitArrayAllocator*/>
class BitArray {
  friend class UntypedBitArray;

 public:
  template <typename>
  friend class ConstSetBitIterator;

  template <typename, typename>
  friend class ConstDualSetBitIterator;

  /**
   * Minimal initialization constructor.
   *
   * \param initial_bit - The value to initial the bits to.
   * \param bit_count - The initial number of bits in the array.
   */
  explicit BitArray(const bool initial_bit = false, const int32 bit_count = 0)
    : bit_count_(0)
    , bit_capacity_(0) {
    Init(initial_bit, bit_count);
  }

  /**
   * Move constructor.
   */
  FUN_ALWAYS_INLINE BitArray(BitArray&& other) {
    MoveOrCopy(*this, other);
  }

  /**
   * Copy constructor.
   */
  FUN_ALWAYS_INLINE BitArray(const BitArray& other)
    : bit_count_(0)
    , bit_capacity_(0) {
    *this = other;
  }

  /**
   * Move assignment.
   */
  FUN_ALWAYS_INLINE BitArray& operator = (BitArray&& other) {
    if (FUN_LIKELY(&other != this)) {
      MoveOrCopy(*this, other);
    }

    return *this;
  }

  /**
   * Assignment operator.
   */
  FUN_ALWAYS_INLINE BitArray& operator = (const BitArray& other) {
    // check for self assignment since we don't use swap() mechanic
    if (FUN_LIKELY(&other != this)) {
      Clear(other.Count());

      bit_count_ = bit_capacity_ = other.bit_count_;

      if (bit_count_ > 0) {
        const int32 dword_count = MathBase::DivideAndRoundUp(bit_capacity_, 32);
        Realloc(0);
        UnsafeMemory::Memcpy(MutableData(), other.ConstData(), dword_count * sizeof(uint32));
      }
    }

    return *this;
  }

 private:
  template <typename BitArrayType>
  static FUN_ALWAYS_INLINE typename EnableIf<ContainerTraits<BitArrayType>::MoveWillEmptyContainer>::Type
    MoveOrCopy(BitArrayType& to_array, BitArrayType& from_array) {
    to_array.allocator_.MoveToEmpty(from_array.allocator_);

    to_array.bit_count_ = from_array.bit_count_;
    to_array.bit_capacity_ = from_array.bit_capacity_;
    from_array.bit_count_ = 0;
    from_array.bit_capacity_ = 0;
  }

  template <typename BitArrayType>
  static FUN_ALWAYS_INLINE typename EnableIf<!ContainerTraits<BitArrayType>::MoveWillEmptyContainer>::Type
    MoveOrCopy(BitArrayType& to_array, BitArrayType& from_array) {
    to_array = from_array;
  }

 public:
  /**
   * Serializer
   */
  friend Archive& operator & (Archive& ar, BitArray& bit_array) {
    // serialize number of bits
    ar & bit_array.bit_count_;

    if (ar.IsLoading()) {
      // no need for slop when reading
      bit_array.bit_capacity_ = bit_array.bit_count_;

      // allocate room for new bits
      bit_array.Realloc(0);
    }

    // calc the number of dwords for all the bits
    const int32 dword_count = MathBase::DivideAndRoundUp(bit_array.bit_count_, 32);

    // serialize the data as one big chunk
    ar.Serialize(bit_array.MutableData(), dword_count * sizeof(uint32));

    return ar;
  }

  /**
   * Adds a bit to the array with the given value.
   *
   * \return The index of the added bit.
   */
  int32 Add(const bool bit) {
    const int32 index = bit_count_;
    const bool requires_reallocation = (bit_count_ + 1) > bit_capacity_;

    ++bit_count_;

    if (requires_reallocation) {
      // Allocate memory for the new bits.
      const uint32 dword_capacity = allocator_.CalculateSlackGrow(
                                            MathBase::DivideAndRoundUp(bit_count_, 32),
                                            MathBase::DivideAndRoundUp(bit_capacity_, 32),
                                            sizeof(uint32));
      bit_capacity_ = dword_capacity * 32;
      Realloc(bit_count_ - 1);
    }

    (*this)[index] = bit;

    return index;
  }

  /**
   * Removes all bits from the array, potentially leaving space allocated for an expected number of bits about to be added.
   *
   * \param bit_count - The expected number of bits about to be added.
   */
  void Clear(int32 bit_count = 0) {
    bit_count_ = 0;

    // If the expected number of bits doesn't match the allocated number of bits, reallocate.
    if (bit_capacity_ != bit_count) {
      bit_capacity_ = bit_count;
      Realloc(0);
    }
  }

  /**
   * Removes all bits from the array retaining any space already allocated.
   */
  void Reset() {
    // We need this because iterators often use whole DWORDs when masking, which includes off-the-end elements
    UnsafeMemory::Memzero(MutableData(), MathBase::DivideAndRoundUp(bit_count_, 32) * sizeof(uint32));

    bit_count_ = 0;
  }

  /**
   * Resets the array's contents.
   *
   * \param bit - The value to initial the bits to.
   * \param count - The number of bits in the array.
   */
  void Init(const bool bit, int32 count) {
    Clear(count);

    if (count > 0) {
      bit_count_ = count;

      UnsafeMemory::Memset( MutableData(),
                            bit ? 0xFF : 0,
                            MathBase::DivideAndRoundUp(bit_count_, 32) * sizeof(uint32));
    }
  }

  /**
   * Sets or unsets a range of bits within the array.
   *
   * \param index - The index of the first bit to set.
   * \param count - The number of bits to set.
   * \param bit - The value to set the bits to.
   */
  FUN_NO_INLINE void SetRange(int32 index, int32 count, const bool bit) {
    fun_check(index >= 0 && count >= 0 && index + count <= bit_count_);

    if (count == 0) {
      return;
    }

    // Work out which uint32 index to set from, and how many
    uint32 dword_index = index / 32;
    uint32 dword_count = (index + count + 31) / 32 - dword_index;

    // Work out masks for the start/end of the sequence
    uint32 start_mask = 0xFFFFFFFFu << (index % 32);
    uint32 end_mask = 0xFFFFFFFFu >> (32 - (index + count) % 32) % 32;

    uint32* dst = MutableData() + dword_index;
    if (bit) {
      if (dword_count == 1) {
        *dst |= start_mask & end_mask;
      }
      else {
        *dst++ |= start_mask;
        dword_count -= 2;
        while (dword_count) {
          *dst++ = ~0;
          --dword_count;
        }
        *dst |= end_mask;
      }
    }
    else {
      if (dword_count == 1) {
        *dst &= ~(start_mask & end_mask);
      }
      else {
        *dst++ &= ~start_mask;
        dword_count -= 2;
        while (dword_count) {
          *dst++ = 0;
          --dword_count;
        }
        *dst &= ~end_mask;
      }
    }
  }

  /**
   * Removes bits from the array.
   *
   * \param index - The index of the first bit to remove.
   * \param count - The number of consecutive bits to remove.
   */
  void RemoveAt(int32 index, int32 count = 1) {
    fun_check(index >= 0 && index + count <= bit_count_);

    // Until otherwise necessary, this is an obviously correct implementation
    // rather than an efficient implementation.
    Iterator write_it(*this);
    for (ConstIterator read_it(*this); read_it; ++read_it) {
      // If this bit isn't being removed, write it back to the array at its potentially new index.
      if (read_it.GetIndex() < index || read_it.GetIndex() >= index + count) {
        if (write_it.GetIndex() != read_it.GetIndex()) {
          write_it.GetValue() = (bool)read_it.GetValue();
        }
        ++write_it;
      }
    }
    bit_count_ -= count;
  }

  /**
   * Removes bits from the array by swapping them with bits at the end of the array.
   * This is mainly implemented so that other code using Array::RemoveSwap will have
   * matching indices.
   *
   * \param base_index - The index of the first bit to remove.
   * \param count - The number of consecutive bits to remove.
   */
  void RemoveAtSwap(int32 base_index, int32 count = 1) {
    fun_check(base_index >= 0 && base_index + count <= bit_count_);
    if (base_index < bit_count_ - count) {
      // Copy bits from the end to the region we are removing
      for (int32 index = 0; index < count; index++) {
#if FUN_PLATFORM_MAC || FUN_PLATFORM_LINUX
        // Clang compiler doesn't understand the short syntax, so let's be explicit
        const int32 from_index = bit_count_ - count + index;
        ConstBitReference from(ConstData()[from_index / 32], 1 << (from_index & (32 - 1)));

        const int32 to_index = base_index + index;
        BitReference to(MutableData()[to_index / 32], 1 << (to_index & (32 - 1)));

        to = (bool)from;
#else
        (*this)[base_index + index] = (bool)(*this)[bit_count_ - count + index];
#endif
      }
    }

    // Remove the bits from the end of the array.
    RemoveAt(bit_count_ - count, count);
  }

  /**
   * Helper function to return the amount of memory allocated by this container
   *
   * \return number of bytes allocated by this container
   */
  uint32 GetAllocatedSize() const {
    return MathBase::DivideAndRoundUp(bit_capacity_, 32) * sizeof(uint32);
  }

  /**
   * Tracks the container's memory use through an archive.
   */
  void CountBytes(Archive& ar) {
    ar.CountBytes(MathBase::DivideAndRoundUp(bit_count_,    32) * sizeof(uint32),
                  MathBase::DivideAndRoundUp(bit_capacity_, 32) * sizeof(uint32));
  }

  /**
   * Finds the first zero bit in the array, sets it to true, and returns the bit index.
   * If there is none, INVALID_INDEX is returned.
   */
  int32 FindAndSetFirstZeroBit() {
    // Iterate over the array until we see a word with a zero bit.
    uint32* __restrict dword_array = MutableData();
    const int32 dword_count = MathBase::DivideAndRoundUp(Count(), 32);
    int32 dword_index = 0;
    while (dword_index < dword_count && dword_array[dword_index] == 0xFFFFFFFFu) {
      ++dword_index;
    }

    if (dword_index < dword_count) {
      // Flip the bits, then we only need to find the first one bit -- easy.
      const uint32 bits = ~(dword_array[dword_index]);
      const uint32 lowest_bit_mask = (bits) & (-(int32)bits);
      const int32 lowest_bit_index = MathBase::FloorLog2(lowest_bit_mask) + (dword_index << NumBitsPerDWORDLogTwo);
      if (lowest_bit_index < bit_count_) {
        dword_array[dword_index] |= lowest_bit_mask;
        return lowest_bit_index;
      }
    }

    return INVALID_INDEX;
  }

  // Accessors.
  FUN_ALWAYS_INLINE bool IsValidIndex(int32 index) const {
    return index >= 0 && index < bit_count_;
  }

  FUN_ALWAYS_INLINE int32 Count() const {
    return bit_count_;
  }

  FUN_ALWAYS_INLINE BitReference operator[](int32 index) {
    fun_check(index >= 0 && index < bit_count_);
    return BitReference(MutableData()[index / 32], 1 << (index & (32 - 1)));
  }

  FUN_ALWAYS_INLINE const ConstBitReference operator[](int32 index) const {
    fun_check(index >= 0 && index < bit_count_);
    return ConstBitReference(ConstData()[index / 32], 1 << (index & (32 - 1)));
  }

  FUN_ALWAYS_INLINE BitReference AccessCorrespondingBit(const RelativeBitReference& ref) {
    fun_check_dbg(ref.mask_);
    fun_check_dbg(ref.dword_index_ >= 0);
    fun_check_dbg(((uint32)ref.dword_index_ + 1) * 32 - 1 - MathBase::CountLeadingZeros(ref.mask_) < (uint32)bit_count_);
    return BitReference(MutableData()[ref.dword_index_], ref.mask_);
  }

  FUN_ALWAYS_INLINE const ConstBitReference AccessCorrespondingBit(const RelativeBitReference& ref) const {
    fun_check_dbg(ref.mask_);
    fun_check_dbg(ref.dword_index_ >= 0);
    fun_check_dbg(((uint32)ref.dword_index_ + 1) * 32 - 1 - MathBase::CountLeadingZeros(ref.mask_) < (uint32)bit_count_);
    return ConstBitReference(ConstData()[ref.dword_index_], ref.mask_);
  }

  /**
   * BitArray iterator.
   */
  class Iterator : public RelativeBitReference {
   public:
    FUN_ALWAYS_INLINE Iterator(BitArray<Allocator>& array, int32 starting_index = 0)
      : RelativeBitReference(starting_index)
      , array_(array)
      , index_(starting_index)
    {}

    FUN_ALWAYS_INLINE Iterator& operator ++ ()
    {
      ++index_;
      this->mask_ <<= 1;
      if (!this->mask_) {
        // Advance to the next uint32.
        this->mask_ = 1;
        ++this->dword_index;
      }

      return *this;
    }

    /**
     * conversion to "bool" returning true if the iterator is valid.
     */
    FUN_ALWAYS_INLINE explicit operator bool () const
    {
      return index_ < array_.Count();
    }

    /**
     * inverse of the "bool" operator
     */
    FUN_ALWAYS_INLINE bool operator ! () const
    {
      return !(bool)*this;
    }

    FUN_ALWAYS_INLINE BitReference GetValue() const
    {
      return BitReference(array_.ConstData()[this->dword_index], this->mask_);
    }

    FUN_ALWAYS_INLINE int32 GetIndex() const
    {
      return index_;
    }

   private:
    BitArray<Allocator>& array_;
    int32 index_;
  };

  /**
   * Const bit_array iterator.
   */
  class ConstIterator : public RelativeBitReference {
   public:
    FUN_ALWAYS_INLINE ConstIterator(const BitArray<Allocator>& array, int32 starting_index = 0)
      : RelativeBitReference(starting_index)
      , array_(array)
      , index_(starting_index)
    {}

    FUN_ALWAYS_INLINE ConstIterator& operator ++ ()
    {
      ++index_;
      this->mask_ <<= 1;
      if (!this->mask_) {
        // Advance to the next uint32.
        this->mask_ = 1;
        ++this->dword_index;
      }

      return *this;
    }

    /**
     * conversion to "bool" returning true if the iterator is valid.
     */
    FUN_ALWAYS_INLINE explicit operator bool () const
    {
      return index_ < array_.Count();
    }

    /**
     * inverse of the "bool" operator.
     */
    FUN_ALWAYS_INLINE bool operator ! () const
    {
      return !(bool)*this;
    }

    FUN_ALWAYS_INLINE ConstBitReference GetValue() const
    {
      return ConstBitReference(array_.ConstData()[this->dword_index], this->mask_);
    }

    FUN_ALWAYS_INLINE int32 GetIndex() const
    {
      return index_;
    }

   private:
    const BitArray<Allocator>& array_;
    int32 index_;
  };

  /**
   * Const reverse iterator.
   */
  class ConstReverseIterator : public RelativeBitReference {
   public:
    FUN_ALWAYS_INLINE ConstReverseIterator(const BitArray<Allocator>& array)
      : RelativeBitReference(array.Count() - 1)
      , array_(array)
      , index_(array.Count() - 1)
    {}

    FUN_ALWAYS_INLINE ConstReverseIterator& operator ++ ()
    {
      --index_;
      this->mask_ >>= 1;
      if (!this->mask_) {
        // Advance to the next uint32.
        this->mask_ = (1 << (32-1));
        --this->dword_index;
      }

      return *this;
    }

    /**
     * conversion to "bool" returning true if the iterator is valid.
     */
    FUN_ALWAYS_INLINE explicit operator bool () const
    {
      return index_ >= 0;
    }

    /**
     * inverse of the "bool" operator
     */
    FUN_ALWAYS_INLINE bool operator ! () const
    {
      return !(bool)*this;
    }

    FUN_ALWAYS_INLINE ConstBitReference GetValue() const
    {
      return ConstBitReference(array_.ConstData()[this->dword_index], this->mask_);
    }

    FUN_ALWAYS_INLINE int32 GetIndex() const
    {
      return index_;
    }

   private:
    const BitArray<Allocator>& array_;
    int32 index_;
  };

  FUN_ALWAYS_INLINE const uint32* ConstData() const {
    return (uint32*)allocator_.GetAllocation();
  }

  FUN_ALWAYS_INLINE uint32* MutableData() {
    return (uint32*)allocator_.GetAllocation();
  }

 private:
  using AllocatorType = typename Allocator::template ForElementType<uint32>;

  AllocatorType allocator_;
  int32 bit_count_;
  int32 bit_capacity_;

  void Realloc(int32 previous_bit_count) {
    const int32 previous_dword_count = MathBase::DivideAndRoundUp(previous_bit_count, 32);
    const int32 dword_capacity = MathBase::DivideAndRoundUp(bit_capacity_, 32);

    allocator_.ResizeAllocation(previous_dword_count, dword_capacity, sizeof(uint32));

    if (dword_capacity) {
      // Reset the newly allocated slack DWORDs.
      UnsafeMemory::Memzero((uint32*)allocator_.GetAllocation() + previous_dword_count, (dword_capacity - previous_dword_count) * sizeof(uint32));
    }
  }
};

template <typename Allocator>
struct ContainerTraits<BitArray<Allocator>>
  : public ContainerTraitsBase<BitArray<Allocator>> {
  enum { MoveWillEmptyContainer = AllocatorTraits<Allocator>::SupportsMove };
};


/**
 * An iterator which only iterates over set bits.
 */
template <typename Allocator>
class ConstSetBitIterator : public RelativeBitReference {
 public:
  ConstSetBitIterator(const BitArray<Allocator>& array, int32 starting_index = 0)
    : RelativeBitReference(starting_index)
    , array_(array)
    , unvisited_bit_mask_((~0) << (starting_index & (32 - 1)))
    , current_bit_index_(starting_index)
    , base_bit_index_(starting_index & ~(32 - 1)) {
    fun_check(starting_index >= 0 && starting_index <= array_.Count());

    if (starting_index != array_.Count()) {
      FindFirstSetBit();
    }
  }

  /**
   * Forwards iteration operator.
   */
  FUN_ALWAYS_INLINE ConstSetBitIterator& operator ++ () {
    // Mark the current bit as visited.
    unvisited_bit_mask_ &= ~this->mask_;

    // Find the first set bit that hasn't been visited yet.
    FindFirstSetBit();

    return *this;
  }

  FUN_ALWAYS_INLINE friend bool operator == (const ConstSetBitIterator& lhs, const ConstSetBitIterator& rhs) {
    // We only need to compare the bit index and the array... all the rest of the state is unobservable.
    return lhs.current_bit_index_ == rhs.current_bit_index_ && &lhs.array_ == &rhs.array_;
  }

  FUN_ALWAYS_INLINE friend bool operator != (const ConstSetBitIterator& lhs, const ConstSetBitIterator& rhs) {
    return !(lhs == rhs);
  }

  /**
   * conversion to "bool" returning true if the iterator is valid.
   */
  FUN_ALWAYS_INLINE explicit operator bool () const {
    return current_bit_index_ < array_.Count();
  }

  /**
   * inverse of the "bool" operator
   */
  FUN_ALWAYS_INLINE bool operator ! () const {
    return !(bool)*this;
  }

  /**
   * index accessor.
   */
  FUN_ALWAYS_INLINE int32 GetIndex() const {
    return current_bit_index_;
  }

 private:
  const BitArray<Allocator>& array_;

  uint32 unvisited_bit_mask_;
  int32 current_bit_index_;
  int32 base_bit_index_;

  /**
   * Find the first set bit starting with the current bit, inclusive.
   */
  void FindFirstSetBit() {
    const uint32* dword_array = array_.ConstData();
    const int32 bit_count = array_.Count();
    const int32 last_dword_index = (bit_count - 1) / 32;

    // Advance to the next non-zero uint32.
    uint32 remaining_bit_mask = dword_array[this->dword_index_] & unvisited_bit_mask_;
    while (!remaining_bit_mask) {
      ++this->dword_index_;
      base_bit_index_ += 32;
      if (this->dword_index_ > last_dword_index) {
        // We've advanced past the end of the array.
        current_bit_index_ = bit_count;
        return;
      }

      remaining_bit_mask = dword_array[this->dword_index_];
      unvisited_bit_mask_ = ~0;
    }

    // This operation has the effect of unsetting the lowest set bit of BitMask
    const uint32 new_remaining_bit_mask = remaining_bit_mask & (remaining_bit_mask - 1);

    // This operation XORs the above mask with the original mask, which has the effect
    // of returning only the bits which differ; specifically, the lowest bit
    this->mask_ = new_remaining_bit_mask ^ remaining_bit_mask;

    // If the Nth bit was the lowest set bit of BitMask, then this gives us N
    current_bit_index_ = base_bit_index_ + 32 - 1 - MathBase::CountLeadingZeros(this->mask_);

    // If we've accidentally iterated off the end of an array but still within the same DWORD
    // then set the index to the last index of the array
    if (current_bit_index_ > bit_count) {
      current_bit_index_ = bit_count;
    }
  }
};


/**
 * An iterator which only iterates over the bits which are set in both of two bit-arrays.
 */
template <typename Allocator, typename OtherAllocator>
class ConstDualSetBitIterator : public RelativeBitReference {
 public:
  /**
   * Constructor.
   */
  FUN_ALWAYS_INLINE ConstDualSetBitIterator(
                          const BitArray<Allocator>& array_a,
                          const BitArray<OtherAllocator>& array_b,
                          int32 starting_index = 0)
    : RelativeBitReference(starting_index)
    , array_a_(array_a)
    , array_b_(array_b)
    , unvisited_bit_mask_((~0) << (starting_index & (32 - 1)))
    , current_bit_index_(starting_index)
    , base_bit_index_(starting_index & ~(32 - 1)) {
    fun_check(array_a_.Count() == array_b_.Count());

    FindFirstSetBit();
  }

  /**
   * Advancement operator.
   */
  FUN_ALWAYS_INLINE ConstDualSetBitIterator& operator ++ () {
    fun_check_dbg(array_a_.Count() == array_b_.Count());

    // Mark the current bit as visited.
    unvisited_bit_mask_ &= ~this->mask_;

    // Find the first set bit that hasn't been visited yet.
    FindFirstSetBit();

    return *this;
  }

  /**
   * conversion to "bool" returning true if the iterator is valid.
   */
  FUN_ALWAYS_INLINE explicit operator bool () const {
    return current_bit_index_ < array_a_.Count();
  }

  /**
   * inverse of the "bool" operator.
   */
  FUN_ALWAYS_INLINE bool operator ! () const {
    return !(bool)*this;
  }

  /**
   * index accessor.
   */
  FUN_ALWAYS_INLINE int32 GetIndex() const {
    return current_bit_index_;
  }

 private:
  const BitArray<Allocator>& array_a_;
  const BitArray<OtherAllocator>& array_b_;

  uint32 unvisited_bit_mask_;
  int32 current_bit_index_;
  int32 base_bit_index_;

  /**
   * Find the first bit that is set in both arrays, starting with the current bit, inclusive.
   */
  void FindFirstSetBit() {
    static const uint32 empty_array_data = 0;
    const uint32* array_a_data = IfAThenAElseB(array_a_.ConstData(), &empty_array_data);
    const uint32* array_b_data = IfAThenAElseB(array_b_.ConstData(), &empty_array_data);

    // Advance to the next non-zero uint32.
    uint32 remaining_bit_mask = array_a_data[this->dword_index] & array_b_data[this->dword_index] & unvisited_bit_mask_;
    while (!remaining_bit_mask) {
      this->dword_index++;
      base_bit_index_ += 32;
      const int32 last_dword_index = (array_a_.Count() - 1) / 32;
      if (this->dword_index <= last_dword_index) {
        remaining_bit_mask = array_a_data[this->dword_index] & array_b_data[this->dword_index];
        unvisited_bit_mask_ = ~0;
      }
      else {
        // We've advanced past the end of the array.
        current_bit_index_ = array_a_.Count();
        return;
      }
    };

    // We can assume that remaining_bit_mask!=0 here.
    fun_check_dbg(remaining_bit_mask);

    // This operation has the effect of unsetting the lowest set bit of BitMask
    const uint32 new_remaining_bit_mask = remaining_bit_mask & (remaining_bit_mask - 1);

    // This operation XORs the above mask with the original mask, which has the effect
    // of returning only the bits which differ; specifically, the lowest bit
    this->mask_ = new_remaining_bit_mask ^ remaining_bit_mask;

    // If the Nth bit was the lowest set bit of BitMask, then this gives us N
    current_bit_index_ = base_bit_index_ + 32 - 1 - MathBase::CountLeadingZeros(this->mask_);
  }
};


/**
 * Untyped bit array type for accessing BitArray data, like UntypedArray for Array.
 * Must have the same memory representation as a BitArray.
 */
class UntypedBitArray {
 public:
  UntypedBitArray()
    : bit_count_(0)
    , bit_capacity_(0) {}

  bool IsValidIndex(int32 index) const {
    return index >= 0 && index < bit_count_;
  }

  BitReference operator [] (int32 index) {
    fun_check(IsValidIndex(index));
    return BitReference(MutableData()[index / 32], 1 << (index & (32 - 1)));
  }

  ConstBitReference operator [] (int32 index) const {
    fun_check(IsValidIndex(index));
    return ConstBitReference(ConstData()[index / 32], 1 << (index & (32 - 1)));
  }

  void Clear(int32 slack = 0) {
    bit_count_ = 0;

    // If the expected number of bits doesn't match
    // the allocated number of bits, reallocate.
    if (bit_capacity_ != slack) {
      bit_capacity_ = slack;
      Realloc(0);
    }
  }

  int32 Add(const bool bit) {
    const int32 index = bit_count_;
    const bool requires_reallocation = (bit_count_ + 1) > bit_capacity_;

    bit_count_++;

    if (requires_reallocation) {
      // Allocate memory for the new bits.
      const uint32 dword_capacity = allocator_.CalculateSlackGrow(
                                MathBase::DivideAndRoundUp(bit_count_, 32),
                                MathBase::DivideAndRoundUp(bit_capacity_, 32),
                                sizeof(uint32));
      bit_capacity_ = dword_capacity * 32;
      Realloc(bit_count_ - 1);
    }

    (*this)[index] = bit;

    return index;
  }

 private:
  using AllocatorType = DefaultBitArrayAllocator::ForElementType<uint32>;

  AllocatorType allocator_;
  int32 bit_count_;
  int32 bit_capacity_;

  // This function isn't intended to be called, just to be compiled to validate the correctness of the type.
  static void CheckConstraints() {
    typedef UntypedBitArray ScriptType;
    typedef BitArray<> RealType;

    // Check that the class footprint is the same
    static_assert(sizeof (ScriptType) == sizeof (RealType), "UntypedBitArray's size doesn't match BitArray");
    static_assert(alignof(ScriptType) == alignof(RealType), "UntypedBitArray's alignment doesn't match BitArray");

    // Check member sizes
    static_assert(sizeof(DeclVal<ScriptType>().allocator_) == sizeof(DeclVal<RealType>().allocator_), "UntypedBitArray's allocator_ member size does not match BitArray's");
    static_assert(sizeof(DeclVal<ScriptType>().bit_count_) == sizeof(DeclVal<RealType>().bit_count_), "UntypedBitArray's bit_count_ member size does not match BitArray's");
    static_assert(sizeof(DeclVal<ScriptType>().bit_capacity_) == sizeof(DeclVal<RealType>().bit_capacity_), "UntypedBitArray's bit_capacity_ member size does not match BitArray's");

    // Check member offsets
    static_assert(offsetof(ScriptType, allocator_) == offsetof(RealType, allocator_), "UntypedBitArray's allocator_ member offset does not match BitArray's");
    static_assert(offsetof(ScriptType, bit_count_) == offsetof(RealType, bit_count_), "UntypedBitArray's bit_count_ member offset does not match BitArray's");
    static_assert(offsetof(ScriptType, bit_capacity_) == offsetof(RealType, bit_capacity_), "UntypedBitArray's bit_capacity_ member offset does not match BitArray's");
  }

  FUN_ALWAYS_INLINE uint32* MutableData() {
    return (uint32*)allocator_.GetAllocation();
  }

  FUN_ALWAYS_INLINE const uint32* ConstData() const {
    return (const uint32*)allocator_.GetAllocation();
  }

  void Realloc(int32 previous_bit_count) {
    const uint32 dword_capacity = (uint32)allocator_.CalculateSlackReserve(
                            MathBase::DivideAndRoundUp(bit_capacity_, 32),
                            sizeof(uint32));
    bit_capacity_ = dword_capacity * 32;
    const int32 previous_dword_count = MathBase::DivideAndRoundUp(previous_bit_count, 32);

    allocator_.ResizeAllocation(previous_dword_count, dword_capacity, sizeof(uint32));

    if (dword_capacity && dword_capacity - previous_dword_count > 0) {
      // Reset the newly allocated slack DWORDs.
      UnsafeMemory::Memzero((uint32*)allocator_.GetAllocation() + previous_dword_count, (dword_capacity - previous_dword_count) * sizeof(uint32));
    }
  }

  FUN_NO_INLINE void ReallocGrow(int32 previous_bit_count) {
    // Allocate memory for the new bits.
    const uint32 dword_capacity = (uint32)allocator_.CalculateSlackGrow(
                                          MathBase::DivideAndRoundUp(bit_count_, 32),
                                          MathBase::DivideAndRoundUp(bit_capacity_, 32),
                                          sizeof(uint32));
    bit_capacity_ = dword_capacity * 32;
    const int32 previous_dword_count = MathBase::DivideAndRoundUp(previous_bit_count, 32);

    allocator_.ResizeAllocation(previous_dword_count, dword_capacity, sizeof(uint32));

    if (dword_capacity && dword_capacity - previous_dword_count > 0) {
      // Reset the newly allocated slack DWORDs.
      UnsafeMemory::Memzero((uint32*)allocator_.GetAllocation() + previous_dword_count, (dword_capacity - previous_dword_count) * sizeof(uint32));
    }
  }

  UntypedBitArray(const UntypedBitArray&) = delete;
  UntypedBitArray& operator = (const UntypedBitArray&) = delete;
};

template <> struct IsZeroConstructible<UntypedBitArray> { enum { Value = true }; };

} // namespace fun
