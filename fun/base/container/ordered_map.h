#pragma once

#include "fun/base/base.h"
//TODO 제거해도 무방
#include "fun/base/container/container_forward_decls.h"

namespace fun {

// 실제로 해싱에 기반한 Map이 아니고, 정렬되어 있는 배열을 토대로 BinarySearching으로 구현되어 있음.

template <
      typename KeyType,
      typename ValueType,
      typename ArrayAllocator/*=DefaultAllocator*/,
      typename SortPredicate/*=Less<KeyType>*/
    >
class OrderedMap
{
  template <typename OtherKeyType, typename OtherValueType, typename OtherArrayAllocator, typename OtherSortPredicate>
  friend class OrderedMap;
  friend struct ContainerTraits<OrderedMap>;

 public:
  using KeyConstPointerType = typename TypeTraits<KeyType>::ConstPointerType;
  using KeyInitType = typename TypeTraits<KeyType>::ConstInitType;
  using ValueInitType = typename TypeTraits<ValueType>::ConstInitType;
  using ElementType = TPair<KeyType, ValueType>;

  OrderedMap() = default;
  OrderedMap(OrderedMap&&) = default;
  OrderedMap(const OrderedMap&) = default;
  OrderedMap& operator = (OrderedMap&&) = default;
  OrderedMap& operator = (const OrderedMap&) = default;

  /**
   * Constructor for moving elements from a OrderedMap
   * with a different ArrayAllocator
   */
  template <typename OtherArrayAllocator>
  OrderedMap(OrderedMap<KeyType, ValueType, OtherArrayAllocator, SortPredicate>&& other)
    : pairs_(MoveTemp(other.pairs_))
  {}

  /**
   * Constructor for copying elements from a OrderedMap
   * with a different ArrayAllocator
   */
  template <typename OtherArrayAllocator>
  OrderedMap(const OrderedMap<KeyType, ValueType, OtherArrayAllocator, SortPredicate>& other)
    : pairs_(other.pairs_)
  {}

  /**
   * Assignment operator for moving elements from a OrderedMap
   * with a different ArrayAllocator
   */
  template <typename OtherArrayAllocator>
  OrderedMap& operator = (OrderedMap<KeyType, ValueType, OtherArrayAllocator, SortPredicate>&& other)
  {
    pairs_ = MoveTemp(other.pairs_);
    return *this;
  }

  /**
   * Assignment operator for copying elements from a OrderedMap
   * with a different ArrayAllocator
   */
  template <typename OtherArrayAllocator>
  OrderedMap& operator = (const OrderedMap<KeyType, ValueType, OtherArrayAllocator, SortPredicate>& other)
  {
    pairs_ = other.pairs_;
    return *this;
  }

  /**
   * Equality operator. This is efficient because pairs are always sorted
   */
  FUN_ALWAYS_INLINE bool operator == (const OrderedMap& other) const
  {
    return pairs_ == other.pairs_;
  }

  /**
   * Inequality operator. This is efficient because pairs are always sorted
   */
  FUN_ALWAYS_INLINE bool operator != (const OrderedMap& other) const
  {
    return pairs_ != other.pairs_;
  }

  /**
   * Removes all elements from the map, potentially leaving space allocated
   * for an expected number of elements about to be added.
   *
   * \param count - The number of elements about to be added to the set.
   */
  FUN_ALWAYS_INLINE void Clear(int32 count = 0)
  {
    pairs_.Clear(count);
  }

  /**
   * Efficiently empties out the map but preserves all allocations
   * and capacities
   */
  FUN_ALWAYS_INLINE void Reset()
  {
      pairs_.Reset();
  }

  /**
   * Shrinks the pair set to avoid slack.
   */
  FUN_ALWAYS_INLINE void Shrink()
  {
    pairs_.Shrink();
  }

  /**
   * Preallocates enough memory to contain Number elements
   */
  FUN_ALWAYS_INLINE void Reserve(int32 count)
  {
    pairs_.Reserve(count);
  }

  /**
   * \return The number of elements in the map.
   */
  FUN_ALWAYS_INLINE int32 Count() const
  {
    return pairs_.Count();
  }

