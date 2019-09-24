#pragma once

#include "fun/http/http.h"

namespace fun {
namespace http {

enum class ContentType {
  None,

  /** Json(application/json) */
  Json,

  /** Binary(application/octet-stream) */
  Binary,
};

} // namespace http


inline String ToString(const http::ContentType value)
{
  switch (value) {
  case http::ContentType::None: return StringLiteral("None");
  case http::ContentType::Json: return StringLiteral("Json");
  case http::ContentType::Binary: return StringLiteral("Binary");
  default: return StringLiteral("Unknown");
  }
}

inline bool FromString(const UNICHAR* str, http::ContentType& out_value)
{
  out_value = http::ContentType::None;

  if (StringCmp::Compare(str, "None", CaseSensitivity::IgnoreCase) == 0) {
    out_value = http::ContentType::None;
    return true;
  }
  else if (StringCmp::Compare(str, "Json", CaseSensitivity::IgnoreCase) == 0) {
    out_value = http::ContentType::Json;
    return true;
  }
  else if (StringCmp::Compare(str, "Binary", CaseSensitivity::IgnoreCase) == 0) {
    out_value = http::ContentType::Binary;
    return true;
  }
  else {
    return false;
  }
}

} // namespace fun
