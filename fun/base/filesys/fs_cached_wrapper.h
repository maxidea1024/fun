#pragma once

#include "fun/base/base.h"

namespace fun {

class FUN_BASE_API CachedFile : public IFile {
 public:
  CachedFile(IFile* file_handle, bool readable, bool writable)
    : file_handle_(file_handle),
      file_pos_(0),
      tell_pos_(0),
      file_size_(file_handle->Size()),
      writable_(writable),
      readable_(readable),
      current_cache_(0) {
    FlushCache();
  }

  virtual ~CachedFile() {}

  int64 Tell() override {
    return file_pos_;
  }

  bool Seek(int64 new_pos) override {
    if (new_pos < 0 || new_pos > file_size_) {
      return false;
    }
    file_pos_ = new_pos;
    return true;
  }

  bool SeekFromEnd(int64 relative_pos_to_end = 0) override {
    return Seek(file_size_ - relative_pos_to_end);
  }

  bool Read(uint8* dst, int64 len_to_read) override {
    if (!readable_ || len_to_read < 0 || (len_to_read + file_pos_ > file_size_)) {
      return false;
    }

    if (len_to_read == 0) {
      return true;
    }

    bool result = false;
    if (len_to_read > BUFFER_CACHE_SIZE) { // reading more than we cache
      // if the file position is within the cache, copy out the remainder of the cache
      const int32 cache_index = GetCacheIndex(file_pos_);
      if (cache_index < CACHE_COUNT) {
        const int64 len_to_copy = cache_end_[cache_index] - file_pos_;
        UnsafeMemory::Memcpy(dst, buffer_cache_[cache_index] + (file_pos_ - cache_start_[cache_index]), len_to_copy);
        file_pos_ += len_to_copy;
        len_to_read -= len_to_copy;
        dst += len_to_copy;
      }

      if (InnerSeek(file_pos_)) {
        result = InnerRead(dst, len_to_read);
      }
      if (result) {
        file_pos_ += len_to_read;
      }
    } else {
      result = true;

      while (len_to_read && result) {
        uint32 cache_index = GetCacheIndex(file_pos_);
        if (cache_index > CACHE_COUNT) {
          // need to update the cache
          uint64 aligned_file_pos = file_pos_ & BUFFER_SIZE_MASK; // Aligned Version
          uint64 len_to_read = MathBase::Min<uint64>(BUFFER_CACHE_SIZE, file_size_ - aligned_file_pos);
          InnerSeek(aligned_file_pos);
          result = InnerRead(buffer_cache_[current_cache_], len_to_read);

          if (result) {
            cache_start_[current_cache_] = aligned_file_pos;
            cache_end_[current_cache_] = aligned_file_pos+len_to_read;
            cache_index = current_cache_;
            // move to next cache for update
            current_cache_++;
            current_cache_ %= CACHE_COUNT;
          }
        }

        // copy from the cache to the destination
        if (result) {
          // Analyzer doesn't see this - if this code ever changes make sure there are no buffer overruns!
          CA_ASSUME(cache_index < CACHE_COUNT);
          uint64 adjusted_len_to_read = MathBase::Min<uint64>(len_to_read, cache_end_[cache_index] - file_pos_);
          UnsafeMemory::Memcpy(dst, buffer_cache_[cache_index] + (file_pos_ - cache_start_[cache_index]), adjusted_len_to_read);
          file_pos_ += adjusted_len_to_read;
          dst += adjusted_len_to_read;
          len_to_read -= adjusted_len_to_read;
        }
      }
    }
    return result;
  }

  bool Write(const uint8* src, int64 len_to_write) override {
    if (!writable_ || len_to_write < 0) {
      return false;
    }

    if (len_to_write == 0) {
      return true;
    }

    InnerSeek(file_pos_);
    bool result = file_handle_->Write(src, len_to_write);
    if (result) {
      file_pos_ += len_to_write;
      file_size_ = MathBase::Max<int64>(file_pos_, file_size_);
      FlushCache();
      tell_pos_ = file_pos_;
    }
    return result;
  }

