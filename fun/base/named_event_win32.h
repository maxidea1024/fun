#pragma once

#include "fun/base/base.h"
#include "fun/base/windows_less.h"
#include "fun/base/string/string.h"

namespace fun {

class FUN_BASE_API NamedEventImpl {
 protected:
  NamedEventImpl(const String& name);
  ~NamedEventImpl();

  void SetImpl();
  void WaitImpl();

 private:
  String name_;
  UString uname_;
  HANDLE event_;
};

} // namespace fun
