#include "fun/base/thread_local.h"
#include "fun/base/thread.h"
#include "fun/base/singleton.h"

namespace fun {

TlsSlotBase::TlsSlotBase() {}

TlsSlotBase::~TlsSlotBase() {}

ThreadLocalStorage::ThreadLocalStorage() {}

ThreadLocalStorage::~ThreadLocalStorage() {
  for (auto& pair : map_) {
    delete pair.value;
  }
}

TlsSlotBase*& ThreadLocalStorage::Get(const void* key) {
  auto* found = map_.Find(key);
  if (found) {
    return *found;
  } else {
    return map_.Emplace(key);
  }
}

ThreadLocalStorage& ThreadLocalStorage::Current() {
  Thread* current_thread = Thread::Current();
  if (current_thread) {
    return current_thread->Tls();
  } else {
    static Singleton<ThreadLocalStorage> instance;
    return *instance.GetPtr();
  }
}

void ThreadLocalStorage::Clear() {
  Thread* current_thread = Thread::Current();
  if (current_thread) {
    current_thread->ClearTls();
  }
}

} // namespace fun
