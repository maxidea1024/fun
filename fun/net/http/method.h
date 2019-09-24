#pragma once

#include "fun/http/http.h"

// TODO hmm...
#ifdef _MSC_VER
#undef DELETE
#endif

namespace fun {
namespace http {

enum class Method {
  None = 0,
  GET,
  POST,
  HEAD,
  PUT,
  DELETE,
  OPTIONS,
  PATCH,
  NumMethods,
};

}  // namespace http

inline String ToString(const http::Method value) {
  switch (value) {
    case http::Method::None:
      return StringLiteral("None");
    case http::Method::GET:
      return StringLiteral("GET");
    case http::Method::POST:
      return StringLiteral("POST");
    case http::Method::HEAD:
      return StringLiteral("HEAD");
    case http::Method::PUT:
      return StringLiteral("PUT");
    case http::Method::DELETE:
      return StringLiteral("DELETE");
    case http::Method::OPTIONS:
      return StringLiteral("OPTIONS");
    case http::Method::PATCH:
      return StringLiteral("PATCH");
    default:
      return StringLiteral("Unknown");
  }
}

inline bool FromString(const UNICHAR* str, http::Method& out_value) {
  out_value = http::Method::None;

  if (StringCmp::Compare(str, "None", CaseSensitivity::IgnoreCase) == 0) {
    out_value = http::Method::None;
    return true;
  } else if (StringCmp::Compare(str, "GET", CaseSensitivity::IgnoreCase) == 0) {
    out_value = http::Method::GET;
    return true;
  } else if (StringCmp::Compare(str, "POST", CaseSensitivity::IgnoreCase) ==
             0) {
    out_value = http::Method::POST;
    return true;
  } else if (StringCmp::Compare(str, "HEAD", CaseSensitivity::IgnoreCase) ==
             0) {
    out_value = http::Method::HEAD;
    return true;
  } else if (StringCmp::Compare(str, "PUT", CaseSensitivity::IgnoreCase) == 0) {
    out_value = http::Method::PUT;
    return true;
  } else if (StringCmp::Compare(str, "DELETE", CaseSensitivity::IgnoreCase) ==
             0) {
    out_value = http::Method::DELETE;
    return true;
  } else if (StringCmp::Compare(str, "OPTIONS", CaseSensitivity::IgnoreCase) ==
             0) {
    out_value = http::Method::OPTIONS;
    return true;
  } else if (StringCmp::Compare(str, "PATCH", CaseSensitivity::IgnoreCase) ==
             0) {
    out_value = http::Method::PATCH;
    return true;
  } else {
    return false;
  }
}

}  // namespace fun
