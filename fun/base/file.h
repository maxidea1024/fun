#pragma once

#include "fun/base/base.h"
#include "fun/base/container/array.h"
#include "fun/base/string/string.h"
#include "fun/base/timestamp.h"

#if FUN_PLATFORM_WINDOWS_FAMILY
#if defined(_WIN32_WCE)
#include "fun/base/file_wince.h"
#else
#include "fun/base/file_win32.h"
#endif
#elif FUN_PLATFORM != FUN_PLATFORM_VXWORKS
#include "fun/base/file_vx.h"
#elif FUN_PLATFORM_UNIX_FAMILY
#include "fun/base/file_unix.h"
#endif

namespace fun {

class Path;

/**
 * The File class provides methods for working with a file.
 *
 * Regarding paths passed to the various methods, note that
 * platform-specific limitations regarding maximum length
 * of the entire path and its components apply.
 *
 * On Windows, the implementation tries to work around the rather
 * low 260 characters MAX_PATH limit by adding the "\\?\" prefix if
 * a path is absolute and exceeds MAX_PATH characters in length.
 * Note that various limitations regarding usage of the "\\?\"
 * prefix apply in that case, e.g. the path must
 * not contain relative components ("." and "..") and must not
 * use the forward slash ("/") as directory separator.
 */
class FUN_BASE_API File : private FileImpl {
 public:
  typedef FileSizeImpl FileSize;

  /**
   * Type of link for LinkTo().
   */
  enum LinkType {
    /** Hard link */
    LINK_HARD = 0,
    /** Symbolic link */
    LINK_SYMBOLIC = 1
  };

  /**
   * Creates the file.
   */
  File();

  /**
   * Creates the file.
   */
  File(const String& path);

  /**
   * Creates the file.
   */
  File(const char* path);

  /**
   * Creates the file.
   */
  File(const Path& path);

  /**
   * Copy constructor.
   */
  File(const File& file);

  /**
   * Destroys the file.
   */
  virtual ~File();

  /**
   * Assignment operator
   */
  File& operator=(const File& file);

  /**
   * Assignment operator
   */
  File& operator=(const String& path);

  /**
   * Assignment operator
   */
  File& operator=(const char* path);

  /**
   * Assignment operator
   */
  File& operator=(const Path& path);

  /**
   * Swaps the file with another one.
   */
  void Swap(File& file);

  /**
   * Returns the path.
   */
  const String& GetPath() const;

  /**
   * Returns true if the file exists.
   */
  bool Exists() const;

  /**
   * Returns true if the file is readable.
   */
  bool CanRead() const;

  /**
   * Returns true if the file is writeable.
   */
  bool CanWrite() const;

  /**
   * Returns true if the file is executable.
   *
   * On Windows, the file must have
   * the extension ".EXE" to be executable.
   * On Unix platforms, the executable permission
   * bit must be set.
   */
  bool CanExecute() const;

  /**
   * Returns true if the file is a regular file.
   */
  bool IsFile() const;

  /**
   * Returns true if the file is a symbolic link.
   */
  bool IsLink() const;

  /**
   * Returns true if the file is a directory.
   */
  bool IsDirectory() const;

  /**
   * Returns true if the file is a device.
   */
  bool IsDevice() const;

  /**
   * Returns true if the file is hidden.
   *
   * On Windows platforms, the file's hidden
   * attribute is set for this to be true.
   *
   * On Unix platforms, the file name must
   * begin with a period for this to be true.
   */
  bool IsHidden() const;

  /**
   * Returns the creation date of the file.
   *
   * Not all platforms or filesystems (e.g. Linux and most Unix
   * platforms with the exception of FreeBSD and Mac OS X)
   * maintain the creation date of a file.
   * On such platforms, GetCreated() returns
   * the time of the last inode modification.
   */
  Timestamp GetCreated() const;

  /**
   * Returns the modification date of the file.
   */
  Timestamp GetLastModified() const;

  /**
   * Sets the modification date of the file.
   */
  File& SetLastModified(const Timestamp& ts);

  /**
   * Returns the size of the file in bytes.
   */
  FileSize GetSize() const;

