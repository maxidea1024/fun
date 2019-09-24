#pragma once

#include <shared_mutex>
#include "fun/base/base.h"

namespace fun {

class FUN_BASE_API RWLockImpl {
 protected:
  RWLockImpl();

  void ReadLockImpl();
  bool TryReadLockImpl();
  void WriteLockImpl();
  bool TryWriteLockImpl();
  void UnlockImpl();

 private:
  std::shared_timed_mutex mutex_;
};

}  // namespace fun
