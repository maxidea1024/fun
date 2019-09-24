#pragma once

#include "fun/base/base.h"
#include "fun/base/container/array.h"
#include "fun/base/string/string.h"

namespace fun {

/**
 * This class represents filesystem paths in a platform-independent manner.
 * Unix, Windows and OpenVMS all use a different syntax for filesystem paths.
 * This class can work with all three formats.
 * A path is made up of an optional node name (only Windows and OpenVMS), an optional
 * device name (also only Windows and OpenVMS),
 * a list of Directory names and an optional filename.
 */
class FUN_BASE_API Path {
 public:
  enum Style {
    /**
     * Unix-style path
     */
    PATH_UNIX,
    /**
     * URI-style path, same as Unix-style
     */
    PATH_URI = PATH_UNIX,
    /**
     * Windows-style path
     */
    PATH_WINDOWS,
    /**
     * VMS-style path
     */
    PATH_VMS,
    /**
     * The current platform's native style
     */
    PATH_NATIVE,
    /**
     * Guess the style by examining the path
     */
    PATH_GUESS
  };

  typedef Array<String> StringList;

  /**
   * Creates an empty relative path.
   */
  Path();

  /**
   * Creates an empty absolute or relative path.
   */
  Path(bool absolute);

  /**
   * Creates a path from a string.
   */
  Path(const char* path);

  /**
   * Creates a path from a string.
   */
  Path(const char* path, Style style);

  /**
   * Creates a path from a string.
   */
  Path(const String& path);

  /**
   * Creates a path from a string.
   */
  Path(const String& path, Style style);

  /**
   * Copy constructor
   */
  Path(const Path& path);

  /**
   * Creates a path from a parent path and a filename.
   * The parent path is expected to reference a directory.
   */
  Path(const Path& parent, const String& filename);

  /**
   * Creates a path from a parent path and a filename.
   * The parent path is expected to reference a directory.
   */
  Path(const Path& parent, const char* filename);

  /**
   * Creates a path from a parent path and a relative path.
   * The parent path is expected to reference a directory.
   * The relative path is appended to the parent path.
   */
  Path(const Path& parent, const Path& relative);

  /**
   * Destroys the Path.
   */
  ~Path();

  /**
   * Assignment operator.
   */
  Path& operator = (const Path& path);

  /**
   * Assigns a string containing a path in native format.
   */
  Path& operator = (const String& path);

  /**
   * Assigns a string containing a path in native format.
   */
  Path& operator = (const char* path);

  /**
   * Swaps the path with another one.
   */
  void Swap(Path& path);

  /**
   * Assigns a string containing a path in native format.
   */
  Path& Assign(const String& path);

  /**
   * Assigns a string containing a path.
   */
  Path& Assign(const String& path, Style style);

  /**
   * Assigns the given path.
   */
  Path& Assign(const Path& path);

  /**
   * Assigns a string containing a path.
   */
  Path& Assign(const char* path);

  /**
   * Returns a string containing the path in native format.
   */
  String ToString() const;

  /**
   * Returns a string containing the path in the given format.
   */
  String ToString(Style style) const;

  /**
   * Same as Assign().
   */
  Path& Parse(const String& path);

  /**
   * Assigns a string containing a path.
   */
  Path& Parse(const String& path, Style style);

  /**
   * Tries to interpret the given string as a path
   * in native format.
   * If the path is syntactically valid, assigns the
   * path and returns true. Otherwise leaves the
   * object unchanged and returns false.
   */
  bool TryParse(const String& path);

  /**
   * Tries to interpret the given string as a path,
   * according to the given style.
   * If the path is syntactically valid, assigns the
   * path and returns true. Otherwise leaves the
   * object unchanged and returns false.
   */
  bool TryParse(const String& path, Style style);

  /**
   * The resulting path always refers to a directory and
   * the filename part is empty.
   */
  Path& ParseDirectory(const String& path);

  /**
   * The resulting path always refers to a directory and
   * the filename part is empty.
   */
  Path& ParseDirectory(const String& path, Style style);

  /**
   * If the path contains a filename, the filename is appended
   * to the directory list and cleared. Thus the resulting path
   * always refers to a directory.
   */
  Path& MakeDirectory();

  /**
   * If the path contains no filename, the last directory
   * becomes the filename.
   */
  Path& MakeFile();

  /**
   * Makes the path refer to its parent.
   */
  Path& MakeParent();

  /**
   * Makes the path absolute if it is relative.
   * The current working directory is taken as base directory.
   */
  Path& MakeAbsolute();

