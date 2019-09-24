#pragma once

#include "fun/base/base.h"
#include "fun/base/container/array.h"
#include "fun/base/container/set.h"
#include "fun/base/ftl/algo/reverse.h"

namespace fun {

/**
 * An initializer type for pairs that's passed to the pair set when adding a new pair.
 */
template <typename KeyInitType, typename ValueInitType>
class PairInitializer
{
 public:
  typename RValueToLValueReference<KeyInitType>::Type key;
  typename RValueToLValueReference<ValueInitType>::Type value;

  /**
   * Initialization constructor.
   */
  FUN_ALWAYS_INLINE PairInitializer(KeyInitType key, ValueInitType value)
    : key(key)
    , value(value)
  {}
};


/**
 * An initializer type for keys that's passed to the pair set when adding a new key.
 */
template <typename KeyInitType>
class KeyInitializer
{
 public:
  typename RValueToLValueReference<KeyInitType>::Type key;

  /**
   * Initialization constructor.
   */
  FUN_ALWAYS_INLINE explicit KeyInitializer(KeyInitType key)
    : key(key)
  {}
};


/**
 * A key-value pair in the map.
 */
template <typename KeyType, typename ValueType>
class TPair
{
 public:
  using KeyInitType = typename TypeTraits<KeyType>::ConstInitType;
  using ValueInitType = typename TypeTraits<ValueType>::ConstInitType;

  KeyType key;
  ValueType value;

  /**
   * Initialization constructor.
   */
  template <typename InitKeyType, typename InitValueType>
  FUN_ALWAYS_INLINE TPair(const PairInitializer<InitKeyType, InitValueType>& init)
    : key(StaticCast<InitKeyType>(init.key))
    , value(StaticCast<InitValueType>(init.value))
  {
    // The seemingly-pointless casts above are to enforce a move (i.e. equivalent to using MoveTemp) when
    // the initializers are themselves rvalue references.
  }

  /**
   * key initialization constructor.
   */
  template <typename InitKeyType>
  FUN_ALWAYS_INLINE explicit TPair(const KeyInitializer<InitKeyType>& init)
    : key(StaticCast<InitKeyType>(init.key))
    , value()
  {
    // The seemingly-pointless cast above is to enforce a move (i.e. equivalent to using MoveTemp) when
    // the initializer is itself an rvalue reference.
  }

  FUN_ALWAYS_INLINE TPair() = default;
  FUN_ALWAYS_INLINE TPair(TPair&&) = default;
  FUN_ALWAYS_INLINE TPair(const TPair&) = default;
  FUN_ALWAYS_INLINE TPair& operator=(TPair&&) = default;
  FUN_ALWAYS_INLINE TPair& operator=(const TPair&) = default;

  /**
   * Serializer.
   */
  FUN_ALWAYS_INLINE friend Archive& operator & (Archive& ar, TPair& pair)
  {
    return ar & pair.key & pair.value;
  }

  // Comparison operators
  FUN_ALWAYS_INLINE bool operator == (const TPair& other) const
  {
    return key == other.key && value == other.value;
  }

  FUN_ALWAYS_INLINE bool operator != (const TPair& other) const
  {
    return key != other.key || value != other.value;
  }

  /**
   * Implicit conversion to pair initializer.
   */
  FUN_ALWAYS_INLINE operator PairInitializer<KeyInitType, ValueInitType>() const
  {
    return PairInitializer<KeyInitType, ValueInitType>(key, value);
  }
};


/**
 * Defines how the map's pairs are hashed.
 */
template <typename KeyType, typename ValueType, bool AllowDuplicateKeys>
struct DefaultMapKeyFuncs
  : BaseKeyFuncs<TPair<KeyType, ValueType>, KeyType, AllowDuplicateKeys>
{
  using KeyInitType = typename TypeTraits<KeyType>::ConstPointerType;
  using ElementInitType = const PairInitializer<typename TypeTraits<KeyType>::ConstInitType, typename TypeTraits<ValueType>::ConstInitType>&;

  static FUN_ALWAYS_INLINE KeyInitType GetSetKey(ElementInitType element)
  {
    return element.key;
  }

  static FUN_ALWAYS_INLINE bool Matches(KeyInitType x, KeyInitType y)
  {
    return x == y;
  }

  static FUN_ALWAYS_INLINE uint32 GetKeyHash(KeyInitType key)
  {
    return HashOf(key);
  }
};


/**
 * The base class of maps from keys to values.  Implemented using a Set of key-value pairs with a custom KeyFuncs,
 * with the same O(1) addition, removal, and finding.
 */
template <typename KeyType, typename ValueType, typename SetAllocator, typename KeyFuncs>
class MapBase
{
  template <typename OtherKeyType, typename OtherValueType, typename OtherSetAllocator, typename OtherKeyFuncs>
  friend class MapBase;

  friend struct ContainerTraits<MapBase>;

 public:
  using KeyConstPointerType = typename TypeTraits<KeyType>::ConstPointerType;
  using KeyInitType = typename TypeTraits<KeyType>::ConstInitType;
  using ValueInitType = typename TypeTraits<ValueType>::ConstInitType;

 protected:
  typedef TPair<KeyType, ValueType> PairType;

  MapBase() = default;
  MapBase(MapBase&&) = default;
  MapBase(const MapBase&) = default;
  MapBase& operator=(MapBase&&) = default;
  MapBase& operator=(const MapBase&) = default;

  /**
   * Constructor for moving elements from a Map with a different SetAllocator
   */
  template <typename OtherSetAllocator>
  MapBase(MapBase<KeyType, ValueType, OtherSetAllocator, KeyFuncs>&& other)
    : pairs_(MoveTemp(other.pairs_))
  {}

