#pragma once

#include "fun/base/base.h"

namespace fun {

class SlicingHelper {
 public:
  enum SlicingResult {
    Null,
    Empty,
    Full,
    Subset,
  };

  FUN_ALWAYS_INLINE static
  SlicingResult Slice(int32 original_len, int32* position, int32* len) {
    int32& adjusted_pos = *position;
    int32& adjusted_len = *len;

    if (adjusted_pos > original_len) {
      return Null;
    }

    if (adjusted_pos < 0) {
      if (adjusted_len < 0 || (adjusted_len + adjusted_pos) >= original_len) {
        return Full;
      }

      if ((adjusted_len + adjusted_pos) <= 0) {
        return Null;
      }

      adjusted_len += adjusted_pos;
      adjusted_pos = 0;
    } else if (uint32(adjusted_len) > uint32(original_len - adjusted_pos)) {
      adjusted_len = original_len - adjusted_pos;
    }

    if (adjusted_pos == 0 && adjusted_len == original_len) {
      return Full;
    }

    return adjusted_len > 0 ? Subset : Empty;
  }
};

} // namespace fun
