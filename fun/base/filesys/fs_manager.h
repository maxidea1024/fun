#pragma once

#include "fun/base/base.h"

#include "Platform/Generic/GenericPlatformFS.h"
#include "HAL/PlatformTypes.h"
#include "fun/base/flags.h"

namespace fun {

// Maximum length of any filename.  For now, we have no restriction.
// We would probably use shortening rules if we have to.
#define MAX_FUN_FILENAME_LENGTH  (PLATFORM_MAX_FILEPATH_LENGTH)

/**
 *
 */
enum class FileWriteFlag {
  None = 0x00,

  NoFail = 0x01,
  NoReplaceExisting = 0x02,
  EvenIfReadOnly = 0x04,
  Append = 0x08,
  AllowRead = 0x10
};
FUN_DECLARE_FLAGS(FileWriteFlags, FileWriteFlag);

/**
 *
 */
enum class FileReadFlag {
  None = 0x00,

  NoFail = 0x01,
  Silent = 0x02,
  AllowWrite = 0x04,
};
FUN_DECLARE_FLAGS(FileReadFlags, FileReadFlag);

/**
 *
 */
enum class FileCopyResult {
  OK = 0,
  Fail = 1,
  Canceled = 2,
};

/**
 *
 */
class FileCopyProgress {
 public:
  virtual ~FileCopyProgress() {}

  virtual bool Poll(float fraction) = 0;
};

/**
 *
 */
enum class FileOpenFlag {
  None = 0x00,

  /** Open for reading */
  Read = 0x01,
  /** Open for writing */
  Write = 0x02,
  /** When writing, keep the existing data, set the filepointer to the end of the existing data */
  Append = 0x40,
};
FUN_DECLARE_FLAGS(FileOpenFlags, FileOpenFlag);


/**
 *
 */
class IFileManager {
 protected:
  /** Construtor. */
  IFileManager() {}

 public:
  /** Singleton access, platform specific, also calls PreInit() */
  FUN_BASE_API static IFileManager& Get();

  /** Allow the file manager to handle the commandline */
  virtual void ProcessCommandLineOptions() = 0;

  /** Enables/disables the sandbox, if it is being used */
  virtual void SetSandboxEnabled(bool enabled) = 0;

  /** Returns whether the sandbox is enabled or not */
  virtual bool IsSandboxEnabled() const = 0;

  /** Creates file reader archive. */
  virtual Archive* CreateFileReader(const char* filename, FileReadFlags read_flags = 0) = 0;

  /** Creates file writer archive. */
  virtual Archive* CreateFileWriter(const char* filename, FileWriteFlags write_flags = 0) = 0;

  // If you're writing to a debug file, you should use CreateDebugFileWriter, and wrap the calling code in #if ALLOW_DEBUG_FILES.
#if ALLOW_DEBUG_FILES
  virtual Archive* CreateDebugFileWriter(const char* filename, FileWriteFlags write_flags = 0) = 0;
#endif

  /** Checks if a file is read-only. */
  virtual bool IsReadOnly(const char* filename) = 0;

  /** Deletes a file. */
  virtual bool Delete(const char* filename, bool require_exists = false, bool event_if_read_only = false, bool quiet = false) = 0;

  /** Copies a file. */
  virtual FileCopyResult Copy(const char* dst, const char* src, bool replace = true, bool event_if_read_only = false, bool copy_attributes = false, FileCopyProgress* progress = nullptr) = 0; // utility

  /** Moves/renames a file. */
  virtual bool Move(const char* dst, const char* src, bool replace = true, bool event_if_read_only = false, bool copy_attributes = false, bool do_not_retry_or_error = false) = 0;

  /** Checks if a file exists */
  virtual bool FileExists(const char* filename) = 0;

  /** Checks if a directory exists. */
  virtual bool DirectoryExists(const char* directory) = 0;

  /** Creates a directory. */
  virtual bool MakeDirectory(const char* path, bool tree = 0) = 0;

  /** Deletes a directory. */
  virtual bool DeleteDirectory(const char* path, bool require_exists = false, bool tree = false) = 0;

  /** Return the stat data for the given file or directory. Check the FileStatData::is_valid member before using the returned data */
  virtual FileStatData GetStatData(const char* filename_or_directory) = 0;

  /** Finds file or directories. */
  virtual void FindFiles(Array<String>& out_filenames, const char* filename, bool with_files, bool with_directories) = 0;

  /**
   * Finds all the files within the given directory, with optional file extension filter.
   *
   * @param directory - the absolute path to the directory to search. Ex: "C:\fun\pictures"
   *
   * @param file_extension - If file_extension is NULL, or an empty string "" then all files are found.
   *          Otherwise file_extension can be of the form .EXT or just EXT and only files with that extension will be returned.
   *
   * @return out_found_files, All the files that matched the optional file_extension filter, or all files if none was specified.
   */
  virtual void FindFiles(Array<String>& out_found_files, const char* directory, const char* file_extension = nullptr) = 0;

