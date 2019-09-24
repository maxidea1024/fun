#pragma once

#include "fun/base/base.h"

namespace fun {

template <uint16 HashSize, uint16 IndexSize>
class StaticHashTable
{
 public:
  StaticHashTable();

  void Clear();

  // Functions used to search
  uint16 First(uint16 key) const;
  uint16 Next(uint16 index) const;
  bool IsValid(uint16 index) const;

  void Add(uint16 key, uint16 index);
  void Remove(uint16 key, uint16 index);

 protected:
  uint16 hash_[HashSize];
  uint16 next_index_[IndexSize];
};


//
// inlines
//

template <uint16 HashSize, uint16 IndexSize>
FUN_ALWAYS_INLINE StaticHashTable<HashSize, IndexSize>::StaticHashTable()
{
  static_assert((HashSize & (HashSize - 1)) == 0, "hash_ size must be power of 2");
  static_assert(IndexSize - 1 < 0xFFFF, "index 0xFFFF is reserved");

  Clear();
}

template <uint16 HashSize, uint16 IndexSize>
FUN_ALWAYS_INLINE void StaticHashTable<HashSize, IndexSize>::Clear()
{
  UnsafeMemory::Memset(hash_, 0xFF, HashSize * 2);
}

// First in hash chain
template <uint16 HashSize, uint16 IndexSize>
FUN_ALWAYS_INLINE uint16 StaticHashTable<HashSize, IndexSize>::First(uint16 key) const
{
  key &= HashSize - 1;
  return hash_[key];
}

// Next in hash chain
template <uint16 HashSize, uint16 IndexSize>
FUN_ALWAYS_INLINE uint16 StaticHashTable<HashSize, IndexSize>::Next(uint16 index) const
{
  fun_check_dbg(index < IndexSize);
  return next_index_[index];
}

template <uint16 HashSize, uint16 IndexSize>
FUN_ALWAYS_INLINE bool StaticHashTable<HashSize, IndexSize>::IsValid(uint16 index) const
{
  return index != 0xFFFF;
}

template <uint16 HashSize, uint16 IndexSize>
FUN_ALWAYS_INLINE void StaticHashTable<HashSize, IndexSize>::Add(uint16 key, uint16 index)
{
  fun_check_dbg(index < IndexSize);

  key &= HashSize - 1;
  next_index_[index] = hash_[key];
  hash_[key] = index;
}

template <uint16 HashSize, uint16 IndexSize>
FUN_ALWAYS_INLINE void StaticHashTable<HashSize, IndexSize>::Remove(uint16 key, uint16 index)
{
  fun_check_dbg(index < IndexSize);

  key &= HashSize - 1;

  if (hash_[key] == index) {
    // Head of chain
    hash_[key] = next_index_[index];
  }
  else {
    for (uint16 i = hash_[key]; IsValid(i); i = next_index_[i]) {
      if (next_index_[i] == index) {
        // Next = Next->Next
        next_index_[i] = next_index_[index];
        break;
      }
    }
  }
}


/**
Dynamically sized hash table, used to index another data structure.
Vastly simpler and faster than TMap.
Example find:
uint32 key = HashFunction(ID);
for (uint32 i = HashTable.First(key); HashTable.IsValid(i); i = HashTable.Next(i)) {
  if (Array[i].ID == ID) {
    return Array[i];
  }
}
*/
class HashTable
{
 public:
  HashTable(uint16 hash_size = 1024, uint32 index_size = 0);
  ~HashTable();

  void Clear();
  void Free();
  FUN_BASE_API void Resize(uint32 new_index_size);

  // Functions used to search
  uint32 First(uint16 key) const;
  uint32 Next(uint32 index) const;
  bool IsValid(uint32 index) const;

  void Add(uint16 key, uint32 index);
  void Remove(uint16 key, uint32 index);

 protected:
  // Avoids allocating hash until first add
  FUN_BASE_API static uint32 EmptyHash[1];

  uint16 hash_size_;
  uint16 hash_mask_;
  uint32 index_size_;

  uint32* hash_;
  uint32* next_index_;
};


//
// inlines
//

FUN_ALWAYS_INLINE HashTable::HashTable(uint16 hash_size, uint32 index_size)
  : hash_size_(hash_size)
  , hash_mask_(0)
  , index_size_(index_size)
  , hash_(EmptyHash)
  , next_index_(nullptr)
{
  fun_check(MathBase::IsPowerOfTwo(hash_size_));

  if (index_size_) {
    hash_mask_ = hash_size_ - 1;

    hash_ = new uint32[hash_size_];
    next_index_ = new uint32[index_size_];

    UnsafeMemory::Memset(hash_, 0xFF, hash_size_ * 4);
  }
}

FUN_ALWAYS_INLINE HashTable::~HashTable()
{
  Free();
}

FUN_ALWAYS_INLINE void HashTable::Clear()
{
  if (index_size_) {
    UnsafeMemory::Memset(hash_, 0xFF, hash_size_ * 4);
  }
}

FUN_ALWAYS_INLINE void HashTable::Free()
{
  if (index_size_) {
    hash_mask_ = 0;
    index_size_ = 0;

    delete[] hash_;
    hash_ = EmptyHash;

    delete[] next_index_;
    next_index_ = nullptr;
  }
}

// First in hash chain
FUN_ALWAYS_INLINE uint32 HashTable::First(uint16 key) const
{
  key &= hash_mask_;
  return hash_[key];
}

// Next in hash chain
FUN_ALWAYS_INLINE uint32 HashTable::Next(uint32 index) const
{
  fun_check_dbg(index < index_size_);
  return next_index_[index];
}

FUN_ALWAYS_INLINE bool HashTable::IsValid(uint32 index) const
{
  return index != ~0u;
}

FUN_ALWAYS_INLINE void HashTable::Add(uint16 key, uint32 index)
{
  if (index >= index_size_) {
    Resize(MathBase::Max<uint32>(32u, MathBase::RoundUpToPowerOfTwo(index + 1)));
  }

  key &= hash_mask_;
  next_index_[index] = hash_[key];
  hash_[key] = index;
}

FUN_ALWAYS_INLINE void HashTable::Remove(uint16 key, uint32 index)
{
  if (index >= index_size_) {
    return;
  }

  key &= hash_mask_;

  if (hash_[key] == index) {
    // Head of chain
    hash_[key] = next_index_[index];
  }
  else {
    for (uint32 i = hash_[key]; IsValid(i); i = next_index_[i]) {
      if (next_index_[i] == index) {
        // Next = Next->Next
        next_index_[i] = next_index_[index];
        break;
      }
    }
  }
}

} // namespace fun
