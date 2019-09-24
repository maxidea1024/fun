#pragma once

#include "fun/base/base.h"
#include "fun/base/ref_counted.h"
#include "fun/base/shared_memory.h"

namespace fun {

class FUN_BASE_API SharedMemoryImpl : public RefCountedObject {
 public:
  SharedMemoryImpl(const String& id, size_t size, SharedMemory::AccessMode mode,
                   const void* addr_hint, bool server);

  SharedMemoryImpl(const File& file, SharedMemory::AccessMode mode,
                   const void* addr);

  SharedMemoryImpl() = delete;
  SharedMemoryImpl(const SharedMemoryImpl&) = delete;
  SharedMemoryImpl& operator=(const SharedMemoryImpl&) = delete;

  char* begin() const;
  char* end() const;

 protected:
  ~SharedMemoryImpl();
};

//
// inlines
//

FUN_ALWAYS_INLINE char* SharedMemoryImpl::begin() const { return nullptr; }

FUN_ALWAYS_INLINE char* SharedMemoryImpl::end() const { return nullptr; }

}  // namespace fun