  /**
   * Helper function to return the amount of memory allocated by this container
   *
   * \return number of bytes allocated by this container
   */
  FUN_ALWAYS_INLINE uint32 GetAllocatedSize() const
  {
    return pairs_.GetAllocatedSize();
  }

  /**
   * Tracks the container's memory use through an archive.
   */
  FUN_ALWAYS_INLINE void CountBytes(Archive& ar)
  {
    pairs_.CountBytes(ar);
  }

  /**
   * Sets the value associated with a key.
   *
   * \param key - The key to associate the value with.
   * \param value - The value to associate with the key.
   *
   * \return A reference to the value as stored in the map.
   *         The reference is only valid until the next change
   *         to any key in the map.
   */
  FUN_ALWAYS_INLINE ValueType& Add(const KeyType& key, const ValueType& value)
  {
    return Emplace(key, value);
  }

  FUN_ALWAYS_INLINE ValueType& Add(const KeyType& key, ValueType&& value)
  {
    return Emplace(key, MoveTemp(value));
  }

  FUN_ALWAYS_INLINE ValueType& Add(KeyType&& key, const ValueType& value)
  {
    return Emplace(MoveTemp(key), value);
  }

  FUN_ALWAYS_INLINE ValueType& Add(KeyType&& key, ValueType&& value)
  {
    return Emplace(MoveTemp(key), MoveTemp(value));
  }

  /**
   * Sets a default value associated with a key.
   *
   * \param key - The key to associate the value with.
   *
   * \return A reference to the value as stored in the map.  The reference is only valid until the next change to any key in the map.
   */
  FUN_ALWAYS_INLINE ValueType& Add(const KeyType& key)
  {
    return Emplace(key);
  }

  FUN_ALWAYS_INLINE ValueType& Add(KeyType&& key)
  {
    return Emplace(MoveTemp(key));
  }

  /**
   * Sets the value associated with a key.
   *
   * \param key - The key to associate the value with.
   *
   * \param value - The value to associate with the key.
   *
   * \return A reference to the value as stored in the map.  The reference is only valid until the next change to any key in the map.
   */
  template <typename InitKeyType, typename InitValueType>
  ValueType& Emplace(InitKeyType&& key, InitValueType&& value)
  {
    ElementType* data_ptr = AllocateMemoryForEmplace(key);
    new(data_ptr) ElementType(PairInitializer<InitKeyType&&, InitValueType&&>(Forward<InitKeyType>(key), Forward<InitValueType>(value)));
    return data_ptr->value;
  }

  /**
   * Sets a default value associated with a key.
   *
   * \param key - The key to associate the value with.
   *
   * \return A reference to the value as stored in the map.  The reference is only valid until the next change to any key in the map.
   */
  template <typename InitKeyType>
  ValueType& Emplace(InitKeyType&& key)
  {
    ElementType* data_ptr = AllocateMemoryForEmplace(key);
    new(data_ptr) ElementType(KeyInitializer<InitKeyType&&>(Forward<InitKeyType>(key)));
    return data_ptr->value;
  }

  /**
   * Removes all value associations for a key.
   *
   * \param key - The key to remove associated values for.
   * \return The number of values that were associated with the key.
   */
  FUN_ALWAYS_INLINE int32 Remove(KeyConstPointerType key)
  {
    const int32 remove_index = FindIndex(key);
    if (remove_index != INVALID_INDEX) {
      pairs_.RemoveAt(remove_index);
      return 1;
    }
    return 0;
  }

  /**
   * Returns the key associated with the specified value.  The time taken is O(N) in the number of pairs.
   *
   * \param  value - The value to search for
   *
   * \return A pointer to the key associated with the specified value, or nullptr if the value isn't contained in this map.  The pointer
   *     is only valid until the next change to any key in the map.
   */
  const KeyType* FindKey(ValueInitType value) const
  {
    for (typename ElementArrayType::ConstIterator pair_it(pairs_); pair_it; ++pair_it) {
      if (pair_it->value == value) {
        return &pair_it->key;
      }
    }
    return nullptr;
  }

  /**
   * Returns the value associated with a specified key.
   *
   * \param  key - The key to search for.
   *
   * \return A pointer to the value associated with the specified key, or nullptr if the key isn't contained in this map.  The pointer
   *     is only valid until the next change to any key in the map.
   */
  FUN_ALWAYS_INLINE ValueType* Find(KeyConstPointerType key)
  {
    const int32 found_index = FindIndex(key);
    if (found_index != INVALID_INDEX) {
      return &pairs_[found_index].value;
    }
    return nullptr;
  }