  int64 Size() override {
    return file_size_;
  }

 private:
  static const uint32 BUFFER_CACHE_SIZE = 64 * 1024; // Seems to be the magic number for best perf
  static const uint64 BUFFER_SIZE_MASK = ~((uint64)BUFFER_CACHE_SIZE-1);
  static const uint32 CACHE_COUNT = 2;

  bool InnerSeek(uint64 new_pos) {
    if (new_pos == tell_pos_) {
      return true;
    }

    bool ok = file_handle_->Seek(new_pos);
    if (ok) {
      tell_pos_ = new_pos;
    }
    return ok;
  }

  bool InnerRead(uint8* dst, uint64 len_to_read) {
    if (file_handle_->Read(dst, len_to_read)) {
      tell_pos_ += len_to_read;
      return true;
    }
    return false;
  }

  int32 GetCacheIndex(int64 pos) const {
    for (uint32 i = 0; i < countof(cache_start_); ++i) {
      if (pos >= cache_start_[i] && pos < cache_end_[i]) {
        return i;
      }
    }
    return CACHE_COUNT + 1;
  }

  void FlushCache() {
    for (uint32 i = 0; i < CACHE_COUNT; ++i) {
      cache_start_[i] = cache_end_[i] = -1;
    }
  }

  AutoPtr<IFile> file_handle_;
  int64 file_pos_; // Desired position in the file stream, this can be different to file_pos_ due to the cache
  int64 tell_pos_; // Actual position in the file,  this can be different to file_pos_
  int64 file_size_;
  bool writable_;
  bool readable_;
  uint8 buffer_cache_[CACHE_COUNT][BUFFER_CACHE_SIZE];
  int64 cache_start_[CACHE_COUNT];
  int64 cache_end_[CACHE_COUNT];
  int32 current_cache_;
};


class FUN_BASE_API CachedReadPlatformFS : public IPlatformFS {
  IPlatformFS* lower_level_;

 public:
  static const char* GetTypeName() {
    return "CachedReadFile";
  }

  CachedReadPlatformFS() : lower_level_(nullptr) {}

  bool Initialize(IPlatformFS* inner, const char* cmdline) override {
    fun_check_ptr(inner);
    lower_level_ = inner;
    return lower_level_ != nullptr;
  }

  bool ShouldBeUsed(IPlatformFS* inner, const char* cmdline) const override {
    // default to false on Windows since CAsyncBufferedFileReaderWindows already buffers the data
    bool result = !PLATFORM_WINDOWS && CPlatformProperties::RequiresCookedData();

    // Allow a choice between shorter load times or less memory on desktop platforms.
    // Note: this cannot be in config since they aren't read at that point.
    if (PLATFORM_DESKTOP) {
      if (Parse::Param(cmdline, "NoCachedReadFile")) {
        result = false;
      } else if (Parse::Param(cmdline, "CachedReadFile")) {
        result = true;
      }

      fun_log(LogPlatformFS, Info, "%s cached read wrapper", result ? "Using" : "Not using");
    }

    return result;
  }

  IPlatformFS* GetLowerLevel() override {
    return lower_level_;
  }

  const char* GetName() const override {
    return CachedReadPlatformFS::GetTypeName();
  }

  bool FileExists(const char* filename) override {
    return lower_level_->FileExists(filename);
  }

  int64 file_size_(const char* filename) override {
    return lower_level_->file_size_(filename);
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
    IFile* file = lower_level_->OpenRead(filename, allow_write);
    if (file == nullptr) {
      return nullptr;
    }
    return new CachedFile(file, true, false);
  }

  IFile* OpenWrite(const char* filename, bool append = false, bool allow_read = false) override {
    IFile* file = lower_level_->OpenWrite(filename, append, allow_read);
    if (file == nullptr) {
      return nullptr;
    }
    return new CachedFile(file, allow_read, true);
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

  bool CopyDirectoryTree(const char* destination_directory, const char* src, bool overwrite_all_existing) override {
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
};

} // namespace fun
