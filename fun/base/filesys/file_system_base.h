#pragma once

#include "fun/base/base.h"

namespace fun {

class DateTime;

/**
 * File handle interface.
 */
class FUN_BASE_API IFile {
 public:
  /**
   * Destructor, also the only way to close the file handle.
   */
  virtual ~IFile() {}

  /** Return the current write or read position. */
  virtual int64 Tell() = 0;

  /**
   * Change the current write or read position.
   *
   * @param new_pos - new write or read position
   *
   * @return true if the operation completed successfully.
   */
  virtual bool Seek(int64 new_pos) = 0;

  /**
   * Change the current write or read position, relative to the end of the file.
   *
   * @param relative_pos_to_end - new write or read position, relative to the
   * end of the file should be <=0!
   *
   * @return true if the operation completed successfully.
   */
  virtual bool SeekFromEnd(int64 relative_pos_to_end = 0) = 0;

  /**
   * Read bytes from the file.
   *
   * @param dst - Buffer to holds the results, should be at least len_to_read in
   * size.
   * @param len_to_read - Number of bytes to read into the destination.
   *
   * @return true if the operation completed successfully.
   */
  virtual bool Read(uint8* dst, int64 len_to_read) = 0;

  /**
   * Write bytes to the file.
   *
   * @param src - Buffer to write, should be at least len_to_write in size.
   * @param len_to_write - Number of bytes to write.
   *
   * @return true if the operation completed successfully.
   */
  virtual bool Write(const uint8* src, int64 len_to_write) = 0;

 public:
  /////////// Utility Functions. These have a default implementation that uses
  /// the pure virtual operations.

  /** Return the total size of the file */
  virtual int64 Size();
};

/**
 * Contains the information that's returned from stat'ing a file or directory
 */
struct FileStatData {
  FileStatData()
      : creation_time(DateTime::Null),
        access_time(DateTime::Null),
        modification_time(DateTime::Null),
        file_size(-1),
        is_directory(false),
        is_readonly(false),
        is_valid(false) {}

  FileStatData(const DateTime& creation_time, const DateTime& access_time,
               const DateTime& modification_time, const int64 file_size,
               const bool is_directory, const bool is_readonly)
      : creation_time(creation_time),
        access_time(access_time),
        modification_time(modification_time),
        file_size(file_size),
        is_directory(is_directory),
        is_readonly(is_readonly),
        is_valid(true) {}

  /** The time that the file or directory was originally created, or
   * DateTime::Null if the creation time is unknown */
  DateTime creation_time;

  /** The time that the file or directory was last accessed, or DateTime::Null
   * if the access time is unknown */
  DateTime access_time;

  /** The time the the file or directory was last modified, or DateTime::Null if
   * the modification time is unknown */
  DateTime modification_time;

  /** Size of the file (in bytes), or -1 if the file size is unknown */
  int64 file_size;

  /** True if this data is for a directory, false if it's for a file */
  bool is_directory : 1;

  /** True if this file is read-only */
  bool is_readonly : 1;

  /** True if file or directory was found, false otherwise. Note that this value
   * being true does not ensure that the other members are filled in with
   * meaningful data, as not all file systems have access to all of this data */
  bool is_valid : 1;
};

/**
 * File I/O Interface
 */
class FUN_BASE_API IFileSystem {
 public:
  /** Physical file system of the _platform_, never wrapped. */
  static IFileSystem& GetPhysicalFileSystem();

  /** Returns the name of the physical platform file type. */
  static const char* GetPhysicalTypeName();

  /** Destructor. */
  virtual ~IFileSystem() {}

  /**
   * Set whether the sandbox is enabled or not
   *
   * @param enabled - true to enable the sandbox, false to disable it
   */
  virtual void SetSandboxEnabled(bool enabled) {}

  /**
   * Returns whether the sandbox is enabled or not
   *
   * @return bool true if enabled, false if not
   */
  virtual bool IsSandboxEnabled() const { return false; }

  /**
   * Checks if this platform file should be used even though it was not asked to
   * be. i.e. pak files exist on disk so we should use a pak file
   */
  virtual bool ShouldBeUsed(IFileSystem* inner, const char* cmdline) const {
    return false;
  }

  /**
   * Initializes platform file.
   *
   * @param inner - Platform file to wrap by this file.
   * @param cmdline - Command line to parse.
   *
   * @return true if the initialization was successful, false otherise.
   */
  virtual bool Initialize(IFileSystem* inner, const char* cmdline) = 0;

  /**
   * Performs initialization of the platform file after it has become the active
   * (PlatformFSManager.GetPlatformFS() will return this
   */
  virtual void InitializeAfterSetActive() {}