  FUN_ALWAYS_INLINE const ValueType* Find(KeyConstPointerType key) const
  {
    return const_cast<OrderedMap*>(this)->Find(key);
  }

  /**
   * Returns the value associated with a specified key, or if none exists,
   * adds a value using the default constructor.
   *
   * \param  key - The key to search for.
   *
   * \return A reference to the value associated with the specified key.
   */
  FUN_ALWAYS_INLINE ValueType& FindOrAdd(const KeyType& key)
  {
    return FindOrAddImpl(key);
  }

  FUN_ALWAYS_INLINE ValueType& FindOrAdd(KeyType&& key)
  {
    return FindOrAddImpl(MoveTemp(key));
  }

  /**
   * Returns a reference to the value associated with a specified key.
   *
   * \param  key - The key to search for.
   *
   * \return The value associated with the specified key, or triggers an assertion if the key does not exist.
   */
  FUN_ALWAYS_INLINE ValueType& FindChecked(KeyConstPointerType key)
  {
    ValueType* found = Find(key);
    fun_check_ptr(found); //TODO 예외로 처리하는게 좋을듯...
    return *found;
  }

  FUN_ALWAYS_INLINE const ValueType& FindChecked(KeyConstPointerType key) const
  {
    const ValueType* found = Find(key);
    fun_check_ptr(found); //TODO 예외로 처리하는게 좋을듯...
    return *found;
  }

  /**
   * Returns the value associated with a specified key.
   *
   * \param  key - The key to search for.
   *
   * \return The value associated with the specified key, or the default value for the ValueType if the key isn't contained in this map.
   */
  //TODO 기본 값을 지정할 수 있도록 하자.
  FUN_ALWAYS_INLINE ValueType FindRef(KeyConstPointerType key) const
  {
    if (const ValueType* found = Find(key)) {
      return *found;
    }

    return ValueType();
  }

  /**
   * Checks if map contains the specified key.
   *
   * \param key - The key to check for.
   *
   * \return true if the map contains the key.
   */
  FUN_ALWAYS_INLINE bool Contains(KeyConstPointerType key) const
  {
    return !!Find(key);
  }

  /**
   * Returns the unique keys contained within this map
   *
   * \param out_keys - Upon return, contains the set of unique keys in this map.
   *
   * \return The number of unique keys in the map.
   */
  template <typename ArrayAllocator>
  int32 GetKeys(Array<KeyType, ArrayAllocator>& out_keys) const
  {
    //TODO 가져갈 목록을 비우지 않는게 맞는건지??
    //out_values.Clear(pairs_.Count());

    for (typename ElementArrayType::ConstIterator pair_it(pairs_); pair_it; ++pair_it) {
      new(out_keys) KeyType(pair_it->key);
    }

    return out_keys.Count();
  }

  /**
   * Generates an array from the keys in this map.
   */
  template <typename ArrayAllocator>
  int32 GenerateKeyArray(Array<KeyType, ArrayAllocator>& out_keys) const
  {
    out_keys.Clear(pairs_.Count());

    for (typename ElementArrayType::ConstIterator pair_it(pairs_); pair_it; ++pair_it) {
      new(out_keys) KeyType(pair_it->key);
    }

    return out_keys.Count();
  }

  /**
   * Generates an array from the values in this map.
   */
  template <typename ArrayAllocator>
  int32 GenerateValueArray(Array<ValueType, ArrayAllocator>& out_values) const
  {
    out_values.Clear(pairs_.Count());

    for (typename ElementArrayType::ConstIterator pair_it(pairs_); pair_it; ++pair_it) {
      new(out_values) ValueType(pair_it->value);
    }

    return out_values.Count();
  }

  /**
   * Serializer.
   */
  FUN_ALWAYS_INLINE friend Archive& operator & (Archive& ar, OrderedMap& map)
  {
    ar & map.pairs_;

    if (ar.IsLoading()) {
      // We need to resort, in case the sorting is not consistent with what it was before
      algo::SortBy(map.pairs_, KeyForward(), SortPredicate());
    }
    return ar;
  }

