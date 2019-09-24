//@deprecated pthread에서 thread owner를 알아내는 방법에 대해서 생각해보자.

#pragma once

#include "fun/net/net.h"

namespace fun {
namespace net {

class FUN_NETX_API CCriticalSection2 {
 public:
  CRITICAL_SECTION cs;

  CCriticalSection2(bool bFasterForWindows6 = true);
  ~CCriticalSection2();

  bool IsLockedByCurrentThread();
  void Lock();
  void UnsafeLock();
  void Unlock();
  bool TryLock();
  bool IsValid();
  /// TODO pthread에서 할수 있는 방법이 있으려나?
  void AssertIsLockedByCurrentThread();
  /// TODO pthread에서 할수 있는 방법이 있으려나?
  void AssertIsNotLockedByCurrentThread();

  bool bNeverCallDtor;

  // disable copy
  CCriticalSection2(const CCriticalSection2&) = delete;
  CCriticalSection2& operator=(const CCriticalSection2&) = delete;

 private:
  void Lock_DetectLongPreviousLockingThread();

  void Lock_DetectLongLockingRoutine();
  void Unlock_DetectLongLockingRoutine();

  uint32 ValidKey;
};

/// A scoped synchronizer utilizing the RAII pattern.
class CScopedLock2 {
 public:
  inline CScopedLock2(CCriticalSection2& cs, bool initial_lock = true) {
    SetCS(cs, initial_lock);
  }

  inline CScopedLock2() {
    cs = nullptr;
    bLocked = false;
  }

  inline void SetCS(CCriticalSection2& InCS, bool initial_lock) {
    cs = &InCS;
    bLocked = false;
    if (initial_lock) {
      cs->Lock();
      bLocked = true;
    }
  }

  inline bool IsLocked() const { return bLocked; }

  inline ~CScopedLock2() {
    if (bLocked) {
      Unlock();
    }
  }

  inline void Lock() {
    fun_check(!bLocked);

    bLocked = true;
    cs->Lock();
  }

  inline bool TryLock() {
    fun_check(!bLocked);
    bLocked = cs->TryLock();
    return bLocked;
  }

  inline void UnsafeLock() {
    bLocked = true;
    cs->UnsafeLock();
  }

  inline void Unlock() {
    cs->Unlock();
    bLocked = false;
  }

  // disable copy
  CScopedLock2(const CScopedLock2&) = delete;
  CScopedLock2& operator=(const CScopedLock2&) = delete;

 private:
  CCriticalSection2* cs;
  bool bLocked;
};

// TODO pthread에서 할수 있는 방법이 있으려나?
/// Check that the specified critical section is locked in the current thread
/// (lock).
FUN_NETX_API bool IsCriticalSectionLockedByCurrentThread(
    const CRITICAL_SECTION& cs);

// TODO pthread에서 할수 있는 방법이 있으려나?
/// Check that the specified critical section is locked.
FUN_NETX_API bool IsCriticalSectionLocked(const CRITICAL_SECTION& cs);

// TODO pthread에서 할수 있는 방법이 있으려나?
/// Check that the specified critical section is locked in the current thread
/// (lock).
inline bool IsCriticalSectionLockedByCurrentThread(
    const CCriticalSection2& cs) {
  return IsCriticalSectionLockedByCurrentThread(cs.cs);
}

// TODO pthread에서 할수 있는 방법이 있으려나?
/// Check that the specified critical section is locked.
inline bool IsCriticalSectionLocked(const CCriticalSection2& cs) {
  return IsCriticalSectionLocked(cs.cs);
}

}  // namespace net
}  // namespace fun
