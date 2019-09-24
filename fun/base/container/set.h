#pragma once

#include "fun/base/base.h"
#include "fun/base/container/sparse_array.h"
#include "fun/base/container/struct_builder.h"
#include "fun/base/ftl/function.h"
#include "fun/base/ftl/sorting.h"
#include "fun/base/serialization/archive.h"

#include <initializer_list>

namespace fun {

class UntypedSet;

/**
 * The base KeyFuncs type with some useful definitions for all KeyFuncs;
 * meant to be derived from instead of used directly.
 *
 * _AllowDuplicateKeys=true is slightly faster because it allows
 * the Set to skip validating that
 * there isn't already a duplicate entry in the Set.
 */
template <typename _ElementType, typename _KeyType,
          bool _AllowDuplicateKeys = false>
struct BaseKeyFuncs {
  using ElementType = _ElementType;
  using KeyType = _KeyType;

  typedef typename CallTraits<KeyType>::ParamType KeyInitType;
  typedef typename CallTraits<ElementType>::ParamType ElementInitType;

  enum { AllowDuplicateKeys = _AllowDuplicateKeys };
};

/**
 * A default implementation of the KeyFuncs used by
 * Set which uses the element as a key.
 */
template <typename ElementType, bool AllowDuplicateKeys = false>
struct DefaultKeyFuncs
    : BaseKeyFuncs<ElementType, ElementType, AllowDuplicateKeys> {
  using KeyInitType = typename CallTraits<ElementType>::ParamType;
  using ElementInitType = typename CallTraits<ElementType>::ParamType;

  /**
   * Returns The key used to index the given element.
   */
  static FUN_ALWAYS_INLINE KeyInitType GetSetKey(ElementInitType element) {
    return element;
  }

  /**
   * Returns True if the keys match.
   */
  static FUN_ALWAYS_INLINE bool Matches(KeyInitType a, KeyInitType b) {
    return a == b;
  }

  /**
   * Calculates a hash index for a key.
   */
  static FUN_ALWAYS_INLINE uint32 GetKeyHash(KeyInitType key) {
    return fun::HashOf(key);
  }
};

// Forward declaration.
template <typename _ElementType,
          typename KeyFuncs = DefaultKeyFuncs<_ElementType>,
          typename Allocator = DefaultSetAllocator>
class Set;

/**
 * This is used to provide type specific behavior for a move which will destroy
 * `b`. Should be in FunTemplate but isn't for Clang build reasons - will move
 * later
 */
template <typename T>
FUN_ALWAYS_INLINE void MoveByRelocate(T& a, T& b) {
  // Destruct the previous value of `a`.
  a.~T();

  // Relocate `b` into the `hole` left by the destruction of `a`, leaving a hole
  // in `b` instead.
  RelocateConstructItems<T>(&a, &b, 1);
}

/**
 * Either NULL or an identifier for an element of a set.
 */
class SetElementId {
 public:
  template <typename, typename, typename>
  friend class Set;

  friend class UntypedSet;

  /**
   * Default constructor.
   */
  FUN_ALWAYS_INLINE SetElementId() : index_(INVALID_INDEX) {}

  /**
   * Returns a boolean value representing whether the id is NULL.
   */
  FUN_ALWAYS_INLINE bool IsValidId() const { return index_ != INVALID_INDEX; }

  /**
   * Comparison operator.
   */
  FUN_ALWAYS_INLINE friend bool operator==(const SetElementId& x,
                                           const SetElementId& y) {
    return x.index_ == y.index_;
  }

  FUN_ALWAYS_INLINE int32 AsInteger() const { return index_; }

  FUN_ALWAYS_INLINE static SetElementId FromInteger(int32 integer) {
    return SetElementId(integer);
  }

 private:
  /** The index of the element in the set's element array. */
  int32 index_;

  /**
   * Initialization constructor.
   */
  FUN_ALWAYS_INLINE SetElementId(int32 index) : index_(index) {}

  /**
   * Implicit conversion to the element index.
   */
  FUN_ALWAYS_INLINE operator int32() const { return index_; }
};

/**
 * An element in the set.
 */
template <typename _ElementType>
class SetElement {
 public:
  using ElementType = _ElementType;

  /** The element's value. */
  ElementType value;

  /** The id of the next element in the same hash bucket. */
  mutable SetElementId hash_next_id;

  /** The hash bucket that the element is currently linked to. */
  mutable int32 hash_index;

  /** Default constructor. */
  FUN_ALWAYS_INLINE SetElement() {}

  /** Initialization constructor. */
  template <typename InitType>
  FUN_ALWAYS_INLINE SetElement(const InitType& value) : value(value) {}

  template <typename InitType>
  FUN_ALWAYS_INLINE SetElement(InitType&& value) : value(MoveTemp(value)) {}

  /** rhs/move constructors */
  FUN_ALWAYS_INLINE SetElement(const SetElement& rhs)
      : value(rhs.value),
        hash_next_id(rhs.hash_next_id),
        hash_index(rhs.hash_index) {}

  FUN_ALWAYS_INLINE SetElement(SetElement&& rhs)
      : value(MoveTemp(rhs.value)),
        hash_next_id(MoveTemp(rhs.hash_next_id)),
        hash_index(rhs.hash_index) {}

  /** rhs/move assignment */
  FUN_ALWAYS_INLINE SetElement& operator=(const SetElement& rhs) {
    value = rhs.value;
    hash_next_id = rhs.hash_next_id;
    hash_index = rhs.hash_index;
    return *this;
  }

  FUN_ALWAYS_INLINE SetElement& operator=(SetElement&& rhs) {
    value = MoveTemp(rhs.value);
    hash_next_id = MoveTemp(rhs.hash_next_id);
    hash_index = rhs.hash_index;
    return *this;
  }

  /**
   * Serializer.
   */
  FUN_ALWAYS_INLINE friend Archive& operator&(Archive& ar,
                                              SetElement& element) {
    return ar & element.value;
  }

