#include "fun/base/shared_memory.h"
#include "fun/base/exception.h"

#if FUN_NO_SHAREDMEMORY
#include "fun/base/shared_memory_dummy.cc"
#elif FUN_PLATFORM_WINDOWS_FAMILY
#include "fun/base/shared_memory_win32.cc"
#elif FUN_PLATFORM_UNIX_FAMILY
#include "fun/base/shared_memory_posix.cc"
#else
#include "fun/base/shared_memory_dummy.cc"
#endif

namespace fun {

SharedMemory::SharedMemory() : impl_(nullptr) {}

SharedMemory::SharedMemory( const String& name,
                            size_t size,
                            AccessMode mode,
                            const void* addr_hint,
                            bool server)
  : impl_(new SharedMemoryImpl(name, size, mode, addr_hint, server)) {}

SharedMemory::SharedMemory(const File& file, AccessMode mode, const void* addr_hint)
  : impl_(new SharedMemoryImpl(file, mode, addr_hint)) {}

SharedMemory::SharedMemory(const SharedMemory& other)
  : impl_(other.impl_) {
  if (impl_) {
    impl_->AddRef();
  }
}

SharedMemory::~SharedMemory() {
  if (impl_) {
    impl_->Release();
  }
}

SharedMemory& SharedMemory::operator = (const SharedMemory& other) {
  SharedMemory tmp(other);
  Swap(tmp);
  return *this;
}

char* SharedMemory::begin() const {
  return impl_ ? impl_->begin() : nullptr;
}

char* SharedMemory::end() const {
  return impl_ ? impl_->end() : nullptr;
}

} // namespace fun
