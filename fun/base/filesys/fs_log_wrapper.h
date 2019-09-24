#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * Wrapper to log the low level file system
 */
DECLARE_LOG_CATEGORY_EXTERN(LogPlatformFS, Info, All);

extern bool g_suppress_file_log;

#define FILE_LOG(CategoryName, Verbosity, Format, ...) \
  if (!g_suppress_file_log) { \
    g_suppress_file_log = true; \
    fun_log(CategoryName, Verbosity, Format, ##__VA_ARGS__); \
    g_suppress_file_log = false; \
  }


class LoggedPlatformFS;

class FUN_BASE_API LoggedFile : public IFile {
 public:
  LoggedFile(IFile* file, const char* filename, LoggedPlatformFS& owner);
  virtual ~LoggedFile();

  int64 Tell() override {
    FILE_LOG(LogPlatformFS, Trace, "Tell %s", *filename_);
    double start_time = SystemTime::Seconds();
    int64 result = file_->Tell();
    float spent_time = 1000.0f * float(SystemTime::Seconds() - start_time);
    FILE_LOG(LogPlatformFS, Trace, "Tell return %lld [%fms]", result, spent_time);
    return result;
  }

  bool Seek(int64 new_pos) override {
    FILE_LOG(LogPlatformFS, Trace, "Seek %s %lld", *filename_, new_pos);
    double start_time = SystemTime::Seconds();
    bool result = file_->Seek(new_pos);
    float spent_time = 1000.0f * float(SystemTime::Seconds() - start_time);
    FILE_LOG(LogPlatformFS, Trace, "Seek return %d [%fms]", int32(result), spent_time);
    return result;
  }

  bool SeekFromEnd(int64 relative_pos_to_end) override {
    FILE_LOG(LogPlatformFS, Trace, "SeekFromEnd %s %lld", *filename_, relative_pos_to_end);
    double start_time = SystemTime::Seconds();
    bool result = file_->SeekFromEnd(relative_pos_to_end);
    float spent_time = 1000.0f * float(SystemTime::Seconds() - start_time);
    FILE_LOG(LogPlatformFS, Trace, "SeekFromEnd return %d [%fms]", int32(result), spent_time);
    return result;
  }

  bool Read(uint8* dst, int64 len_to_read) override {
    FILE_LOG(LogPlatformFS, Trace, "Read %s %lld", *filename_, len_to_read);
    double start_time = SystemTime::Seconds();
    bool result = file_->Read(dst, len_to_read);
    float spent_time = 1000.0f * float(SystemTime::Seconds() - start_time);
    FILE_LOG(LogPlatformFS, Trace, "Read return %d [%fms]", int32(result), spent_time);
    return result;
  }

  bool Write(const uint8* src, int64 len_to_write) override {
    FILE_LOG(LogPlatformFS, Trace, "Write %s %lld", *filename_, len_to_write);
    double start_time = SystemTime::Seconds();
    bool result = file_->Write(src, len_to_write);
    float spent_time = 1000.0f * float(SystemTime::Seconds() - start_time);
    FILE_LOG(LogPlatformFS, Trace, "Write return %d [%fms]", int32(result), spent_time);
    return result;
  }

  int64 Size() override {
    FILE_LOG(LogPlatformFS, Trace, "Size %s", *filename_);
    double start_time = SystemTime::Seconds();
    int64 result = file_->Size();
    float spent_time = 1000.0f * float(SystemTime::Seconds() - start_time);
    FILE_LOG(LogPlatformFS, Trace, "Size return %lld [%fms]", result, spent_time);
    return result;
  }

 private:
  AutoPtr<IFile> file_;
  String filename_;

#if !FUN_BUILD_SHIPPING
  LoggedPlatformFS& platform_fs_;
#endif
};

class FUN_BASE_API LoggedPlatformFS : public IPlatformFS {
 public:
  static const char* GetTypeName() {
    return "LogFile";
  }

  LoggedPlatformFS() : lower_level_(nullptr) {}

  bool ShouldBeUsed(IPlatformFS* inner, const char* cmdline) const override;

  bool Initialize(IPlatformFS* inner, const char* cmdline) override;

  IPlatformFS* GetLowerLevel() override {
    return lower_level_;
  }

  const char* GetName() const override {
    return LoggedPlatformFS::GetTypeName();
  }

  bool FileExists(const char* filename) override {
    FILE_LOG(LogPlatformFS, Info, "FileExists %s", filename);
    double start_time = SystemTime::Seconds();
    bool result = lower_level_->FileExists(filename);
    float spent_time = 1000.0f * float(SystemTime::Seconds() - start_time);
    FILE_LOG(LogPlatformFS, Info, "FileExists return %d [%fms]", int32(result), spent_time);
    return result;
  }

  int64 FileSize(const char* filename) override {
    FILE_LOG(LogPlatformFS, Info, "FileSize %s", filename);
    double start_time = SystemTime::Seconds();
    int64 result = lower_level_->FileSize(filename);
    float spent_time = 1000.0f * float(SystemTime::Seconds() - start_time);
    FILE_LOG(LogPlatformFS, Info, "FileSize return %lld [%fms]", result, spent_time);
    return result;
  }

  bool DeleteFile(const char* filename) override {
    FILE_LOG(LogPlatformFS, Info, "DeleteFile %s", filename);
    double start_time = SystemTime::Seconds();
    bool result = lower_level_->DeleteFile(filename);
    float spent_time = 1000.0f * float(SystemTime::Seconds() - start_time);
    FILE_LOG(LogPlatformFS, Info, "DeleteFile return %d [%fms]", int32(result), spent_time);
    return result;
  }

  bool IsReadOnly(const char* filename) override {
    FILE_LOG(LogPlatformFS, Info, "IsReadOnly %s", filename);
    double start_time = SystemTime::Seconds();
    bool result = lower_level_->IsReadOnly(filename);
    float spent_time = 1000.0f * float(SystemTime::Seconds() - start_time);
    FILE_LOG(LogPlatformFS, Info, "IsReadOnly return %d [%fms]", int32(result), spent_time);
    return result;
  }

  bool MoveFile(const char* to, const char* from) override {
    FILE_LOG(LogPlatformFS, Info, "MoveFile %s %s", to, from);
    double start_time = SystemTime::Seconds();
    bool result = lower_level_->MoveFile(to, from);
    float spent_time = 1000.0f * float(SystemTime::Seconds() - start_time);
    FILE_LOG(LogPlatformFS, Info, "MoveFile return %d [%fms]", int32(result), spent_time);
    return result;
  }

  bool SetReadOnly(const char* filename, bool readonly) override {
    FILE_LOG(LogPlatformFS, Info, "SetReadOnly %s %d", filename, int32(readonly));
    double start_time = SystemTime::Seconds();
    bool result = lower_level_->SetReadOnly(filename, readonly);
    float spent_time = 1000.0f * float(SystemTime::Seconds() - start_time);
    FILE_LOG(LogPlatformFS, Info, "SetReadOnly return %d [%fms]", int32(result), spent_time);
    return result;
  }

  DateTime GetTimestamp(const char* filename) override {
    FILE_LOG(LogPlatformFS, Info, "GetTimestamp %s", filename);
    double start_time = SystemTime::Seconds();
    DateTime result = lower_level_->GetTimestamp(filename);
    float spent_time = 1000.0f * float(SystemTime::Seconds() - start_time);
    FILE_LOG(LogPlatformFS, Info, "GetTimestamp return %llx [%fms]", result.ToUtcTicks() / DateTimeConstants::TICKS_PER_MILLISECOND, spent_time);
    return result;
  }

  void SetTimestamp(const char* filename, const DateTime& timestamp) override {
    FILE_LOG(LogPlatformFS, Info, "SetTimestamp %s", filename);
    double start_time = SystemTime::Seconds();
    lower_level_->SetTimestamp(filename, timestamp);
    float spent_time = 1000.0f * float(SystemTime::Seconds() - start_time);
    FILE_LOG(LogPlatformFS, Info, "SetTimestamp [%fms]", spent_time);
  }

  DateTime GetAccessTimestamp(const char* filename) override {
    FILE_LOG(LogPlatformFS, Info, "GetAccessTimestamp %s", filename);
    double start_time = SystemTime::Seconds();
    DateTime result = lower_level_->GetAccessTimestamp(filename);
    float spent_time = 1000.0f * float(SystemTime::Seconds() - start_time);
    FILE_LOG(LogPlatformFS, Info, "GetAccessTimestamp return %llx [%fms]", result.ToUtcTicks() / DateTimeConstants::TICKS_PER_MILLISECOND, spent_time);
    return result;
  }

  String GetFilenameOnDisk(const char* filename) override {
    FILE_LOG(LogPlatformFS, Info, "GetFilenameOnDisk %s", filename);
    double start_time = SystemTime::Seconds();
    String result = lower_level_->GetFilenameOnDisk(filename);
    float spent_time = 1000.0f * float(SystemTime::Seconds() - start_time);
    FILE_LOG(LogPlatformFS, Info, "GetFilenameOnDisk return %s [%fms]", *result, spent_time);
    return result;
  }

  IFile* OpenRead(const char* filename, bool allow_write) override {
    FILE_LOG(LogPlatformFS, Info, "OpenRead %s", filename);
    double start_time = SystemTime::Seconds();
    IFile* result = lower_level_->OpenRead(filename, allow_write);
    float spent_time = 1000.0f * float(SystemTime::Seconds() - start_time);
    FILE_LOG(LogPlatformFS, Info, "OpenRead return %llx [%fms]", uint64(result), spent_time);
    return result != nullptr ? (new LoggedFile(result, filename, *this)) : result;
  }

  IFile* OpenWrite(const char* filename, bool append = false, bool allow_read = false) override {
    FILE_LOG(LogPlatformFS, Info, "OpenWrite %s %d %d", filename, int32(append), int32(allow_read));
    double start_time = SystemTime::Seconds();
    IFile* result = lower_level_->OpenWrite(filename, append, allow_read);
    float spent_time = 1000.0f * float(SystemTime::Seconds() - start_time);
    FILE_LOG(LogPlatformFS, Info, "OpenWrite return %llx [%fms]", uint64(result), spent_time);
    return result != nullptr ? (new LoggedFile(result, filename, *this)) : result;
  }

  bool DirectoryExists(const char* directory) override {
    FILE_LOG(LogPlatformFS, Info, "DirectoryExists %s", directory);
    double start_time = SystemTime::Seconds();
    bool result = lower_level_->DirectoryExists(directory);
    float spent_time = 1000.0f * float(SystemTime::Seconds() - start_time);
    FILE_LOG(LogPlatformFS, Info, "DirectoryExists return %d [%fms]", int32(result), spent_time);
    return result;
  }

  bool CreateDirectory(const char* directory) override {
    FILE_LOG(LogPlatformFS, Info, "CreateDirectory %s", directory);
    double start_time = SystemTime::Seconds();
    bool result = lower_level_->CreateDirectory(directory);
    float spent_time = 1000.0f * float(SystemTime::Seconds() - start_time);
    FILE_LOG(LogPlatformFS, Info, "CreateDirectory return %d [%fms]", int32(result), spent_time);
    return result;
  }

  bool DeleteDirectory(const char* directory) override {
    FILE_LOG(LogPlatformFS, Info, "DeleteDirectory %s", directory);
    double start_time = SystemTime::Seconds();
    bool result = lower_level_->DeleteDirectory(directory);
    float spent_time = 1000.0f * float(SystemTime::Seconds() - start_time);
    FILE_LOG(LogPlatformFS, Info, "DeleteDirectory return %d [%fms]", int32(result), spent_time);
    return result;
  }

  FileStatData GetStatData(const char* filename_or_directory) override {
    FILE_LOG(LogPlatformFS, Info, "GetStatData %s", filename_or_directory);
    double start_time = SystemTime::Seconds();
    FileStatData result = lower_level_->GetStatData(filename_or_directory);
    float spent_time = 1000.0f * float(SystemTime::Seconds() - start_time);
    FILE_LOG(LogPlatformFS, Info, "GetStatData return %d [%fms]", int32(result.is_valid), spent_time);
    return result;
  }

  struct LogVisitor : public IPlatformFS::DirectoryVisitor {
    DirectoryVisitor& visitor;

    LogVisitor(DirectoryVisitor& visitor) : visitor(visitor) {}

    bool Visit(const char* filename_or_directory, bool is_directory) override {
      FILE_LOG(LogPlatformFS, Trace, "Visit %s %d", filename_or_directory, int32(is_directory));
      double start_time = SystemTime::Seconds();
      bool result = visitor.Visit(filename_or_directory, is_directory);
      float spent_time = 1000.0f * float(SystemTime::Seconds() - start_time);
      FILE_LOG(LogPlatformFS, Trace, "Visit return %d [%fms]", int32(result), spent_time);
      return result;
    }
  };

  bool IterateDirectory(const char* directory, IPlatformFS::DirectoryVisitor& visitor) override {
    FILE_LOG(LogPlatformFS, Info, "IterateDirectory %s", directory);
    double start_time = SystemTime::Seconds();
    LogVisitor LogVisitor(visitor);
    bool result = lower_level_->IterateDirectory(directory, LogVisitor);
    float spent_time = 1000.0f * float(SystemTime::Seconds() - start_time);
    FILE_LOG(LogPlatformFS, Info, "IterateDirectory return %d [%fms]", int32(result), spent_time);
    return result;
  }

  bool IterateDirectoryRecursively(const char* directory, IPlatformFS::DirectoryVisitor& visitor) override {
    FILE_LOG(LogPlatformFS, Info, "IterateDirectoryRecursively %s", directory);
    double start_time = SystemTime::Seconds();
    LogVisitor LogVisitor(visitor);
    bool result = lower_level_->IterateDirectoryRecursively(directory, LogVisitor);
    float spent_time = 1000.0f * float(SystemTime::Seconds() - start_time);
    FILE_LOG(LogPlatformFS, Info, "IterateDirectoryRecursively return %d [%fms]", int32(result), spent_time);
    return result;
  }

  struct LogStatVisitor : public IPlatformFS::DirectoryStatVisitor {
    DirectoryStatVisitor& visitor;

    LogStatVisitor(DirectoryStatVisitor& visitor) : visitor(visitor) {}

    bool Visit(const char* filename_or_directory, const FileStatData& stat_data) override {
      FILE_LOG(LogPlatformFS, Trace, "Visit %s %d", filename_or_directory, int32(stat_data.is_directory));
      double start_time = SystemTime::Seconds();
      bool result = visitor.Visit(filename_or_directory, stat_data);
      float spent_time = 1000.0f * float(SystemTime::Seconds() - start_time);
      FILE_LOG(LogPlatformFS, Trace, "Visit return %d [%fms]", int32(result), spent_time);
      return result;
    }
  };

  bool IterateDirectoryStat(const char* directory, IPlatformFS::DirectoryStatVisitor& visitor) override {
    FILE_LOG(LogPlatformFS, Info, "IterateDirectoryStat %s", directory);
    double start_time = SystemTime::Seconds();
    LogStatVisitor LogVisitor(visitor);
    bool result = lower_level_->IterateDirectoryStat(directory, LogVisitor);
    float spent_time = 1000.0f * float(SystemTime::Seconds() - start_time);
    FILE_LOG(LogPlatformFS, Info, "IterateDirectoryStat return %d [%fms]", int32(result), spent_time);
    return result;
  }

  bool IterateDirectoryStatRecursively(const char* directory, IPlatformFS::DirectoryStatVisitor& visitor) override {
    FILE_LOG(LogPlatformFS, Info, "IterateDirectoryStatRecursively %s", directory);
    double start_time = SystemTime::Seconds();
    LogStatVisitor LogVisitor(visitor);
    bool result = lower_level_->IterateDirectoryStatRecursively(directory, LogVisitor);
    float spent_time = 1000.0f * float(SystemTime::Seconds() - start_time);
    FILE_LOG(LogPlatformFS, Info, "IterateDirectoryStatRecursively return %d [%fms]", int32(result), spent_time);
    return result;
  }

  virtual bool DeleteDirectoryRecursively(const char* directory) override {
    FILE_LOG(LogPlatformFS, Info, "DeleteDirectoryRecursively %s", directory);
    double start_time = SystemTime::Seconds();
    bool result = lower_level_->DeleteDirectoryRecursively(directory);
    float spent_time = 1000.0f * float(SystemTime::Seconds() - start_time);
    FILE_LOG(LogPlatformFS, Info, "DeleteDirectoryRecursively return %d [%fms]", int32(result), spent_time);
    return result;
  }

  bool CopyFile(const char* to, const char* from) override {
    FILE_LOG(LogPlatformFS, Info, "CopyFile %s %s", to, from);
    double start_time = SystemTime::Seconds();
    bool result = lower_level_->CopyFile(to, from);
    float spent_time = 1000.0f * float(SystemTime::Seconds() - start_time);
    FILE_LOG(LogPlatformFS, Info, "CopyFile return %d [%fms]", int32(result), spent_time);
    return result;
  }

#if !FUN_BUILD_SHIPPING
  void OnHandleOpen(const String& filename) {
    ScopedLock<FastMutex> log_file_guard(log_file_mutex_);
    int32& open_handle_count = open_handles_.FindOrAdd(filename);
    open_handle_count++;
  }

  void OnHandleClosed(const String& filename) {
    ScopedLock<FastMutex> log_file_guard(log_file_mutex_);
    int32& open_handle_count = open_handles_.FindChecked(filename);
    if (--open_handle_count == 0) {
      open_handles_.Remove(filename);
    }
  }

  void HandleDumpCommand(const char* cmd, Printer& out);
#endif //!FUN_BUILD_SHIPPING

 private:
  IPlatformFS* lower_level_;

#if !FUN_BUILD_SHIPPING
  FastMutex log_file_mutex_;
  Map<String, int32> open_handles_;
#endif
};

} // namespace fun
