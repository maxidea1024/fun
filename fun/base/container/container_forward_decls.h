#pragma once

//#include "fun/base/base.h"

namespace fun {

class DefaultAllocator;
class DefaultSetAllocator;

// TODO 이름을 KeyValuePair로 바꿔주어야할듯...
template <typename KeyType, typename ValueType>
class Pair;

template <typename T, typename Allocator = DefaultAllocator>
class Array;

// template <typename T> class TransArray;

template <typename KeyType, typename ValueType, bool AllowDuplicateKeys>
struct DefaultMapKeyFuncs;

template <typename KeyType, typename ValueType,
          typename SetAllocator = DefaultSetAllocator,
          typename KeyFuncs = DefaultMapKeyFuncs<KeyType, ValueType, false> >
class Map;

template <typename KeyType, typename ValueType,
          typename SetAllocator = DefaultSetAllocator,
          typename KeyFuncs = DefaultMapKeyFuncs<KeyType, ValueType, true> >
class MultiMap;

template <typename KeyType, typename ValueType,
          typename ArrayAllocator = DefaultAllocator,
          typename SortPredicate = Less<KeyType> >
class OrderedMap;

}  // namespace fun
