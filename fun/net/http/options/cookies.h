#pragma once

#include "fun/http/http.h"
#include <initializer_list>
#include <map>

#include "fun/http/uri.h"

namespace fun {
namespace http {

class Cookies {
public:
  Cookies() {}

  Cookies(const std::initializer_list<std::pair<const String, String>>& pairs)
    : map_{pairs} {}

  Cookies(const std::map<String, String>& map)
    : map_{map} {}

  String& operator[](const String& key) {
    return map_[key];
  }

  String GetEncoded() const {
    String encoded;

    for (const auto& pair : map_) {
      encoded << Uri::Encode(pair.first) << "=";

      // special case version 1 cookies, which can be distinguished by
      // beginning and trailing quotes
      if (!pair.second.IsEmpty() && pair.second.First() == '"' && pair.second.Last() == '"') {
        encoded << pair.second;
      }
      else {
        encoded << Uri::Encode(pair.second);
      }

      encoded << "; ";
    }

    return encoded;
  }

 private:
  std::map<String, String> map_;
};

} // namespace http
} // namespace fun