  // Comparison operators
  FUN_ALWAYS_INLINE bool operator==(const SetElement& other) const {
    return value == other.value;
  }

  FUN_ALWAYS_INLINE bool operator!=(const SetElement& other) const {
    return value != other.value;
  }
};

/**
 * A set with an optional KeyFuncs parameters for customizing how the elements
 * are compared and searched. E.g. You can specify a mapping from elements to
 * keys if you want to find elements by specifying a subset of the element type.
 * it uses a SparseArray of the elements, and also links the elements into a
 * hash with a number of buckets proportional to the number of elements.
 * Addition, removal, and finding are O(1).
 */
template <typename _ElementType,
          typename KeyFuncs /*= DefaultKeyFuncs<ElementType>*/,
          typename Allocator /*= DefaultSetAllocator*/
          >
class Set {
 public:
  using ElementType = _ElementType;

 private:
  friend struct ContainerTraits<Set>;
  friend class UntypedSet;

  using KeyInitType = typename KeyFuncs::KeyInitType;
  using ElementInitType = typename KeyFuncs::ElementInitType;
  using SetElementType = SetElement<ElementType>;

 public:
  /**
   * Initialization constructor.
   */
  FUN_ALWAYS_INLINE Set() : hash_size_(0) {}

  /**
   * Copy constructor.
   */
  FUN_ALWAYS_INLINE Set(const Set& rhs) : hash_size_(0) { *this = rhs; }

  template <typename ArrayAllocator>
  FUN_ALWAYS_INLINE explicit Set(
      const Array<ElementType, ArrayAllocator>& array)
      : hash_size_(0) {
    Append(array);
  }

  template <typename ArrayAllocator>
  FUN_ALWAYS_INLINE explicit Set(Array<ElementType, ArrayAllocator>&& array)
      : hash_size_(0) {
    Append(MoveTemp(array));
  }

  /** Destructor. */
  FUN_ALWAYS_INLINE ~Set() { hash_size_ = 0; }

  /** Assignment operator. */
  Set& operator=(const Set& rhs) {
    if (FUN_LIKELY(&rhs != this)) {
      Clear(rhs.Count());

      for (ConstIterator it(rhs); it; ++it) {
        Add(*it);
      }
    }

    return *this;
  }

 private:
  template <typename SetType>
  static FUN_ALWAYS_INLINE
      typename EnableIf<ContainerTraits<SetType>::MoveWillEmptyContainer>::Type
      MoveOrCopy(SetType& to_set, SetType& from_set) {
    to_set.elements_ = (ElementArrayType &&) from_set.elements_;

    to_set.hash_.MoveToEmpty(from_set.hash_);

    to_set.hash_size_ = from_set.hash_size_;
    from_set.hash_size_ = 0;
  }

  template <typename SetType>
  static FUN_ALWAYS_INLINE
      typename EnableIf<!ContainerTraits<SetType>::MoveWillEmptyContainer>::Type
      MoveOrCopy(SetType& to_set, SetType& from_set) {
    to_set = from_set;
  }

 public:
  /**
   * Initializer list constructor.
   */
  Set(std::initializer_list<ElementType> init_list) : hash_size_(0) {
    Append(init_list);
  }

  /**
   * Move constructor.
   */
  Set(Set&& other) : hash_size_(0) { MoveOrCopy(*this, other); }

  /**
   * Move assignment operator.
   */
  Set& operator=(Set&& other) {
    if (FUN_LIKELY(&other != this)) {
      MoveOrCopy(*this, other);
    }

    return *this;
  }

  /**
   * Constructor for moving elements from a Set with a different SetAllocator
   */
  template <typename OtherAllocator>
  Set(Set<ElementType, KeyFuncs, OtherAllocator>&& other) : hash_size_(0) {
    Append(MoveTemp(other));
  }

  /**
   * Constructor for copying elements from a Set with a different SetAllocator
   */
  template <typename OtherAllocator>
  Set(const Set<ElementType, KeyFuncs, OtherAllocator>& other) : hash_size_(0) {
    Append(other);
  }

  /**
   * Assignment operator for moving elements from a Set with a different
   * SetAllocator
   */
  template <typename OtherAllocator>
  Set& operator=(Set<ElementType, KeyFuncs, OtherAllocator>&& other) {
    Reset();
    Append(MoveTemp(other));
    return *this;
  }

  /**
   * Assignment operator for copying elements from a Set with a different
   * SetAllocator
   */
  template <typename OtherAllocator>
  Set& operator=(const Set<ElementType, KeyFuncs, OtherAllocator>& other) {
    Reset();
    Append(other);
    return *this;
  }

  /**
   * Initializer list assignment operator.
   */
  Set& operator=(std::initializer_list<ElementType> init_list) {
    Reset();
    Append(init_list);
    return *this;
  }

  /**
   * Removes all elements from the set, potentially leaving space
   * allocated for an expected number of elements about to be added.
   *
   * \param count - The number of elements about to be added to the set.
   */
  void Clear(int32 count = 0) {
    // Clear the elements array, and reallocate it for the expected number of
    // elements.
    elements_.Clear(count);

    // Resize the hash to the desired size for the expected number of elements.
    if (!ConditionalRehash(count, true)) {
      // If the hash was already the desired size, clear the references to the
      // elements that have now been removed.
      for (int32 hash_index = 0, local_hash_size = hash_size_;
           hash_index < local_hash_size; ++hash_index) {
        GetTypedHash(hash_index) = SetElementId();
      }
    }
  }

  /**
   * Efficiently empties out the set but preserves
   * all allocations and capacities
   */
  void Reset() {
    // Reset the elements array.
    elements_.Reset();

    // Clear the references to the elements that have now been removed.
    for (int32 hash_index = 0, local_hash_size = hash_size_;
         hash_index < local_hash_size; ++hash_index) {
      GetTypedHash(hash_index) = SetElementId();
    }
  }

