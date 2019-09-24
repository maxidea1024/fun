#pragma once

#include "fun/base/base.h"
#include "fun/base/windows_less.h"

namespace fun {

class FUN_BASE_API RWLockImpl {
 protected:
  RWLockImpl();
  ~RWLockImpl();

  void ReadLockImpl();
  bool TryReadLockImpl();
  void WriteLockImpl();
  bool TryWriteLockImpl();
  void UnlockImpl();

 private:
  void AddWriter();
  void RemoveWriter();
  DWORD TryReadLockOnce();

  HANDLE mutex_;
  HANDLE read_event_;
  HANDLE write_event_;
  uint32 readers_;
  uint32 writes_waiting_;
  uint32 writers_;
};

}  // namespace fun
