#pragma once

#include "fun/base/base.h"
#include "fun/base/ref_counted.h"
#include "fun/base/shared_memory.h"
#include "fun/base/string/string.h"

namespace fun {

class FUN_BASE_API SharedMemoryImpl : public RefCountedObject {
 public:
  SharedMemoryImpl(const String& name, size_t size,
                   SharedMemory::AccessMode mode, const void* addr_hint,
                   bool server);

  SharedMemoryImpl(const File& file, SharedMemory::AccessMode mode,
                   const void* addr_hint);

  char* begin() const;
  char* end() const;

  SharedMemoryImpl() = delete;
  SharedMemoryImpl(const SharedMemoryImpl&) = delete;
  SharedMemoryImpl& operator=(const SharedMemoryImpl&) = delete;

 protected:
  void Map(const void* addr_hint);
  void Unmap();
  void Close();

  ~SharedMemoryImpl();

 private:
  size_t size_;
  int fd_;
  char* address_;
  SharedMemory::AccessMode access_mode_;
  String name_;
  bool file_mapped_;
  bool server_;
};

//
// inlines
//

FUN_ALWAYS_INLINE char* SharedMemoryImpl::begin() const { return address_; }

FUN_ALWAYS_INLINE char* SharedMemoryImpl::end() const {
  return address_ + size_;
}

}  // namespace fun