  /**
   * Shrinks the set's element storage to avoid slack.
   */
  FUN_ALWAYS_INLINE void Shrink() {
    elements_.Shrink();
    Relax();
  }

  /**
   * Compacts the allocated elements into a contiguous range.
   */
  FUN_ALWAYS_INLINE void Compact() {
    if (elements_.Compact()) {
      Rehash();
    }
  }

  /**
   * Compacts the allocated elements into a contiguous range.
   *  Does not change the iteration order of the elements.
   */
  FUN_ALWAYS_INLINE void CompactStable() {
    if (elements_.CompactStable()) {
      Rehash();
    }
  }

  /**
   * Preallocates enough memory to contain Number elements
   */
  FUN_ALWAYS_INLINE void Reserve(int32 capacity) {
    // makes sense only when capacity > elements_.Count() since
    // SparseArray::Reserve does any work only if that's the case
    if (capacity > elements_.Count()) {
      elements_.Reserve(capacity);
    }
  }

  /**
   * Relaxes the set's hash to a size strictly bounded by
   * the number of elements in the Set.
   */
  FUN_ALWAYS_INLINE void Relax() { ConditionalRehash(elements_.Count(), true); }

  /**
   * Helper function to return the amount of memory allocated
   * by this container
   *
   *  \return number of bytes allocated by this container
   */
  FUN_ALWAYS_INLINE uint32 GetAllocatedSize() const {
    return elements_.GetAllocatedSize() + (hash_size_ * sizeof(SetElementId));
  }

  /**
   * Tracks the container's memory use through an archive.
   */
  FUN_ALWAYS_INLINE void CountBytes(Archive& ar) {
    elements_.CountBytes(ar);
    ar.CountBytes(hash_size_ * sizeof(int32),
                  hash_size_ * sizeof(SetElementId));
  }

  /**
   * Returns the number of elements.
   */
  FUN_ALWAYS_INLINE int32 Count() const { return elements_.Count(); }

  FUN_ALWAYS_INLINE bool IsEmpty() const { return elements_.IsEmpty(); }

  /**
   * Checks whether an element id is valid.
   *
   * \param id - The element id to check.
   *
   * Returns true if the element identifier refers to a valid element in this
   * set.
   */
  FUN_ALWAYS_INLINE bool IsValidId(SetElementId id) const {
    return id.IsValidId() && id >= 0 && id < elements_.GetMaxIndex() &&
           elements_.IsAllocated(id);
  }

  /**
   * Accesses the identified element's value.
   */
  FUN_ALWAYS_INLINE ElementType& operator[](SetElementId id) {
    return elements_[id].value;
  }

  /**
   * Accesses the identified element's value.
   */
  FUN_ALWAYS_INLINE const ElementType& operator[](SetElementId id) const {
    return elements_[id].value;
  }

  /**
   * Adds an element to the set.
   *
   * \param element - element to add to set
   * \param out_is_already_in_set - [out] Optional pointer to bool that will be
   * set depending on whether element is already in set
   *
   * Returns A pointer to the element stored in the set.
   */
  FUN_ALWAYS_INLINE SetElementId Add(const ElementType& element,
                                     bool* out_is_already_in_set = nullptr) {
    return Emplace(element, out_is_already_in_set);
  }

  FUN_ALWAYS_INLINE SetElementId Add(ElementType&& element,
                                     bool* out_is_already_in_set = nullptr) {
    return Emplace(MoveTemp(element), out_is_already_in_set);
  }

  /**
   * Adds an element to the set.
   *
   * \param args - The argument(s) to be forwarded to the set element's
   * constructor. \param out_is_already_in_set - [out] Optional pointer to bool
   * that will be set depending on whether element is already in set
   *
   * Returns A pointer to the element stored in the set.
   */
  template <typename ArgsType>
  SetElementId Emplace(ArgsType&& args, bool* out_is_already_in_set = nullptr) {
    // Create a new element.
    SparseArrayAllocationInfo element_allocation = elements_.AddUninitialized();
    SetElementId element_id(element_allocation.index);
    auto& element =
        *new (element_allocation) SetElementType(Forward<ArgsType>(args));

    bool is_already_in_set = false;
    if (!KeyFuncs::AllowDuplicateKeys) {
      // If the set doesn't allow duplicate keys, check for an existing element
      // with the same key as the element being added.

      // Don't bother searching for a duplicate if this is the first element
      // we're adding
      if (elements_.Count() != 1) {
        SetElementId existing_id = FindId(KeyFuncs::GetSetKey(element.value));
        is_already_in_set = existing_id.IsValidId();
        if (is_already_in_set) {
          // If there's an existing element with the same key as the new
          // element, replace the existing element with the new element.
          MoveByRelocate(elements_[existing_id].value, element.value);

          // Then remove the new element.
          elements_.RemoveAtUninitialized(element_id);

          // Then point the return value at the replaced element.
          element_id = existing_id;
        }
      }
    }

    if (!is_already_in_set) {
      // Check if the hash needs to be resized.
      if (!ConditionalRehash(elements_.Count())) {
        // If the rehash didn't add the new element to the hash, add it.
        HashElement(element_id, element);
      }
    }

    if (out_is_already_in_set) {
      *out_is_already_in_set = is_already_in_set;
    }

    return element_id;
  }

  template <typename ArrayAllocator>
  void Append(const Array<ElementType, ArrayAllocator>& array) {
    Reserve(elements_.Count() + array.Count());
    for (auto& element : array) {
      Add(element);
    }
  }

  template <typename ArrayAllocator>
  void Append(Array<ElementType, ArrayAllocator>&& array) {
    Reserve(elements_.Count() + array.Count());
    for (auto& element : array) {
      Add(MoveTemp(element));
    }

    array.Reset();
  }

