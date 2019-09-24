#pragma once

#include "fun/http/http.h"

namespace fun {
namespace http {

enum class Protocol {
  None,
  HTTP,
  HTTPS,
};

}  // namespace http

inline String ToString(const http::Protocol value) {
  switch (value) {
    case http::Protocol::None:
      return StringLiteral("None");
    case http::Protocol::HTTP:
      return StringLiteral("HTTP");
    case http::Protocol::HTTPS:
      return StringLiteral("HTTPS");
    default:
      return StringLiteral("Unknown");
  }
}

inline bool FromString(const UNICHAR* str, http::Protocol& out_value) {
  out_value = http::Protocol::None;

  /*

  if (icompare(str, "none") == 0) {
    value = http::Protocol::None;
    return true;
  } else if (icompare(str, "http") == 0) {
    value = http::Protocol::Http;
    return true;
  } else if (icompare(str, "https") == 0) {
    value = http::Protocol::Https;
    return true;
  } else {
    return false;
  }

  */

  if (StringCmp::Compare(str, "None", CaseSensitivity::IgnoreCase) == 0) {
    out_value = http::Protocol::None;
    return true;
  } else if (StringCmp::Compare(str, "HTTP", CaseSensitivity::IgnoreCase) ==
             0) {
    out_value = http::Protocol::HTTP;
    return true;
  } else if (StringCmp::Compare(str, "HTTPS", CaseSensitivity::IgnoreCase) ==
             0) {
    out_value = http::Protocol::HTTPS;
    return true;
  } else {
    return false;
  }
}

}  // namespace fun
