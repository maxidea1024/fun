#pragma once

#include "fun/base/base.h"
#include "fun/base/timestamp.h"
#include "fun/base/string/string.h"
//#include "fun/base/windows_less.h"

namespace fun {

/**
 * The implementation of LogFile for Windows.
 * The native filesystem APIs are used for
 * total control over locking behavior.
 */
class FUN_BASE_API LogFileImpl {
 public:
  LogFileImpl(const String& path);
  ~LogFileImpl();

 public:
  void WriteImpl(const String& text, bool flush);
  uint64 GetSizeImpl() const;
  Timestamp GetCreationDateImpl() const;
  const String& GetPathImpl() const;

 private:
  void CreateFile();

  String path_;
  HANDLE file_handle_;
  Timestamp creation_date_;
};

} // namespace fun
