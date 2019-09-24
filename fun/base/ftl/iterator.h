#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/type_traits.h"

namespace fun {

template <typename... Ts>
using Void_T = void;


template <typename T>
constexpr T* AddressOf(T& value) noexcept {
  return __builtin_addressof(val);
}

template <typename T>
const T* AddressOf(const T&&) = delete;


// Identifying tag for input iterators
struct InputIterator_TAG {};
// Identifying tag for output iterators
struct OutputIterator_TAG {};
// Identifying tag for forward iterators
struct ForwardIterator_TAG {};
// Identifying tag for bidirectional iterators
struct BidrirectionalIterator_TAG {};
// Identifying tag for random-access iterators
struct RandomAccessIterator_TAG {};

// General case, no special optimizations
struct _GeneralPtrIterator_TAG {};
// Iterator is a Pointer to a trivially copyable type
struct TriviallyCopyablePtrIterator_TAG {};
// Iterator is a Pointer to a trivial type
struct ReallyTrivialPtrIterator_TAG {};


/**
 * Wrap pushes to front of container as output iterator
 */
template <typename _Container>
class BackInsertIterator {
 public:
  using IteratorCategory  = OutputIterator_TAG;
  using ValueType         = void;
  using DifferenceType    = void;
  using Pointer           = void;
  using Reference         = void;
  using Container         = _Container;

  explicit BackInsertIterator(Container& container)
    : container_(AddressOf(container)) {
  }

  BackInsertIterator& operator = (const ValueType& value) {
    container_->PushBack(value);
    return *this;
  }

  BackInsertIterator& operator = (ValueType&& value) {
    container_->PushBack(MoveTemp(value));
    return *this;
  }

  BackInsertIterator& operator * () {
    return *this;
  }

  BackInsertIterator& operator ++ () {
    return *this;
  }

  BackInsertIterator& operator ++ (int) {
    return *this;
  }

 protected:
  Container* container_;
};

template <typename Container>
inline BackInsertIterator<Container>
BackInserter(Container& container) {
  return BackInsertIterator<Container>(container);
}


/**
 * Wrap pushes to front of container as output iterator
 */
template <typename _Container>
class FrontInsertIterator {
 public:
  using IteratorCategory  = OutputIterator_TAG;
  using ValueType         = void;
  using DifferenceType    = void;
  using Pointer           = void;
  using Reference         = void;
  using Container         = _Container;

  explicit FrontInsertIterator(Container& container)
    : container_(AddressOf(container)) {
  }

  FrontInsertIterator& operator = (const ValueType& value) {
    container_.PushFront(value);
    return *this;
  }

  FrontInsertIterator& operator = (ValueType&& value) {
    container_.PushFront(MoveTemp(value));
    return *this;
  }

  FrontInsertIterator& operator * () {
    return *this;
  }

  FrontInsertIterator& operator ++ () {
    return *this;
  }

  FrontInsertIterator& operator ++ (int) {
    return *this;
  }
 protected:
  Container* container_;
};

template <typename Container>
inline FrontInsertIterator<Container>
FrontInserter(Container& container) {
  return FrontInsertIterator<Container>(container);
}


//TODO 이건 딱히 쓸모가 없어보이는데... 일단은 냅두자.
/*
template <typename _Container>
class InsertIterator {
 public:
  using IteratorCategory  = OutputIterator_TAG;
  using ValueType         = void;
  using DifferenceType    = void;
  using Pointer           = void;
  using Reference         = void;
  using Container         = _Container;

  InsertIterator(Container& container, typename Container::Iterator where)
    : container_(AddressOf(container))
    , iter_(where) {
  }

  InsertIterator& operator = (const ValueType& value) {
    iter_ = container_->Insert(iter_, value);
    ++iter_;
    return *this;
  }

  InsertIterator& operator = (ValueType&& value) {
    iter_ = container_->Insert(iter_, MoveTemp(value));
    ++iter_;
    return *this;
  }

  InsertIterator& operator * () {
    return *this;
  }

  InsertIterator& operator ++ () {
    return *this;
  }

  InsertIterator& operator ++ (int) {
    return *this;
  }

 protected:
  Container* container_;
  typename Container::Iterator iter_;
};

template <typename Container>
inline InsertIterator<Container>
Inserter(Container& container, typename Container::Iterator where) {
  return InsertIterator<Container>(container, where);
}
*/


//
// IteratorTraits
//

template <typename, typename = void>
struct IteratorTraitsBase {
};

template <typename It>
struct IteratorTraitsBase<
    It,
    Void_T<
      typename It::IteratorCategory,
      typename It::ValueType,
      typename It::DifferenceType,
      typename It::Pointer,
      typename It::Reference
    >
  > {
  using IteratorCategory  = typename It::IteratorCategory;
  using ValueType         = typename It::ValueType;
  using DifferenceType    = typename It::DifferenceType;
  using Pointer           = typename It::Pointer;
  using Reference         = typename It::Reference;
};

template <typename T, bool = IsObject<T>::Value>
struct IteratorTraitsPointerBase {
  using IteratorCategory  = RandomAccessIterator_TAG;
  using ValueType         = typename RemoveCV<T>::Type;
  using DifferenceType    = ptrdiff_t;
  using Pointer           = T*;
  using Reference         = T&;
};

template <typename T>
struct IteratorTraitsPointerBase<T, false> {
};

template <typename It>
struct IteratorTraits : IteratorTraitsBase<It> {
};

template <typename T>
struct IteratorTraits<T*> : IteratorTraitsPointerBase<T> {
};


template <typename It>
struct IteratorCategoryOf {
  using Category = typename IteratorTraits<T>::IteratorCategory;
};


template <typename T, typename = void>
struct _IsIterator {
  enum { Value = false };
};

template <typename T>
struct _IsIterator<T, Void_T<typename IteratorCategoryOf<It>::Category> {
  enum { Value = true };
};

template <typename T>
struct IsIterator : BoolConstant<_IsIterator<T>::Value> {
};


template <typename It>
struct IsInputIterator {
  enum { Value = IsConvertible<typename IteratorCategoryOf<It>::Category, InputIteator_TAG>::Value };
};

template <typename It>
struct IsForwardIterator {
  enum { Value = IsConvertible<typename IteratorCategoryOf<It>::Category, ForwardIterator_TAG>::Value };
};

template <typename It>
struct IsBidirectionalIterator {
  enum { Value = IsConvertible<typename IteratorCategoryOf<It>::Category, BidirectionalIterator_TAG>::Value };
};

template <typename It>
struct IsRandomAccessIterator {
  enum { Value = IsConvertible<typename IteratorCategoryOf<It>::Category, RandomAccessIterator_TAG>::Value };
};

} // namespace fun
