#include "fun/base/filesys/file_system_apple.h"
#include <sys/stat.h>

namespace fun {

// make an Timespan object that represents the "epoch" for time_t (from a stat
// struct)
const DateTime MAC_EPOCH(1970, 1, 1);

namespace {

FileStatData MacStatToFunFileData(struct stat& file_info) {
  const bool is_directory = S_ISDIR(file_info.st_mode);

  int64 file_size_ = -1;
  if (!is_directory) {
    file_size_ = file_info.st_size;
  }

  return FileStatData(MAC_EPOCH + Timespan(0, 0, file_info.st_ctime),
                      MAC_EPOCH + Timespan(0, 0, file_info.st_atime),
                      MAC_EPOCH + Timespan(0, 0, file_info.st_mtime),
                      file_size_, is_directory,
                      !!(file_info.st_mode & S_IWUSR));
}

}  // namespace

/**
 * Mac file handle implementation which limits number of open files per thread.
 * This is to prevent running out of system file handles (250). Should not be
 * neccessary when using pak file (e.g., SHIPPING?) so not particularly
 * optimized. Only manages files which are opened READ_ONLY.
 */
#define MANAGE_FILE_HANDLES \
  (FUN_PLATFORM == FUN_PLATFORM_MAC)  // !FUN_BUILD_SHIPPING

class FUN_BASE_API AppleFile : public IFile {
  enum { READWRITE_SIZE = 1024 * 1024 };

 public:
  AppleFile(int32 file_handle, const char* filename, bool is_readonly)
      : file_handle_(file_handle)
#if MANAGE_FILE_HANDLES
        ,
        filename_(filename),
        handle_slot_(-1),
        file_offset_(0),
        file_size_(0)
#endif
  {
    fun_check(file_handle_ > -1);

#if MANAGE_FILE_HANDLES
    // Only files opened for read will be managed
    if (is_readonly) {
      ReserveSlot();
      g_active_handles[handle_slot_] = this;
      struct stat file_info;
      fstat(file_handle_, &file_info);
      file_size_ = file_info.st_size;
    }
#endif
  }

  virtual ~AppleFile() {
#if MANAGE_FILE_HANDLES
    if (IsManaged()) {
      if (g_active_handles[handle_slot_] == this) {
        close(file_handle_);
        g_active_handles[handle_slot_] = nullptr;
      }
    } else
#endif
    {
      close(file_handle_);
    }
    file_handle_ = -1;
  }

  virtual int64 Tell() override {
#if MANAGE_FILE_HANDLES
    if (IsManaged()) {
      return file_offset_;
    } else
#endif
    {
      fun_check(IsValid());
      return lseek(file_handle_, 0, SEEK_CUR);
    }
  }

  virtual bool Seek(int64 new_pos) override {
    fun_check(new_pos >= 0);

#if MANAGE_FILE_HANDLES
    if (IsManaged()) {
      file_offset_ = new_pos >= file_size_ ? file_size_ - 1 : new_pos;
      return IsValid() && g_active_handles[handle_slot_] == this
                 ? lseek(file_handle_, file_offset_, SEEK_SET) != -1
                 : true;
    } else
#endif
    {
      fun_check(IsValid());
      return lseek(file_handle_, new_pos, SEEK_SET) != -1;
    }
  }

  virtual bool SeekFromEnd(int64 relative_pos_to_end = 0) override {
    fun_check(relative_pos_to_end <= 0);

#if MANAGE_FILE_HANDLES
    if (IsManaged()) {
      file_offset_ = (relative_pos_to_end >= file_size_)
                         ? 0
                         : (file_size_ + relative_pos_to_end - 1);
      return IsValid() && g_active_handles[handle_slot_] == this
                 ? lseek(file_handle_, file_offset_, SEEK_SET) != -1
                 : true;
    } else
#endif
    {
      fun_check(IsValid());
      return lseek(file_handle_, relative_pos_to_end, SEEK_END) != -1;
    }
  }

  virtual bool Read(uint8* dst, int64 len_to_read) override {
#if MANAGE_FILE_HANDLES
    if (IsManaged()) {
      ActivateSlot();
      int64 bytes_read = ReadInternal(dst, len_to_read);
      file_offset_ += bytes_read;
      return bytes_read == len_to_read;
    } else
#endif
    {
      return ReadInternal(dst, len_to_read) == len_to_read;
    }
  }

