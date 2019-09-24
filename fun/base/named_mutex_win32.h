#pragma once

#include "fun/base/base.h"
#include "fun/base/windows_less.h"
#include "fun/base/string/string.h"

namespace fun {

class FUN_BASE_API NamedMutexImpl {
 protected:
  NamedMutexImpl(const String& name);
  ~NamedMutexImpl();

  void LockImpl();
  bool TryLockImpl();
  void UnlockImpl();

 private:
  String name_;
  UString uname_;
  HANDLE mutex_;
};

} // namespace fun