  /**
   * Add all items from another set to our set (union without creating a new
   * set)
   *
   * \param set - The other set of items to add.
   */
  template <typename OtherAllocator>
  void Append(const Set<ElementType, KeyFuncs, OtherAllocator>& set) {
    Reserve(elements_.Count() + set.Count());
    for (auto& element : set) {
      Add(element);
    }
  }

  template <typename OtherAllocator>
  void Append(Set<ElementType, KeyFuncs, OtherAllocator>&& set) {
    Reserve(elements_.Count() + set.Count());
    for (auto& element : set) {
      Add(MoveTemp(element));
    }

    set.Reset();
  }

  void Append(std::initializer_list<ElementType> init_list) {
    Reserve(elements_.Count() + (int32)init_list.size());
    for (const ElementType& element : init_list) {
      Add(element);
    }
  }

  /**
   * Removes an element from the set.
   *
   * \param element - A pointer to the element in the set, as returned by Add or
   * Find.
   */
  void Remove(SetElementId element_id) {
    if (elements_.Count()) {
      const auto& element_being_removed = elements_[element_id];

      // Remove the element from the hash.
      for (SetElementId* next_element_id =
               &GetTypedHash(element_being_removed.hash_index);
           next_element_id->IsValidId();
           next_element_id = &elements_[*next_element_id].hash_next_id) {
        if (*next_element_id == element_id) {
          *next_element_id = element_being_removed.hash_next_id;
          break;
        }
      }
    }

    // Remove the element from the elements array.
    elements_.RemoveAt(element_id);
  }

  /**
   * Finds an element with the given key in the set.
   *
   * \param key - The key to search for.
   *
   * Returns The id of the set element matching the given key, or the NULL id if
   * none matches.
   */
  SetElementId FindId(KeyInitType key) const {
    if (elements_.Count() > 0) {
      for (SetElementId element_id = GetTypedHash(KeyFuncs::GetKeyHash(key));
           element_id.IsValidId();
           element_id = elements_[element_id].hash_next_id) {
        if (KeyFuncs::Matches(KeyFuncs::GetSetKey(elements_[element_id].value),
                              key)) {
          // Return the first match, regardless of whether the set has multiple
          // matches for the key or not.
          return element_id;
        }
      }
    }

    return SetElementId();
  }

  /**
   * Finds an element with the given key in the set.
   *
   * \param key - The key to search for.
   *
   * \return A pointer to an element with the given key.  If no element in the
   * set has the given key, this will return NULL.
   */
  FUN_ALWAYS_INLINE ElementType* Find(KeyInitType key) {
    SetElementId element_id = FindId(key);
    if (element_id.IsValidId()) {
      return &elements_[element_id].value;
    } else {
      return nullptr;
    }
  }

  /**
   * Finds an element with the given key in the set.
   *
   * \param key - The key to search for.
   *
   * Returns A const pointer to an element with the given key.
   * If no element in the set has the given key, this will return NULL.
   */
  FUN_ALWAYS_INLINE const ElementType* Find(KeyInitType key) const {
    SetElementId element_id = FindId(key);
    if (element_id.IsValidId()) {
      return &elements_[element_id].value;
    } else {
      return nullptr;
    }
  }

  /**
   * Removes all elements from the set matching the specified key.
   *
   * \param key - The key to match elements against.
   *
   * Returns The number of elements removed.
   */
  int32 Remove(KeyInitType key) {
    int32 removed_count = 0;

    if (elements_.Count() > 0) {
      SetElementId* next_element_id = &GetTypedHash(KeyFuncs::GetKeyHash(key));
      while (next_element_id->IsValidId()) {
        auto& element = elements_[*next_element_id];
        if (KeyFuncs::Matches(KeyFuncs::GetSetKey(element.value), key)) {
          // This element matches the key, remove it from the set.
          // Note that Remove sets *next_element_id to point to the next
          // element after the removed element in the hash bucket.
          Remove(*next_element_id);
          ++removed_count;

          if (!KeyFuncs::AllowDuplicateKeys) {
            // If the hash disallows duplicate keys, we're done removing after
            // the first matched key.
            break;
          }
        } else {
          next_element_id = &element.hash_next_id;
        }
      }
    }

    return removed_count;
  }

  /**
   * Checks if the element contains an element with the given key.
   *
   * \param key - The key to check for.
   *
   * Returns true if the set contains an element with the given key.
   */
  FUN_ALWAYS_INLINE bool Contains(KeyInitType key) const {
    return FindId(key).IsValidId();
  }

  /**
   * Sorts the set's elements using the provided comparison class.
   */
  template <typename Predicate>
  void Sort(const Predicate& pred) {
    // Sort the elements according to the provided comparison class.
    elements_.Sort(ElementCompareClass<Predicate>(pred));

    // Rehash.
    Rehash();
  }

  /**
   * Serializer.
   */
  friend Archive& operator&(Archive& ar, Set& set) {
    // Load the set's new elements.
    ar& set.elements_;

    if (ar.IsLoading()) {
      // Free the old hash.
      set.hash_.ResizeAllocation(0, 0, sizeof(SetElementId));
      set.hash_size_ = 0;

      // Hash the newly loaded elements.
      set.ConditionalRehash(set.elements_.Count());
    }

    return ar;
  }

  // TODO
  // TODO
  // TODO
  /**
   * Describes the set's contents through an printer.
   *
   * \param out - The printer to describe the set's contents through.
   */
  // void Dump(Printer& out)
  //{
  //  out.Printf("Set: %i elements, %i hash slots", elements_.Count(),
  //  hash_size_); for (int32 hash_index = 0, local_hash_size = hash_size_;
  //  hash_index < local_hash_size; ++hash_index) {
  //    // Count the number of elements in this hash bucket.
  //    int32 elements_in_bucket = 0;
  //    for ( SetElementId element_id = GetTypedHash(hash_index);
  //          element_id.IsValidId();
  //          element_id = elements_[element_id].hash_next_id) {
  //      elements_in_bucket++;
  //    }
  //
  //    out.Printf("   Hash[%i] = %i", hash_index, elements_in_bucket);
  //  }
  //}

