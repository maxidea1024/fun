#include "fun/framework/win_registry_key.h"

#include "fun/base/exception.h"

using fun::InvalidArgumentException;
using fun::NotFoundException;
using fun::SystemException;

namespace fun {
namespace framework {

namespace {

class AutoHandle {
 public:
  AutoHandle(HMODULE h) : handle_(h) {}
  ~AutoHandle() { FreeLibrary(handle_); }

  HMODULE GetHandle() { return handle_; }

 private:
  HMODULE handle_;
};

}  // namespace

WinRegistryKey::WinRegistryKey(const String& key, bool read_only,
                               REGSAM extra_sam)
    : hkey_(0), read_only_(read_only), extra_sam_(extra_sam) {
  int32 pos = key.IndexOf('\\');
  if (pos != INVALID_INDEX) {
    String root_key = key.Mid(0, pos);
    hroot_key_ = HandleFor(root_key);
    sub_key_ = key.Mid(pos + 1);
  } else {
    throw InvalidArgumentException("Not a valid registry key", key);
  }
}

WinRegistryKey::WinRegistryKey(HKEY hroot_key, const String& sub_key,
                               bool read_only, REGSAM extra_sam)
    : hroot_key_(hroot_key),
      sub_key_(sub_key),
      hkey_(0),
      read_only_(read_only),
      extra_sam_(extra_sam) {}

WinRegistryKey::~WinRegistryKey() { Close(); }

void WinRegistryKey::SetString(const String& name, const String& value) {
  Open();

  UString uname = UString::FromUtf8(name);
  UString uvalue = UString::FromUtf8(value);
  if (RegSetValueExW(
          hkey_, uname.c_str(), 0, REG_SZ, (CONST BYTE*)uvalue.c_str(),
          (DWORD)(uvalue.Len() + 1) * sizeof(wchar_t)) != ERROR_SUCCESS) {
    HandleSetError(name);
  }
}

String WinRegistryKey::GetString(const String& name) {
  Open();

  DWORD type;
  DWORD size;
  UString uname = UString::FromUtf8(name);
  if (RegQueryValueExW(hkey_, uname.c_str(), NULL, &type, NULL, &size) !=
          ERROR_SUCCESS ||
      (type != REG_SZ && type != REG_EXPAND_SZ && type != REG_LINK)) {
    throw NotFoundException(GetKey(name));
  }

  if (size > 0) {
    DWORD len = size / 2;
    Array<wchar_t> buffer(len + 1, NoInit);
    RegQueryValueExW(hkey_, uname.c_str(), NULL, NULL,
                     (BYTE*)buffer.MutableData(), &size);
    buffer[len] = 0;
    // TODO optimize
    UString uresult(buffer.ConstData());
    return uresult.ToUtf8();
  }

  return String();
}

void WinRegistryKey::SetStringExpand(const String& name, const String& value) {
  Open();

  UString uname = UString::FromUtf8(name);
  UString uvalue = UString::FromUtf8(value);
  if (RegSetValueExW(
          hkey_, uname.c_str(), 0, REG_EXPAND_SZ, (CONST BYTE*)uvalue.c_str(),
          (DWORD)(uvalue.Len() + 1) * sizeof(wchar_t)) != ERROR_SUCCESS) {
    HandleSetError(name);
  }
}

String WinRegistryKey::GetStringExpand(const String& name) {
  Open();

  DWORD type;
  DWORD size;
  UString uname = UString::FromUtf8(name);
  if (RegQueryValueExW(hkey_, uname.c_str(), NULL, &type, NULL, &size) !=
          ERROR_SUCCESS ||
      (type != REG_SZ && type != REG_EXPAND_SZ && type != REG_LINK)) {
    throw NotFoundException(GetKey(name));
  }

  if (size > 0) {
    DWORD len = size / 2;
    Array<wchar_t> buffer(len + 1, NoInit);
    RegQueryValueExW(hkey_, uname.c_str(), NULL, NULL,
                     (BYTE*)buffer.MutableData(), &size);
    buffer[len] = 0;
#if !defined(_WIN32_WCE)
    wchar_t temp;
    DWORD exp_size = ExpandEnvironmentStringsW(buffer.MutableData(), &temp, 1);
    Array<wchar_t> exp_buffer(exp_size);
    ExpandEnvironmentStringsW(buffer.ConstData(), exp_buffer.MutableData(),
                              exp_size);
    String result = WCHAR_TO_UTF8(exp_buffer.ConstData());
#else
    String result = WCHAR_TO_UTF8(buffer.ConstData());
#endif  // _WIN32_WCE
    return result;
  }

  return String();
}

void WinRegistryKey::SetBinary(const String& name, const Array<char>& value) {
  Open();

  UString uname = UString::FromUtf8(name);
  if (RegSetValueExW(hkey_, uname.c_str(), 0, REG_BINARY,
                     (CONST BYTE*)&value[0],
                     (DWORD)value.Count()) != ERROR_SUCCESS) {
    HandleSetError(name);
  }
}

Array<char> WinRegistryKey::GetBinary(const String& name) {
  Open();

  DWORD type;
  DWORD size;
  Array<char> result;
  UString uname = UString::FromUtf8(name);
  if (RegQueryValueExW(hkey_, uname.c_str(), NULL, &type, NULL, &size) !=
          ERROR_SUCCESS ||
      type != REG_BINARY) {
    throw NotFoundException(GetKey(name));
  }

  if (size > 0) {
    result.Resize(size);
    RegQueryValueExW(hkey_, uname.c_str(), NULL, NULL, (BYTE*)&result[0],
                     &size);
  }

  return result;
}

void WinRegistryKey::SetInt(const String& name, int32 value) {
  Open();

  DWORD data = value;
  UString uname = UString::FromUtf8(name);
  if (RegSetValueExW(hkey_, uname.c_str(), 0, REG_DWORD, (CONST BYTE*)&data,
                     sizeof(data)) != ERROR_SUCCESS) {
    HandleSetError(name);
  }
}

int32 WinRegistryKey::GetInt(const String& name) {
  Open();

  DWORD type;
  DWORD data;
  DWORD size = sizeof(data);
  UString uname = UString::FromUtf8(name);
  if (RegQueryValueExW(hkey_, uname.c_str(), NULL, &type, (BYTE*)&data,
                       &size) != ERROR_SUCCESS ||
      (type != REG_DWORD && type != REG_DWORD_BIG_ENDIAN)) {
    throw NotFoundException(GetKey(name));
  }

  return data;
}

void WinRegistryKey::SetInt64(const String& name, int64 value) {
  Open();

  UString uname = UString::FromUtf8(name);
  if (RegSetValueExW(hkey_, uname.c_str(), 0, REG_QWORD, (CONST BYTE*)&value,
                     sizeof(value)) != ERROR_SUCCESS) {
    HandleSetError(name);
  }
}

int64 WinRegistryKey::GetInt64(const String& name) {
  Open();

  DWORD type;
  int64 data;
  DWORD size = sizeof(data);
  UString uname = UString::FromUtf8(name);
  if (RegQueryValueExW(hkey_, uname.c_str(), NULL, &type, (BYTE*)&data,
                       &size) != ERROR_SUCCESS ||
      type != REG_QWORD) {
    throw NotFoundException(GetKey(name));
  }

  return data;
}

void WinRegistryKey::DeleteValue(const String& name) {
  Open();

  UString uname = UString::FromUtf8(name);
  if (RegDeleteValueW(hkey_, uname.c_str()) != ERROR_SUCCESS) {
    throw NotFoundException(GetKey(name));
  }
}

void WinRegistryKey::DeleteKey() {
  Keys keys;
  GetSubKeys(keys);
  Close();

  for (const auto& key : keys) {
    String sub_key(sub_key_);
    sub_key += "\\";
    sub_key += key;
    WinRegistryKey sub_reg_key(hroot_key_, sub_key);
    sub_reg_key.DeleteKey();
  }

  // NOTE: RegDeleteKeyEx is only available on Windows XP 64-bit SP3, Windows
  // Vista or later. We cannot call it directly as this would prevent the code
  // running on Windows XP 32-bit. Therefore, if we need to call RegDeleteKeyEx
  // (extra_sam_ != 0) we need to check for its existence in ADVAPI32.DLL and
  // call it indirectly.
  UString usub_key = UString::FromUtf8(sub_key_);

#if !defined(_WIN32_WCE)
  typedef LONG(WINAPI * RegDeleteKeyExWFunc)(HKEY hKey, const wchar_t* lpSubKey,
                                             REGSAM samDesired, DWORD Reserved);
  if (extra_sam_ != 0) {
    AutoHandle adv_api32(LoadLibraryW(L"ADVAPI32.DLL"));
    if (adv_api32.GetHandle()) {
      RegDeleteKeyExWFunc pRegDeleteKeyExW =
          reinterpret_cast<RegDeleteKeyExWFunc>(
              GetProcAddress(adv_api32.GetHandle(), "RegDeleteKeyExW"));
      if (pRegDeleteKeyExW) {
        if ((*pRegDeleteKeyExW)(hroot_key_, usub_key.c_str(), extra_sam_, 0) !=
            ERROR_SUCCESS) {
          throw NotFoundException(GetKey());
        }
        return;
      }
    }
  }
#endif

  if (RegDeleteKeyW(hroot_key_, usub_key.c_str()) != ERROR_SUCCESS) {
    throw NotFoundException(GetKey());
  }
}

bool WinRegistryKey::Exists() {
  HKEY hkey;
  UString usub_key = UString::FromUtf8(sub_key_);
  if (RegOpenKeyExW(hroot_key_, usub_key.c_str(), 0, KEY_READ | extra_sam_,
                    &hkey) == ERROR_SUCCESS) {
    RegCloseKey(hkey);
    return true;
  }

  return false;
}

WinRegistryKey::Type WinRegistryKey::GetType(const String& name) {
  Open();

  DWORD type = REG_NONE;
  DWORD size;
  UString uname = UString::FromUtf8(name);
  if (RegQueryValueExW(hkey_, uname.c_str(), NULL, &type, NULL, &size) !=
      ERROR_SUCCESS) {
    throw NotFoundException(GetKey(name));
  }

  return (Type)type;
}

bool WinRegistryKey::Exists(const String& name) {
  bool exists = false;
  HKEY hkey;
  UString usub_key = UString::FromUtf8(sub_key_);
  if (RegOpenKeyExW(hroot_key_, usub_key.c_str(), 0, KEY_READ | extra_sam_,
                    &hkey) == ERROR_SUCCESS) {
    UString uname = UString::FromUtf8(name);
    exists = RegQueryValueExW(hkey, uname.c_str(), NULL, NULL, NULL, NULL) ==
             ERROR_SUCCESS;
    RegCloseKey(hkey);
  }

  return exists;
}

void WinRegistryKey::Open() {
  if (!hkey_) {
    UString usub_key = UString::FromUtf8(sub_key_);
    if (read_only_) {
      if (RegOpenKeyExW(hroot_key_, usub_key.c_str(), 0, KEY_READ | extra_sam_,
                        &hkey_) != ERROR_SUCCESS) {
        throw NotFoundException("Cannot Open registry key: ", GetKey());
      }
    } else {
      if (RegCreateKeyExW(hroot_key_, usub_key.c_str(), 0, NULL,
                          REG_OPTION_NON_VOLATILE,
                          KEY_READ | KEY_WRITE | extra_sam_, NULL, &hkey_,
                          NULL) != ERROR_SUCCESS) {
        throw SystemException("Cannot Open registry key: ", GetKey());
      }
    }
  }
}

void WinRegistryKey::Close() {
  if (hkey_) {
    RegCloseKey(hkey_);
    hkey_ = 0;
  }
}

String WinRegistryKey::GetKey() const {
  String result;
  if (hroot_key_ == HKEY_CLASSES_ROOT) {
    result = "HKEY_CLASSES_ROOT";
  }
#if defined(HKEY_CURRENT_CONFIG)
  else if (hroot_key_ == HKEY_CURRENT_CONFIG) {
    result = "HKEY_CURRENT_CONFIG";
  }
#endif
  else if (hroot_key_ == HKEY_CURRENT_USER) {
    result = "HKEY_CURRENT_USER";
  } else if (hroot_key_ == HKEY_LOCAL_MACHINE) {
    result = "HKEY_LOCAL_MACHINE";
  } else if (hroot_key_ == HKEY_USERS) {
    result = "HKEY_USERS";
  }
#if defined(HKEY_PERFORMANCE_DATA)
  else if (hroot_key_ == HKEY_PERFORMANCE_DATA) {
    result = "HKEY_PERFORMANCE_DATA";
  }
#endif
  else {
    result = "(UNKNOWN)";
  }

  result += '\\';
  result += sub_key_;

  return result;
}

String WinRegistryKey::GetKey(const String& value_name) const {
  String result = GetKey();
  if (!value_name.IsEmpty()) {
    result += '\\';
    result += value_name;
  }

  return result;
}

HKEY WinRegistryKey::GetHandle() {
  Open();

  return hkey_;
}

HKEY WinRegistryKey::HandleFor(const String& root_key) {
  if (root_key == "HKEY_CLASSES_ROOT") {
    return HKEY_CLASSES_ROOT;
  }
#if defined(HKEY_CURRENT_CONFIG)
  else if (root_key == "HKEY_CURRENT_CONFIG") {
    return HKEY_CURRENT_CONFIG;
  }
#endif
  else if (root_key == "HKEY_CURRENT_USER") {
    return HKEY_CURRENT_USER;
  } else if (root_key == "HKEY_LOCAL_MACHINE") {
    return HKEY_LOCAL_MACHINE;
  } else if (root_key == "HKEY_USERS") {
    return HKEY_USERS;
  }
#if defined(HKEY_PERFORMANCE_DATA)
  else if (root_key == "HKEY_PERFORMANCE_DATA") {
    return HKEY_PERFORMANCE_DATA;
  }
#endif
  else {
    throw InvalidArgumentException("Not a valid root key", root_key);
  }
}

void WinRegistryKey::HandleSetError(const String& name) {
  String msg = "Failed to set registry value";
  throw SystemException(msg, GetKey(name));
}

void WinRegistryKey::GetSubKeys(WinRegistryKey::Keys& keys) {
  Open();

  DWORD sub_key_count = 0;
  DWORD value_count = 0;
  if (RegQueryInfoKey(hkey_, NULL, NULL, NULL, &sub_key_count, NULL, NULL,
                      &value_count, NULL, NULL, NULL, NULL) != ERROR_SUCCESS) {
    return;
  }

  wchar_t buf[256];
  DWORD buf_size = countof(buf);
  for (DWORD i = 0; i < sub_key_count; ++i) {
    if (RegEnumKeyExW(hkey_, i, buf, &buf_size, NULL, NULL, NULL, NULL) ==
        ERROR_SUCCESS) {
      keys.Add(WCHAR_TO_UTF8(buf));
    }
    buf_size = countof(buf);
  }
}

void WinRegistryKey::GetValues(WinRegistryKey::Values& vals) {
  Open();

  DWORD value_count = 0;
  if (RegQueryInfoKey(hkey_, NULL, NULL, NULL, NULL, NULL, NULL, &value_count,
                      NULL, NULL, NULL, NULL) != ERROR_SUCCESS) {
    return;
  }

  wchar_t buf[256];
  DWORD buf_size = countof(buf);
  for (DWORD i = 0; i < value_count; ++i) {
    if (RegEnumValueW(hkey_, i, buf, &buf_size, NULL, NULL, NULL, NULL) ==
        ERROR_SUCCESS) {
      vals.Add(WCHAR_TO_UTF8(buf));
    }
    buf_size = countof(buf);
  }
}

}  // namespace framework
}  // namespace fun