  /**
   * Identifies any platform specific paths that are guaranteed to be local
   * (i.e. cache, scratch space)
   */
  virtual void AddLocalDirectories(Array<String>& local_directories) {
    if (GetLowerLevel()) {
      GetLowerLevel()->AddLocalDirectories(local_directories);
    }
  }

  /** Gets the platform file wrapped by this file. */
  virtual IFileSystem* GetLowerLevel() = 0;

  /** Gets this platform file type name. */
  virtual const char* GetName() const = 0;

  /** Return true if the file exists. */
  virtual bool FileExists(const char* filename) = 0;

  /** Return the size of the file, or -1 if it doesn't exist. */
  virtual int64 FileSize(const char* filename) = 0;

  /** Delete a file and return true if the file exists. Will not delete read
   * only files. */
  virtual bool DeleteFile(const char* filename) = 0;

  /** Return true if the file is read only. */
  virtual bool IsReadOnly(const char* filename) = 0;

  /** Attempt to move a file. Return true if successful. Will not overwrite
   * existing files. */
  virtual bool MoveFile(const char* to, const char* from) = 0;

  /** Attempt to change the read only status of a file. Return true if
   * successful. */
  virtual bool SetReadOnly(const char* filename, bool readonly) = 0;

  /** Return the modification time of a file. Returns DateTime::Null on failure
   */
  virtual DateTime GetTimestamp(const char* filename) = 0;

  /** Sets the modification time of a file */
  virtual void SetTimestamp(const char* filename,
                            const DateTime& timestamp) = 0;

  /** Return the last access time of a file. Returns DateTime::Null on failure
   */
  virtual DateTime GetAccessTimestamp(const char* filename) = 0;

  /** For case insensitive filesystems, returns the full path of the file with
   * the same case as in the filesystem */
  virtual String GetFilenameOnDisk(const char* filename) = 0;

  /**
   * Attempt to open a file for reading.
   *
   * @param filename - file to be opened
   * @param allow_write - (applies to certain platforms only) whether this file
   * is allowed to be written to by other processes. This flag is needed to open
   * files that are currently being written to as well.
   *
   * @return If successful will return a non-nullptr pointer. Close the file by
   * delete'ing the handle.
   */
  virtual IFile* OpenRead(const char* filename, bool allow_write = false) = 0;

  /** Attempt to open a file for writing. If successful will return a
   * non-nullptr pointer. Close the file by delete'ing the handle. */
  virtual IFile* OpenWrite(const char* filename, bool append = false,
                           bool allow_read = false) = 0;

  /** Return true if the directory exists. */
  virtual bool DirectoryExists(const char* directory) = 0;

  /** Create a directory and return true if the directory was created or already
   * existed. */
  virtual bool CreateDirectory(const char* directory) = 0;

  /** Delete a directory and return true if the directory was deleted or
   * otherwise does not exist. */
  virtual bool DeleteDirectory(const char* directory) = 0;

  /** Return the stat data for the given file or directory. Check the
   * FileStatData::is_valid member before using the returned data */
  virtual FileStatData GetStatData(const char* filename_or_directory) = 0;

  /** Base class for file and directory visitors that take only the name. */
  struct DirectoryVisitor {
    virtual ~DirectoryVisitor() {}

    /**
     * Callback for a single file or a directory in a directory iteration.
     *
     * @param filename_or_directory - If is_directory is true, this is a
     * directory (with no trailing path delimiter), otherwise it is a file name.
     * @param is_directory - true if filename_or_directory is a directory.
     *
     * @return true if the iteration should continue.
     */
    virtual bool Visit(const char* filename_or_directory,
                       bool is_directory) = 0;
  };

  /** Base class for file and directory visitors that take all the stat data. */
  struct DirectoryStatVisitor {
    virtual ~DirectoryStatVisitor() {}

    /**
     * Callback for a single file or a directory in a directory iteration.
     *
     * @param filename_or_directory - If is_directory is true, this is a
     * directory (with no trailing path delimiter), otherwise it is a file name.
     * @param stat_data - The stat data for the file or directory.
     *
     * @return true if the iteration should continue.
     */
    virtual bool Visit(const char* filename_or_directory,
                       const FileStatData& stat_data) = 0;
  };

  /**
   * Call the Visit function of the visitor once for each file or directory in a
   * single directory. This function does not explore subdirectories.
   *
   * @param directory - The directory to iterate the contents of.
   * @param visitor - visitor to call for each element of the directory
   *
   * @return false if the directory did not exist or if the visitor returned
   * false.
   */
  virtual bool IterateDirectory(const char* directory,
                                DirectoryVisitor& visitor) = 0;

  /**
   * Call the Visit function of the visitor once for each file or directory in a
   * single directory. This function does not explore subdirectories.
   *
   * @param directory - The directory to iterate the contents of.
   * @param visitor - visitor to call for each element of the directory
   *
   * @return false if the directory did not exist or if the visitor returned
   * false.
   */
  virtual bool IterateDirectoryStat(const char* directory,
                                    DirectoryStatVisitor& visitor) = 0;