  bool VerifyHashElementsKey(KeyInitType key) {
    bool result = true;

    if (elements_.Count() > 0) {
      // iterate over all elements for the hash entry of the given key
      // and verify that the ids are valid
      SetElementId element_id = GetTypedHash(KeyFuncs::GetKeyHash(key));
      while (element_id.IsValidId()) {
        if (!IsValidId(element_id)) {
          result = false;
          break;
        }
        element_id = elements_[element_id].hash_next_id;
      }
    }

    return result;
  }

  // TODO
  // TODO
  // TODO
  // void DumpHashElements(Printer& out)
  //{
  //  for (int32 hash_index = 0, local_hash_size = hash_size_; hash_index <
  //  local_hash_size; ++hash_index) {
  //    out.Printf(TEXT("   Hash[%i]"), hash_index);
  //
  //    // iterate over all elements for the all hash entries
  //    // and dump info for all elements
  //    SetElementId element_id = GetTypedHash(hash_index);
  //    while (element_id.IsValidId()) {
  //      if (!IsValidId(element_id)) {
  //        out.Printf(TEXT("      !!INVALID!! element_id = %d"),
  //        element_id.index);
  //      }
  //      else {
  //        out.Printf(TEXT("      VALID element_id = %d"), element_id.index);
  //      }
  //      element_id = elements_[element_id].hash_next_id;
  //    }
  //  }
  //}

  // Legacy comparison operators.
  // Note that these also test whether the set's elements
  // were added in the same order!
  // friend bool LegacyCompareEqual(const Set& x, const Set& y)
  //{
  //  return x.elements_ == y.elements_;
  //}
  //
  // friend bool LegacyCompareNotEqual(const Set& x, const Set& y)
  //{
  //  return x.elements_ != y.elements_;
  //}

  /**
   * Returns the intersection of two sets. (A AND B)
   */
  Set Intersect(const Set& other) const {
    Set result;

    for (ConstIterator it(*this); it; ++it) {
      if (other.Contains(KeyFuncs::GetSetKey(*it))) {
        result.Add(*it);
      }
    }

    return result;
  }

  /**
   * Returns the union of two sets. (A OR B)
   */
  Set Union(const Set& other) const {
    Set result;

    for (ConstIterator it(*this); it; ++it) {
      result.Add(*it);
    }

    for (ConstIterator it(other); it; ++it) {
      result.Add(*it);
    }

    return result;
  }

  /**
   * Returns the complement of two sets. (A not in B)
   */
  Set Difference(const Set& other) const {
    Set result;

    for (ConstIterator it(*this); it; ++it) {
      if (!other.Contains(KeyFuncs::GetSetKey(*it))) {
        result.Add(*it);
      }
    }

    return result;
  }

  /**
   * Determine whether the specified set is entirely included within this set
   *
   * \param other - Set to check
   *
   * Returns True if the other set is entirely included in this set, false if it
   * is not
   */
  bool Includes(const Set<ElementType, KeyFuncs, Allocator>& other) const {
    bool includes_set = true;

    for (ConstIterator other_it(other); other_it; ++other_it) {
      if (!Contains(KeyFuncs::GetSetKey(*other_it))) {
        includes_set = false;
        break;
      }
    }

    return includes_set;
  }

  /**
   * Returns a Array of the elements
   */
  Array<ElementType> ToArray() const {
    Array<ElementType> result;
    result.Clear(Count());

    for (ConstIterator it(*this); it; ++it) {
      result.Add(*it);
    }

    return result;
  }

  /**
   * Checks that the specified address is not part of an element within the
   * container. Used for implementations to check that reference arguments
   * aren't going to be invalidated by possible reallocation.
   *
   * \param addr - The address to check.
   */
  FUN_ALWAYS_INLINE void CheckAddress(const ElementType* addr) const {
    elements_.CheckAddress(addr);
  }

 private:
  /**
   * Extracts the element value from the set's element structure and passes it
   * to the user provided comparison class.
   */
  template <typename Predicate>
  class ElementCompareClass {
   public:
    FUN_ALWAYS_INLINE ElementCompareClass(const Predicate& pred)
        : pred_(pred) {}

    FUN_ALWAYS_INLINE bool operator()(const SetElementType& a,
                                      const SetElementType& b) const {
      return pred_(a.value, b.value);
    }

   private:
    DereferenceWrapper<ElementType, Predicate> pred_;
  };

  typedef SparseArray<SetElementType, typename Allocator::SparseArrayAllocator>
      ElementArrayType;
  typedef
      typename Allocator::HashAllocator::template ForElementType<SetElementId>
          HashType;

  ElementArrayType elements_;

  mutable HashType hash_;
  mutable int32 hash_size_;

  FUN_ALWAYS_INLINE SetElementId& GetTypedHash(int32 hash_index) const {
    return (
        (SetElementId*)hash_.GetAllocation())[hash_index & (hash_size_ - 1)];
  }

  /**
   * Accesses an element in the set.
   * This is needed because the iterator classes aren't friends of
   * SetElementId and so can't access the element index.
   */
  FUN_ALWAYS_INLINE const SetElementType& GetInternalElement(
      const SetElementId& id) const {
    return elements_[id];
  }

  FUN_ALWAYS_INLINE SetElementType& GetInternalElement(const SetElementId& id) {
    return elements_[id];
  }

  /**
   * Translates an element index into an element ID.
   * This is needed because the iterator classes aren't friends of
   * SetElementId and so can't access the SetElementId private constructor.
   */
  static FUN_ALWAYS_INLINE SetElementId IndexToId(int32 index) {
    return SetElementId(index);
  }