  /**
   * Constructor for copying elements from a Map with a different SetAllocator
   */
  template <typename OtherSetAllocator>
  MapBase(const MapBase<KeyType, ValueType, OtherSetAllocator, KeyFuncs>& other)
    : pairs_(other.pairs_)
  {}

  /**
   * Assignment operator for moving elements from a Map with a different SetAllocator
   */
  template <typename OtherSetAllocator>
  MapBase& operator = (MapBase<KeyType, ValueType, OtherSetAllocator, KeyFuncs>&& other)
  {
    pairs_ = MoveTemp(other.pairs_);
    return *this;
  }

  /**
   * Assignment operator for copying elements from a Map with a different SetAllocator
   */
  template <typename OtherSetAllocator>
  MapBase& operator = (const MapBase<KeyType, ValueType, OtherSetAllocator, KeyFuncs>& other)
  {
    pairs_ = other.pairs_;
    return *this;
  }

 public:
  // Legacy comparison operators.  Note that these also test whether the map's key-value pairs were added in the same order!
  friend bool LegacyCompareEqual(const MapBase& x, const MapBase& y)
  {
    return LegacyCompareEqual(x.pairs_, y.pairs_);
  }

  friend bool LegacyCompareNotEqual(const MapBase& x, const MapBase& y)
  {
    return LegacyCompareNotEqual(x.pairs_, y.pairs_);
  }

  /**
   * Compare this map with another for equality. Does not make any assumptions about key order.
   * NOTE: this might be a candidate for operator== but it was decided to make it an explicit function
   *  since it can potentially be quite slow.
   *
   * \param other - The other map to compare against
   *
   * \returns True if both this and other contain the same keys with values that compare ==
   */
  bool OrderIndependentCompareEqual(const MapBase& other) const
  {
    // first check counts (they should be the same obviously)
    if (Count() != other.Count()) {
      return false;
    }

    // since we know the counts are the same, we can just iterate one map and check for existence in the other
    for (typename PairSetType::ConstIterator it(pairs_); it; ++it) {
      const ValueType* val2 = other.Find(it->key);
      if (val2 == nullptr) {
        return false;
      }
      if (!(*val2 == it->value)) {
        return false;
      }
    }

    // all fields in A match B and A and B's counts are the same (so there can be no fields in B not in A)
    return true;
  }

  /**
   * Removes all elements from the map, potentially leaving space allocated for an expected number of elements about to be added.
   *
   * \param count - The number of elements about to be added to the set.
   */
  FUN_ALWAYS_INLINE void Clear(int32 count = 0)
  {
    pairs_.Clear(count);
  }

  /**
   * Efficiently empties out the map but preserves all allocations and capacities
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
   * Compacts the pair set to remove holes
   */
  FUN_ALWAYS_INLINE void Compact()
  {
    pairs_.Compact();
  }

  /**
   * Compacts the pair set to remove holes.
   * Does not change the iteration order of the elements.
   */
  FUN_ALWAYS_INLINE void CompactStable()
  {
    pairs_.CompactStable();
  }

  /**
   * Preallocates enough memory to contain Number elements
   */
  FUN_ALWAYS_INLINE void Reserve(int32 min_capacity)
  {
    pairs_.Reserve(min_capacity);
  }

  /**
   * \return The number of elements in the map.
   */
  FUN_ALWAYS_INLINE int32 Count() const
  {
    return pairs_.Count();
  }