  /**
   * Sets the size of the file in bytes. Can be used
   * to truncate a file.
   */
  File& SetSize(FileSize size);

  /**
   * Makes the file writeable (if flag is true), or
   * non-writeable (if flag is false) by setting the
   * file's flags in the filesystem accordingly.
   */
  File& SetWritable(bool flag = true);

  /**
   * Makes the file non-writeable (if flag is true), or
   * writeable (if flag is false) by setting the
   * file's flags in the filesystem accordingly.
   */
  File& SetReadOnly(bool flag = true);

  /**
   * Makes the file executable (if flag is true), or
   * non-executable (if flag is false) by setting
   * the file's permission bits accordingly.
   *
   * Does nothing on Windows.
   */
  File& SetExecutable(bool flag = true);

  /**
   * Copies the file (or directory) to the given path.
   * The target path can be a directory.
   *
   * A directory is copied recursively.
   */
  void CopyTo(const String& path) const;

  /**
   * Copies the file (or directory) to the given path and
   * removes the original file. The target path can be a directory.
   */
  void MoveTo(const String& path);

  /**
   * Renames the file to the new name.
   */
  void RenameTo(const String& path);

  /**
   * Creates a link (symbolic or hard, depending on type argument)
   * at the given path to the file or directory.
   *
   * May not be supported on all platforms.
   * Furthermore, some operating systems do not allow creating
   * hard links to directories.
   */
  void LinkTo(const String& path, LinkType type = LINK_SYMBOLIC) const;

  /**
   * Deletes the file. If recursive is true and the
   * file is a directory, recursively deletes all
   * files in the directory.
   */
  void Remove(bool recursive = false);

  /**
   * Creates a new, empty file in an atomic operation.
   * Returns true if the file has been created and false
   * if the file already exists. Throws an exception if
   * an error occurs.
   */
  bool CreateFile();

  /**
   * Creates a directory. Returns true if the directory
   * has been created and false if it already exists.
   * Throws an exception if an error occurs.
   */
  bool CreateDirectory();

  /**
   * Creates a directory (and all parent directories if necessary).
   */
  void CreateDirectories();

  /**
   * Fills the vector with the names of all files in the directory.
   */
  void List(Array<String>& files) const;

  /**
   * Fills the vector with the names of all files in the directory.
   */
  void List(Array<File>& files) const;

  /**
   * Returns the total size in bytes of the partition containing this path.
   */
  FileSize TotalSpace() const;

  /**
   * Returns the number of usable free bytes on the partition containing this
   * path.
   */
  FileSize UsableSpace() const;

  /**
   * Returns the number of free bytes on the partition containing this path.
   */
  FileSize FreeSpace() const;

  bool operator==(const File& file) const;
  bool operator!=(const File& file) const;
  bool operator<(const File& file) const;
  bool operator<=(const File& file) const;
  bool operator>(const File& file) const;
  bool operator>=(const File& file) const;

  static void HandleLastError(const String& path);

 protected:
  /**
   * Copies a directory. Used internally by CopyTo().
   */
  void CopyDirectory(const String& path) const;
};

//
// inlines
//

FUN_ALWAYS_INLINE const String& File::GetPath() const { return GetPathImpl(); }

FUN_ALWAYS_INLINE bool File::operator==(const File& file) const {
  return GetPathImpl() == file.GetPathImpl();
}

FUN_ALWAYS_INLINE bool File::operator!=(const File& file) const {
  return GetPathImpl() != file.GetPathImpl();
}

FUN_ALWAYS_INLINE bool File::operator<(const File& file) const {
  return GetPathImpl() < file.GetPathImpl();
}

FUN_ALWAYS_INLINE bool File::operator<=(const File& file) const {
  return GetPathImpl() <= file.GetPathImpl();
}

FUN_ALWAYS_INLINE bool File::operator>(const File& file) const {
  return GetPathImpl() > file.GetPathImpl();
}

FUN_ALWAYS_INLINE bool File::operator>=(const File& file) const {
  return GetPathImpl() >= file.GetPathImpl();
}

FUN_ALWAYS_INLINE void Swap(File& lhs, File& rhs) { lhs.Swap(rhs); }

}  // namespace fun
