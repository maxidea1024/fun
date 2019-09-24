#pragma once

#include "fun/base/base.h"

#if defined(sun) || defined(__APPLE__) || defined(__osf__) || defined(__QNX__) || defined(_AIX) || defined(__EMSCRIPTEN__)
#include <semaphore.h>
#endif

namespace fun {

class FUN_BASE_API NamedEventImpl {
 protected:
  NamedEventImpl(const String& name);
  ~NamedEventImpl();

  void SetImpl();
  void WaitImpl();

 private:
  String GetFileName();

  String name_;
#if defined(sun) || defined(__APPLE__) || defined(__osf__) || defined(__QNX__) || defined(_AIX) || defined(__EMSCRIPTEN__)
  sem_t* sem_;
#else
  int sem_id_; // semaphore id
#endif
};

} // namespace fun
