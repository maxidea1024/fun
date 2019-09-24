#pragma once

#include "fun/base/base.h"

#if FUN_PLATFORM_WINDOWS_FAMILY
#include "fun/base/logging/log_file_win32.h"
#else
#include "fun/base/logging/log_file_std.h"
#endif

namespace fun {

/**
 * This class is used by FileChannel to work
 * with a log file.
 */
class FUN_BASE_API LogFile : public LogFileImpl {
 public:
  LogFile(const String& path);
  ~LogFile();

  void Write(const String& text, bool flush);
  uint64 GetSize() const;
  Timestamp GetCreationDate() const;
  const String GetPath() const;
};

//
// inlines
//

FUN_ALWAYS_INLINE void LogFile::Write(const String& text, bool flush) {
  WriteImpl(text, flush);
}

FUN_ALWAYS_INLINE uint64 LogFile::GetSize() const { return GetSizeImpl(); }

FUN_ALWAYS_INLINE Timestamp LogFile::GetCreationDate() const {
  return GetCreationDateImpl();
}

FUN_ALWAYS_INLINE const String LogFile::GetPath() const {
  return GetPathImpl();
}

}  // namespace fun