  FUN_ALWAYS_INLINE bool IsEmpty() const
  {
    return pairs_.IsEmpty();
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
    Set<KeyType> visited_keys;
    for (typename PairSetType::ConstIterator it(pairs_); it; ++it) {
      if (!visited_keys.Contains(it->key)) {
        out_keys.Add(it->key);
        visited_keys.Add(it->key);
      }
    }

    return out_keys.Count();
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
   * \return A reference to the value as stored in the map.  The reference is only valid until the next change to any key in the map.
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
   * \param value - The value to associate with the key.
   *
   * \return A reference to the value as stored in the map.  The reference is only valid until the next change to any key in the map.
   */
  template <typename InitKeyType, typename InitValueType>
  ValueType& Emplace(InitKeyType&& key, InitValueType&& value)
  {
    const SetElementId pair_id = pairs_.Emplace(PairInitializer<InitKeyType&&, InitValueType&&>(Forward<InitKeyType>(key), Forward<InitValueType>(value)));
    return pairs_[pair_id].value;
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
    const SetElementId pair_id = pairs_.Emplace(KeyInitializer<InitKeyType&&>(Forward<InitKeyType>(key)));
    return pairs_[pair_id].value;
  }

  /**
   * Removes all value associations for a key.
   *
   * \param key - The key to remove associated values for.
   *
   * \return The number of values that were associated with the key.
   */
  FUN_ALWAYS_INLINE int32 Remove(KeyConstPointerType key)
  {
    const int32 removed_pairs_count = pairs_.Remove(key);
    return removed_pairs_count;
  }

  /**
   * Returns the key associated with the specified value.  The time taken is O(N) in the number of pairs.
   *
   * \param value - The value to search for
   *
   * \return A pointer to the key associated with the specified value, or nullptr if the value isn't contained in this map.  The pointer
   *          is only valid until the next change to any key in the map.
   */
  const KeyType* FindKey(ValueInitType value) const
  {
    for (typename PairSetType::ConstIterator pair_it(pairs_); pair_it; ++pair_it) {
      if (pair_it->value == value) {
        return &pair_it->key;
      }
    }
    return nullptr;
  }

  /**
   * Returns the value associated with a specified key.
   *
   * \param key - The key to search for.
   *
   * \return  A pointer to the value associated with the specified key, or nullptr if the key isn't contained in this map.  The pointer
   *          is only valid until the next change to any key in the map.
   */
  FUN_ALWAYS_INLINE ValueType* Find(KeyConstPointerType key)
  {
    if (auto* pair = pairs_.Find(key)) {
      return &pair->value;
    }

    return nullptr;
  }

  FUN_ALWAYS_INLINE const ValueType* Find(KeyConstPointerType key) const
  {
    return const_cast<MapBase*>(this)->Find(key);
  }

 private:
  /**
   * Returns the value associated with a specified key, or if none exists,
   * adds a value using the default constructor.
   *
   * \param key - The key to search for.
   *
   * \return A reference to the value associated with the specified key.
   */
  template <typename ArgType>
  FUN_ALWAYS_INLINE ValueType& FindOrAdd_helper(ArgType&& arg)
  {
    if (auto* pair = pairs_.Find(arg)) {
      return pair->value;
    }

    return Add(Forward<ArgType>(arg));
  }

 public:
  /**
   * Returns the value associated with a specified key, or if none exists,
   * adds a value using the default constructor.
   *
   * \param key - The key to search for.
   *
   * \return A reference to the value associated with the specified key.
   */
  FUN_ALWAYS_INLINE ValueType& FindOrAdd(const KeyType& key)
  {
    return FindOrAdd_helper(key);
  }

  FUN_ALWAYS_INLINE ValueType& FindOrAdd(KeyType&& key)
  {
    return FindOrAdd_helper(MoveTemp(key));
  }

  /**
   * Returns a reference to the value associated with a specified key.
   *
   * \param key - The key to search for.
   *
   * \return The value associated with the specified key, or triggers an assertion if the key does not exist.
   */
  FUN_ALWAYS_INLINE const ValueType& FindChecked(KeyConstPointerType key) const
  {
    const auto* pair = pairs_.Find(key);
    //TODO 예외를 던지는게 바람직해보임...
    fun_check_ptr(pair);
    return pair->value;
  }

  /**
   * Returns a reference to the value associated with a specified key.
   *
   * \param key - The key to search for.
   *
   * \return The value associated with the specified key, or triggers an assertion if the key does not exist.
   */
  FUN_ALWAYS_INLINE ValueType& FindChecked(KeyConstPointerType key)
  {
    auto* pair = pairs_.Find(key);
    //TODO 예외를 던지는게 바람직해보임...
    fun_check(pair);
    return pair->value;
  }

  //TODO FindOrDefault로 이름을 바꾸는게 좋지 않으려나?
  /**
   * Returns the value associated with a specified key.
   *
   * \param key - The key to search for.
   *
   * \return The value associated with the specified key, or the default value for the ValueType if the key isn't contained in this map.
   */
  FUN_ALWAYS_INLINE ValueType FindRef(KeyConstPointerType key) const
  {
    if (const auto* pair = pairs_.Find(key)) {
      return pair->value;
    }

    return ValueType();
  }

  // Like C# Dictionary<T,K>::TryGetValue
  FUN_ALWAYS_INLINE bool TryGetValue(KeyConstPointerType key, ValueType& out_value) const
  {
    if (const auto* pair = pairs_.Find(key)) {
      out_value = pair->value;
      return true;
    }
    return false;
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
    return pairs_.Contains(key);
  }

  /**
   * Generates an array from the keys in this map.
   */
  template <typename ArrayAllocator>
  void GenerateKeyArray(Array<KeyType, ArrayAllocator>& out_keys) const
  {
    out_keys.Clear(pairs_.Count());

    for (typename PairSetType::ConstIterator pair_it(pairs_); pair_it; ++pair_it) {
      new(out_keys) KeyType(pair_it->key);
    }
  }

  /**
   * Generates an array from the values in this map.
   */
  template <typename ArrayAllocator>
  void GenerateValueArray(Array<ValueType, ArrayAllocator>& out_values) const
  {
    out_values.Clear(pairs_.Count());

    for (typename PairSetType::ConstIterator pair_it(pairs_); pair_it; ++pair_it) {
      new(out_values) ValueType(pair_it->value);
    }
  }

  /**
   * Serializer.
   */
  FUN_ALWAYS_INLINE friend Archive& operator & (Archive& ar, MapBase& map)
  {
    return ar & map.pairs_;
  }

  //TODO
  //TODO
  //TODO
  /**
   * Describes the map's contents through an printer.
   *
   * \param out - The printer to describe the map's contents through.
   */
  //void Dump(Printer& out)
  //{
  //  pairs_.Dump(out);
  //}

 protected:
  typedef Set<PairType, KeyFuncs, SetAllocator> PairSetType;

  /**
   * The base of MapBase iterators.
   */
  template <bool IsConst>
  class BaseIterator
  {
   public:
    using PairItType = typename Conditional<IsConst, typename PairSetType::ConstIterator, typename PairSetType::Iterator>::Result;

   private:
    using MapType = typename Conditional<IsConst, const MapBase, MapBase>::Result;
    using ItKeyType = typename Conditional<IsConst, const KeyType, KeyType>::Result;
    using ItValueType = typename Conditional<IsConst, const ValueType, ValueType>::Result;
    using PairType = typename Conditional<IsConst, const typename PairSetType::ElementType, typename PairSetType::ElementType>::Result;

   protected:
    FUN_ALWAYS_INLINE BaseIterator(const PairItType& pair_it)
      : pair_it(pair_it)
    {}

   public:
    FUN_ALWAYS_INLINE BaseIterator& operator ++ ()
    {
      ++pair_it;
      return *this;
    }

    /**
     * conversion to "bool" returning true if the iterator is valid.
     */
    FUN_ALWAYS_INLINE explicit operator bool () const
    {
      return !!pair_it;
    }

    /**
     * inverse of the "bool" operator
     */
    FUN_ALWAYS_INLINE bool operator ! () const
    {
      return !(bool)*this;
    }

    FUN_ALWAYS_INLINE friend bool operator == (const BaseIterator& lhs, const BaseIterator& rhs)
    {
      return lhs.pair_it == rhs.pair_it;
    }

    FUN_ALWAYS_INLINE friend bool operator != (const BaseIterator& lhs, const BaseIterator& rhs)
    {
      return lhs.pair_it != rhs.pair_it;
    }

    FUN_ALWAYS_INLINE ItKeyType& Key() const
    {
      return pair_it->key;
    }

    FUN_ALWAYS_INLINE ItValueType& Value() const
    {
      return pair_it->value;
    }

    FUN_ALWAYS_INLINE PairType& operator * () const
    {
      return *pair_it;
    }

    FUN_ALWAYS_INLINE PairType* operator -> () const
    {
      return &*pair_it;
    }

   protected:
    PairItType pair_it;
  };


  /**
   * The base type of iterators that iterate over the values associated with a specified key.
   */
  template <bool IsConst>
  class BaseKeyIterator
  {
   private:
    using SetItType = typename Conditional<IsConst, typename PairSetType::ConstKeyIterator, typename PairSetType::KeyIterator>::Result;
    using ItKeyType = typename Conditional<IsConst, const KeyType, KeyType>::Result;
    using ItValueType = typename Conditional<IsConst, const ValueType, ValueType>::Result;

   public:
    /**
     * Initialization constructor.
     */
    FUN_ALWAYS_INLINE BaseKeyIterator(const SetItType& set_it)
      : set_it_(set_it)
    {}

    FUN_ALWAYS_INLINE BaseKeyIterator& operator ++ ()
    {
      ++set_it_;
      return *this;
    }

    /**
     * conversion to "bool" returning true if the iterator is valid.
     */
    FUN_ALWAYS_INLINE explicit operator bool () const
    {
      return !!set_it_;
    }

    /**
     * inverse of the "bool" operator
     */
    FUN_ALWAYS_INLINE bool operator ! () const
    {
      return !(bool)*this;
    }

    FUN_ALWAYS_INLINE ItKeyType& Key() const
    {
      return set_it_->key;
    }

    FUN_ALWAYS_INLINE ItValueType& Value() const
    {
      return set_it_->value;
    }

   protected:
    SetItType set_it_;
  };

  /** A set of the key-value pairs in the map. */
  PairSetType pairs_;

 public:
  /**
   * Map iterator.
   */
  class Iterator : public BaseIterator<false>
  {
   public:
    /**
     * Initialization constructor.
     */
    FUN_ALWAYS_INLINE Iterator(MapBase& map, bool requires_rehash_on_removal = false)
      : BaseIterator<false>(begin(map.pairs_))
      , map_(map)
      , elements_has_been_removed_(false)
      , requires_rehash_on_removal_(requires_rehash_on_removal)
    {}

    /**
     * Initialization constructor.
     */
    FUN_ALWAYS_INLINE Iterator(MapBase& map, const typename BaseIterator<false>::PairItType& pair_it)
      : BaseIterator<false>(pair_it)
      , map_(map)
      , elements_has_been_removed_(false)
      , requires_rehash_on_removal_(false)
    {}

    /**
     * Destructor.
     */
    FUN_ALWAYS_INLINE ~Iterator()
    {
      if (elements_has_been_removed_ && requires_rehash_on_removal_) {
        map_.pairs_.Relax();
      }
    }

    /**
     * Removes the current pair from the map.
     */
    FUN_ALWAYS_INLINE void RemoveCurrent()
    {
      BaseIterator<false>::pair_it.RemoveCurrent();
      elements_has_been_removed_ = true;
    }

   private:
    MapBase& map_;
    bool elements_has_been_removed_;
    bool requires_rehash_on_removal_;
  };

  /**
   * Const map iterator.
   */
  class ConstIterator : public BaseIterator<true>
  {
  public:
    FUN_ALWAYS_INLINE ConstIterator(const MapBase& map)
      : BaseIterator<true>(begin(map.pairs_))
    {}

    FUN_ALWAYS_INLINE ConstIterator(const typename BaseIterator<true>::PairItType& pair_it)
      : BaseIterator<true>(pair_it)
    {}
  };

  /**
   * Iterates over values associated with a specified key in a const map.
   */
  class ConstKeyIterator : public BaseKeyIterator<true>
  {
  public:
    FUN_ALWAYS_INLINE ConstKeyIterator(const MapBase& map, KeyInitType key)
      : BaseKeyIterator<true>(typename PairSetType::ConstKeyIterator(map.pairs_, key))
    {}
  };

  /**
   * Iterates over values associated with a specified key in a map.
   */
  class KeyIterator : public BaseKeyIterator<false>
  {
   public:
    FUN_ALWAYS_INLINE KeyIterator(MapBase& map, KeyInitType key)
      : BaseKeyIterator<false>(typename PairSetType::KeyIterator(map.pairs_, key))
    {}

    /**
     * Removes the current key-value pair from the map.
     */
    FUN_ALWAYS_INLINE void RemoveCurrent()
    {
      BaseKeyIterator<false>::set_it_.RemoveCurrent();
    }
  };

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
   * Creates an iterator over the values associated with a specified key in a map
   */
  FUN_ALWAYS_INLINE KeyIterator CreateKeyIterator(KeyInitType key)
  {
    return KeyIterator(*this, key);
  }

  /**
   * Creates a const iterator over the values associated with a specified key in a map
   */
  FUN_ALWAYS_INLINE ConstKeyIterator CreateConstKeyIterator(KeyInitType key) const
  {
    return ConstKeyIterator(*this, key);
  }

 private:
  // DO NOT USE DIRECTLY
  // STL-like iterators to enable range-based for loop support.
  FUN_ALWAYS_INLINE friend Iterator begin(MapBase& map) { return Iterator(map, begin(map.pairs_)); }
  FUN_ALWAYS_INLINE friend ConstIterator begin(const MapBase& map) { return ConstIterator(begin(map.pairs_)); }
  FUN_ALWAYS_INLINE friend Iterator end(MapBase& map) { return Iterator(map, end(map.pairs_)); }
  FUN_ALWAYS_INLINE friend ConstIterator end(const MapBase& map) { return ConstIterator(end(map.pairs_)); }
};


/**
 * The base type of sortable maps.
 */
template <typename KeyType, typename ValueType, typename SetAllocator, typename KeyFuncs>
class SortableMapBase : public MapBase<KeyType, ValueType, SetAllocator, KeyFuncs>
{
  friend struct ContainerTraits<SortableMapBase>;

