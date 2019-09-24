#pragma once

#include "fun/base/base.h"
#include "PlatformFSManager.h"

namespace fun {

/**
 * Base class for file managers.
 *
 * This base class simplifies IFileManager implementations by providing
 * simple, unoptimized implementations of functions whose implementations
 * can be derived from other functions.
 */
class FUN_BASE_API FSManagerGeneric : public IFileManager {
  // instead of caching the LowLevel, we call the singleton each time to never be incorrect
  inline IPlatformFS& GetLowLevel() const {
    return PlatformFSManager::Get().GetPlatformFS();
  }

 public:
  /** Default constructor. */
  FSManagerGeneric() {}

  /** Virtual destructor. */
  virtual ~FSManagerGeneric() {}


  //
  // IFileManager interface
  //

  virtual void ProcessCommandLineOptions() override;

  virtual void SetSandboxEnabled(bool enabled) override {
    GetLowLevel().SetSandboxEnabled(enabled);
  }

  virtual bool IsSandboxEnabled() const override {
    return GetLowLevel().IsSandboxEnabled();
  }

  Archive* CreateFileReader(const char* filename, FileReadFlags read_flags = 0) override;
  Archive* CreateFileWriter(const char* filename, FileWriteFlags write_flags = 0) override;

#if ALLOW_DEBUG_FILES
  Archive* CreateDebugFileWriter(const char* filename, FileWriteFlags write_flags = 0) override {
    return CreateFileWriter(filename, write_flags);
  }
#endif

  bool Delete(const char* filename, bool require_exists = false, bool event_if_read_only = false, bool quiet = false) override;
  bool IsReadOnly(const char* filename) override;
  bool Move(const char* dst, const char* src, bool replace = true, bool event_if_read_only = false, bool copy_attributes = false, bool do_not_retry_or_error = false) override;
  bool FileExists(const char* filename) override;
  bool DirectoryExists(const char* directory) override;
  void FindFiles(Array<String>& result, const char* filename, bool with_files, bool with_directories) override;
  void FindFilesRecursive(Array<String>& out_filenames, const char* start_directory, const char* filename, bool with_files, bool with_directories, bool clear_filenames = true) override;
  double GetFileAgeSeconds(const char* filename) override;
  DateTime GetTimestamp(const char* filename) override;
  DateTime GetAccessTimestamp(const char* filename) override;
  void GetTimestampPair(const char* path_a, const char* path_b, DateTime& out_timestamp_a, DateTime& out_timestamp_b);
  bool SetTimestamp(const char* filename, const DateTime& timestamp) override;
  virtual String GetFilenameOnDisk(const char* filename) override;

  virtual FileCopyResult Copy(const char* dest_file, const char* src_file, bool replace_existing, bool event_if_read_only, bool copy_attributes, FileCopyProgress* progress) override;
  virtual bool MakeDirectory(const char* path, bool tree = false) override;
  virtual bool DeleteDirectory(const char* path, bool require_exists = false, bool tree = false) override;

  virtual FileStatData GetStatData(const char* filename_or_directory) override;

  /**
   * Finds all the files within the given directory, with optional file extension filter.
   *
   * @param directory - the absolute path to the directory to search. Ex: "C:\Fun\Pictures"
   *
   * @param file_extension - If file_extension is NULL, or an empty string "" then all files are found.
   *                Otherwise file_extension can be of the form .EXT or just EXT and only files with that extension will be returned.
   *
   * @return out_found_files, All the files that matched the optional file_extension filter, or all files if none was specified.
   */
  virtual void FindFiles(Array<String>& out_found_files, const char* directory, const char* file_extension = nullptr) override;

  /**
   * Call the Visit function of the visitor once for each file or directory in a single directory. This function does not explore subdirectories.
   *
   * @param directory - The directory to iterate the contents of.
   * @param visitor - visitor to call for each element of the directory
   *
   * @return false if the directory did not exist or if the visitor returned false.
   */
  bool IterateDirectory(const char* directory, IPlatformFS::DirectoryVisitor& visitor) override;

  /**
   * Call the Visit function of the visitor once for each file or directory in a directory tree. This function explores subdirectories.
   *
   * @param directory - The directory to iterate the contents of, recursively.
   * @param visitor - visitor to call for each element of the directory and each element of all subdirectories.
   *
   * @return false if the directory did not exist or if the visitor returned false.
   */
  bool IterateDirectoryRecursively(const char* directory, IPlatformFS::DirectoryVisitor& visitor) override;

  /**
   * Call the Visit function of the visitor once for each file or directory in a single directory. This function does not explore subdirectories.
   *
   * @param directory - The directory to iterate the contents of.
   * @param visitor - visitor to call for each element of the directory
   *
   * @return false if the directory did not exist or if the visitor returned false.
   */
  bool IterateDirectoryStat(const char* directory, IPlatformFS::DirectoryStatVisitor& visitor) override;

  /**
   * Call the Visit function of the visitor once for each file or directory in a directory tree. This function explores subdirectories.
   *
   * @param directory - The directory to iterate the contents of, recursively.
   * @param visitor - visitor to call for each element of the directory and each element of all subdirectories.
   *
   * @return false if the directory did not exist or if the visitor returned false.
   */
  bool IterateDirectoryStatRecursively(const char* directory, IPlatformFS::DirectoryStatVisitor& visitor) override;