  //TODO
  //TODO
  //TODO
  //TODO
  /**
   * Describes the map's contents through an output device.
   *
   * \param ar - The output device to describe the map's contents through.
   */
  //void Dump(Printer& ar)
  //{
  //  pairs_.Dump(ar);
  //}

  /**
   * Removes the pair with the specified key and copies the value that
   * was removed to the ref parameter
   *
   * \param key - the key to search for
   * \param out_removed_value - if found, the value that was removed
   *                  (not modified if the key was not found)
   *
   * \return whether or not the key was found
   */
  FUN_ALWAYS_INLINE bool RemoveAndCopyValue(KeyInitType key, ValueType& out_removed_value)
  {
    const int32 found_index = FindIndex(key);
    if (found_index == INVALID_INDEX) {
      return false;
    }

    out_removed_value = MoveTemp(pairs_[found_index].value);
    pairs_.RemoveAt(found_index);
    return true;
  }

  /**
   * Finds a pair with the specified key, removes it from the map, and
   * returns the value part of the pair.
   * If no pair was found, an exception is thrown.
   *
   * \param key - the key to search for
   * \return whether or not the key was found
   */
  FUN_ALWAYS_INLINE ValueType FindAndRemoveChecked(KeyConstPointerType key)
  {
    const int32 found_index = FindIndex(key);
    fun_check(found_index != INVALID_INDEX);

    ValueType out_removed_value = MoveTemp(pairs_[found_index].value);
    pairs_.RemoveAt(found_index);
    return out_removed_value;
  }

  /**
   * Move all items from another map into our map
   * (if any keys are in both, the value from the other map wins)
   * and empty the other map.
   *
   * \param other_map - The other map of items to move the elements from.
   */
  template <typename OtherArrayAllocator, typename OtherSortPredicate>
  void Append(OrderedMap<KeyType, ValueType, OtherArrayAllocator, OtherSortPredicate>&& other_map)
  {
    this->Reserve(this->Count() + other_map.Count());

    for (auto& pair : other_map) {
      this->Add(MoveTemp(pair.key), MoveTemp(pair.value));
    }

    other_map.Reset();
  }

  /**
   * Add all items from another map to our map
   * (if any keys are in both, the value from the other map wins)
   *
   * \param other_map - The other map of items to add.
   */
  template <typename OtherArrayAllocator, typename OtherSortPredicate>
  void Append(const OrderedMap<KeyType, ValueType, OtherArrayAllocator, OtherSortPredicate>& other_map)
  {
    this->Reserve(this->Count() + other_map.Count());

    for (auto& pair : other_map) {
      this->Add(pair.key, pair.value);
    }
  }

  FUN_ALWAYS_INLINE ValueType& operator[](KeyConstPointerType key)
  {
    return this->FindChecked(key);
  }

  FUN_ALWAYS_INLINE const ValueType& operator[](KeyConstPointerType key) const
  {
    return this->FindChecked(key);
  }

 private:
  using ElementArrayType = Array<ElementType, ArrayAllocator>;

  /**
   * Implementation of find and add
   */
  template <typename ArgType>
  FUN_ALWAYS_INLINE ValueType& FindOrAddImpl(ArgType&& key)
  {
    if (ValueType* found = Find(key)) {
      return *found;
    }

    return Add(Forward<ArgType>(key));
  }

  /**
   * Find index of key
   */
  FUN_ALWAYS_INLINE int32 FindIndex(KeyConstPointerType key)
  {
    return algo::BinarySearchBy(pairs_, key, KeyForward(), SortPredicate());
  }

  /**
   * Allocates raw memory for emplacing
   */
  template <typename InitKeyType>
  FUN_ALWAYS_INLINE ElementType* AllocateMemoryForEmplace(InitKeyType&& key)
  {
    const int32 insert_index = algo::LowerBoundBy(pairs_, key, KeyForward(), SortPredicate());
    fun_check(insert_index >= 0 && insert_index <= pairs_.Count());

    ElementType* data_ptr = nullptr;
    // Since we returned lower bound we already know key <= insert_index key.
    // So if key is not < insert_index key, they must be equal
    if (pairs_.IsValidIndex(insert_index) && !SortPredicate()(key, pairs_[insert_index].key)) {
      // Replacing element, delete old one
      data_ptr = pairs_.GetData() + insert_index;
      DestructItems(data_ptr, 1);
    }
    else {
      // Adding new one, this may reallocate pairs_
      pairs_.InsertUninitialized(insert_index, 1);
      data_ptr = pairs_.GetData() + insert_index;
    }

    return data_ptr;
  }

