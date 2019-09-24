#pragma once

#include "fun/http/http.h"
#include <initializer_list>


namespace fun {
namespace http {


class Proxies
{
public:
  Proxies()
  {
  }

  Proxies(const std::initializer_list<std::pair<const String, String>>& hosts)
    : hosts_{hosts}
  {
  }

  bool Has(const String& protocol) const
  {
    return hosts_.count(protocol) > 0;
  }

  const String& operator[](const String& protocol)
  {
    return hosts_[protocol];
  }

private:
  std::map<String, String> hosts_;
};


} // namespace http
} // namespace fun
