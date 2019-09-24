#pragma once

#include "fun/http/http.h"
#include <initializer_list>

namespace fun {
namespace http {

class Body {
 public:
  Body() = default;
  Body(const Body& rhs) = default;
  Body(Body&& rhs) = default;
  Body& operator = (const Body& rhs) = default;
  Body& operator = (Body&& rhs) = default;

  // utf8 string.
  explicit Body(const char* Utf8String)
    : Data(Utf8String) {}

  explicit Body(const char* StringOrBinary, int32 Length)
    : Data(StringOrBinary, Length) {}

  explicit Body(const String& StringOrBinary)
    : Data(StringOrBinary) {}

  // unicode string.
  explicit Body(const UNICHAR* UnicodeString)
    : Data(UnicodeString) {}

  // unicode string.
  explicit Body(const UNICHAR* UnicodeString, int32 Length)
    : Data(UnicodeString, Length) {}

  explicit Body(const String& UnicodeString)
    : Data(UnicodeString) {}

  Body(const Array<uint8>& Binary)
    : Data((const char*)Binary.GetData(), Binary.Num()) {}

  explicit Body(String&& String)
    : Data(MoveTemp(String)) {}

  explicit Body(const std::initializer_list<char>& Chars)
    : Data(Chars.begin(), Chars.size()) {}

  //template <typename it>
  //explicit Body(it Begin, it End)
  //  : string(Begin, End) {}

  String Data;
};

} // namespace http
} // namespace fun
