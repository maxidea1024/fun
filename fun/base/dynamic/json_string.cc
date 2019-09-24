#include "fun/base/json_string.h"
#include "fun/base/utf8_string.h"

#include <ostream>

namespace {

template<typename T, typename S>
struct WriteFunc {
  typedef T& (T::*Type)(const char* s, S n);
};

template<typename T, typename S>
void WriteString(const String &value, T& obj, typename WriteFunc<T, S>::Type write, int options) {
  bool wrap = ((options & fun::JSON_WRAP_STRINGS) != 0);
  bool escape_all_unicode = ((options & fun::JSON_ESCAPE_UNICODE) != 0);

  if (value.size() == 0) {
    if (wrap) {
      (obj.*write)("\"\"", 2);
    }
    return;
  }

  if (wrap) {
    (obj.*write)("\"", 1);
  }

  if (escape_all_unicode) {
    String str = fun::Utf8::Escape(value.begin(), value.end(), true);
    (obj.*write)(str.c_str(), str.size());
  } else {
    for(String::const_iterator it = value.begin(), end = value.end(); it != end; ++it) {
      // Forward slash isn't strictly required by JSON spec, but some parsers expect it
      if ((*it >= 0 && *it <= 31) || (*it == '"') || (*it == '\\') || (*it == '/')) {
        String str = fun::Utf8::Escape(it, it + 1, true);
        (obj.*write)(str.c_str(), str.size());
      } else {
        (obj.*write)(&(*it), 1);
      }
    }
  }

  if (wrap) {
    (obj.*write)("\"", 1);
  }
};

} // namespace


namespace fun {

void ToJson(const String& value, std::ostream& out, bool wrap) {
  int options = (wrap ? fun::JSON_WRAP_STRINGS : 0);
  WriteString<std::ostream,
        std::streamsize>(value, out, &std::ostream::write, options);
}

String ToJson(const String& value, bool wrap) {
  int options = (wrap ? fun::JSON_WRAP_STRINGS : 0);
  String ret;
  WriteString<String,
        String::size_type>(value, ret, &String::Append, options);
  return ret;
}

void ToJson(const String& value, std::ostream& out, int options) {
  WriteString<std::ostream, std::streamsize>(value, out, &std::ostream::write, options);
}

String ToJson(const String& value, int options) {
  String ret;
  WriteString<String,
        String::size_type>(value, ret, &String::Append, options);
  return ret;
}

} // namespace fun