  /**
   * Converts passed in filename to use a relative path.
   *
   * @param filename - filename to convert to use a relative path
   *
   * @return filename using relative path
   */
  static String DefaultConvertToRelativePath(const char* filename);

  /**
   * Converts passed in filename to use a relative path.
   *
   * @param filename - filename to convert to use a relative path
   *
   * @return filename using relative path
   */
  String ConvertToRelativePath(const char* filename) override;

  /**
   * Converts passed in filename to use an absolute path (for reading)
   *
   * @param filename - filename to convert to use an absolute path, safe to pass in already using absolute path
   *
   * @return filename using absolute path
   */
  String ConvertToAbsolutePathForExternalAppForRead(const char* filename) override;

  /**
   * Converts passed in filename to use an absolute path (for writing)
   *
   * @param filename - filename to convert to use an absolute path, safe to pass in already using absolute path
   *
   * @return filename using absolute path
   */
  String ConvertToAbsolutePathForExternalAppForWrite(const char* filename) override;

  /**
   * Returns the size of a file. (Thread-safe)
   *
   * @param filename - Platform-independent Fun filename.
   *
   * @return File size in bytes or INVALID_INDEX if the file didn't exist.
   */
  int64 FileSize(const char* filename) override;

  /**
   * Sends a message to the file server, and will block until it's complete. Will return
   * immediately if the file manager doesn't support talking to a server.
   *
   * @param message - The string message to send to the server
   *
   * @return true if the message was sent to server and it returned success, or false if there is no server, or the command failed
   */
  virtual bool SendMessageToServer(const char* message, IPlatformFS::IFileServerMessageHandler* handler) override {
    return GetLowLevel().SendMessageToServer(message, handler);
  }

 private:
  /**
   * Helper called from Copy if progress is available
   */
  FileCopyResult CopyWithProgress(const char* dest_file, const char* src_file, bool replace_existing, bool event_if_read_only, bool copy_attributes, FileCopyProgress* progress);

  void FindFilesRecursiveInternal(Array<String>& out_filenames, const char* start_directory, const char* filename, bool with_files, bool with_directories);
};


/**
 * ArchiveFileReaderGeneric
 */
class FUN_BASE_API ArchiveFileReaderGeneric : public Archive {
 public:
  ArchiveFileReaderGeneric(IFile* handle, const char* filename, int64 size);
  ~ArchiveFileReaderGeneric();

  void Seek(int64 new_pos) final;
  int64 Tell() final { return pos_; }
  int64 TotalSize() final { return size_; }
  bool Close() final;
  void Serialize(void* v, int64 len) final;
  String GetArchiveName() const override { return filename_; }

 protected:
  bool InternalPrecache(int64 precache_offset, int64 precache_size);

  /**
   * Platform specific seek
   *
   * @param new_pos - Offset from beginning of file to seek to
   *
   * @return false on failure
   */
  virtual bool SeekLowLevel(int64 new_pos);

  /** Close the file handle **/
  virtual void CloseLowLevel();

  /**
   * Platform specific read
   *
   * @param dst - Buffer to fill in
   * @param len_to_read - Number of bytes to read
   * @param out_readed_len - Bytes actually read
   */
  virtual void ReadLowLevel(uint8* dst, int64 len_to_read, int64& out_readed_len);

  /** filename for debugging purposes. */
  String filename_;
  int64 size_;
  int64 pos_;
  int64 buffer_base_;
  int64 buffer_count_;
  AutoPtr<IFile> handle_;
  uint8 buffer_[1024];
};


/**
 * ArchiveFileWriterGeneric
 */
class ArchiveFileWriterGeneric : public Archive {
 public:
  ArchiveFileWriterGeneric(IFile* handle, const char* filename, int64 pos);
  ~ArchiveFileWriterGeneric();

  void Seek(int64 new_pos) final;
  int64 Tell() final { return pos_; }
  int64 TotalSize() override;
  bool Close() final;
  void Serialize(void* v, int64 len) final;
  void Flush() final;
  String GetArchiveName() const override { return filename_; }

 protected:
  /**
   * Platform specific seek
   *
   * @param new_pos - Offset from beginning of file to seek to
   *
   * @return false on failure
   */
  virtual bool SeekLowLevel(int64 new_pos);

  /**
   * Close the file handle
   *
   * @return false on failure
   */
  virtual bool CloseLowLevel();

  /**
   * Platform specific write
   *
   * @param src - Buffer to write out
   * @param len_to_write - Number of bytes to write
   *
   * @return false on failure
   */
  virtual bool WriteLowLevel(const uint8* src, int64 len_to_write);

  /**
   * Logs I/O error
   * It is important to not call any platform API functions after the error occurred and before
   * calling this functions as the system error code may get reset and will not be properly
   * logged in this message.
   *
   * @param message - Brief description of what went wrong
   */
  void LogWriteError(const char* message);

  /** filename for debugging purposes */
  String filename_;
  int64 pos_;
  int64 buffer_count_;
  AutoPtr<IFile> handle_;
  uint8 buffer_[4096];
  bool is_logging_error_;
};

} // namespace fun