  /**
   * Adds an element to the hash.
   */
  FUN_ALWAYS_INLINE void HashElement(SetElementId element_id,
                                     const SetElementType& element) const {
    // Compute the hash bucket the element goes in.
    element.hash_index =
        KeyFuncs::GetKeyHash(KeyFuncs::GetSetKey(element.value)) &
        (hash_size_ - 1);

    // Link the element into the hash bucket.
    element.hash_next_id = GetTypedHash(element.hash_index);
    GetTypedHash(element.hash_index) = element_id;
  }

  /**
   * Checks if the hash has an appropriate number of buckets, and if not resizes
   * it.
   *
   * \param hashed_element_count - The number of elements to size the hash for.
   * \param allow_shrinking - true if the hash is allowed to shrink.
   *
   * \return true if the set was rehashed.
   */
  bool ConditionalRehash(int32 hashed_element_count,
                         bool allow_shrinking = false) const {
    // TODO 강제로 고정된 해시 버킷수를 지정해서 사용하고, Rehash를 금지시키는
    // 기능을 넣는건 어떨런지?

    // Calculate the desired hash size for the specified number of elements.
    const int32 desired_hash_size =
        Allocator::GetNumberOfHashBuckets(hashed_element_count);

    // If the hash hasn't been created yet, or is smaller than the desired hash
    // size, rehash.
    if (hashed_element_count > 0 &&
        (hash_size_ == 0 || hash_size_ < desired_hash_size ||
         (hash_size_ > desired_hash_size && allow_shrinking))) {
      hash_size_ = desired_hash_size;
      Rehash();
      return true;
    } else {
      return false;
    }
  }

  /**
   * Resizes the hash.
   */
  void Rehash() const {
    // Free the old hash.
    hash_.ResizeAllocation(0, 0, sizeof(SetElementId));

    const int32 local_hash_size = hash_size_;
    if (local_hash_size > 0) {
      // Allocate the new hash.
      fun_check_dbg(!(local_hash_size & (hash_size_ - 1)));
      hash_.ResizeAllocation(0, local_hash_size, sizeof(SetElementId));
      for (int32 hash_index = 0; hash_index < local_hash_size; ++hash_index) {
        GetTypedHash(hash_index) = SetElementId();
      }

      // Add the existing elements to the new hash.
      for (typename ElementArrayType::ConstIterator element_it_(elements_);
           element_it_; ++element_it_) {
        HashElement(SetElementId(element_it_.GetIndex()), *element_it_);
      }
    }
  }

  /**
   * The base type of whole set iterators.
   */
  template <bool IsConst>
  class BaseIterator {
   private:
    friend class Set;

    using ItElementType =
        typename Conditional<IsConst, const ElementType, ElementType>::Result;

   public:
    using ElementItType =
        typename Conditional<IsConst, typename ElementArrayType::ConstIterator,
                             typename ElementArrayType::Iterator>::Result;

    FUN_ALWAYS_INLINE BaseIterator(const ElementItType& element_it)
        : element_it_(element_it) {}

    /**
     * Advances the iterator to the next element.
     */
    FUN_ALWAYS_INLINE BaseIterator& operator++() {
      ++element_it_;
      return *this;
    }

    /**
     * conversion to "bool" returning true if the iterator is valid.
     */
    FUN_ALWAYS_INLINE explicit operator bool() const { return !!element_it_; }

    /**
     * inverse of the "bool" operator
     */
    FUN_ALWAYS_INLINE bool operator!() const { return !(bool)*this; }

    // Accessors.
    FUN_ALWAYS_INLINE SetElementId GetId() const {
      return Set::IndexToId(element_it_.GetIndex());
    }

    FUN_ALWAYS_INLINE ItElementType* operator->() const {
      return &element_it_->value;
    }

    FUN_ALWAYS_INLINE ItElementType& operator*() const {
      return element_it_->value;
    }

    FUN_ALWAYS_INLINE friend bool operator==(const BaseIterator& lhs,
                                             const BaseIterator& rhs) {
      return lhs.element_it_ == rhs.element_it_;
    }

    FUN_ALWAYS_INLINE friend bool operator!=(const BaseIterator& lhs,
                                             const BaseIterator& rhs) {
      return lhs.element_it_ != rhs.element_it_;
    }

    ElementItType element_it_;
  };

  /**
   * The base type of whole set iterators.
   */
  template <bool IsConst>
  class BaseKeyIterator {
   private:
    using SetType = typename Conditional<IsConst, const Set, Set>::Result;
    using ItElementType =
        typename Conditional<IsConst, const ElementType, ElementType>::Result;

   public:
    /**
     * Initialization constructor.
     */
    FUN_ALWAYS_INLINE BaseKeyIterator(SetType& set, KeyInitType key)
        : set_(set), key_(key), id_() {
      // The set's hash needs to be initialized to find the elements with the
      // specified key.
      set_.ConditionalRehash(set_.elements_.Count());
      if (Set.hash_size_ > 0) {
        next_id_ = set_.GetTypedHash(KeyFuncs::GetKeyHash(key_));
        ++(*this);
      }
    }

    /**
     * Advances the iterator to the next element.
     */
    FUN_ALWAYS_INLINE BaseKeyIterator& operator++() {
      id_ = next_id_;

      while (id_.IsValidId()) {
        next_id_ = set_.GetInternalElement(id_).hash_next_id;
        fun_check_dbg(id_ != next_id_);

        if (KeyFuncs::Matches(KeyFuncs::GetSetKey(set_[id_]), key_)) {
          break;
        }

        id_ = next_id_;
      }
      return *this;
    }

    /**
     * conversion to "bool" returning true if the iterator is valid.
     */
    FUN_ALWAYS_INLINE explicit operator bool() const { return id_.IsValidId(); }

    /**
     * inverse of the "bool" operator
     */
    FUN_ALWAYS_INLINE bool operator!() const { return !(bool)*this; }

    // Accessors.
    FUN_ALWAYS_INLINE ItElementType* operator->() const { return &set_[id_]; }

    FUN_ALWAYS_INLINE ItElementType& operator*() const { return set_[id_]; }

