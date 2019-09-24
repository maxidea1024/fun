#pragma once

#include "fun/http/http.h"


namespace fun {
namespace http {


class Redirect
{
public:
  Redirect()
    : redirect(true)
  {
  }

	Redirect(bool redirect)
    : redirect(redirect)
  {
  }

	bool redirect;
};


} // namespace http
} // namespace fun
