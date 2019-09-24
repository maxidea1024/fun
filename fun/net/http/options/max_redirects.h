#pragma once

#include "fun/http/http.h"


namespace fun {
namespace http {


class MaxRedirects
{
public:
  MaxRedirects() = delete;

  explicit MaxRedirects(int32 limit)
    : limit(limit)
  {
  }

  int32 limit;
};


} // namespace http
} // namespace fun
