#pragma once

#include "fun/base/base.h"
#include "fun/base/string/string.h"
#include "fun/base/timestamp.h"

// TODO 생짜로 하나 만들어보자!!
//#include "fun/base/file_stream.h"

namespace fun {

/**
 * The implementation of LogFile for non-Windows platforms.
 * The native filesystem APIs are used for
 * total control over locking behavior.
 */
class FUN_BASE_API LogFileImpl {
 public:
  LogFileImpl(const String& path);
  ~LogFileImpl();

  void WriteImpl(const String& text, bool flush);
  uint64 GetSizeImpl() const;
  Timestamp GetCreationDateImpl() const;
  const String& GetPathImpl() const;

 private:
  String path_;
  // TODO
  // mutable FileOutputStream str_;
  uint64 size_;
  Timestamp creation_date_;
};

}  // namespace fun