  virtual bool Write(const uint8* src, int64 len_to_write) override {
    fun_check(IsValid());
    while (len_to_write) {
      fun_check(len_to_write >= 0);
      int64 this_size = MathBase::Min<int64>(READWRITE_SIZE, len_to_write);
      fun_check(src);
      if (write(file_handle_, src, this_size) != this_size) {
        return false;
      }
      src += this_size;
      len_to_write -= this_size;
    }
    return true;
  }

  virtual int64 Size() override {
#if MANAGE_FILE_HANDLES
    if (IsManaged()) {
      return file_size_;
    } else
#endif
    {
      struct stat file_info;
      fstat(file_handle_, &file_info);
      return file_info.st_size;
    }
  }

 private:
#if MANAGE_FILE_HANDLES
  inline bool IsManaged() { return handle_slot_ != -1; }

  void ActivateSlot() {
    if (IsManaged()) {
      if (g_active_handles[handle_slot_] != this ||
          (g_active_handles[handle_slot_] &&
           g_active_handles[handle_slot_]->file_handle_ == -1)) {
        ReserveSlot();

        file_handle_ = open(TCHAR_TO_UTF8(*filename_), O_RDONLY);
        if (file_handle_ != -1) {
          lseek(file_handle_, file_offset_, SEEK_SET);
          g_active_handles[handle_slot_] = this;
        }
      } else {
        g_access_times[handle_slot_] = SystemTime::Seconds();
      }
    }
  }

  void ReserveSlot() {
    handle_slot_ = -1;

    // Look for non-reserved slot
    for (int32 i = 0; i < ACTIVE_HANDLE_COUNT; ++i) {
      if (g_active_handles[i] == nullptr) {
        handle_slot_ = i;
        break;
      }
    }

    // Take the oldest handle
    if (handle_slot_ == -1) {
      int32 oldest = 0;
      for (int32 i = 1; i < ACTIVE_HANDLE_COUNT; ++i) {
        if (g_access_times[oldest] > g_access_times[i]) {
          oldest = i;
        }
      }

      close(g_active_handles[oldest]->file_handle_);
      g_active_handles[oldest]->file_handle_ = -1;
      handle_slot_ = oldest;
    }

    g_active_handles[handle_slot_] = nullptr;
    g_access_times[handle_slot_] = SystemTime::Seconds();
  }
#endif

  int64 ReadInternal(uint8* dst, int64 len_to_read) {
    fun_check(IsValid());
    int64 max_read_size = READWRITE_SIZE;
    int64 bytes_read = 0;
    while (len_to_read) {
      fun_check(len_to_read >= 0);
      int64 this_size = MathBase::Min<int64>(max_read_size, len_to_read);
      fun_check(dst);
      int64 this_read = read(file_handle_, dst, this_size);
      if (this_read == -1) {
        // Reading from smb can sometimes result in a EINVAL error. Try again a
        // few times with a smaller read buffer.
        if (errno == EINVAL && max_read_size > 1024) {
          max_read_size /= 2;
          continue;
        }
        return bytes_read;
      }
      bytes_read += this_read;
      if (this_read != this_size) {
        return bytes_read;
      }
      dst += this_size;
      len_to_read -= this_size;
    }
    return bytes_read;
  }

  // Holds the internal file handle.
  int32 file_handle_;

#if MANAGE_FILE_HANDLES
  // Holds the name of the file that this handle represents. Kept around for
  // possible reopen of file.
  string filename_;

  // Most recent valid slot index for this handle; >=0 for handles which are
  // managed.
  int32 handle_slot_;

  // Current file offset; valid if a managed handle.
  int64 file_offset_;

  // Cached file size; valid if a managed handle.
  int64 file_size_;

  // Each thread keeps a collection of active handles with access times.
  static const int32 ACTIVE_HANDLE_COUNT = 256;
  static __thread AppleFile* g_active_handles[ACTIVE_HANDLE_COUNT];
  static __thread double g_access_times[ACTIVE_HANDLE_COUNT];
#endif