   protected:
    SetType& set_;
    typename TypeTraits<typename KeyFuncs::KeyType>::ConstPointerType key_;
    SetElementId id_;
    SetElementId next_id_;
  };

 public:
  /**
   * Used to iterate over the elements of a const Set.
   */
  class ConstIterator : public BaseIterator<true> {
    friend class Set;

   public:
    FUN_ALWAYS_INLINE ConstIterator(
        const typename BaseIterator<true>::ElementItType& element_id)
        : BaseIterator<true>(element_id) {}

    FUN_ALWAYS_INLINE ConstIterator(const Set& set)
        : BaseIterator<true>(begin(set.elements_)) {}
  };

  /**
   * Used to iterate over the elements of a Set.
   */
  class Iterator : public BaseIterator<false> {
    friend class Set;

   public:
    FUN_ALWAYS_INLINE Iterator(
        Set& set, const typename BaseIterator<false>::ElementItType& element_id)
        : BaseIterator<false>(element_id), set_(set) {}

    FUN_ALWAYS_INLINE Iterator(Set& set)
        : BaseIterator<false>(begin(set.elements_)), set_(set) {}

    /**
     * Removes the current element from the set.
     */
    FUN_ALWAYS_INLINE void RemoveCurrent() {
      set_.Remove(BaseIterator<false>::GetId());
    }

   private:
    Set& set_;
  };

  /**
   * Used to iterate over the elements of a const Set.
   */
  class ConstKeyIterator : public BaseKeyIterator<true> {
   public:
    FUN_ALWAYS_INLINE ConstKeyIterator(const Set& set, KeyInitType key)
        : BaseKeyIterator<true>(set, key) {}
  };

  /**
   * Used to iterate over the elements of a Set.
   */
  class KeyIterator : public BaseKeyIterator<false> {
   public:
    FUN_ALWAYS_INLINE KeyIterator(Set& set, KeyInitType key)
        : BaseKeyIterator<false>(set, key), set_(set) {}

    /**
     * Removes the current element from the set.
     */
    FUN_ALWAYS_INLINE void RemoveCurrent() {
      set_.Remove(BaseKeyIterator<false>::id_);
      BaseKeyIterator<false>::id_ = SetElementId();
    }

   private:
    Set& set_;
  };

  /**
   * Creates an iterator for the contents of this set
   */
  FUN_ALWAYS_INLINE Iterator CreateIterator() { return Iterator(*this); }

  /**
   * Creates a const iterator for the contents of this set
   */
  FUN_ALWAYS_INLINE ConstIterator CreateConstIterator() const {
    return ConstIterator(*this);
  }

 private:
  // DO NOT USE DIRECTLY
  // STL-like iterators to enable range-based for loop support.
  FUN_ALWAYS_INLINE friend Iterator begin(Set& set) {
    return Iterator(set, begin(set.elements_));
  }
  FUN_ALWAYS_INLINE friend ConstIterator begin(const Set& set) {
    return ConstIterator(begin(set.elements_));
  }
  FUN_ALWAYS_INLINE friend Iterator end(Set& set) {
    return Iterator(set, end(set.elements_));
  }
  FUN_ALWAYS_INLINE friend ConstIterator end(const Set& set) {
    return ConstIterator(end(set.elements_));
  }
};

template <typename ElementType, typename KeyFuncs, typename Allocator>
struct ContainerTraits<Set<ElementType, KeyFuncs, Allocator>>
    : public ContainerTraitsBase<Set<ElementType, KeyFuncs, Allocator>> {
  enum {
    MoveWillEmptyContainer =
        ContainerTraits<typename Set<ElementType, KeyFuncs, Allocator>::
                            ElementArrayType>::MoveWillEmptyContainer &&
        AllocatorTraits<typename Allocator::HashAllocator>::SupportsMove
  };
};

//
// UntypedSparseSet
//

struct UntypedSetLayout {
  int32 element_offset;
  int32 hash_next_id_offset;
  int32 hash_index_offset;
  int32 size;

  UntypedSparseArrayLayout sparse_array_layout;
};

/**
 * Untyped set type for accessing Set data, like UntypedArray for Array.
 * Must have the same memory representation as a Set.
 */
class UntypedSet {
 public:
  static UntypedSetLayout GetUntypedLayout(int32 element_size,
                                           int32 element_alignment) {
    UntypedSetLayout result;

    // SetElement<TPair<key, value>>
    StructBuilder set_element_struct;
    result.element_offset =
        set_element_struct.AddMember(element_size, element_alignment);
    result.hash_next_id_offset = set_element_struct.AddMember(
        sizeof(SetElementId), alignof(SetElementId));
    result.hash_index_offset =
        set_element_struct.AddMember(sizeof(int32), alignof(int32));
    result.size = set_element_struct.GetSize();
    result.sparse_array_layout = UntypedSparseArray::GetUntypedLayout(
        set_element_struct.GetSize(), set_element_struct.GetAlignment());

    return result;
  }

  UntypedSet() : hash_size_(0) {}

  bool IsValidIndex(int32 index) const { return elements_.IsValidIndex(index); }

  int32 Count() const { return elements_.Count(); }

  int32 GetMaxIndex() const { return elements_.GetMaxIndex(); }

  void* MutableData(int32 index, const UntypedSetLayout& layout) {
    return elements_.MutableData(index, layout.sparse_array_layout);
  }

  const void* ConstData(int32 index, const UntypedSetLayout& layout) const {
    return elements_.ConstData(index, layout.sparse_array_layout);
  }

