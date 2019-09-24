#pragma once

#include "fun/base/base.h"

namespace fun {

class String;

class FUN_BASE_API NamedMutexImpl {
 protected:
  NamedMutexImpl(const String& name);
  ~NamedMutexImpl();

  void LockImpl();
  bool TryLockImpl();
  void UnlockImpl();
};

} // namespace fun
