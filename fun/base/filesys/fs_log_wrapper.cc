#include "fun/base/filesys/IPlatformFSLogWrapper.h"

namespace fun {

bool g_suppress_file_log = false;
DEFINE_LOG_CATEGORY(LogPlatformFS);

#if !FUN_BUILD_SHIPPING

class FileLogExec : private SelfRegisteringExec {
 public:
  FileLogExec(LoggedPlatformFS& platform_fs)
    : platform_fs_(platform_fs) {}

  // Exec interface
  bool Exec(CRuntimeEnv* env, const char* cmd, Printer& out) override {
    if (Parse::Command(&cmd, "LogFileDump")) {
      platform_fs_.HandleDumpCommand(cmd, out);
      return true;
    }
    return false;
  }

 private:
  LoggedPlatformFS& platform_fs_;
};
static AutoPtr<FileLogExec> g_file_log_exec;

#endif //!FUN_BUILD_SHIPPING


bool LoggedPlatformFS::ShouldBeUsed(IPlatformFS* inner, const char* cmdline) const {
  return Parse::Param(cmdline, "FileLog");
}

bool LoggedPlatformFS::Initialize(IPlatformFS* inner, const char* cmdline) {
  fun_check_ptr(inner);
  lower_level_ = inner;

#if !FUN_BUILD_SHIPPING
  g_file_log_exec = new FileLogExec(*this);
#endif

  return !!lower_level_;
}

#if !FUN_BUILD_SHIPPING
void LoggedPlatformFS::HandleDumpCommand(const char* cmd, Printer& out) {
  ScopedLock<FastMutex> log_file_guard(log_file_mutex_);
  g_suppress_file_log = true;
  out.Printf("Open file handles: %d", open_handles_.Count());
  for (auto& pair : open_handles_) {
    out.Printf("%s: %d", *pair.key, pair.value);
  }
  g_suppress_file_log = false;
}
#endif


LoggedFile::LoggedFile(IFile* file, const char* filename, LoggedPlatformFS& owner)
  : file_(file)
  , filename_(filename)
#if !FUN_BUILD_SHIPPING
  , platform_fs_(owner)
#endif {
#if !FUN_BUILD_SHIPPING
  platform_fs_.OnHandleOpen(filename_);
#endif
}

LoggedFile::~LoggedFile() {
#if !FUN_BUILD_SHIPPING
  platform_fs_.OnHandleClosed(filename_);
#endif
  FILE_LOG(LogPlatformFS, Info, "Close %s", *filename_);
}

} // namespace fun