  /////////////////////////////////////////////////////////////////////////////////////////////////////////
  /////////// Utility Functions. These have a default implementation that uses
  /// the pure virtual operations.
  /////////// Generally, these do not need to be implemented per platform.
  /////////////////////////////////////////////////////////////////////////////////////////////////////////

  virtual void GetTimestampPair(const char* path_a, const char* path_b,
                                DateTime& out_timestamp_a,
                                DateTime& out_timestamp_b);

  /**
   * Call the Visit function of the visitor once for each file or directory in a
   * directory tree. This function explores subdirectories.
   *
   * @param directory - The directory to iterate the contents of, recursively.
   * @param visitor - visitor to call for each element of the directory and each
   * element of all subdirectories.
   *
   * @return false if the directory did not exist or if the visitor returned
   * false.
   */
  virtual bool IterateDirectoryRecursively(const char* directory,
                                           DirectoryVisitor& visitor);

  /**
   * Call the Visit function of the visitor once for each file or directory in a
   * directory tree. This function explores subdirectories.
   *
   * @param directory - The directory to iterate the contents of, recursively.
   * @param visitor - visitor to call for each element of the directory and each
   * element of all subdirectories.
   *
   * @return false if the directory did not exist or if the visitor returned
   * false.
   */
  virtual bool IterateDirectoryStatRecursively(const char* directory,
                                               DirectoryStatVisitor& visitor);

  /**
   * Delete all files and subdirectories in a directory, then delete the
   * directory itself
   *
   * @param directory - The directory to delete.
   *
   * @return true if the directory was deleted or did not exist.
   */
  virtual bool DeleteDirectoryRecursively(const char* directory);

  /** Create a directory, including any parent directories and return true if
   * the directory was created or already existed. */
  virtual bool CreateDirectoryTree(const char* directory);

  /**
   * Copy a file. This will fail if the destination file already exists.
   *
   * @param to - File to copy to.
   * @param from - File to copy from.
   *
   * @return true if the file was copied sucessfully.
   */
  virtual bool CopyFile(const char* to, const char* from);

  /**
   * Copy a file or a hierarchy of files (directory).
   *
   * @param destination_directory - Target path (either absolute or relative) to
   * copy to - always a directory! (e.g. "/home/dst/").
   * @param src - src file (or directory) to copy (e.g. "/home/src/stuff").
   * @param overwwrite_all_existing - Whether to overwrite everything that
   * exists at target
   *
   * @return true if operation completed successfully.
   */
  virtual bool CopyDirectoryTree(const char* destination_directory,
                                 const char* src, bool overwwrite_all_existing);

  /**
   * Converts passed in filename to use an absolute path (for reading).
   *
   * @param filename - filename to convert to use an absolute path, safe to pass
   * in already using absolute path
   *
   * @return filename using absolute path
   */
  virtual String ConvertToAbsolutePathForExternalAppForRead(
      const char* filename);

  /**
   * Converts passed in filename to use an absolute path (for writing)
   *
   * @param filename - filename to convert to use an absolute path, safe to pass
   * in already using absolute path
   *
   * @return filename using absolute path
   */
  virtual String ConvertToAbsolutePathForExternalAppForWrite(
      const char* filename);

  /**
   * Helper class to send/receive data to the file server function
   */
  struct IFileServerMessageHandler {
    virtual ~IFileServerMessageHandler() {}

    /** Subclass fills out an archive to send to the server */
    virtual void FillPayload(Archive& payload) = 0;

    /** Subclass pulls data response from the server */
    virtual void ProcessResponse(Archive& response) = 0;
  };

  /**
   * Sends a message to the file server, and will block until it's complete.
   * Will return immediately if the file manager doesn't support talking to a
   * server.
   *
   * @param message - The string message to send to the server
   *
   * @return true if the message was sent to server and it returned success, or
   * false if there is no server, or the command failed
   */
  virtual bool SendMessageToServer(const char* message,
                                   IFileServerMessageHandler* handler) {
    // by default, IFileSystem's can't talk to a server
    return false;
  }
};

/**
 * Common base for physical platform File I/O Interface
 */
class FUN_BASE_API IPhysicalFileSystem : public IFileSystem {
 public:
  bool ShouldBeUsed(IFileSystem* inner, const char* cmdline) const override {
    return true;
  }

  bool Initialize(IFileSystem* inner, const char* cmdline) override;

  IFileSystem* GetLowerLevel() override { return nullptr; }

  const char* GetName() const override {
    return IFileSystem::GetPhysicalTypeName();
  }
};

}  // namespace fun