 protected:
  using BaseType = MapBase<KeyType, ValueType, SetAllocator, KeyFuncs>;

  SortableMapBase() = default;
  SortableMapBase(SortableMapBase&&) = default;
  SortableMapBase(const SortableMapBase&) = default;
  SortableMapBase& operator = (SortableMapBase&&) = default;
  SortableMapBase& operator = (const SortableMapBase&) = default;

  /**
   * Constructor for moving elements from a Map with a different SetAllocator
   */
  template <typename OtherSetAllocator>
  SortableMapBase(SortableMapBase<KeyType, ValueType, OtherSetAllocator, KeyFuncs>&& other)
    : BaseType(MoveTemp(other))
  {}

  /**
   * Constructor for copying elements from a Map with a different SetAllocator
   */
  template <typename OtherSetAllocator>
  SortableMapBase(const SortableMapBase<KeyType, ValueType, OtherSetAllocator, KeyFuncs>& other)
    : BaseType(other)
  {}

  /**
   * Assignment operator for moving elements from a Map with a different SetAllocator
   */
  template <typename OtherSetAllocator>
  SortableMapBase& operator = (SortableMapBase<KeyType, ValueType, OtherSetAllocator, KeyFuncs>&& other)
  {
    (BaseType&)*this = MoveTemp(other);
    return *this;
  }

