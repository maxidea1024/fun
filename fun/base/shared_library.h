#pragma once

#include "fun/base/base.h"

#if FUN_PLATFORM_WINDOWS_FAMILY
#include "fun/base/shared_library_win32.h"
#else
#include "fun/base/shared_library_unix.h"
#endif

namespace fun {

/**
 * The SharedLibrary class dynamically
 * loads shared libraries at run-time.
 */
class FUN_BASE_API SharedLibrary : private SharedLibrayImpl {
 public:
  enum Flags {
    /**
     * On platforms that use dlopen(), use RTLD_GLOBAL. This is the default
     * if no flags are given.
     *
     * This flag is ignored on platforms that do not use dlopen().
     */
    SHLIB_GLOBAL = 1,

    /**
     * On platforms that use dlopen(), use RTLD_LOCAL instead of RTLD_GLOBAL.
     *
     * Note that if this flag is specified, RTTI (including dynamic_cast and throw) will
     * not work for types defined in the shared library with GCC and possibly other
     * compilers as well. See http://gcc.gnu.org/faq.html#dso for more information.
     *
     * This flag is ignored on platforms that do not use dlopen().
     */
    SHLIB_LOCAL = 2,
  };

  /**
   * Creates a SharedLibrary object.
   */
  SharedLibrary();

  /**
   * Creates a SharedLibrary object and loads a library
   * from the given path.
   */
  SharedLibrary(const String& path);

  /**
   * Creates a SharedLibrary object and loads a library
   * from the given path, using the given flags.
   * See the Flags enumeration for valid values.
   */
  SharedLibrary(const String& path, int32 flags);

  /**
   * Destroys the SharedLibrary. The actual library remains loaded.
   */
  virtual ~SharedLibrary();

  /**
   * Loads a shared library from the given path.
   * Throws a LibraryAlreadyLoadedException if
   * a library has already been loaded.
   *
   * Throws a LibraryLoadException if the library
   * cannot be loaded.
   */
  void Load(const String& path);

  /**
   * Loads a shared library from the given path,
   * using the given flags. See the Flags enumeration
   * for valid values.
   *
   * Throws a LibraryAlreadyLoadedException if
   * a library has already been loaded.
   * Throws a LibraryLoadException if the library
   * cannot be loaded.
   */
  void Load(const String& path, int32 flags);

  /**
   * Unloads a shared library.
   */
  void Unload();

  /**
   * Returns true if a library has been loaded.
   */
  bool IsLoaded() const;

  /**
   * Returns true if the loaded library contains
   * a symbol with the given name.
   */
  bool HasSymbol(const String& symbol);

  /**
   * Returns the address of the symbol with
   * the given name. For functions, this
   * is the entry point of the function.
   * Throws a NotFoundException if the symbol
   * does not exist.
   */
  void* GetSymbol(const String& symbol);

  /**
   * Returns the path of the library, as specified in a call to Load()
   * or the constructor.
   */
  const String& GetPath() const;

  /**
   * Returns the platform-specific filename prefix
   * for shared libraries.
   *
   * Most platforms would return "lib" as prefix, while
   * on Cygwin, the "cyg" prefix will be returned.
   */
  static String Prefix();

  /**
   * Returns the platform-specific filename suffix
   * for shared libraries (including the period).
   *
   * In debug mode, the suffix also includes a
   * "d" to specify the debug version of a library.
   */
  static String Suffix();

  /**
   * Returns the platform-specific filename
   * for shared libraries by prefixing and suffixing name
   * with Prefix() and Suffix()
   */
  static String GetOsName(const String& name);

  SharedLibrary(const SharedLibrary&) = delete;
  SharedLibrary& operator = (const SharedLibrary&) = delete;
};

} // namespace fun