  /**
   * Makes the path absolute if it is relative.
   * The given path is taken as base.
   */
  Path& MakeAbsolute(const Path& base);

  /**
   * Appends the given path.
   */
  Path& Append(const Path& path);

  /**
   * Resolves the given path against the current one.
   *
   * If the given path is absolute, it replaces the current one.
   * Otherwise, the relative path is appended to the current path.
   */
  Path& Resolve(const Path& path);

  /**
   * Returns true if the path is absolute.
   */
  bool IsAbsolute() const;

  /**
   * Returns true if the path is relative.
   */
  bool IsRelative() const;

  /**
   * Returns true if the path references a directory
   * (the filename part is empty).
   */
  bool IsDirectory() const;

  /**
   * Returns true if the path references a file
   * (the filename part is not empty).
   */
  bool IsFile() const;

  /**
   * Sets the node name.
   * Setting a non-empty node automatically makes
   * the path an absolute one.
   */
  Path& SetNode(const String& node);

  /**
   * Returns the node name.
   */
  const String& GetNode() const;

  /**
   * Sets the device name.
   * Setting a non-empty device automatically makes
   * the path an absolute one.
   */
  Path& SetDevice(const String& device);

  /**
   * Returns the device name.
   */
  const String& GetDevice() const;

  /**
   * Returns the number of directories in the directory list.
   */
  int GetDepth() const;

  /**
   * Returns the n'th directory in the directory list.
   * If n == GetDepth(), returns the filename.
   */
  const String& GetDirectory(int n) const;

  /**
   * Returns the n'th directory in the directory list.
   * If n == GetDepth(), returns the filename.
   */
  const String& operator [] (int n) const;

  /**
   * Adds a directory to the directory list.
   */
  Path& PushDirectory(const String& dir);

  /**
   * Removes the last directory from the directory list.
   */
  Path& PopDirectory();

  /**
   * Removes the first directory from the directory list.
   */
  Path& PopFrontDirectory();

  /**
   * Sets the filename.
   */
  Path& SetFileName(const String& name);

  /**
   * Returns the filename.
   */
  const String& GetFileName() const;

  /**
   * Sets the basename part of the filename and
   * does not change the extension.
   */
  Path& SetBaseName(const String& name);

  /**
   * Returns the basename (the filename sans
   * extension) of the path.
   */
  String GetBaseName() const;

  /**
   * Sets the filename extension.
   */
  Path& SetExtension(const String& extension);

  /**
   * Returns the filename extension.
   */
  String GetExtension() const;

  /**
   * Returns the file version. VMS only.
   */
  const String& GetVersion() const;

  /**
   * Clears all components.
   */
  Path& Clear();

  /**
   * Returns a path referring to the path's
   * directory.
   */
  Path Parent() const;

  /**
   * Returns an absolute variant of the path,
   * taking the current working directory as base.
   */
  Path Absolute() const;

  /**
   * Returns an absolute variant of the path,
   * taking the given path as base.
   */
  Path Absolute(const Path& base) const;

  /**
   * Creates a path referring to a directory.
   */
  static Path ForDirectory(const String& path);

  /**
   * Creates a path referring to a directory.
   */
  static Path ForDirectory(const String& path, Style style);

  /**
   * Returns the platform's path name separator, which separates
   * the components (names) in a path.
   *
   * On Unix systems, this is the slash '/'. On Windows systems,
   * this is the backslash '\'. On OpenVMS systems, this is the
   * period '.'.
   */
  static char Separator();

  /**
   * Returns the platform's path separator, which separates
   * single paths in a list of paths.
   *
   * On Unix systems, this is the colon ':'. On Windows systems,
   * this is the semicolon ';'. On OpenVMS systems, this is the
   * comma ','.
   */
  static char PathSeparator();

  /**
   * Returns the current working directory.
   */
  static String Current();

  /**
   * Returns the user's home directory.
   */
  static String GetHome();

  /**
   * Returns the user's config directory.
   *
   * On Unix systems, this is the '~/.config/'. On Windows systems,
   * this is '%APPDATA%'.
   */
  static String GetConfigHome();

  /**
   * Returns the user's data directory.
   *
   * On Unix systems, this is the '~/.local/share/'. On Windows systems,
   * this is '%APPDATA%'.
   */
  static String GetDataHome();

  /**
   * Returns the user's cache directory.
   *
   * On Unix systems, this is the '~/.cache/'. On Windows systems,
   * this is '%APPDATA%'.
   */
  static String GetCacheHome();

  /**
   * Returns the executable name with path.
   */
  static String GetSelf();

