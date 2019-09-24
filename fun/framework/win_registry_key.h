#pragma once

#include "fun/base/container/array.h"
#include "fun/base/windows_less.h"
#include "fun/framework/framework.h"

namespace fun {
namespace framework {

/**
 * This class implements a convenient interface to the
 * Windows Registry.
 *
 * This class is only available on Windows platforms.
 */
class FUN_FRAMEWORK_API WinRegistryKey {
 public:
  typedef Array<String> Keys;
  typedef Array<String> Values;

  enum Type {
    REGT_NONE = 0,
    REGT_STRING = 1,
    REGT_STRING_EXPAND = 2,
    REGT_BINARY = 3,
    REGT_DWORD = 4,
    REGT_DWORD_BIG_ENDIAN = 5,
    REGT_LINK = 6,
    REGT_MULTI_STRING = 7,
    REGT_RESOURCE_LIST = 8,
    REGT_FULL_RESOURCE_DESCRIPTOR = 9,
    REGT_RESOURCE_REQUIREMENTS_LIST = 10,
    REGT_QWORD = 11
  };

  /**
   * Creates the WinRegistryKey.
   *
   * The key must start with one of the root key names
   * like HKEY_CLASSES_ROOT, e.g.
   * HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services.
   *
   * If read_only is true, then only read access to the registry
   * is available and any attempt to write to the registry will
   * result in an exception.
   *
   * extra_sam is used to pass extra flags (in addition to KEY_READ and
   * KEY_WRITE) to the samDesired argument of RegOpenKeyEx() or
   * RegCreateKeyEx().
   */
  WinRegistryKey(const String& key, bool read_only = false,
                 REGSAM extra_sam = 0);

  /**
   * Creates the WinRegistryKey.
   *
   * If read_only is true, then only read access to the registry
   * is available and any attempt to write to the registry will
   * result in an exception.
   *
   * extra_sam is used to pass extra flags (in addition to KEY_READ and
   * KEY_WRITE) to the samDesired argument of RegOpenKeyEx() or
   * RegCreateKeyEx().
   */
  WinRegistryKey(HKEY hroot_key, const String& sub_key, bool read_only = false,
                 REGSAM extra_sam = 0);

  /**
   * Destroys the WinRegistryKey.
   */
  ~WinRegistryKey();

  // Disable default constructor and copy.
  WinRegistryKey() = delete;
  WinRegistryKey(const WinRegistryKey&) = delete;
  WinRegistryKey& operator=(const WinRegistryKey&) = delete;

  /**
   * Sets the string value (REG_SZ) with the given name.
   * An empty name denotes the default value.
   */
  void SetString(const String& name, const String& value);

  /**
   * Returns the string value (REG_SZ) with the given name.
   * An empty name denotes the default value.
   *
   * Throws a NotFoundException if the value does not exist.
   */
  String GetString(const String& name);

  /**
   * Sets the expandable string value (REG_EXPAND_SZ) with the given name.
   * An empty name denotes the default value.
   */
  void SetStringExpand(const String& name, const String& value);

  /**
   * Returns the string value (REG_EXPAND_SZ) with the given name.
   * An empty name denotes the default value.
   * All references to environment variables (%VAR%) in the string
   * are expanded.
   *
   * Throws a NotFoundException if the value does not exist.
   */
  String GetStringExpand(const String& name);

  /**
   * Sets the string value (REG_BINARY) with the given name.
   * An empty name denotes the default value.
   */
  void SetBinary(const String& name, const Array<char>& value);

  /**
   * Returns the string value (REG_BINARY) with the given name.
   * An empty name denotes the default value.
   *
   * Throws a NotFoundException if the value does not exist.
   */
  Array<char> GetBinary(const String& name);

  /**
   * Sets the numeric (REG_DWORD) value with the given name.
   * An empty name denotes the default value.
   */
  void SetInt(const String& name, int32 value);

  /**
   * Returns the numeric value (REG_DWORD) with the given name.
   * An empty name denotes the default value.
   *
   * Throws a NotFoundException if the value does not exist.
   */
  int32 GetInt(const String& name);

  /**
   * Sets the numeric (REG_QWORD) value with the given name.
   * An empty name denotes the default value.
   */
  void SetInt64(const String& name, int64 value);

  /**
   * Returns the numeric value (REG_QWORD) with the given name.
   * An empty name denotes the default value.
   *
   * Throws a NotFoundException if the value does not exist.
   */
  int64 GetInt64(const String& name);

  /**
   * Deletes the value with the given name.
   *
   * Throws a NotFoundException if the value does not exist.
   */
  void DeleteValue(const String& name);

  /**
   * Recursively deletes the key and all subkeys.
   */
  void DeleteKey();

  /**
   * Returns true if the key exists.
   */
  bool Exists();

  /**
   * Returns the type of the key value.
   */
  Type GetType(const String& name);

  /**
   * Returns true if the given value exists under that key.
   */
  bool Exists(const String& name);

  /**
   * Appends all sub_key names to keys.
   */
  void GetSubKeys(Keys& keys);

  /**
   * Appends all value names to vals;
   */
  void GetValues(Values& vals);

  /**
   * Returns true if the key has been opened for read-only access only.
   */
  bool IsReadOnly() const;

 protected:
  void Open();
  void Close();
  String GetKey() const;
  String GetKey(const String& value_name) const;
  HKEY GetHandle();
  void HandleSetError(const String& name);
  static HKEY HandleFor(const String& root_key);

  HKEY hroot_key_;
  String sub_key_;
  HKEY hkey_;
  bool read_only_;
  REGSAM extra_sam_;
};

//
// inlines
//

FUN_ALWAYS_INLINE bool WinRegistryKey::IsReadOnly() const { return read_only_; }

}  // namespace framework
}  // namespace fun
