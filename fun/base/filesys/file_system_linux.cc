#include "fun/base/filesys/file_system_linux.h"

#include <sys/file.h>   // flock()
#include <sys/stat.h>   // mkdirp()

namespace fun {

DEFINE_LOG_CATEGORY_STATIC(LogLinuxFS, info, All);

// make an Timespan object that represents the "epoch" for time_t (from a stat struct)
const DateTime UNIX_EPOCH(1970, 1, 1);

namespace {

FileStatData UnixStatToFunFileData(struct stat& file_info) {
  const bool is_directory = S_ISDIR(file_info.st_mode);

  int64 file_size_ = -1;
  if (!is_directory) {
    file_size_ = file_info.st_size;
  }

  return FileStatData(
    UNIX_EPOCH + Timespan(0, 0, file_info.st_ctime),
    UNIX_EPOCH + Timespan(0, 0, file_info.st_atime),
    UNIX_EPOCH + Timespan(0, 0, file_info.st_mtime),
    file_size_,
    is_directory,
    !!(file_info.st_mode & S_IWUSR));
}

}


// Linux file handle implementation which limits number of open files per thread. This
// is to prevent running out of system file handles. Should not be neccessary when
// using pak file (e.g., SHIPPING?) so not particularly optimized. Only manages
// files which are opened READ_ONLY.
#define MANAGE_FILE_HANDLES  (FUN_PLATFORM == FUN_PLATFORM_LINUX) // !FUN_BUILD_SHIPPING


/**
 * Linux file handle implementation
 */
class FUN_BASE_API LinuxFile : public IFile {
  enum { READWRITE_SIZE = 1024 * 1024 };

  inline bool IsValid() {
    return file_handle_ != -1;
  }

 public:
  LinuxFileSystem(int32 file_handle, const char* filename, bool is_readonly)
    : file_handle_(file_handle)
#if MANAGE_FILE_HANDLES
    , filename_(filename)
    , handle_slot_(-1)
    , file_offset_(0)
    , file_size_(0)
#endif // MANAGE_FILE_HANDLES
  {
    fun_check(file_handle_ > -1);
    fun_check(filename_.Len() > 0);

#if MANAGE_FILE_HANDLES
    // Only files opened for read will be managed
    if (is_readonly) {
      ReserveSlot();
      g_active_handles[handle_slot_] = this;
      struct stat file_info;
      fstat(file_handle_, &file_info);
      file_size_ = file_info.st_size;
    }
#endif // MANAGE_FILE_HANDLES
  }

  virtual ~LinuxFile() {
#if MANAGE_FILE_HANDLES
    if (IsManaged()) {
      if (g_active_handles[handle_slot_] == this) {
        close(file_handle_);
        g_active_handles[handle_slot_] = nullptr;
      }
    }
    else
#endif // MANAGE_FILE_HANDLES
    {
      close(file_handle_);
    }
    file_handle_ = -1;
  }

  int64 Tell() override {
#if MANAGE_FILE_HANDLES
    if (IsManaged()) {
      return file_offset_;
    }
    else
#endif // MANAGE_FILE_HANDLES
    {
      fun_check(IsValid());
      return lseek(file_handle_, 0, SEEK_CUR);
    }
  }

  bool Seek(int64 new_pos) override {
    fun_check(new_pos >= 0);

#if MANAGE_FILE_HANDLES
    if (IsManaged()) {
      file_offset_ = new_pos >= file_size_ ? file_size_ - 1 : new_pos;
      return IsValid() && g_active_handles[handle_slot_] == this ? lseek(file_handle_, file_offset_, SEEK_SET) != -1 : true;
    }
    else
#endif // MANAGE_FILE_HANDLES
    {
      fun_check(IsValid());
      return lseek(file_handle_, new_pos, SEEK_SET) != -1;
    }
  }

  bool SeekFromEnd(int64 relative_pos_to_end = 0) override {
    fun_check(relative_pos_to_end <= 0);

#if MANAGE_FILE_HANDLES
    if (IsManaged()) {
      file_offset_ = (relative_pos_to_end >= file_size_) ? 0 : (file_size_ + relative_pos_to_end - 1);
      return IsValid() && g_active_handles[handle_slot_] == this ? lseek(file_handle_, file_offset_, SEEK_SET) != -1 : true;
    }
    else
#endif // MANAGE_FILE_HANDLES
    {
      fun_check(IsValid());
      return lseek(file_handle_, relative_pos_to_end, SEEK_END) != -1;
    }
  }

