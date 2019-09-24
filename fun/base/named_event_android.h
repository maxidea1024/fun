#pragma once

#include "fun/base/base.h"

namespace fun {

class String;

class FUN_BASE_API NamedEventImpl {
 protected:
  NamedEventImpl(const String& name);
  ~NamedEventImpl();

  void SetImpl();
  void WaitImpl();
};

} // namespace fun
