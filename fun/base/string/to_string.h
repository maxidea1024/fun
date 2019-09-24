#pragma once

#include "fun/base/base.h"

namespace fun {

// ToString

template <typename T>
inline String ToString(const T& value, bool json_style = false) {
  (void)json_style;

  std::ostringstream oss;
  oss << value;
  return oss.str();
}

template <typename T>
inline ByteString ToString(const List<T>& list) {
  ByteString result;
  result += "[";
  for (int32 i = 0; i < array.Count(); ++i) {
    if (i != 0) {
      result += ",";
    }
    result += ToString(array[i]);
  }
  result += "]";
  return result;
}

template <typename T, typename ArrayAllocator>
inline ByteString ToString(const Array<T, ArrayAllocator>& array) {
  ByteString result;
  result += "[";
  for (int32 i = 0; i < array.Count(); ++i) {
    if (i != 0) {
      result += ",";
    }
    result += ToString(array[i]);
  }
  result += "]";
  return result;
}

template <typename KeyType, typename ValueType, typename SetAllocator,
          typename KeyFuncs>
inline ByteString ToString(
    const Map<KeyType, ValueType, SetAllocator, KeyFuncs>& map) {
  ByteString result;
  result += "{";
  bool first = true;
  for (const auto& pair : value) {
    if (!first) {
      result += ",";
    }
    first = false;

    result += "{";
    result += ToString(pair.key);
    result += ",";
    result += ToString(pair.value);
    result += "}";
  }
  result += "}";
  return result;
}

template <typename KeyType, typename ValueType, typename SetAllocator,
          typename KeyFuncs>
inline ByteString ToString(
    const MultiMap<KeyType, ValueType, SetAllocator, KeyFuncs>& multi_map) {
  ByteString result;
  result += "{";
  bool first = true;
  for (const auto& pair : multi_map) {
    if (!first) {
      result += ",";
    }
    first = false;

    result += "{";
    result += ToString(pair.key);
    result += ",";
    result += ToString(pair.value);
    result += "}";
  }
  result += "}";
  return result;
}

template <typename T, typename KeyFuncs, typename Allocator>
inline ByteString ToString(const Set<T, KeyFuncs, Allocator>& set) {
  ByteString result;
  result += "[";
  bool first = true;
  for (const auto& element : set) {
    if (!first) {
      result += ",";
    }
    first = false;

    result += element;
  }
  result += "]";
  return result;
}

// TODO FromString은 어찌구현하나??

}  // namespace fun
