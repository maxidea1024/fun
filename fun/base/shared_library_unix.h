#pragma once

#include "fun/base/base.h"
#include "fun/base/mutex.h"

namespace fun {

class FUN_BASE_API SharedLibrayImpl {
 public:
  enum Flags { SHLIB_GLOBAL_IMPL = 1, SHLIB_LOCAL_IMPL = 2 };

  SharedLibrayImpl();
  virtual ~SharedLibrayImpl();

  void LoadImpl(const String& path, int32 flags);
  void UnloadImpl();
  bool IsLoadedImpl() const;
  void* FindSymbolImpl(const String& symbol);

  const String& GetPathImpl() const;

  static String PrefixImpl();
  static String SuffixImpl();

 private:
  String path_;
  void* handle_;

  static FastMutex mutex_;
};

}  // namespace fun
