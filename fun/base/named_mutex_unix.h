#pragma once

#include "fun/base/base.h"

#include <sys/types.h>
#include <sys/stat.h>

#if defined(sun) || defined(__APPLE__) || defined(__osf__) || defined(__QNX__) || defined(_AIX) || defined(__EMSCRIPTEN__)
#include <semaphore.h>
#endif

namespace fun {

class String;

class FUN_BASE_API NamedMutexImpl {
 protected:
  NamedMutexImpl(const String& name);
  ~NamedMutexImpl();

  void LockImpl();
  bool TryLockImpl();
  void UnlockImpl();

 private:
  String GetFileName();

  String name_;
#if defined(sun) || defined(__APPLE__) || defined(__osf__) || defined(__QNX__) || defined(_AIX) || defined(__EMSCRIPTEN__)
  sem_t* sem_;
#else
  int sem_id_; // semaphore id
  bool owned_;
#endif
};

} // namespace fun
