#pragma once

#include "fun/base/base.h"
#include "fun/base/string/cstring_traits.h"

namespace fun {

template <typename SourceStringType, typename ElementType>
inline static int32 IndexOfAnyStrings_helper(
    const SourceStringType& source, const ElementType* strings,
    int32 string_count,
    CaseSensitivity casesense = CaseSensitivity::CaseSensitive, int32 from = 0,
    int32* matched_index = nullptr, int32* matched_len = nullptr) {
  for (int32 i = 0; i < string_count; ++i) {
    const int32 index =
        source.IndexOf(strings[i], casesense, from, matched_len);
    if (index != INVALID_INDEX) {
      if (matched_index) {
        *matched_index = i;
      }
      return index;
    }
  }

  if (matched_index) {
    *matched_index = INVALID_INDEX;
  }

  return INVALID_INDEX;
}

}  // namespace fun