  /**
   * Forwards sorting into key of pair
   */
  struct KeyForward
  {
    FUN_ALWAYS_INLINE const KeyType& operator()(const ElementType& pair) const
    {
      return pair.key;
    }
  };

  /**
   * The base of OrderedMap iterators
   */
  template <bool IsConst>
  class BaseIterator
  {
   public:
    using PairItType = typename Conditional<IsConst, typename ElementArrayType::ConstIterator, typename ElementArrayType::Iterator>::Result;

   private:
    using MapType = typename Conditional<IsConst, const OrderedMap, OrderedMap>::Result;
    using ItKeyType = typename Conditional<IsConst, const KeyType, KeyType>::Result;
    using ItValueType = typename Conditional<IsConst, const ValueType, ValueType>::Result;
    using PairType = typename Conditional<IsConst, const typename ElementArrayType::ElementType, typename ElementArrayType::ElementType>::Result ;

   protected:
    FUN_ALWAYS_INLINE BaseIterator(const PairItType& starting_iter)
      : pair_it_(starting_iter)
    {}

   public:
    FUN_ALWAYS_INLINE BaseIterator& operator ++ ()
    {
      ++pair_it_;
      return *this;
    }

    FUN_ALWAYS_INLINE explicit operator bool () const
    {
      return !!pair_it_;
    }

    FUN_ALWAYS_INLINE friend bool operator == (const BaseIterator& lhs, const BaseIterator& rhs)
    {
      return lhs.pair_it_ == rhs.pair_it_;
    }

    FUN_ALWAYS_INLINE friend bool operator != (const BaseIterator& lhs, const BaseIterator& rhs)
    {
      return lhs.pair_it_ != rhs.pair_it_;
    }

    FUN_ALWAYS_INLINE ItKeyType& Key() const
    {
      return pair_it_->key;
    }

    FUN_ALWAYS_INLINE ItValueType& Value() const
    {
      return pair_it_->value;
    }

    FUN_ALWAYS_INLINE PairType& operator * () const
    {
      return *pair_it_;
    }

    FUN_ALWAYS_INLINE PairType* operator -> () const
    {
      return &*pair_it_;
    }

  protected:
    PairItType pair_it_;
  };

  /**
   * An array of the key-value pairs in the map
   */
  ElementArrayType pairs_;

 public:
  /**
   * Map iterator
   */
  class Iterator : public BaseIterator<false>
  {
  public:
    FUN_ALWAYS_INLINE Iterator(OrderedMap& map)
      : BaseIterator<false>(map.pairs_.CreateIterator())
    {}

    FUN_ALWAYS_INLINE Iterator(const typename BaseIterator<false>::PairItType& pair_it)
      : BaseIterator<false>(pair_it)
    {}

    /**
     * Removes the current pair from the map
     */
    FUN_ALWAYS_INLINE void RemoveCurrent()
    {
      BaseIterator<false>::pair_it_.RemoveCurrent();
    }
  };

  /**
   * Const map iterator
   */
  class ConstIterator : public BaseIterator<true>
  {
  public:
    FUN_ALWAYS_INLINE ConstIterator(const OrderedMap& map)
      : BaseIterator<true>(map.pairs_.CreateConstIterator())
    {}

    FUN_ALWAYS_INLINE ConstIterator(const typename BaseIterator<true>::PairItType& pair_it)
      : BaseIterator<true>(pair_it)
    {}
  };

  /**
   * Iterates over values associated with a specified key in a const map.
   * This will be at most one value because keys must be unique
   */
  class ConstKeyIterator : public BaseIterator<true>
  {
   public:
    FUN_ALWAYS_INLINE ConstKeyIterator(const OrderedMap& map, KeyInitType key)
      : BaseIterator<true>(map.pairs_.CreateIterator())
    {
      const int32 new_index = FindIndex(key);

      if (new_index != INVALID_INDEX) {
        BaseIterator<true>::pair_it_ += new_index;
      }
      else {
        BaseIterator<true>::pair_it_.SetToEnd();
      }
    }