  /**
   * Assignment operator for copying elements from a Map with a different SetAllocator
   */
  template <typename OtherSetAllocator>
  SortableMapBase& operator = (const SortableMapBase<KeyType, ValueType, OtherSetAllocator, KeyFuncs>& other)
  {
    (BaseType&)*this = other;
    return *this;
  }

 public:
  /**
   * Sorts the pairs array using each pair's key as the sort criteria, then rebuilds the map's hash.
   * Invoked using "MyMapVar.KeySort(Predicate());"
   */
  template <typename Predicate>
  FUN_ALWAYS_INLINE void KeySort(const Predicate& pred)
  {
    BaseType::pairs_.Sort(KeyComparisonClass<Predicate>(pred));
  }

  /**
   * Sorts the pairs array using each pair's value as the sort criteria, then rebuilds the map's hash.
   * Invoked using "MyMapVar.ValueSort(Predicate());"
   */
  template <typename Predicate>
  FUN_ALWAYS_INLINE void ValueSort(const Predicate& pred)
  {
    BaseType::pairs_.Sort(ValueComparisonClass<Predicate>(pred));
  }

 private:
  /**
   * Extracts the pair's key from the map's pair structure and passes
   * it to the user provided comparison class.
   */
  template <typename Predicate>
  class KeyComparisonClass
  {
   public:
    FUN_ALWAYS_INLINE KeyComparisonClass(const Predicate& pred)
      : pred_(pred)
    {}

    FUN_ALWAYS_INLINE bool operator()(const typename BaseType::PairType& a, const typename BaseType::PairType& b) const
    {
      return pred(a.key, b.key);
    }

   private:
    DereferenceWrapper<KeyType, Predicate> pred_;
  };

  /**
   * Extracts the pair's value from the map's pair structure and passes
   * it to the user provided comparison class.
   */
  template <typename Predicate>
  class ValueComparisonClass
  {
   public:
    FUN_ALWAYS_INLINE ValueComparisonClass(const Predicate& pred)
      : pred_(pred)
    {}

    FUN_ALWAYS_INLINE bool operator()(const typename BaseType::PairType& a, const typename BaseType::PairType& b) const
    {
      return pred_(a.value, b.value);
    }

   private:
    DereferenceWrapper<ValueType, Predicate> pred_;
  };
};


class UntypedMap;

/**
 * A MapBase specialization that only allows a single value
 * associated with each key.
 */
template <
      typename KeyType,
      typename ValueType,
      typename SetAllocator /*= DefaultSetAllocator*/,
      typename KeyFuncs /*= DefaultMapKeyFuncs<KeyType, ValueType, false>*/
      >
class Map : public SortableMapBase<KeyType, ValueType, SetAllocator, KeyFuncs>
{
  friend struct ContainerTraits<Map>;
  friend class UntypedMap;

  static_assert(!KeyFuncs::AllowDuplicateKeys, "Map cannot be instantiated with a KeyFuncs which allows duplicate keys");

 public:
  using BaseType = SortableMapBase<KeyType, ValueType, SetAllocator, KeyFuncs>;
  using KeyInitType = typename BaseType::KeyInitType;
  using KeyConstPointerType = typename BaseType::KeyConstPointerType;