  inline bool IsValid() { return file_handle_ != -1; }
};

#if MANAGE_FILE_HANDLES
__thread AppleFile* AppleFile::g_active_handles[AppleFile::ACTIVE_HANDLE_COUNT];
__thread double AppleFile::g_access_times[AppleFile::ACTIVE_HANDLE_COUNT];
#endif

String AppleFileSystem::NormalizeFilename(const char* filename) {
  String result(filename);
  String.ReplaceInline(TEXT("\\"), TEXT("/"));
  return result;
}

String AppleFileSystem::NormalizeDirectory(const char* directory) {
  String result(directory);
  result.ReplaceInline(TEXT("\\"), TEXT("/"));
  if (result.EndsWith(TEXT("/"))) {
    result.LeftChop(1);
  }
  return result;
}

bool AppleFileSystem::FileExists(const char* filename) {
  struct stat file_info;
  if (stat(filename, &file_info) != -1) {
    return S_ISREG(file_info.st_mode);
  }
  return false;
}

int64 AppleFileSystem::file_size_(const char* filename) {
  struct stat file_info;
  file_info.st_size = -1;
  stat(filename, &file_info);
  // make sure to return -1 for directories
  if (S_ISDIR(file_info.st_mode)) {
    file_info.st_size = -1;
  }
  return file_info.st_size;
}

bool AppleFileSystem::DeleteFile(const char* filename) {
  return unlink(TCHAR_TO_UTF8(*NormalizeFilename(filename))) == 0;
}

bool AppleFileSystem::IsReadOnly(const char* filename) {
  if (access(TCHAR_TO_UTF8(*NormalizeFilename(filename)), F_OK) == -1) {
    return false;  // file doesn't exist
  }

  if (access(TCHAR_TO_UTF8(*NormalizeFilename(filename)), W_OK) == -1) {
    return errno == EACCES;
  }
  return false;
}

bool AppleFileSystem::MoveFile(const char* to, const char* from) {
  int32 result = rename(TCHAR_TO_UTF8(*NormalizeFilename(from)),
                        TCHAR_TO_UTF8(*NormalizeFilename(to)));
  if (result == -1 && errno == EXDEV) {
    // Copy the file if rename failed because to and from are on different file
    // systems
    if (CopyFile(to, from)) {
      DeleteFile(from);
      result = 0;
    }
  }
  return result != -1;
}

bool AppleFileSystem::SetReadOnly(const char* filename, bool readonly) {
  struct stat file_info;
  if (stat(filename, &file_info) == 0) {
    if (readonly) {
      file_info.st_mode &= ~S_IWUSR;
    } else {
      file_info.st_mode |= S_IWUSR;
    }
    return chmod(TCHAR_TO_UTF8(*NormalizeFilename(filename)),
                 file_info.st_mode) == 0;
  }
  return false;
}

DateTime AppleFileSystem::GetTimestamp(const char* filename) {
  // get file times
  struct stat file_info;
  if (stat(filename, &file_info) == -1) {
    return DateTime::Null;
  }

  // convert _stat time to DateTime
  Timespan time_since_epoch(0, 0, file_info.st_mtime);
  return MAC_EPOCH + time_since_epoch;
}

void AppleFileSystem::SetTimestamp(const char* filename,
                                   const DateTime& timestamp) {
  // get file times
  struct stat file_info;
  if (stat(filename, &file_info) == 0) {
    return;
  }

  // change the modification time only
  struct utimbuf times;
  times.actime = file_info.st_atime;
  times.modtime = (timestamp - MAC_EPOCH).GetTotalSeconds();
  utime(TCHAR_TO_UTF8(*NormalizeFilename(filename)), &times);
}

DateTime AppleFileSystem::GetAccessTimestamp(const char* filename) {
  // get file times
  struct stat file_info;
  if (stat(filename, &file_info) == -1) {
    return DateTime::Null;
  }

  // convert _stat time to DateTime
  Timespan time_since_epoch(0, 0, file_info.st_atime);
  return MAC_EPOCH + time_since_epoch;
}

string AppleFileSystem::GetFilenameOnDisk(const char* filename) {
  return filename;
}

IFile* AppleFileSystem::OpenRead(const char* filename, bool allow_write) {
  int32 handle = open(TCHAR_TO_UTF8(*NormalizeFilename(filename)), O_RDONLY);
  if (handle != -1) {
#if MANAGE_FILE_HANDLES
    return new AppleFile(handle, *NormalizeDirectory(filename), true);
#else
    return new AppleFile(handle, filename, true);
#endif
  }
  return nullptr;
}

IFile* AppleFileSystem::OpenWrite(const char* filename, bool append,
                                  bool allow_read) {
  int flags = O_CREAT;
  if (!append) {
    flags |= O_TRUNC;
  }
  if (allow_read) {
    flags |= O_RDWR;
  } else {
    flags |= O_WRONLY;
  }

  int32 handle =
      open(TCHAR_TO_UTF8(*NormalizeFilename(filename)), flags,
           S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
  if (handle != -1) {
#if MANAGE_FILE_HANDLES
    AppleFile* file =
        new AppleFile(handle, *NormalizeDirectory(filename), false);
#else
    AppleFile* file = new AppleFile(handle, filename, false);
#endif
    if (append) {
      file->SeekFromEnd(0);
    }
    return file;
  }

  return nullptr;
}

bool AppleFileSystem::DirectoryExists(const char* directory) {
  struct stat file_info;
  if (stat(directory, &file_info) != -1) {
    return S_ISDIR(file_info.st_mode);
  }
  return false;
}

bool AppleFileSystem::CreateDirectory(const char* directory) {
  @autoreleasepool {
    CFStringRef cf_directory =
        CPlatformString::TCHARToCFString(*NormalizeFilename(directory));
    bool result = [[NSFileManager defaultManager]
              createDirectoryAtPath:(NSString*)cf_directory
        withIntermediateDirectories:true
                         attributes:nil
                              error:nil];
    CFRelease(cf_directory);
    return result;
  }
}

bool AppleFileSystem::DeleteDirectory(const char* directory) {
  return rmdir(TCHAR_TO_UTF8(*NormalizeFilename(directory))) == 0;
}

FileStatData AppleFileSystem::GetStatData(const char* filename_or_directory) {
  struct stat file_info;
  if (stat(filename_or_directory, &file_info) != -1) {
    return MacStatToFunFileData(file_info);
  }

  return FileStatData();
}

bool AppleFileSystem::IterateDirectory(const char* directory,
                                       DirectoryVisitor& visitor) {
  @autoreleasepool {
    const String directory_str = directory;
    const String normalized_directory_str = NormalizeFilename(directory);

    return IterateDirectoryCommon(directory, [&](struct dirent* de) -> bool {
      // Normalize any unicode forms so we match correctly
      const string normalized_filename =
          UTF8_TO_TCHAR(([[[NSString stringWithUTF8String:de->d_name]
              precomposedStringWithCanonicalMapping]
              cStringUsingEncoding:NSUTF8StringEncoding]));

      // Figure out whether it's a directory. Some protocols (like NFS) do not
      // voluntarily return this as part of the directory entry, and need to be
      // queried manually.
      bool is_directory = (de->d_type == DT_DIR);
      if (de->d_type == DT_UNKNOWN) {
        struct stat file_info;
        if (stat(TCHAR_TO_UTF8(
                     *(normalized_directory_str / normalized_filename)),
                 &file_info) == 0) {
          is_directory = S_ISDIR(file_info.st_mode);
        }
      }

      return visitor.Visit(*(directory_str / normalized_filename),
                           is_directory);
    });
  }
}

bool AppleFileSystem::IterateDirectoryStat(const char* directory,
                                           DirectoryStatVisitor& visitor) {
  @autoreleasepool {
    const string directory_str = directory;
    const string normalized_directory_str = NormalizeFilename(directory);

    return IterateDirectoryCommon(directory, [&](struct dirent* de) -> bool {
      // Normalize any unicode forms so we match correctly
      const string normalized_filename =
          UTF8_TO_TCHAR(([[[NSString stringWithUTF8String:de->d_name]
              precomposedStringWithCanonicalMapping]
              cStringUsingEncoding:NSUTF8StringEncoding]));

      struct stat file_info;
      if (stat(TCHAR_TO_UTF8(*(normalized_directory_str / normalized_filename)),
               &file_info) == 0) {
        return visitor.Visit(*(directory_str / normalized_filename),
                             MacStatToFunFileData(file_info));
      }

      return true;
    });
  }
}

bool AppleFileSystem::IterateDirectoryCommon(
    const char* directory, const FunctionRef<bool(struct dirent*)>& visitor) {
  bool result = false;
  DIR* handle = opendir(directory[0] ? TCHAR_TO_UTF8(directory) : ".");
  if (handle) {
    result = true;
    struct dirent* de;
    while ((de = readdir(handle)) != nullptr) {
      if (CStringTraitsA::Strcmp(de->d_name, ".") &&
          CStringTraitsA::Strcmp(de->d_name, "..") &&
          CStringTraitsA::Strcmp(de->d_name, ".DS_Store")) {
        result = visitor(de);
      }
    }
    closedir(handle);
  }
  return result;
}

bool AppleFileSystem::CopyFile(const char* to, const char* from) {
  bool result = IFileSystem::CopyFile(to, from);
  if (result) {
    struct stat file_info;
    if (stat(from, &file_info) == 0) {
      file_info.st_mode |= S_IWUSR;
      chmod(TCHAR_TO_UTF8(*NormalizeFilename(to)), file_info.st_mode);
    }
  }
  return result;
}

int32 AppleFileSystem::stat(const char* filename, struct stat* out_file_info) {
  return stat(TCHAR_TO_UTF8(*NormalizeFilename(filename)), out_file_info);
}

}  // namespace fun
