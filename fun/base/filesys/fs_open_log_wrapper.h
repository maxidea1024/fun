#pragma once

#include "fun/base/base.h"

#if !FUN_BUILD_SHIPPING

namespace fun {

class FUN_BASE_API PlatformFSOpenLog : public IPlatformFS {
 public:
  PlatformFSOpenLog()
    : lower_level_(nullptr), open_order_(0) {}

  virtual ~PlatformFSOpenLog() {}

  bool ShouldBeUsed(IPlatformFS* inner, const char* cmdline) const override {
    return Parse::Param(cmdline, "FileOpenLog");
  }

  bool Initialize(IPlatformFS* inner, const char* cmdline) override {
    lower_level_ = inner;

    String local_file_directory;
    String log_file_path;
    String platform_str;

    if (Parse::Value(cmdline, "TARGETPLATFORM=", platform_str)) {
      Array<String> platform_names;
      if (!(platform_str == "None" || platform_str == "All")) {
        platform_str.Split(platform_names, "+", 0, StringSplitOption::CullEmpty);
      }

      for (int32 platform = 0; platform < platform_names.Count(); ++platform) {
        local_file_directory = CPaths::Combine(PlatformMisc::GameDir(), "Build", *platform_names[platform], "FileOpenOrder");
#if FUN_WITH_EDITOR
        log_file_path = CPaths::Combine(*local_file_directory, "EditorOpenOrder.log");
#else
        log_file_path = CPaths::Combine(*local_file_directory, "GameOpenOrder.log");
#endif
        inner->CreateDirectoryTree(*local_file_directory);
        auto* file = inner->OpenWrite(*log_file_path, false, false);
        if (file) {
          log_output_.Add(file);
        }
      }
    } else {
      local_file_directory = CPaths::Combine(PlatformMisc::GameDir(), "Build", StringCast<char>(CPlatformProperties::PlatformName()).Get(), "FileOpenOrder");
#if FUN_WITH_EDITOR
      log_file_path = CPaths::Combine(*local_file_directory, "EditorOpenOrder.log");
#else
      log_file_path = CPaths::Combine(*local_file_directory, "GameOpenOrder.log");
#endif
      inner->CreateDirectoryTree(*local_file_directory);
      auto* file = inner->OpenWrite(*log_file_path, false, false);
      if (file) {
        log_output_.Add(file);
      }
    }
    return true;
  }

  IPlatformFS* GetLowerLevel() override {
    return lower_level_;
  }

  static const char* GetTypeName() {
    return "FileOpenLog";
  }

  const char* GetName() const override {
    return GetTypeName();
  }

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
    if (result) {
      mutex_.Lock();
      if (filename_access_map_.Find(filename) == nullptr) {
        filename_access_map_.Emplace(filename, ++open_order_);
        String text = String::Format("\"%s\" %llu\n", filename, open_order_);
        for (auto file = log_output_.CreateIterator(); file; ++file) {
          (*file)->Write((uint8*)StringCast<ANSICHAR>(*text).Get(), text.Len());
        }
      }
      mutex_.Unlock();
    }
    return result;
  }

  IFile* OpenWrite(const char* filename, bool append = false, bool allow_read = false) override {
    return lower_level_->OpenWrite(filename, append, allow_read);
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

  bool IterateDirectory(const char* directory, IPlatformFS::DirectoryVisitor& visitor) override {
    return lower_level_->IterateDirectory(directory, visitor);
  }

  bool IterateDirectoryRecursively(const char* directory, IPlatformFS::DirectoryVisitor& visitor) override {
    return lower_level_->IterateDirectoryRecursively(directory, visitor);
  }

  bool IterateDirectoryStat(const char* directory, IPlatformFS::DirectoryStatVisitor& visitor) override {
    return lower_level_->IterateDirectoryStat(directory, visitor);
  }

  bool IterateDirectoryStatRecursively(const char* directory, IPlatformFS::DirectoryStatVisitor& visitor) override {
    return lower_level_->IterateDirectoryStatRecursively(directory, visitor);
  }

  bool DeleteDirectoryRecursively(const char* directory) override {
    return lower_level_->DeleteDirectoryRecursively(directory);
  }

  bool CopyFile(const char* to, const char* from) override {
    return lower_level_->CopyFile(to, from);
  }

  bool CreateDirectoryTree(const char* directory) override {
    return lower_level_->CreateDirectoryTree(directory);
  }

  bool CopyDirectoryTree(const char* DestinationDirectory, const char* src, bool overwrite_all_existing) override {
    return lower_level_->CopyDirectoryTree(DestinationDirectory, src, overwrite_all_existing);
  }

  String ConvertToAbsolutePathForExternalAppForRead(const char* filename) override {
    return lower_level_->ConvertToAbsolutePathForExternalAppForRead(filename);
  }

  String ConvertToAbsolutePathForExternalAppForWrite(const char* filename) override {
    return lower_level_->ConvertToAbsolutePathForExternalAppForWrite(filename);
  }

  bool SendMessageToServer(const char* message, IFileServerMessageHandler* handler) override {
    return lower_level_->SendMessageToServer(message, handler);
  }

 protected:
  IPlatformFS* lower_level_;
  FastMutex mutex_;
  int64 open_order_;
  Map<String, int64> filename_access_map_;
  Array<IFile*> log_output_;
};

} // namespace fun

#endif //!FUN_BUILD_SHIPPING