  Map() = default;
  Map(Map&&) = default;
  Map(const Map&) = default;
  Map& operator = (Map&&) = default;
  Map& operator = (const Map&) = default;

  /**
   * Constructor for moving elements from a Map with a different SetAllocator
   */
  template <typename OtherSetAllocator>
  Map(Map<KeyType, ValueType, OtherSetAllocator, KeyFuncs>&& other)
    : BaseType(MoveTemp(other))
  {}

  /**
   * Constructor for copying elements from a Map with a different SetAllocator
   */
  template <typename OtherSetAllocator>
  Map(const Map<KeyType, ValueType, OtherSetAllocator, KeyFuncs>& other)
    : BaseType(other)
  {}

  /**
   * Assignment operator for moving elements from a Map with a different SetAllocator
   */
  template <typename OtherSetAllocator>
  Map& operator = (Map<KeyType, ValueType, OtherSetAllocator, KeyFuncs>&& other)
  {
    (BaseType&)*this = MoveTemp(other);
    return *this;
  }

  /**
   * Assignment operator for copying elements from a Map with a different SetAllocator
   */
  template <typename OtherSetAllocator>
  Map& operator = (const Map<KeyType, ValueType, OtherSetAllocator, KeyFuncs>& other)
  {
    (BaseType&)*this = other;
    return *this;
  }

  /**
   * Removes the pair with the specified key and copies the value
   * that was removed to the ref parameter
   *
   * \param key - the key to search for
   * \param out_removed_value - if found, the value that was removed (not modified if the key was not found)
   *
   * \return whether or not the key was found
   */
  FUN_ALWAYS_INLINE bool RemoveAndCopyValue(KeyInitType key, ValueType& out_removed_value)
  {
    const SetElementId pair_id = BaseType::pairs_.FindId(key);
    if (!pair_id.IsValidId()) {
      return false;
    }

    out_removed_value = MoveTemp(BaseType::pairs_[pair_id].value);
    BaseType::pairs_.Remove(pair_id);
    return true;
  }

  /**
   * Finds a pair with the specified key, removes it from the map,
   * and returns the value part of the pair.
   * If no pair was found, an exception is thrown.
   *
   * \param key - the key to search for
   *
   * \return whether or not the key was found
   */
  FUN_ALWAYS_INLINE ValueType FindAndRemoveChecked(KeyConstPointerType key)
  {
    const SetElementId pair_id = BaseType::pairs_.FindId(key);
    fun_check(pair_id.IsValidId());
    ValueType result = MoveTemp(BaseType::pairs_[pair_id].value);
    BaseType::pairs_.Remove(pair_id);
    return result;
  }

  /**
   * Move all items from another map into our map
   * (if any keys are in both, the value from the other map wins)
   * and empty the other map.
   *
   * \param other_map - The other map of items to move the elements from.
   */
  template <typename OtherSetAllocator>
  void Append(Map<KeyType, ValueType, OtherSetAllocator, KeyFuncs>&& other_map)
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
  template <typename OtherSetAllocator>
  void Append(const Map<KeyType, ValueType, OtherSetAllocator, KeyFuncs>& other_map)
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

  //FUN_ALWAYS_INLINE ValueType& operator[](const KeyType& key) { return this->FindOrAdd(key); }
  FUN_ALWAYS_INLINE const ValueType& operator[](KeyConstPointerType key) const
  {
    return this->FindChecked(key);
  }
};


/**
 * A MapBase specialization that allows multiple values
 * to be associated with each key.
 */
template <
      typename KeyType,
      typename ValueType,
      typename SetAllocator /* = DefaultSetAllocator */,
      typename KeyFuncs /*= DefaultMapKeyFuncs<KeyType, ValueType, true>*/
    >
class MultiMap : public SortableMapBase<KeyType, ValueType, SetAllocator, KeyFuncs>
{
  friend struct ContainerTraits<MultiMap>;

  static_assert(KeyFuncs::AllowDuplicateKeys, "MultiMap cannot be instantiated with a KeyFuncs which disallows duplicate keys");

 public:
  using BaseType = SortableMapBase<KeyType, ValueType, SetAllocator, KeyFuncs>;
  using KeyConstPointerType = typename BaseType::KeyConstPointerType;
  using KeyInitType = typename BaseType::KeyInitType;
  using ValueInitType = typename BaseType::ValueInitType;

  MultiMap() = default;
  MultiMap(MultiMap&&) = default;
  MultiMap(const MultiMap&) = default;
  MultiMap& operator = (MultiMap&&) = default;
  MultiMap& operator = (const MultiMap&) = default;

  /**
   * Constructor for moving elements from a Map with a different SetAllocator
   */
  template <typename OtherSetAllocator>
  MultiMap(MultiMap<KeyType, ValueType, OtherSetAllocator, KeyFuncs>&& other)
    : BaseType(MoveTemp(other))
  {}

  /**
   * Constructor for copying elements from a Map with a different SetAllocator
   */
  template <typename OtherSetAllocator>
  MultiMap(const MultiMap<KeyType, ValueType, OtherSetAllocator, KeyFuncs>& other)
    : BaseType(other)
  {}

  /**
   * Assignment operator for moving elements from a Map with a different SetAllocator
   */
  template <typename OtherSetAllocator>
  MultiMap& operator = (MultiMap<KeyType, ValueType, OtherSetAllocator, KeyFuncs>&& other)
  {
    (BaseType&)*this = MoveTemp(other);
    return *this;
  }