  /**
   * Returns the temporary directory.
   */
  static String GetTemp();

  /**
   * Returns the systemwide config directory.
   *
   * On Unix systems, this is the '/etc/'.
   */
  static String GetConfig();

  /**
   * Returns the name of the null device.
   */
  static String GetNull();

  /**
   * Expands all environment variables contained in the path.
   *
   * On Unix, a tilde as first character in the path is
   * replaced with the path to user's home directory.
   */
  static String Expand(const String& path);

  /**
   * Fills the vector with all filesystem roots available on the
   * system. On Unix, there is exactly one root, "/".
   * On Windows, the roots are the drive letters.
   * On OpenVMS, the roots are the mounted disks.
   */
  static void ListRoots(Array<String>& roots);

  /**
   * Searches the file with the given name in the locations (paths) specified
   * by it and end. A relative path may be given in name.
   *
   * If the file is found in one of the locations, the complete
   * path of the file is stored in the path given as argument and true is returned.
   * Otherwise false is returned and the path argument remains unchanged.
   */
  static bool Find(const Array<String>& path_list, const String& name, Path& path);
  static bool Find(const Array<StringRef>& path_list, const String& name, Path& path);

  /**
   * Searches the file with the given name in the locations (paths) specified
   * in pathList. The paths in pathList must be delimited by the platform's
   * path separator (see PathSeparator()). A relative path may be given in name.
   *
   * If the file is found in one of the locations, the complete
   * path of the file is stored in the path given as argument and true is returned.
   * Otherwise false is returned and the path argument remains unchanged.
   */
  static bool Find(const String& path_list, const String& name, Path& path);

  /**
   * On Windows, this function converts a string (usually containing a path)
   * encoded in UTF-8 into a string encoded in the current Windows code page.
   *
   * This function should be used for every string passed as a file name to
   * a string stream or fopen().
   *
   * On all other platforms, or if FUN has not been compiled with Windows UTF-8
   * support, this function returns the string unchanged.
   */
  static String Transcode(const String& path);

 protected:
  void ParseUnix(const String& path);
  void ParseWindows(const String& path);
  void ParseVMS(const String& path);
  void ParseGuess(const String& path);

  String BuildUnix() const;
  String BuildWindows() const;
  String BuildVMS() const;

 private:
  String node_;
  String device_;
  String name_;
  String version_;
  StringList dirs_;
  bool absolute_;
};


//
// inlines
//

FUN_ALWAYS_INLINE bool Path::IsAbsolute() const {
  return absolute_;
}

FUN_ALWAYS_INLINE bool Path::IsRelative() const {
  return !absolute_;
}

FUN_ALWAYS_INLINE bool Path::IsDirectory() const {
  return name_.IsEmpty();
}

FUN_ALWAYS_INLINE bool Path::IsFile() const {
  return !name_.IsEmpty();
}

FUN_ALWAYS_INLINE Path& Path::Parse(const String& path) {
  return Assign(path);
}

FUN_ALWAYS_INLINE Path& Path::Parse(const String& path, Style style) {
  return Assign(path, style);
}

FUN_ALWAYS_INLINE const String& Path::GetNode() const {
  return node_;
}

FUN_ALWAYS_INLINE const String& Path::GetDevice() const {
  return device_;
}

FUN_ALWAYS_INLINE const String& Path::GetFileName() const {
  return name_;
}

FUN_ALWAYS_INLINE int Path::GetDepth() const {
  return dirs_.Count();
}

FUN_ALWAYS_INLINE const String& Path::GetVersion() const {
  return version_;
}

FUN_ALWAYS_INLINE Path Path::ForDirectory(const String& path) {
  Path p;
  return p.ParseDirectory(path);
}

FUN_ALWAYS_INLINE Path Path::ForDirectory(const String& path, Style style) {
  Path p;
  return p.ParseDirectory(path, style);
}

FUN_ALWAYS_INLINE char Path::Separator() {
#if FUN_PLATFORM_VMS_FAMILY
  return '.';
#elif FUN_PLATFORM_WINDOWS_FAMILY
  return '\\';
#else
  return '/';
#endif
}

FUN_ALWAYS_INLINE char Path::PathSeparator() {
#if FUN_PLATFORM_VMS_FAMILY
  return ',';
#elif FUN_PLATFORM_WINDOWS_FAMILY
  return ';';
#else
  return ':';
#endif
}

//TODO 이건 제거하도록 하자.
FUN_ALWAYS_INLINE void Swap(Path& x, Path& y) {
  x.Swap(y);
}

} // namespace fun
