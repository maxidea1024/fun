#pragma once

#include "fun/http/http.h"

#include <map>
#include <string>

namespace fun {
namespace http {

struct HeadersKeyFuncs : BaseKeyFuncs<pair<String, String>, String, false> {
  static inline const String& GetSetKey(const pair<String, String>& element)  {
    return element.key;
  }

  static inline bool Matches(const String& x, const String& y) {
    return x.Equals(y, CaseSensitivity::IgnoreCase);
  }

  static inline uint32 GetKeyHash(const String& key) {
    return Crc::StringCrc32<char>(key.ConstData(), key.Len());
  }
};

typedef Map<String, String, DefaultSetAllocator, HeadersKeyFuncs> Headers;

} // namespace http
} // namespace fun