  /**
   * Assignment operator for copying elements from a Map with a different SetAllocator
   */
  template <typename OtherSetAllocator>
  MultiMap& operator = (const MultiMap<KeyType, ValueType, OtherSetAllocator, KeyFuncs>& other)
  {
    (BaseType&)*this = other;
    return *this;
  }

  /**
   * Finds all values associated with the specified key.
   *
   * \param key - The key to find associated values for.
   * \param out_values - Upon return, contains the values associated with the key.
   * \param keep_order - true if the Values array should be in the same order as the map's pairs.
   */
  template <typename ArrayAllocator>
  void MultiFind(KeyInitType key, Array<ValueType, ArrayAllocator>& out_values, bool keep_order = false) const
  {
    for (typename BaseType::PairSetType::ConstKeyIterator it(BaseType::pairs_, key); it; ++it) {
      new(out_values) ValueType(it->value);
    }

    if (keep_order) {
      algo::Reverse(out_values);
    }
  }

  /**
   * Finds all values associated with the specified key.
   *
   * \param key - The key to find associated values for.
   * \param out_values - Upon return, contains pointers to the values associated with the key.
   *                  Pointers are only valid until the next change to any key in the map.
   * \param keep_order - true if the Values array should be in the same order as the map's pairs.
   */
  template <typename ArrayAllocator>
  void MultiFindPointer(KeyInitType key, Array<const ValueType*, ArrayAllocator>& out_values, bool keep_order = false) const
  {
    for (typename BaseType::PairSetType::ConstKeyIterator it(BaseType::pairs_, key); it; ++it) {
      new(out_values) const ValueType*(&it->value);
    }

    if (keep_order) {
      algo::Reverse(out_values);
    }
  }

  /**
   * Adds a key-value association to the map.  The association doesn't replace any of the key's existing associations.
   * However, if both the key and value match an existing association in the map, no new association is made and the existing association's
   * value is returned.
   *
   * \param key - The key to associate.
   * \param value - The value to associate.
   *
   * \return A reference to the value as stored in the map; the reference is only valid until the next change to any key in the map.
   */
  FUN_ALWAYS_INLINE ValueType& AddUnique(const KeyType& key, const ValueType& value)
  {
    return EmplaceUnique(key, value);
  }

  FUN_ALWAYS_INLINE ValueType& AddUnique(const KeyType& key, ValueType&& value)
  {
    return EmplaceUnique(key, MoveTemp(value));
  }

  FUN_ALWAYS_INLINE ValueType& AddUnique(KeyType&& key, const ValueType& value)
  {
    return EmplaceUnique(MoveTemp(key), value);
  }

  FUN_ALWAYS_INLINE ValueType& AddUnique(KeyType&& key, ValueType&& value)
  {
    return EmplaceUnique(MoveTemp(key), MoveTemp(value));
  }

  /**
   * Adds a key-value association to the map.  The association doesn't replace any of the key's existing associations.
   * However, if both the key and value match an existing association in the map, no new association is made and the existing association's
   * value is returned.
   *
   * \param key - The key to associate.
   * \param value - The value to associate.
   *
   * \return A reference to the value as stored in the map; the reference is only valid until the next change to any key in the map.
   */
  template <typename InitKeyType, typename InitValueType>
  ValueType& EmplaceUnique(InitKeyType&& key, InitValueType&& value)
  {
    if (ValueType* found = FindPair(key, value)) {
      return *found;
    }

    // If there's no existing association with the same key and value, create one.
    return BaseType::Add(Forward<InitKeyType>(key), Forward<InitValueType>(value));
  }

  /**
   * Removes all value associations for a key.
   *
   * \param key - The key to remove associated values for.
   *
   * \return The number of values that were associated with the key.
   */
  FUN_ALWAYS_INLINE int32 Remove(KeyConstPointerType key)
  {
    return BaseType::Remove(key);
  }

  /**
   * Removes associations between the specified key and value from the map.
   *
   * \param key - The key part of the pair to remove.
   * \param value - The value part of the pair to remove.
   *
   * \return The number of associations removed.
   */
  int32 Remove(KeyInitType key, ValueInitType value)
  {
    // Iterate over pairs with a matching key.
    int32 removed_pair_count = 0;
    for (typename BaseType::PairSetType::KeyIterator it(BaseType::pairs_, key); it; ++it) {
      // If this pair has a matching value as well, remove it.
      if (it->value == value) {
        it.RemoveCurrent();
        ++removed_pair_count;
      }
    }

    return removed_pair_count;
  }

  /**
   * Removes the first association between the specified key and value from the map.
   *
   * \param key - The key part of the pair to remove.
   * \param value - The value part of the pair to remove.
   *
   * \return The number of associations removed.
   */
  int32 RemoveSingle(KeyInitType key, ValueInitType value)
  {
    // Iterate over pairs with a matching key.
    int32 removed_pair_count = 0;
    for (typename BaseType::PairSetType::KeyIterator it(BaseType::pairs_, key); it; ++it) {
      // If this pair has a matching value as well, remove it.
      if (it->value == value) {
        it.RemoveCurrent();
        ++removed_pair_count;

        // We were asked to remove only the first association, so bail out.
        break;
      }
    }

    return removed_pair_count;
  }

  /**
   * Finds an association between a specified key and value. (const)
   *
   * \param key - The key to find.
   * \param value - The value to find.
   *
   * \return If the map contains a matching association, a pointer to the value in the map is returned.  Otherwise nullptr is returned.
   *          The pointer is only valid as long as the map isn't changed.
   */
  FUN_ALWAYS_INLINE const ValueType* FindPair(KeyInitType key, ValueInitType value) const
  {
    return const_cast<MultiMap*>(this)->FindPair(key, value);
  }

