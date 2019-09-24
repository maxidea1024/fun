#pragma once

#include "fun/http/http.h"

namespace fun {
namespace http {

enum class Version {
  None,
  V10,
  V11,
  V20,
};

} // namespace http

inline String ToString(const http::Version value) {
  switch (value) {
    case http::Version::None: return StringLiteral("None");
    case http::Version::V10: return StringLiteral("1.0");
    case http::Version::V11: return StringLiteral("1.1");
    case http::Version::V20: return StringLiteral("2.0");
    default: return StringLiteral("Unknown");
  }
}

inline bool FromString(const UNICHAR* str, http::Version& out_value) {
  out_value = http::Version::None;

  if (StringCmp::Compare(str, "1.0", CaseSensitivity::IgnoreCase) == 0) {
    out_value = http::Version::V10;
    return true;
  } else if (StringCmp::Compare(str, "1.1", CaseSensitivity::IgnoreCase) == 0) {
    out_value = http::Version::V11;
    return true;
  } else if (StringCmp::Compare(str, "2.0", CaseSensitivity::IgnoreCase) == 0) {
    out_value = http::Version::V20;
    return true;
  } else {
    return false;
  }
}

} // namespace http
} // namespace fun
