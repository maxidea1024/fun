#pragma once

#include "fun/http/http.h"

namespace fun {
namespace http {

class Timeout {
 public:
  Timeout(const Timespan& timeout)
      : millisec((int64)timeout.TotalMilliseconds()) {}

  Timeout(int64 millisec) : millisec(InMillisec) {}

  int64 millisec;
};

}  // namespace http
}  // namespace fun
