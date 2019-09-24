#pragma once

#include "fun/base/base.h"
#include "fun/base/string/string.h"

#include <dirent.h>

namespace fun {

class FUN_BASE_API DirectoryIteratorImpl {
 public:
  DirectoryIteratorImpl(const String& path);
  ~DirectoryIteratorImpl();

  void AddRef();
  void Release();

  const String& Get() const;
  const String& Next();

 private:
  DIR* dir_;
  String current_;
  int rc_;
};


//
// inlines
//

FUN_ALWAYS_INLINE const String& DirectoryIteratorImpl::Get() const {
  return current_;
}

FUN_ALWAYS_INLINE void DirectoryIteratorImpl::AddRef() {
  ++rc_;
}

FUN_ALWAYS_INLINE void DirectoryIteratorImpl::Release() {
  if (--rc_ == 0) {
    delete this;
  }
}

} // namespace fun