  /** Finds file or directories recursively. */
  virtual void FindFilesRecursive(Array<String>& out_filenames, const char* start_directory, const char* filename, bool with_files, bool with_directories, bool clear_filenames = true) = 0; // utility

  /**
   * Call the Visit function of the visitor once for each file or directory in a single directory. This function does not explore subdirectories.
   *
   * @param directory - The directory to iterate the contents of.
   * @param visitor - visitor to call for each element of the directory
   *
   * @return false if the directory did not exist or if the visitor returned false.
   */
  virtual bool IterateDirectory(const char* directory, IPlatformFS::DirectoryVisitor& visitor) = 0;

  /**
   * Call the Visit function of the visitor once for each file or directory in a directory tree. This function explores subdirectories.
   *
   * @param directory - The directory to iterate the contents of, recursively.
   * @param visitor - visitor to call for each element of the directory and each element of all subdirectories.
   *
   * @return false if the directory did not exist or if the visitor returned false.
   */
  virtual bool IterateDirectoryRecursively(const char* directory, IPlatformFS::DirectoryVisitor& visitor) = 0;

  /**
   * Call the Visit function of the visitor once for each file or directory in a single directory. This function does not explore subdirectories.
   *
   * @param directory - The directory to iterate the contents of.
   * @param visitor - visitor to call for each element of the directory
   *
   * @return false if the directory did not exist or if the visitor returned false.
   */
  virtual bool IterateDirectoryStat(const char* directory, IPlatformFS::DirectoryStatVisitor& visitor) = 0;

  /**
   * Call the Visit function of the visitor once for each file or directory in a directory tree. This function explores subdirectories.
   *
   * @param directory - The directory to iterate the contents of, recursively.
   * @param visitor - visitor to call for each element of the directory and each element of all subdirectories.
   *
   * @return false if the directory did not exist or if the visitor returned false.
   */
  virtual bool IterateDirectoryStatRecursively(const char* directory, IPlatformFS::DirectoryStatVisitor& visitor) = 0;

  /** Gets the age of a file measured in seconds. */
  virtual double GetFileAgeSeconds(const char* filename) = 0;

  /**
   * @return the modification time of the given file (or DateTime::MinValue() on failure)
   */
  virtual DateTime GetTimestamp(const char* path) = 0;

  /**
   * @return the modification time of the given file (or DateTime::MinValue() on failure)
   */
  virtual void GetTimestampPair(const char* path_a, const char* path_b, DateTime& out_timestamp_a, DateTime& out_timestamp_b) = 0;

  /**
   * Sets the modification time of the given file
   */
  virtual bool SetTimestamp(const char* path, const DateTime& timestamp) = 0;

  /**
   * @return the last access time of the given file (or DateTime::MinValue() on failure)
   */
  virtual DateTime GetAccessTimestamp(const char* filename) = 0;

  /**
   * Converts passed in filename to use a relative path.
   *
   * @param filename - filename to convert to use a relative path
   *
   * @return filename using relative path
   */
  virtual String ConvertToRelativePath(const char* filename) = 0;

  /**
   * Converts passed in filename to use an absolute path (for reading)
   *
   * @param filename - filename to convert to use an absolute path, safe to pass in already using absolute path
   *
   * @return filename using absolute path
   */
  virtual String ConvertToAbsolutePathForExternalAppForRead(const char* absolute_path) = 0;

  /**
   * Converts passed in filename to use an absolute path (for writing)
   *
   * @param filename - filename to convert to use an absolute path, safe to pass in already using absolute path
   *
   * @return filename using absolute path
   */
  virtual String ConvertToAbsolutePathForExternalAppForWrite(const char* absolute_path) = 0;

  /**
   *  Returns the size of a file. (Thread-safe)
   *
   *  @param filename - Platform-independent Fun filename.
   *  @return File size in bytes or INVALID_INDEX if the file didn't exist.
   */
  virtual int64 FileSize(const char* filename) = 0;

  /**
   * Sends a message to the file server, and will block until it's complete. Will return
   * immediately if the file manager doesn't support talking to a server.
   *
   * @param message - The string message to send to the server
   *
   * @return true if the message was sent to server and it returned success, or false if there is no server, or the command failed
   */
  virtual bool SendMessageToServer(const char* message, IPlatformFS::IFileServerMessageHandler* handler) = 0;

  /**
  * For case insensitive filesystems, returns the full path of the file with the same case as in the filesystem.
  *
  * @param filename - filename to query
  *
  * @return filename with the same case as in the filesystem.
  */
  virtual String GetFilenameOnDisk(const char* filename) = 0;
};

} // namespace fun
