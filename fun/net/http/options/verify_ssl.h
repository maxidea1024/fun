#pragma once

#include "fun/http/http.h"

namespace fun {
namespace http {

class VerifySsl {
 public:
  VerifySsl() : verify(true) {}
  VerifySsl(bool verify) : verify(verify) {}

  bool verify;
};

} // namespace http
} // namespace fun
