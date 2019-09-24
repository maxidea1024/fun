#pragma once

#include "fun/base/base.h"
#include "fun/base/string/cstring_traits.h"

namespace fun {

// TODO 공용으로 빼줄까??

template <typename T>
inline static int32 GetTypedStrlen(const T& str) {
  return str.Len();
}

inline static int32 GetTypedStrlen(const char* str) {
  return CStringTraitsA::Strlen(str);
}

inline static int32 GetTypedStrlen(const UNICHAR* str) {
  return CStringTraitsU::Strlen(str);
}

template <typename ResultType, typename ElementType, typename SeparatorType>
inline static ResultType StringJoin(const ElementType* list, int32 count,
                                    const SeparatorType& separator) {
  if (list == nullptr || count <= 0) {
    return ResultType();
  }

  int32 required_space = separator.Len() * (count - 1);
  for (int32 i = 0; i < count; ++i) {
    required_space += GetTypedStrlen(list[i]);
  }

  ResultType result(required_space, ReservationInit);

  bool first = true;
  for (int32 i = 0; i < count; ++i) {
    if (!first) {
      result.Append(separator);
    }
    first = false;

    result.Append(list[i]);
  }

  return result;
}

}  // namespace fun
