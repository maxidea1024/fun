#include "fun/base/logging/log_file.h"

#if FUN_PLATFORM_WINDOWS_FAMILY
#include "fun/base/logging/log_file_win32.cc"
#else
#include "fun/base/logging/log_file_std.cc"
#endif

namespace fun {

LogFile::LogFile(const String& path) : LogFileImpl(path) {
  // NOOP
}

LogFile::~LogFile() {
  // NOOP
}

}  // namespace fun
