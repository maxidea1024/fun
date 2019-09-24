#pragma once

#include "fun/http/http.h"


namespace fun {
namespace http {


class LowSpeed
{
public:
  LowSpeed() = delete;

  LowSpeed(const int32 limit, const int32 time)
    : limit(limit)
    , time(time)
  {
  }

  int32 limit;
  int32 time;
};


} // namespace http
} // namespace fun