    FUN_ALWAYS_INLINE ConstKeyIterator& operator ++ ()
    {
      BaseIterator<true>::pair_it_.SetToEnd();
      return *this;
    }
  };

  /**
   * Iterates over values associated with a specified key in a map.
   * This will be at most one value because keys must be unique
   */
  class KeyIterator : public BaseIterator<false>
  {
   public:
    FUN_ALWAYS_INLINE KeyIterator(OrderedMap& map, KeyInitType key)
      : BaseIterator<false>(map.pairs_.CreateConstIterator())
    {
      const int32 new_index = FindIndex(key);

      if (new_index != INVALID_INDEX) {
        BaseIterator<true>::pair_it_ += new_index;
      }
      else {
        BaseIterator<true>::pair_it_.SetToEnd();
      }
    }

    FUN_ALWAYS_INLINE KeyIterator& operator ++ ()
    {
      BaseIterator<false>::pair_it_.SetToEnd();
      return *this;
    }

    /**
     * Removes the current key-value pair from the map.
     */
    FUN_ALWAYS_INLINE void RemoveCurrent()
    {
      BaseIterator<false>::pair_it_.RemoveCurrent();
      BaseIterator<false>::pair_it_.SetToEnd();
    }
  };

  //아래처럼 하는게 좋지 않을까??
  //
  //FUN_ALWAYS_INLINE iterator begin();
  //FUN_ALWAYS_INLINE const_iterator begin() const;
  //FUN_ALWAYS_INLINE iterator end();
  //FUN_ALWAYS_INLINE const_iterator end() const;

  /**
   * Creates an iterator over all the pairs in this map
   */
  FUN_ALWAYS_INLINE Iterator CreateIterator()
  {
    return Iterator(*this);
  }

  /**
   * Creates a const iterator over all the pairs in this map
   */
  FUN_ALWAYS_INLINE ConstIterator CreateConstIterator() const
  {
    return ConstIterator(*this);
  }

  /**
   * Creates an iterator over the values associated with
   * a specified key in a map.
   * This will be at most one value because keys must be unique
   */
  FUN_ALWAYS_INLINE KeyIterator CreateKeyIterator(KeyInitType key)
  {
    return KeyIterator(*this, key);
  }

  /**
   * Creates a const iterator over the values associated
   * with a specified key in a map.
   * This will be at most one value because keys must be unique
   */
  FUN_ALWAYS_INLINE ConstKeyIterator CreateConstKeyIterator(KeyInitType key) const
  {
    return ConstKeyIterator(*this, key);
  }

  /**
   * Ranged For iterators. Unlike normal Map these are not the same
   * as the normal iterator for performance reasons
   */
  using RangedForIteratorType = typename ElementArrayType::RangedForIteratorType;
  using RangedForConstIteratorType = typename ElementArrayType::RangedForConstIteratorType;

 private:
  // DO NOT USE DIRECTLY
  // STL-like iterators to enable range-based for loop support.
  FUN_ALWAYS_INLINE friend RangedForIteratorType begin(OrderedMap& map) { return begin(map.pairs_); }
  FUN_ALWAYS_INLINE friend RangedForConstIteratorType  begin(const OrderedMap& map) { return begin(map.pairs_); }
  FUN_ALWAYS_INLINE friend RangedForIteratorType end(OrderedMap& map) { return end(map.pairs_); }
  FUN_ALWAYS_INLINE friend RangedForConstIteratorType  end(const OrderedMap& map) { return end(map.pairs_); }
};

template <typename KeyType, typename ValueType, typename ArrayAllocator, typename SortPredicate>
struct ContainerTraits<OrderedMap<KeyType, ValueType, ArrayAllocator, SortPredicate>>
  : public ContainerTraitsBase<OrderedMap<KeyType, ValueType, ArrayAllocator, SortPredicate>>
{
  enum { MoveWillEmptyContainer = ContainerTraits<typename OrderedMap<KeyType, ValueType, ArrayAllocator, SortPredicate>::ElementArrayType>::MoveWillEmptyContainer };
};

} // namespace fun