  bool Read(uint8* dst, int64 len_to_read) override {
#if MANAGE_FILE_HANDLES
    if (IsManaged()) {
      ActivateSlot();
      int64 readed_len = ReadInternal(dst, len_to_read);
      file_offset_ += readed_len;
      return readed_len == len_to_read;
    }
    else
#endif // MANAGE_FILE_HANDLES
    {
      return ReadInternal(dst, len_to_read) == len_to_read;
    }
  }

  bool Write(const uint8* src, int64 len_to_write) override {
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

  int64 Size() override {
    #if MANAGE_FILE_HANDLES
    if (IsManaged()) {
      return file_size_;
    }
    else
    #endif
    {
      struct stat file_info;
      fstat(file_handle_, &file_info);
      return file_info.st_size;
    }
  }

 private:
#if MANAGE_FILE_HANDLES
  inline bool IsManaged() {
    return handle_slot_ != -1;
  }

  void ActivateSlot() {
    if (IsManaged()) {
      if (g_active_handles[handle_slot_] != this || (g_active_handles[handle_slot_] && g_active_handles[handle_slot_]->file_handle_ == -1)) {
        ReserveSlot();

        file_handle_ = open(TCHAR_TO_UTF8(*filename_), O_RDONLY | O_CLOEXEC);
        if (file_handle_ != -1) {
          lseek(file_handle_, file_offset_, SEEK_SET);
          g_active_handles[handle_slot_] = this;
        } else {
          fun_log(LogLinuxFS, Warning, "Could not (re)activate slot for file '%s'", *filename_);
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
#endif // MANAGE_FILE_HANDLES

  int64 ReadInternal(uint8* dst, int64 len_to_read) {
    fun_check(IsValid());
    int64 readed_len = 0;
    while (len_to_read) {
      fun_check(len_to_read >= 0);
      int64 this_size = MathBase::Min<int64>(READWRITE_SIZE, len_to_read);
      fun_check(dst);
      int64 this_read = read(file_handle_, dst, this_size);
      readed_len += this_read;
      if (this_read != this_size) {
        return readed_len;
      }
      dst += this_size;
      len_to_read -= this_size;
    }
    return readed_len;
  }

  // Holds the internal file handle.
  int32 file_handle_;

#if MANAGE_FILE_HANDLES
  // Holds the name of the file that this handle represents. Kept around for possible reopen of file.
  String filename_;
  // Most recent valid slot index for this handle; >=0 for handles which are managed.
  int32 handle_slot_;
  // Current file offset; valid if a managed handle.
  int64 file_offset_;
  // Cached file size; valid if a managed handle.
  int64 file_size_;

  // Each thread keeps a collection of active handles with access times.
  static const int32 ACTIVE_HANDLE_COUNT = 256;
  static __thread LinuxFile* g_active_handles[ACTIVE_HANDLE_COUNT];
  static __thread double g_access_times[ACTIVE_HANDLE_COUNT];
#endif // MANAGE_FILE_HANDLES
};

#if MANAGE_FILE_HANDLES
__thread LinuxFile* LinuxFile::g_active_handles[LinuxFile::ACTIVE_HANDLE_COUNT];
__thread double LinuxFile::g_access_times[LinuxFile::ACTIVE_HANDLE_COUNT];
#endif // MANAGE_FILE_HANDLES


/**
 * A class to handle case insensitive file opening. This is a band-aid, non-performant approach,
 * without any caching.
 */
class LinuxFileMapper {
 public:
  LinuxFileMapper() {}

  String GetPathComponent(const String& filename, int32 path_component_count) {
    // skip over empty part
    int start_position = (filename[0] == '/') ? 1 : 0;

    for (int32 component_index = 0; component_index < path_component_count; ++component_index) {
      int32 found_at_index = filename.Find("/", CaseSensitivity::CaseSensitive, ESearchDir::FromStart, start_position);

      if (found_at_index == INVALID_INDEX) {
        fun_check_msg(false, "Asked to get {0}-th path component, but filename '{1}' doesn't have that many!",
             path_component_count, filename);
        break;
      }

      start_position = found_at_index + 1;   // skip the '/' itself
    }

    // now return the
    int32 next_slash = filename.Find("/", CaseSensitivity::CaseSensitive, ESearchDir::FromStart, start_position);
    if (next_slash == INVALID_INDEX) {
      // just return the rest of the String
      return filename.RightChop(start_position);
    }
    else if (next_slash == start_position) {
      return ""; // encountered an invalid path like /foo/bar//baz
    }

    return filename.Mid(start_position, next_slash - start_position);
  }

  int32 CountPathComponents(const String & filename) {
    if (filename.Len() == 0) {
      return 0;
    }

    // if the first character is not a separator, it's part of a distinct component
    int32 component_count = (filename[0] != '/') ? 1 : 0;
    for (const auto& ch : filename) {
      if (ch == '/') {
        ++component_count;
      }
    }

    // cannot be 0 components if path is non-empty
    return MathBase::Max(component_count, 1);
  }

  /**
   * Tries to recursively find (using case-insensitive comparison) and open the file.
   * The first file found will be opened.
   *
   * @param filename - Original file path as requested (absolute)
   * @param path_component_to_look_for - Part of path we are currently trying to find.
   * @param max_path_components - Maximum number of path components (directories), i.e. how deep the path is.
   * @param constructed_path - The real (absolute) path that we have found so far
   *
   * @return a handle opened with open()
   */
  bool MapFileRecursively(const String& filename,
                          int32 path_component_to_look_for,
                          int32 max_path_components,
                          String& constructed_path) {
    // get the directory without the last path component
    String base_dir = constructed_path;

    // get the path component to compare
    String PathComponent = GetPathComponent(filename, path_component_to_look_for);
    String PathComponentLower = PathComponent.ToLower();

    bool found = false;

    // see if we can open this (we should)
    DIR* dir_handle = opendir(TCHAR_TO_UTF8(*base_dir));
    if (dir_handle) {
      struct dirent* de;
      while ((de = readdir(dir_handle)) != nullptr) {
        String de_name = UTF8_TO_TCHAR(de->d_name);
        if (de_name.ToLower() == PathComponentLower) {
          if (path_component_to_look_for < max_path_components - 1) {
            // make sure this is a directory
            bool is_directory = de->d_type == DT_DIR;
            if (de->d_type == DT_UNKNOWN) {
              struct stat file_info;
              if (stat(TCHAR_TO_UTF8(*(base_dir / de->d_name)), &file_info) == 0) {
                is_directory = S_ISDIR(file_info.st_mode);
              }
            }

            if (is_directory) {
              // recurse with the new filename
              String new_constructed_path = constructed_path;
              new_constructed_path /= de_name;

              found = MapFileRecursively(filename, path_component_to_look_for + 1, max_path_components, new_constructed_path);
              if (found) {
                constructed_path = new_constructed_path;
                break;
              }
            }
          } else {
            // last level, try opening directly
            String constructed_filename = constructed_path;
            constructed_filename /= de_name;

            struct stat file_info;
            found = (stat(TCHAR_TO_UTF8(*constructed_filename), &file_info) == 0);
            if (found) {
              constructed_path = constructed_filename;
              break;
            }
          }
        }
      }
      closedir(dir_handle);
    }

    return found;
  }

  /**
   * Tries to map a filename (one with a possibly wrong case) to one that exists.
   *
   * @param possibly_wrong_filename - absolute filename (that has possibly a wrong case)
   * @param existing_filename - filename that exists (only valid to use if the function returned success).
   */
  bool MapCaseInsensitiveFile(const String& possibly_wrong_filename, String& existing_filename) {
    // Cannot log anything here, as this may result in infinite recursion when this function is called on log file itself

    // We can get some "absolute" filenames like "D:/Blah/" here (e.g. non-Linux paths to src files embedded in assets).
    // In that case, fail silently.
    if (possibly_wrong_filename.IsEmpty() || possibly_wrong_filename[0] != '/') {
      return false;
    }

    // try the filename as given first
    struct stat file_info;
    bool found = stat(TCHAR_TO_UTF8(*possibly_wrong_filename), &file_info) == 0;

    if (found) {
      existing_filename = possibly_wrong_filename;
    } else {
      // perform a case-insensitive search from /

      int32 max_path_components = CountPathComponents(possibly_wrong_filename);
      if (max_path_components > 0) {
        String found_filename("/");   // start with root
        found = MapFileRecursively(possibly_wrong_filename, 0, max_path_components, found_filename);
        if (found) {
          existing_filename = found_filename;
        }
      }
    }

    return found;
  }

  /**
   * Opens a file for reading, disregarding the case.
   *
   * @param filename - absolute filename
   * @param mapped_to_filename - absolute filename that we mapped the filename to (always filled out on success, even if the same as filename)
   */
  int32 OpenCaseInsensitiveRead(const String& filename, String& mapped_to_filename) {
    // We can get some "absolute" filenames like "D:/Blah/" here (e.g. non-Linux paths to src files embedded in assets).
    // In that case, fail silently.
    if (filename.IsEmpty() || filename[0] != '/') {
      return -1;
    }

    // try opening right away
    int32 handle = open(TCHAR_TO_UTF8(*filename), O_RDONLY | O_CLOEXEC);
    if (handle != -1) {
      mapped_to_filename = filename;
    } else {
      // log non-standard errors only
      if (ENOENT != errno) {
        int err = errno;
        fun_log(LogLinuxFS, Warning, "open('{0}', O_RDONLY | O_CLOEXEC) failed: errno={1} ({2})", filename, err, ANSI_TO_TCHAR(strerror(err)));
      } else {
        // perform a case-insensitive search
        // make sure we get the absolute filename
        fun_check_msg(filename[0] == '/', "filename '{0}' given to OpenCaseInsensitiveRead is not absolute!", filename);

        int max_path_components = CountPathComponents(filename);
        if (max_path_components > 0) {
          String found_filename("/"); // start with root
          if (MapFileRecursively(filename, 0, max_path_components, found_filename)) {
            handle = open(TCHAR_TO_UTF8(*found_filename), O_RDONLY | O_CLOEXEC);
            if (handle != -1) {
              mapped_to_filename = found_filename;
              if (filename != mapped_to_filename) {
                fun_log(LogLinuxFS, info, "Mapped '{0}' to '{1}'", filename, mapped_to_filename);
              }
            }
          }
        }
      }
    }
    return handle;
  }
};

LinuxFileMapper g_caseinsens_mapper;

String LinuxFileSystem::NormalizeFilename(const char* filename) {
  String result(filename);
  CPaths::NormalizeFilename(result);
  return CPaths::ConvertRelativePathToFull(result);
}

String LinuxFileSystem::NormalizeDirectory(const char* directory) {
  String result(directory);
  CPaths::NormalizeDirectoryName(result);
  return CPaths::ConvertRelativePathToFull(result);
}

bool LinuxFileSystem::FileExists(const char* filename) {
  String casesensitive_filename;
  if (!g_caseinsens_mapper.MapCaseInsensitiveFile(NormalizeFilename(filename), casesensitive_filename)) {
    // could not find the file
    return false;
  }

  struct stat file_info;
  if (stat(TCHAR_TO_UTF8(*casesensitive_filename), &file_info) != -1) {
    return S_ISREG(file_info.st_mode);
  }
  return false;
}

int64 LinuxFileSystem::FileSize(const char* filename) {
  String casesensitive_filename;
  if (!g_caseinsens_mapper.MapCaseInsensitiveFile(NormalizeFilename(filename), casesensitive_filename)) {
    // could not find the file
    return -1;
  }

  struct stat file_info;
  file_info.st_size = -1;
  if (stat(TCHAR_TO_UTF8(*casesensitive_filename), &file_info) != -1) {
    // make sure to return -1 for directories
    if (S_ISDIR(file_info.st_mode)) {
      file_info.st_size = -1;
    }
  }
  return file_info.st_size;
}

bool LinuxFileSystem::DeleteFile(const char* filename) {
  String casesensitive_filename;
  String intended_filename(NormalizeFilename(filename));
  if (!g_caseinsens_mapper.MapCaseInsensitiveFile(intended_filename, casesensitive_filename)) {
    // could not find the file
    return false;
  }

  // removing mapped file is too dangerous
  if (intended_filename != casesensitive_filename) {
    fun_log(LogLinuxFS, Warning, "Could not find file '{0}', deleting file '{1}' instead (for consistency with the rest of file ops)", intended_filename, casesensitive_filename);
  }
  return unlink(TCHAR_TO_UTF8(*casesensitive_filename)) == 0;
}

bool LinuxFileSystem::IsReadOnly(const char* filename) {
  String casesensitive_filename;
  if (!g_caseinsens_mapper.MapCaseInsensitiveFile(NormalizeFilename(filename), casesensitive_filename)) {
    // could not find the file
    return false;
  }

  // skipping checking F_OK since this is already taken care of by case mapper

  if (access(TCHAR_TO_UTF8(*casesensitive_filename), W_OK) == -1) {
    return errno == EACCES;
  }
  return false;
}

bool LinuxFileSystem::MoveFile(const char* to, const char* from) {
  String casesensitive_filename;
  if (!g_caseinsens_mapper.MapCaseInsensitiveFile(NormalizeFilename(from), casesensitive_filename)) {
    // could not find the file
    return false;
  }

  return rename(TCHAR_TO_UTF8(*casesensitive_filename), TCHAR_TO_UTF8(*NormalizeFilename(to))) != -1;
}

bool LinuxFileSystem::SetReadOnly(const char* filename, bool readonly) {
  String casesensitive_filename;
  if (!g_caseinsens_mapper.MapCaseInsensitiveFile(NormalizeFilename(filename), casesensitive_filename)) {
    // could not find the file
    return false;
  }

  struct stat file_info;
  if (stat(TCHAR_TO_UTF8(*casesensitive_filename), &file_info) != -1) {
    if (readonly) {
      file_info.st_mode &= ~S_IWUSR;
    } else {
      file_info.st_mode |= S_IWUSR;
    }
    return chmod(TCHAR_TO_UTF8(*casesensitive_filename), file_info.st_mode);
  }
  return false;
}

DateTime LinuxFileSystem::GetTimestamp(const char* filename) {
  String casesensitive_filename;
  if (!g_caseinsens_mapper.MapCaseInsensitiveFile(NormalizeFilename(filename), casesensitive_filename)) {
    // could not find the file
    return DateTime::Null;
  }

  // get file times
  struct stat file_info;
  if (stat(TCHAR_TO_UTF8(*casesensitive_filename), &file_info) == -1) {
    if (errno == EOVERFLOW) {
      // hacky workaround for files mounted on Samba (see https://bugzilla.samba.org/show_bug.cgi?id=7707)
      return DateTime::Now();
    } else {
      return DateTime::Null;
    }
  }

  // convert _stat time to DateTime
  Timespan time_since_epoch(0, 0, file_info.st_mtime);
  return UNIX_EPOCH + time_since_epoch;
}

void LinuxFileSystem::SetTimestamp(const char* filename, const DateTime& timestamp) {
  String casesensitive_filename;
  if (!g_caseinsens_mapper.MapCaseInsensitiveFile(NormalizeFilename(filename), casesensitive_filename)) {
    // could not find the file
    return;
  }

  // get file times
  struct stat file_info;
  if (stat(TCHAR_TO_UTF8(*casesensitive_filename), &file_info) == -1) {
    return;
  }

  // change the modification time only
  struct utimbuf times;
  times.actime = file_info.st_atime;
  times.modtime = (timestamp - UNIX_EPOCH).GetTotalSeconds();
  utime(TCHAR_TO_UTF8(*casesensitive_filename), &times);
}

DateTime LinuxFileSystem::GetAccessTimestamp(const char* filename) {
  String casesensitive_filename;
  if (!g_caseinsens_mapper.MapCaseInsensitiveFile(NormalizeFilename(filename), casesensitive_filename)) {
    // could not find the file
    return DateTime::Null;
  }

  // get file times
  struct stat file_info;
  if (stat(TCHAR_TO_UTF8(*casesensitive_filename), &file_info) == -1) {
    return DateTime::Null;
  }

  // convert _stat time to DateTime
  Timespan time_since_epoch(0, 0, file_info.st_atime);
  return UNIX_EPOCH + time_since_epoch;
}

String LinuxFileSystem::GetFilenameOnDisk(const char* filename) {
  return filename;
/*
  String casesensitive_filename;
  if (!g_caseinsens_mapper.MapCaseInsensitiveFile(NormalizeFilename(filename), casesensitive_filename)) {
    return filename;
  }

  return casesensitive_filename;
*/
}

IFile* LinuxFileSystem::OpenRead(const char* filename, bool allow_write) {
  String mapped_to_name;
  int32 handle = g_caseinsens_mapper.OpenCaseInsensitiveRead(NormalizeFilename(filename), mapped_to_name);
  if (handle != -1) {
    return new LinuxFile(handle, *mapped_to_name, true);
  }
  return nullptr;
}

IFile* LinuxFileSystem::OpenWrite(const char* filename, bool append, bool allow_read) {
  int flags = O_CREAT | O_CLOEXEC; // prevent children from inheriting this

  if (allow_read) {
    flags |= O_RDWR;
  } else {
    flags |= O_WRONLY;
  }

  // create directories if needed.
  if (!CreateDirectoriesFromPath(filename)) {
    return nullptr;
  }

  // Caveat: cannot specify O_TRUNC in flags, as this will corrupt the file which may be "locked" by other process. We will ftruncate() it once we "lock" it
  int32 handle = open(TCHAR_TO_UTF8(*NormalizeFilename(filename)), flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (handle != -1) {
    // mimic Windows "exclusive write" behavior (we don't use FILE_SHARE_WRITE) by locking the file.
    // note that the (non-mandatory) "lock" will be removed by itself when the last file descriptor is close()d
    if (flock(handle, LOCK_EX | LOCK_NB) == -1) {
      // if locked, consider operation a failure
      if (EAGAIN == errno || EWOULDBLOCK == errno) {
        close(handle);
        return nullptr;
      }
      // all the other locking errors are ignored.
    }

    // truncate the file now that we locked it
    if (!append) {
      if (ftruncate(handle, 0) != 0) {
        int err = errno;
        fun_log(LogLinuxFS, Warning, "ftruncate() failed for '%s': errno=%d (%s)",
                              filename, err, ANSI_TO_TCHAR(strerror(err)));
        close(handle);
        return nullptr;
      }
    }

#if MANAGE_FILE_HANDLES
    LinuxFileSystem* file = new LinuxFile(handle, *NormalizeDirectory(filename), false);
#else
    LinuxFileSystem* file = new LinuxFile(handle, filename, false);
#endif // MANAGE_FILE_HANDLES

    if (append) {
      file->SeekFromEnd(0);
    }
    return file;
  }

  int err = errno;
  fun_log(LogLinuxFS, Warning, "open('%s', flags=0x%08X) failed: errno=%d (%s)", *NormalizeFilename(filename), flags, err, ANSI_TO_TCHAR(strerror(err)));
  return nullptr;
}

bool LinuxFileSystem::DirectoryExists(const char* directory) {
  String casesensitive_filename;
  if (!g_caseinsens_mapper.MapCaseInsensitiveFile(NormalizeFilename(directory), casesensitive_filename)) {
    // could not find the file
    return false;
  }

  struct stat file_info;
  if (stat(TCHAR_TO_UTF8(*casesensitive_filename), &file_info) != -1) {
    return S_ISDIR(file_info.st_mode);
  }
  return false;
}

bool LinuxFileSystem::CreateDirectory(const char* directory) {
  return mkdir(TCHAR_TO_UTF8(*NormalizeFilename(directory)), 0755) == 0;
}

bool LinuxFileSystem::DeleteDirectory(const char* directory) {
  String casesensitive_filename;
  String intended_filename(NormalizeFilename(directory));
  if (!g_caseinsens_mapper.MapCaseInsensitiveFile(intended_filename, casesensitive_filename)) {
    // could not find the directory
    return false;
  }

  // removing mapped directory is too dangerous
  if (intended_filename != casesensitive_filename) {
    fun_log(LogLinuxFS, Warning, "Could not find directory '%s', deleting '%s' instead (for consistency with the rest of file ops)", *intended_filename, *casesensitive_filename);
  }
  return rmdir(TCHAR_TO_UTF8(*casesensitive_filename)) == 0;
}

FileStatData LinuxFileSystem::GetStatData(const char* filename_or_directory) {
  String casesensitive_filename;
  if (!g_caseinsens_mapper.MapCaseInsensitiveFile(NormalizeFilename(filename_or_directory), casesensitive_filename)) {
    // could not find the file
    return FileStatData();
  }

  struct stat file_info;
  if (stat(TCHAR_TO_UTF8(*casesensitive_filename), &file_info) == -1) {
    return FileStatData();
  }

  return UnixStatToFunFileData(file_info);
}

bool LinuxFileSystem::IterateDirectory(const char* directory, DirectoryVisitor& visitor) {
  const String directory_str = directory;
  const String normalized_directory_str = NormalizeFilename(directory);

  return IterateDirectoryCommon(directory, [&](struct dirent* de) -> bool {
    const String UnicodeEntryName = UTF8_TO_TCHAR(de->d_name);

    bool is_directory = false;
    if (de->d_type != DT_UNKNOWN) {
      is_directory = de->d_type == DT_DIR;
    } else {
      // filesystem does not support d_type, fallback to stat
      struct stat file_info;
      const String absolute_unicode_name = normalized_directory_str / UnicodeEntryName;
      if (stat(TCHAR_TO_UTF8(*absolute_unicode_name), &file_info) != -1) {
        is_directory = ((file_info.st_mode & S_IFMT) == S_IFDIR);
      } else {
        int err = errno;
        fun_log(LogLinuxFS, Warning, "Cannot determine whether '%s' is a directory - d_type not supported and stat() failed with errno=%d (%s)", *absolute_unicode_name, err, UTF8_TO_TCHAR(strerror(err)));
      }
    }

    return visitor.Visit(*(directory_str / UnicodeEntryName), is_directory);
  });
}

bool LinuxFileSystem::IterateDirectoryStat(const char* directory, DirectoryStatVisitor& visitor) {
  const String directory_str = directory;
  const String normalized_directory_str = NormalizeFilename(directory);

  return IterateDirectoryCommon(directory, [&](struct dirent* de) -> bool {
    const String UnicodeEntryName = UTF8_TO_TCHAR(de->d_name);

    struct stat file_info;
    const String absolute_unicode_name = normalized_directory_str / UnicodeEntryName;
    if (stat(TCHAR_TO_UTF8(*absolute_unicode_name), &file_info) != -1) {
      return visitor.Visit(*(directory_str / UnicodeEntryName), UnixStatToFunFileData(file_info));
    }

    return true;
  });
}

bool LinuxFileSystem::IterateDirectoryCommon(const char* directory, const FunctionRef<bool(struct dirent*)>& visitor) {
  bool result = false;

  String normalized_directory = NormalizeFilename(directory);
  DIR* handle = opendir(TCHAR_TO_UTF8(*normalized_directory));
  if (handle) {
    result = true;
    struct dirent* de;
    while ((de = readdir(handle)) != nullptr) {
      if (CStringTraitsA::Strcmp(UTF8_TO_TCHAR(de->d_name), ".") && CStringTraitsA::Strcmp(UTF8_TO_TCHAR(de->d_name), "..")) {
        result = visitor(de);
      }
    }
    closedir(handle);
  }
  return result;
}

bool LinuxFileSystem::CreateDirectoriesFromPath(const char* path) {
  // if the file already exists, then directories exist.
  struct stat file_info;
  if (stat(TCHAR_TO_UTF8(*NormalizeFilename(path)), &file_info) != -1) {
    return true;
  }

  // convert path to native char String.
  int32 len = strlen(TCHAR_TO_UTF8(*NormalizeFilename(path)));
  char* dir_path = reinterpret_cast<char*>(UnsafeMemory::Malloc((len + 2) * sizeof(char)));
  char* sub_path = reinterpret_cast<char*>(UnsafeMemory::Malloc((len + 2) * sizeof(char)));
  strcpy(dir_path, TCHAR_TO_UTF8(*NormalizeFilename(path)));

  for (int32 i = 0; i < len; ++i) {
    sub_path[i] = dir_path[i];

    if (sub_path[i] == '/') {
      sub_path[i + 1] = 0;

      // directory exists?
      struct stat sub_path_file_info;
      if (stat(sub_path, &sub_path_file_info) == -1) {
        // nope. create it.
        if (mkdir(sub_path, 0755) == -1) {
          int err = errno;
          fun_log(LogLinuxFS, Warning, "create dir('%s') failed: errno=%d (%s)", dir_path, err, ANSI_TO_TCHAR(strerror(err)));
          UnsafeMemory::Free(dir_path);
          UnsafeMemory::Free(sub_path);
          return false;
        }
      }
    }
  }

  UnsafeMemory::Free(dir_path);
  UnsafeMemory::Free(sub_path);
  return true;
}

IFileSystem& IFileSystem::GetPhysicalFileSystem() {
  static LinuxFileSystem singleton;
  return singleton;
}

} // namespace fun