  /**
   * Finds an association between a specified key and value.
   *
   * \param key - The key to find.
   * \param value - The value to find.
   *
   * \return If the map contains a matching association, a pointer to the value in the map is returned.  Otherwise nullptr is returned.
   *          The pointer is only valid as long as the map isn't changed.
   */
  ValueType* FindPair(KeyInitType key, ValueInitType value)
  {
    // Iterate over pairs with a matching key.
    for (typename BaseType::PairSetType::KeyIterator it(BaseType::pairs_, key); it; ++it) {
      // If the pair's value matches, return a pointer to it.
      if (it->value == value) {
        return &it->value;
      }
    }

    return nullptr;
  }

  /**
   * Returns the number of values within this map associated with the specified key
   */
  int32 Count(KeyInitType key) const
  {
    // Iterate over pairs with a matching key.
    int32 matched_pair_count = 0;
    for (typename BaseType::PairSetType::ConstKeyIterator it(BaseType::pairs_, key); it; ++it) {
      ++matched_pair_count;
    }
    return matched_pair_count;
  }

  /**
   * Since we implement an overloaded Count() function in MultiMap, we need to reimplement MapBase::Count to make it visible.
   */
  FUN_ALWAYS_INLINE int32 Count() const
  {
    return BaseType::Count();
  }
};


struct UntypedMapLayout {
  int32 key_offset;
  int32 value_offset;

  UntypedSetLayout set_layout;
};


/**
 * Untyped map type for accessing Map data, like UntypedArray for Array.
 * Must have the same memory representation as a Map.
 */
class UntypedMap
{
 public:
  static UntypedMapLayout GetUntypedLayout(int32 key_size, int32 key_alignment, int32 value_size, int32 value_alignment)
  {
    UntypedMapLayout result;
    StructBuilder pair_struct;
    result.key_offset = pair_struct.AddMember(key_size, key_alignment);
    result.value_offset = pair_struct.AddMember(value_size, value_alignment);
    result.set_layout = UntypedSet::GetUntypedLayout(pair_struct.GetSize(), pair_struct.GetAlignment());
    return result;
  }

  UntypedMap() {}

  bool IsValidIndex(int32 index) const
  {
    return pairs_.IsValidIndex(index);
  }

  int32 Count() const
  {
    return pairs_.Count();
  }

  int32 GetMaxIndex() const
  {
    return pairs_.GetMaxIndex();
  }

  void* MutableData(int32 index, const UntypedMapLayout& layout)
  {
    return pairs_.MutableData(index, layout.set_layout);
  }

  const void* ConstData(int32 index, const UntypedMapLayout& layout) const
  {
    return pairs_.ConstData(index, layout.set_layout);
  }

  void Clear(int32 slack, const UntypedMapLayout& layout)
  {
    pairs_.Clear(slack, layout.set_layout);
  }

  void RemoveAt(int32 index, const UntypedMapLayout& layout)
  {
    pairs_.RemoveAt(index, layout.set_layout);
  }

  /**
   * Adds an uninitialized object to the map.
   * The map will need rehashing at some point after this call to make it valid.
   *
   * \return The index of the added element.
   */
  int32 AddUninitialized(const UntypedMapLayout& layout)
  {
    return pairs_.AddUninitialized(layout.set_layout);
  }

  void Rehash(const UntypedMapLayout& layout, TFunctionRef<uint32 (const void*)> get_key_hash)
  {
    pairs_.Rehash(layout.set_layout, get_key_hash);
  }

 private:
  UntypedSet pairs_;

  // This function isn't intended to be called, just to be compiled to validate the correctness of the type.
  static void CheckConstraints()
  {
    typedef UntypedMap RawType;
    typedef Map<int32, int8> RealType;

    // Check that the class footprint is the same
    static_assert(sizeof (RawType) == sizeof(RealType), "UntypedMap's size doesn't match Map");
    static_assert(alignof(RawType) == alignof(RealType), "UntypedMap's alignment doesn't match Map");

    // Check member sizes
    static_assert(sizeof(DeclVal<RawType>().pairs_) == sizeof(DeclVal<RealType>().pairs_), "UntypedMap's pairs_ member size does not match Map's");

    // Check member offsets
    static_assert(offsetof(RawType, pairs_) == offsetof(RealType, pairs_), "UntypedMap's pairs_ member offset does not match Map's");
  }

 public:
  // These should really be private, because they shouldn't be called, but there's a bunch of code
  // that needs to be fixed first.
  UntypedMap(const UntypedMap&) = delete;
  UntypedMap& operator = (const UntypedMap&) = delete;
};

template <> struct IsZeroConstructible<UntypedMap> { enum { Value = true }; };


template <typename KeyType, typename ValueType, typename SetAllocator, typename KeyFuncs>
struct ContainerTraits<Map<KeyType, ValueType, SetAllocator, KeyFuncs>>
  : public ContainerTraitsBase<Map<KeyType, ValueType, SetAllocator, KeyFuncs>>
{
  enum { MoveWillEmptyContainer = ContainerTraits<typename Map<KeyType, ValueType, SetAllocator, KeyFuncs>::PairSetType>::MoveWillEmptyContainer };
};

template <typename KeyType, typename ValueType, typename SetAllocator, typename KeyFuncs>
struct ContainerTraits<MultiMap<KeyType, ValueType, SetAllocator, KeyFuncs>>
  : public ContainerTraitsBase<MultiMap<KeyType, ValueType, SetAllocator, KeyFuncs>>
{
  enum { MoveWillEmptyContainer = ContainerTraits<typename MultiMap<KeyType, ValueType, SetAllocator, KeyFuncs>::PairSetType>::MoveWillEmptyContainer };
};

} // namespace fun
