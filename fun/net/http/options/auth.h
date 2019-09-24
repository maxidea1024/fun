#pragma once

#include "fun/http/http.h"

namespace fun {
namespace http {

class Auth {
 public:
  template <typename UserType, typename PassType>
  Auth(UserType&& user_name, PassType&& password)
      : user_name_{FORWARD(user_name)},
        password_{FORWARD(password)},
        auth_string_{user_name_ + ":" + password_} {}

  const char* GetAuthString() const noexcept { return auth_string_.c_str(); }

 private:
  /** Username for authentication */
  String user_name_;

  /** password_ for authentication */
  String password_;

  /** Combined authentication string */
  String auth_string_;
};

}  // namespace http
}  // namespace fun