  void Clear(int32 slack, const UntypedSetLayout& layout) {
    // Clear the elements array, and reallocate it for the expected number of
    // elements.
    elements_.Clear(slack, layout.sparse_array_layout);

    // Calculate the desired hash size for the specified number of elements.
    const int32 desired_hash_size = Allocator::GetNumberOfHashBuckets(slack);

    // If the hash hasn't been created yet, or is smaller than the desired hash
    // size, rehash.
    if (slack != 0 && (hash_size_ == 0 || hash_size_ != desired_hash_size)) {
      hash_size_ = desired_hash_size;

      // Free the old hash.
      hash_.ResizeAllocation(0, hash_size_, sizeof(SetElementId));
    }

    for (auto *it = (SetElementId*)hash_.GetAllocation(),
              *end = it + hash_size_;
         it != end; ++it) {
      *it = SetElementId();
    }
  }

  void RemoveAt(int32 index, const UntypedSetLayout& layout) {
    fun_check(IsValidIndex(index));

    auto* element_being_removed =
        elements_.MutableData(index, layout.sparse_array_layout);

    // Remove the element from the hash.
    for (SetElementId* next_element_id =
             &GetTypedHash(GetHashIndexRef(element_being_removed, layout));
         next_element_id->IsValidId();
         next_element_id = &GetHashNextIdRef(
             elements_.MutableData(next_element_id->AsInteger(),
                                   layout.sparse_array_layout),
             layout)) {
      if (next_element_id->AsInteger() == index) {
        *next_element_id = GetHashNextIdRef(element_being_removed, layout);
        break;
      }
    }

    // Remove the element from the elements array.
    elements_.RemoveAtUninitialized(layout.sparse_array_layout, index);
  }

  /**
   * Adds an uninitialized object to the set.
   * The set will need rehashing at some point after this call to make it valid.
   *
   * Returns The index of the added element.
   */
  int32 AddUninitialized(const UntypedSetLayout& layout) {
    int32 result = elements_.AddUninitialized(layout.sparse_array_layout);
    ++hash_size_;
    return result;
  }

  void Rehash(const UntypedSetLayout& layout,
              TFunctionRef<uint32(const void*)> get_type_hash) {
    // Free the old hash.
    hash_.ResizeAllocation(0, 0, sizeof(SetElementId));

    hash_size_ = Allocator::GetNumberOfHashBuckets(elements_.Count());
    if (hash_size_ > 0) {
      // Allocate the new hash.
      fun_check_dbg(MathBase::IsPowerOfTwo(hash_size_));
      hash_.ResizeAllocation(0, hash_size_, sizeof(SetElementId));
      for (int32 hash_index = 0; hash_index < hash_size_; ++hash_index) {
        GetTypedHash(hash_index) = SetElementId();
      }

      int32 bytes_per_set_element = layout.size;

      // Add the existing elements to the new hash.
      int32 index = 0;
      int32 count = elements_.Count();
      while (count > 0) {
        if (elements_.IsValidIndex(index)) {
          SetElementId element_id(index);

          void* element =
              (uint8*)elements_.MutableData(index, layout.sparse_array_layout);

          // Compute the hash bucket the element goes in.
          uint32 element_hash = get_type_hash(element);
          int32 hash_index = element_hash & (hash_size_ - 1);
          GetHashIndexRef(element, layout) = element_hash & (hash_size_ - 1);

          // Link the element into the hash bucket.
          GetHashNextIdRef(element, layout) = GetTypedHash(hash_index);
          GetTypedHash(hash_index) = element_id;

          --count;
        }

        ++index;
      }
    }
  }

 private:
  using Allocator = DefaultSetAllocator;
  using HashType = Allocator::HashAllocator::ForElementType<SetElementId>;

  UntypedSparseArray elements_;
  mutable HashType hash_;
  mutable int32 hash_size_;

  FUN_ALWAYS_INLINE SetElementId& GetTypedHash(int32 hash_index) const {
    return (
        (SetElementId*)hash_.GetAllocation())[hash_index & (hash_size_ - 1)];
  }

  static SetElementId& GetHashNextIdRef(const void* element,
                                        const UntypedSetLayout& layout) {
    return *(SetElementId*)((uint8*)element + layout.hash_next_id_offset);
  }

  static int32& GetHashIndexRef(const void* element,
                                const UntypedSetLayout& layout) {
    return *(int32*)((uint8*)element + layout.hash_index_offset);
  }

  // This function isn't intended to be called, just to be compiled
  // to validate the correctness of the type.
  static void CheckConstraints() {
    typedef UntypedSet UntypedType;
    typedef Set<int32> TypedType;

    // Check that the class footprint is the same
    static_assert(sizeof(UntypedType) == sizeof(TypedType),
                  "UntypedSet's size doesn't match Set");
    static_assert(alignof(UntypedType) == alignof(TypedType),
                  "UntypedSet's alignment doesn't match Set");

    // Check member sizes
    static_assert(sizeof(DeclVal<UntypedType>().elements_) ==
                      sizeof(DeclVal<TypedType>().elements_),
                  "UntypedSet's elements_ member size does not match Set's");
    static_assert(sizeof(DeclVal<UntypedType>().hash_) ==
                      sizeof(DeclVal<TypedType>().hash_),
                  "UntypedSet's Hash member size does not match Set's");
    static_assert(sizeof(DeclVal<UntypedType>().hash_size_) ==
                      sizeof(DeclVal<TypedType>().hash_size_),
                  "UntypedSet's hash_size_ member size does not match Set's");

    // Check member offsets
    static_assert(
        offsetof(UntypedType, elements_) == offsetof(TypedType, elements_),
        "UntypedSet's elements_ member offset does not match Set's");
    static_assert(offsetof(UntypedType, hash_) == offsetof(TypedType, hash_),
                  "UntypedSet's Hash member offset does not match Set's");
    static_assert(
        offsetof(UntypedType, hash_size_) == offsetof(TypedType, hash_size_),
        "UntypedSet's FirstFreeIndex member offset does not match Set's");
  }

 public:
  UntypedSet(const UntypedSet&) = delete;
  UntypedSet& operator=(const UntypedSet&) = delete;
};

template <>
struct IsZeroConstructible<UntypedSet> {
  enum { Value = true };
};

}  // namespace fun
