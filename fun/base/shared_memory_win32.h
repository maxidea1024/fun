#pragma once

#include "fun/base/base.h"
#include "fun/base/shared_memory.h"
#include "fun/base/ref_counted.h"
#include "fun/base/string/string.h"
#include "fun/base/windows_less.h"

namespace fun {

class FUN_BASE_API SharedMemoryImpl : public RefCountedObject {
 public:
  SharedMemoryImpl( const String& name,
                    size_t size,
                    SharedMemory::AccessMode mode,
                    const void* addr_hint,
                    bool server);

  SharedMemoryImpl( const File& file,
                    SharedMemory::AccessMode mode,
                    const void* addr_hint);

  char* begin() const;
  char* end() const;

  SharedMemoryImpl() = delete;
  SharedMemoryImpl(const SharedMemoryImpl&) = delete;
  SharedMemoryImpl& operator = (const SharedMemoryImpl&) = delete;

 protected:
  void Map();
  void Unmap();
  void Close();

  ~SharedMemoryImpl();

 private:
  String name_;
  HANDLE mem_handle_;
  HANDLE file_handle_;
  DWORD size_;
  DWORD mode_;
  char* address_;
};


//
// inlines
//

FUN_ALWAYS_INLINE char* SharedMemoryImpl::begin() const {
  return address_;
}

FUN_ALWAYS_INLINE char* SharedMemoryImpl::end() const {
  return address_ + size_;
}

} // namespace fun
