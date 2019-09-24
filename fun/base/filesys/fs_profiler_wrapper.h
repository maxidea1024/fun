#pragma once

#include "fun/base/base.h"

#if !FUN_BUILD_SHIPPING

namespace fun {

/**
 * Wrapper to log the low level file system
 */
DECLARE_LOG_CATEGORY_EXTERN(LogProfiledFile, Info, All);

extern bool g_suppress_profiled_file_log;

#define PROFILERFILE_LOG(CategoryName, Verbosity, Format, ...) \
  if (!g_suppress_profiled_file_log) {                         \
    g_suppress_profiled_file_log = true;                       \
    fun_log2(CategoryName, Verbosity, Format, ##__VA_ARGS__);  \
    g_suppress_profiled_file_log = false;                      \
  }

class ProfiledFSStatsBase {
 public:
  /** Start time (ms) */
  double start_time;
  /** duration (ms) */
  double duration;

  ProfiledFSStatsBase() : start_time(0.0), duration(0.0) {}
};

class ProfiledFSStatsOp : public ProfiledFSStatsBase {
 public:
  enum class OpType : uint8 {
    Unknown = 0,
    Tell = 1,
    Seek,
    Read,
    Write,
    Size,
    OpenRead,
    OpenWrite,
    Exists,
    Delete,
    Move,
    IsReadOnly,
    SetReadOnly,
    GetTimestamp,
    SetTimestamp,
    GetFilenameOnDisk,
    Create,
    Copy,
    Iterate,
    IterateStat,
    GetStatData,

    Count
  };

  /** Operation type */
  OpType type;

  /** Number of bytes processed */
  int64 byte_count;

  /** The last time this operation was executed */
  double last_op_time;

  ProfiledFSStatsOp(OpType type)
      : type(type), byte_count(0), last_op_time(0.0) {}
};

class ProfiledFSStatsFileBase : public ProfiledFSStatsBase {
 public:
  /** File name */
  String name_;

  /** Child stats */
  Array<SharedPtr<ProfiledFSStatsOp>> children_;

  /** Critical section for synchronization */
  FastMutex mutex_;

  ProfiledFSStatsFileBase(const char* filename) : name_(filename) {}
};

class ProfiledFSStatsFileDetailed : public ProfiledFSStatsFileBase {
 public:
  ProfiledFSStatsFileDetailed(const char* filename)
      : ProfiledFSStatsFileBase(filename) {}

  inline ProfiledFSStatsOp* CreateOpStat(ProfiledFSStatsOp::OpType type) {
    ScopedLock<FastMutex> guard(mutex_);
    SharedPtr<ProfiledFSStatsOp> stat(new ProfiledFSStatsOp(type));
    children_.Add(stat);
    stat->start_time = SystemTime::Seconds() * 1000.0;
    stat->last_op_time = stat->start_time;
    return stat.Get();
  }
};

class ProfiledFSStatsFileSimple : public ProfiledFSStatsFileBase {
 public:
  ProfiledFSStatsFileSimple(const char* filename)
      : ProfiledFSStatsFileBase(filename) {
    for (uint8 type_index = 0;
         type_index < (uint8)ProfiledFSStatsOp::OpType::Count; ++type_index) {
      SharedPtr<ProfiledFSStatsOp> stat(
          new ProfiledFSStatsOp(ProfiledFSStatsOp::OpType(type_index)));
      children_.Add(stat);
    }
  }

  inline ProfiledFSStatsOp* CreateOpStat(ProfiledFSStatsOp::OpType type) {
    SharedPtr<ProfiledFSStatsOp> stat = children_[(uint8)type];
    stat->last_op_time = SystemTime::Seconds() * 1000.0;
    if (stat->start_time == 0.0) {
      stat->start_time = stat->last_op_time;
    }
    return stat.Get();
  }
};

template <typename StatType>
class ProfiledFile : public IFile {
 public:
  ProfiledFile(IFile* file, const char* filename, StatType* stats)
      : file_(file), filename_(filename), file_stats_(stats) {}

  int64 Tell() override {
    ProfiledFSStatsOp* stat(
        file_stats_->CreateOpStat(ProfiledFSStatsOp::OpType::Tell));
    int64 result = file->Tell();
    stat->duration += SystemTime::Seconds() * 1000.0 - stat->last_op_time;
    return result;
  }

  bool Seek(int64 new_pos) override {
    ProfiledFSStatsOp* stat(
        file_stats_->CreateOpStat(ProfiledFSStatsOp::OpType::Seek));
    bool result = file->Seek(new_pos);
    stat->duration += SystemTime::Seconds() * 1000.0 - stat->last_op_time;
    return result;
  }

  bool SeekFromEnd(int64 relative_pos_to_end) override {
    ProfiledFSStatsOp* stat(
        file_stats_->CreateOpStat(ProfiledFSStatsOp::OpType::Seek));
    bool result = file->SeekFromEnd(relative_pos_to_end);
    stat->duration += SystemTime::Seconds() * 1000.0 - stat->last_op_time;
    return result;
  }

  bool Read(uint8* dst, int64 len_to_read) override {
    ProfiledFSStatsOp* stat(
        file_stats_->CreateOpStat(ProfiledFSStatsOp::OpType::Read));
    bool result = file->Read(dst, len_to_read);
    stat->duration += SystemTime::Seconds() * 1000.0 - stat->last_op_time;
    stat->byte_count += len_to_read;
    return result;
  }

  bool Write(const uint8* src, int64 len_to_write) override {
    ProfiledFSStatsOp* stat(
        file_stats_->CreateOpStat(ProfiledFSStatsOp::OpType::Write));
    bool result = file->Write(src, len_to_write);
    stat->duration += SystemTime::Seconds() * 1000.0 - stat->last_op_time;
    stat->byte_count += len_to_write;
    return result;
  }

  int64 Size() override {
    ProfiledFSStatsOp* stat(
        file_stats_->CreateOpStat(ProfiledFSStatsOp::OpType::Size));
    int64 result = file->Size();
    stat->duration += SystemTime::Seconds() * 1000.0 - stat->last_op_time;
    return result;
  }

 private:
  AutoPtr<IFile> file_;
  String filename_;
  StatType* file_stats_;
};

class FUN_BASE_API ProfiledPlatformFileSystem : public IPlatformFS {
 public:
  virtual ~ProfiledPlatformFileSystem() {}

  bool ShouldBeUsed(IPlatformFS* inner, const char* cmdline) const override {
    return Parse::Param(cmdline, GetName());
  }

  bool Initialize(IPlatformFS* inner, const char* cmdline) override {
    fun_check_ptr(inner);
    lower_level_ = inner;
    start_time_ = SystemTime::Seconds() * 1000.0;
    return lower_level_ != nullptr;
  }

  IPlatformFS* GetLowerLevel() override { return lower_level_; }

  double GetStartTime() const { return start_time_; }

  const Map<String, SharedPtr<ProfiledFSStatsFileBase>>& GetStats() const {
    return stats_;
  }

 protected:
  IPlatformFS* lower_level_;
  Map<String, SharedPtr<ProfiledFSStatsFileBase>> stats_;
  double start_time_;
  FastMutex mutex_;

  ProfiledPlatformFileSystem() : lower_level_(nullptr), start_time_(0.0) {}
};

template <typename StatsType>
class TypedProfiledPlatformFileSystem : public ProfiledPlatformFileSystem {
 public:
  TypedProfiledPlatformFileSystem() {}

  static const char* GetTypeName() { return nullptr; }

  const char* GetName() const override {
    return TypedProfiledPlatformFileSystem::GetTypeName();
  }

  bool FileExists(const char* filename) override {
    StatsType* file_stat = CreateStat(filename);
    ProfiledFSStatsOp* op_stat =
        file_stat->CreateOpStat(ProfiledFSStatsOp::OpType::Exists);
    bool result = lower_level_->FileExists(filename);
    op_stat->duration += SystemTime::Seconds() * 1000.0 - op_stat->last_op_time;
    return result;
  }

  int64 FileSize(const char* filename) override {
    StatsType* file_stat = CreateStat(filename);
    ProfiledFSStatsOp* op_stat =
        file_stat->CreateOpStat(ProfiledFSStatsOp::OpType::Size);
    int64 result = lower_level_->FileSize(filename);
    op_stat->duration += SystemTime::Seconds() * 1000.0 - op_stat->last_op_time;
    return result;
  }

  bool DeleteFile(const char* filename) override {
    StatsType* file_stat = CreateStat(filename);
    ProfiledFSStatsOp* op_stat =
        file_stat->CreateOpStat(ProfiledFSStatsOp::OpType::Delete);
    bool result = lower_level_->DeleteFile(filename);
    op_stat->duration += SystemTime::Seconds() * 1000.0 - op_stat->last_op_time;
    return result;
  }

  bool IsReadOnly(const char* filename) override {
    StatsType* file_stat = CreateStat(filename);
    ProfiledFSStatsOp* op_stat =
        file_stat->CreateOpStat(ProfiledFSStatsOp::OpType::IsReadOnly);
    bool result = lower_level_->IsReadOnly(filename);
    op_stat->duration += SystemTime::Seconds() * 1000.0 - op_stat->last_op_time;
    return result;
  }

  bool MoveFile(const char* to, const char* from) override {
    StatsType* file_stat = CreateStat(from);
    ProfiledFSStatsOp* op_stat =
        file_stat->CreateOpStat(ProfiledFSStatsOp::OpType::Move);
    bool result = lower_level_->MoveFile(to, from);
    op_stat->duration += SystemTime::Seconds() * 1000.0 - op_stat->last_op_time;
    return result;
  }

  bool SetReadOnly(const char* filename, bool readonly) override {
    StatsType* file_stat = CreateStat(filename);
    ProfiledFSStatsOp* op_stat =
        file_stat->CreateOpStat(ProfiledFSStatsOp::OpType::SetReadOnly);
    bool result = lower_level_->SetReadOnly(filename, readonly);
    op_stat->duration += SystemTime::Seconds() * 1000.0 - op_stat->last_op_time;
    return result;
  }

  DateTime GetTimestamp(const char* filename) override {
    StatsType* file_stat = CreateStat(filename);
    ProfiledFSStatsOp* op_stat =
        file_stat->CreateOpStat(ProfiledFSStatsOp::OpType::GetTimestamp);
    double op_start_time = SystemTime::Seconds();
    DateTime result = lower_level_->GetTimestamp(filename);
    op_stat->duration += SystemTime::Seconds() * 1000.0 - op_stat->last_op_time;
    return result;
  }

  void SetTimestamp(const char* filename, const DateTime& timestamp) override {
    StatsType* file_stat = CreateStat(filename);
    ProfiledFSStatsOp* op_stat =
        file_stat->CreateOpStat(ProfiledFSStatsOp::OpType::SetTimestamp);
    double op_start_time = SystemTime::Seconds();
    lower_level_->SetTimestamp(filename, timestamp);
    op_stat->duration += SystemTime::Seconds() * 1000.0 - op_stat->last_op_time;
  }

  DateTime GetAccessTimestamp(const char* filename) override {
    StatsType* file_stat = CreateStat(filename);
    ProfiledFSStatsOp* op_stat =
        file_stat->CreateOpStat(ProfiledFSStatsOp::OpType::GetTimestamp);
    double op_start_time = SystemTime::Seconds();
    DateTime result = lower_level_->GetAccessTimestamp(filename);
    op_stat->duration += SystemTime::Seconds() * 1000.0 - op_stat->last_op_time;
    return result;
  }

  String GetFilenameOnDisk(const char* filename) override {
    StatsType* file_stat = CreateStat(filename);
    ProfiledFSStatsOp* op_stat =
        file_stat->CreateOpStat(ProfiledFSStatsOp::OpType::GetFilenameOnDisk);
    double op_start_time = SystemTime::Seconds();
    String result = lower_level_->GetFilenameOnDisk(filename);
    op_stat->duration += SystemTime::Seconds() * 1000.0 - op_stat->last_op_time;
    return result;
  }

  IFile* OpenRead(const char* filename, bool allow_write = false) override {
    StatsType* file_stat = CreateStat(filename);
    ProfiledFSStatsOp* op_stat =
        file_stat->CreateOpStat(ProfiledFSStatsOp::OpType::OpenRead);
    IFile* result = lower_level_->OpenRead(filename, allow_write);
    op_stat->duration += SystemTime::Seconds() * 1000.0 - op_stat->last_op_time;
    return result ? (new ProfiledFile<StatsType>(result, filename, file_stat))
                  : result;
  }

  IFile* OpenWrite(const char* filename, bool append = false,
                   bool allow_read = false) override {
    StatsType* file_stat = CreateStat(filename);
    ProfiledFSStatsOp* op_stat =
        file_stat->CreateOpStat(ProfiledFSStatsOp::OpType::OpenWrite);
    IFile* result = lower_level_->OpenWrite(filename, append, allow_read);
    op_stat->duration += SystemTime::Seconds() * 1000.0 - op_stat->last_op_time;
    return result ? (new ProfiledFile<StatsType>(result, filename, file_stat))
                  : result;
  }

  bool DirectoryExists(const char* directory) override {
    StatsType* file_stat = CreateStat(directory);
    ProfiledFSStatsOp* op_stat =
        file_stat->CreateOpStat(ProfiledFSStatsOp::OpType::Exists);
    bool result = lower_level_->DirectoryExists(directory);
    op_stat->duration += SystemTime::Seconds() * 1000.0 - op_stat->last_op_time;
    return result;
  }

  bool CreateDirectory(const char* directory) override {
    StatsType* file_stat = CreateStat(directory);
    ProfiledFSStatsOp* op_stat =
        file_stat->CreateOpStat(ProfiledFSStatsOp::OpType::Create);
    bool result = lower_level_->CreateDirectory(directory);
    op_stat->duration += SystemTime::Seconds() * 1000.0 - op_stat->last_op_time;
    return result;
  }

  bool DeleteDirectory(const char* directory) override {
    StatsType* file_stat = CreateStat(directory);
    ProfiledFSStatsOp* op_stat =
        file_stat->CreateOpStat(ProfiledFSStatsOp::OpType::Delete);
    bool result = lower_level_->DeleteDirectory(directory);
    op_stat->duration += SystemTime::Seconds() * 1000.0 - op_stat->last_op_time;
    return result;
  }

  FileStatData GetStatData(const char* filename_or_directory) override {
    StatsType* file_stat = CreateStat(filename_or_directory);
    ProfiledFSStatsOp* op_stat =
        file_stat->CreateOpStat(ProfiledFSStatsOp::OpType::GetStatData);
    FileStatData result = lower_level_->GetStatData(filename_or_directory);
    op_stat->duration += SystemTime::Seconds() * 1000.0 - op_stat->last_op_time;
    return result;
  }

  bool IterateDirectory(const char* directory,
                        IPlatformFS::DirectoryVisitor& visitor) override {
    StatsType* file_stat = CreateStat(directory);
    ProfiledFSStatsOp* op_stat =
        file_stat->CreateOpStat(ProfiledFSStatsOp::OpType::Iterate);
    bool result = lower_level_->IterateDirectory(directory, visitor);
    op_stat->duration += SystemTime::Seconds() * 1000.0 - op_stat->last_op_time;
    return result;
  }

  bool IterateDirectoryRecursively(
      const char* directory, IPlatformFS::DirectoryVisitor& visitor) override {
    StatsType* file_stat = CreateStat(directory);
    ProfiledFSStatsOp* op_stat =
        file_stat->CreateOpStat(ProfiledFSStatsOp::OpType::Iterate);
    bool result = lower_level_->IterateDirectoryRecursively(directory, visitor);
    op_stat->duration += SystemTime::Seconds() * 1000.0 - op_stat->last_op_time;
    return result;
  }

  bool IterateDirectoryStat(
      const char* directory,
      IPlatformFS::DirectoryStatVisitor& visitor) override {
    StatsType* file_stat = CreateStat(directory);
    ProfiledFSStatsOp* op_stat =
        file_stat->CreateOpStat(ProfiledFSStatsOp::OpType::IterateStat);
    bool result = lower_level_->IterateDirectoryStat(directory, visitor);
    op_stat->duration += SystemTime::Seconds() * 1000.0 - op_stat->last_op_time;
    return result;
  }

  bool IterateDirectoryStatRecursively(
      const char* directory,
      IPlatformFS::DirectoryStatVisitor& visitor) override {
    StatsType* file_stat = CreateStat(directory);
    ProfiledFSStatsOp* op_stat =
        file_stat->CreateOpStat(ProfiledFSStatsOp::OpType::IterateStat);
    bool result =
        lower_level_->IterateDirectoryStatRecursively(directory, visitor);
    op_stat->duration += SystemTime::Seconds() * 1000.0 - op_stat->last_op_time;
    return result;
  }

  bool DeleteDirectoryRecursively(const char* directory) override {
    StatsType* file_stat = CreateStat(directory);
    ProfiledFSStatsOp* op_stat =
        file_stat->CreateOpStat(ProfiledFSStatsOp::OpType::Delete);
    bool result = lower_level_->DeleteDirectoryRecursively(directory);
    op_stat->duration += SystemTime::Seconds() * 1000.0 - op_stat->last_op_time;
    return result;
  }

  bool CopyFile(const char* to, const char* from) override {
    StatsType* file_stat = CreateStat(from);
    ProfiledFSStatsOp* op_stat =
        file_stat->CreateOpStat(ProfiledFSStatsOp::OpType::Copy);
    bool result = lower_level_->CopyFile(to, from);
    op_stat->duration += SystemTime::Seconds() * 1000.0 - op_stat->last_op_time;
    return result;
  }

  // static void CreateProfileVisualizer

 private:
  inline StatsType* CreateStat(const char* filename) {
    String path(filename);
    ScopedLock<FastMutex> guard(mutex_);

    SharedPtr<ProfiledFSStatsFileBase>* existing_stat = stats_.Find(path);
    if (existing_stat) {
      return (StatsType*)(existing_stat->Get());
    } else {
      SharedPtr<StatsType> stat(new StatsType(*path));
      stats_.Add(path, stat);
      stat->start_time = SystemTime::Seconds() * 1000.0;
      return stat.Get();
    }
  }
};

template <>
inline const char*
TypedProfiledPlatformFileSystem<ProfiledFSStatsFileDetailed>::GetTypeName() {
  return "ProfileFile";
}

template <>
inline const char*
TypedProfiledPlatformFileSystem<ProfiledFSStatsFileSimple>::GetTypeName() {
  return "SimpleProfileFile";
}

class PlatformFSReadStatsHandle : public IFile {
 public:
  PlatformFSReadStatsHandle(IFile* file, const char* filename,
                            volatile int32* bytes_per_sec_counter,
                            volatile int32* bytes_read_counter,
                            volatile int32* reads_counter)
      : file_(file),
        filename(filename),
        bytes_per_sec_counter_(bytes_per_sec_counter),
        bytes_read_counter_(bytes_read_counter),
        reads_counter_(reads_counter) {}

  int64 Tell() override { return file_->Tell(); }

  bool Seek(int64 new_pos) override { return file_->Seek(new_pos); }

  bool SeekFromEnd(int64 relative_pos_to_end) override {
    return file_->SeekFromEnd(relative_pos_to_end);
  }

  bool Read(uint8* dst, int64 len_to_read) override;

  bool Write(const uint8* src, int64 len_to_write) override {
    return file_->Write(src, len_to_write);
  }

  int64 Size() override { return file_->Size(); }

 private:
  AutoPtr<IFile> file_;
  String filename_;
  volatile int32* bytes_per_sec_counter_;
  volatile int32* bytes_read_counter_;
  volatile int32* reads_counter_;
};

class FUN_BASE_API PlatformFSReadStats : public IPlatformFS {
 public:
  PlatformFSReadStats() : lower_level_(nullptr), timer_(0.f) {}

  virtual ~PlatformFSReadStats() {}

  bool ShouldBeUsed(IPlatformFS* inner, const char* cmdline) const override {
#if STATS
    bool result = Parse::Param(cmdline, "FileReadStats");
    return result;
#else
    return false;
#endif
  }

  bool Initialize(IPlatformFS* inner, const char* cmdline) override;

  bool Tick(float Delta);

  IPlatformFS* GetLowerLevel() override { return lower_level_; }

  static const char* GetTypeName() { return "FileReadStats"; }

  const char* GetName() const override { return GetTypeName(); }

  bool FileExists(const char* filename) override {
    return lower_level_->FileExists(filename);
  }

  int64 FileSize(const char* filename) override {
    return lower_level_->FileSize(filename);
  }

  bool DeleteFile(const char* filename) override {
    return lower_level_->DeleteFile(filename);
  }

  bool IsReadOnly(const char* filename) override {
    return lower_level_->IsReadOnly(filename);
  }

  bool MoveFile(const char* to, const char* from) override {
    return lower_level_->MoveFile(to, from);
  }

  bool SetReadOnly(const char* filename, bool readonly) override {
    return lower_level_->SetReadOnly(filename, readonly);
  }

  DateTime GetTimestamp(const char* filename) override {
    return lower_level_->GetTimestamp(filename);
  }

  void SetTimestamp(const char* filename, const DateTime& timestamp) override {
    lower_level_->SetTimestamp(filename, timestamp);
  }

  DateTime GetAccessTimestamp(const char* filename) override {
    return lower_level_->GetAccessTimestamp(filename);
  }

  String GetFilenameOnDisk(const char* filename) override {
    return lower_level_->GetFilenameOnDisk(filename);
  }

  IFile* OpenRead(const char* filename, bool allow_write) override {
    IFile* result = lower_level_->OpenRead(filename, allow_write);
    return result ? (new PlatformFSReadStatsHandle(
                        result, filename, &bytes_per_sec_this_tick_,
                        &bytes_read_this_tick_, &reads_this_tick_))
                  : result;
  }

  IFile* OpenWrite(const char* filename, bool append = false,
                   bool allow_read = false) override {
    IFile* result = lower_level_->OpenWrite(filename, append, allow_read);
    return result ? (new PlatformFSReadStatsHandle(
                        result, filename, &bytes_per_sec_this_tick_,
                        &bytes_read_this_tick_, &reads_this_tick_))
                  : result;
  }

  bool DirectoryExists(const char* directory) override {
    return lower_level_->DirectoryExists(directory);
  }

  bool CreateDirectory(const char* directory) override {
    return lower_level_->CreateDirectory(directory);
  }

  bool DeleteDirectory(const char* directory) override {
    return lower_level_->DeleteDirectory(directory);
  }

  FileStatData GetStatData(const char* filename_or_directory) override {
    return lower_level_->GetStatData(filename_or_directory);
  }

  bool IterateDirectory(const char* directory,
                        IPlatformFS::DirectoryVisitor& visitor) override {
    return lower_level_->IterateDirectory(directory, visitor);
  }

  bool IterateDirectoryRecursively(
      const char* directory, IPlatformFS::DirectoryVisitor& visitor) override {
    return lower_level_->IterateDirectoryRecursively(directory, visitor);
  }

  bool IterateDirectoryStat(
      const char* directory,
      IPlatformFS::DirectoryStatVisitor& visitor) override {
    return lower_level_->IterateDirectoryStat(directory, visitor);
  }

  bool IterateDirectoryStatRecursively(
      const char* directory,
      IPlatformFS::DirectoryStatVisitor& visitor) override {
    return lower_level_->IterateDirectoryStatRecursively(directory, visitor);
  }

  bool DeleteDirectoryRecursively(const char* directory) override {
    return lower_level_->DeleteDirectoryRecursively(directory);
  }

  bool CopyFile(const char* to, const char* from) override {
    return lower_level_->CopyFile(to, from);
  }

 protected:
  IPlatformFS* lower_level_;
  double lifetime_read_speed_;  // Total maintained over lifetime of runtime, in
                                // KB per sec
  double lifetime_read_size_;   // Total maintained over lifetime of runtime, in
                                // bytes
  int64 lifetime_read_calls_;   // Total maintained over lifetime of runtime
  double timer_;
  volatile int32 bytes_per_sec_this_tick_;
  volatile int32 bytes_read_this_tick_;
  volatile int32 reads_this_tick_;
};

}  // namespace fun

#endif  //! FUN_BUILD_SHIPPING
