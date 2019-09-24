#pragma once

#include "fun/http/auth.h"

namespace fun {
namespace http {

class Digest : public Auth {
 public:
  Digest() = delete;

  template <typename UserType, typename PassType>
  Digest(UserType&& user_name, PassType&& password)
    : Auth{FORWARD(user_name), FORWARD(password)} {}

  const char* GetAuthString() const noexcept {
    return Auth::GetAuthString();
  }
};

} // namespace http
} // namespace fun
