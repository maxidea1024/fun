#include "fun/framework/win_registry_configuration.h"

#include "fun/base/exception.h"
#include "fun/framework/win_registry_key.h"

namespace fun {
namespace framework {

WinRegistryConfiguration::WinRegistryConfiguration(const String& root_path,
                                                   REGSAM extra_sam)
    : root_path_(root_path), extra_sam_(extra_sam) {
  // root_path must end with backslash
  if (!root_path_.IsEmpty()) {
    if (root_path_[root_path_.Len() - 1] != '\\') {
      root_path_ += '\\';
    }
  }
}

WinRegistryConfiguration::~WinRegistryConfiguration() {}

bool WinRegistryConfiguration::GetRaw(const String& key, String& value) const {
  String key_name;
  String full_path = root_path_ + ConvertToRegFormat(key, key_name);
  WinRegistryKey reg_key(full_path, true, extra_sam_);
  bool exists = reg_key.Exists(key_name);
  if (exists) {
    WinRegistryKey::Type type = reg_key.GetType(key_name);

    switch (type) {
      case WinRegistryKey::REGT_STRING:
        value = reg_key.GetString(key_name);
        break;
      case WinRegistryKey::REGT_STRING_EXPAND:
        value = reg_key.GetStringExpand(key_name);
        break;
      case WinRegistryKey::REGT_BINARY: {
        Array<char> tmp = reg_key.GetBinary(key_name);
        value = String(tmp.ConstData(), tmp.Count());
      } break;
      case WinRegistryKey::REGT_DWORD:
        value = String::FromNumber(reg_key.GetInt(key_name));
        break;
      case WinRegistryKey::REGT_QWORD:
        value = String::FromNumber(reg_key.GetInt64(key_name));
        break;
      default:
        exists = false;
    }
  }
  return exists;
}

void WinRegistryConfiguration::SetRaw(const String& key, const String& value) {
  String key_name;
  String full_path = root_path_ + ConvertToRegFormat(key, key_name);
  WinRegistryKey reg_key(full_path, false, extra_sam_);
  reg_key.SetString(key_name, value);
}

void WinRegistryConfiguration::Enumerate(const String& key, Keys& range) const {
  String key_name;
  String full_path = root_path_ + ConvertToRegFormat(key, key_name);
  if (full_path.IsEmpty()) {
    // return all root level keys
#if defined(_WIN32_WCE)
    range.Add("HKEY_CLASSES_ROOT");
    range.Add("HKEY_CURRENT_USER");
    range.Add("HKEY_LOCAL_MACHINE");
    range.Add("HKEY_USERS");
#else
    range.Add("HKEY_CLASSES_ROOT");
    range.Add("HKEY_CURRENT_CONFIG");
    range.Add("HKEY_CURRENT_USER");
    range.Add("HKEY_LOCAL_MACHINE");
    range.Add("HKEY_PERFORMANCE_DATA");
    range.Add("HKEY_USERS");
#endif
  } else {
    full_path += '\\';
    full_path += key_name;
    WinRegistryKey reg_key(full_path, true, extra_sam_);
    reg_key.GetValues(range);
    reg_key.GetSubKeys(range);
  }
}

void WinRegistryConfiguration::RemoveRaw(const String& /*key*/) {
  throw fun::NotImplementedException(
      "Removing a key in a WinRegistryConfiguration");
}

String WinRegistryConfiguration::ConvertToRegFormat(const String& key,
                                                    String& value) const {
  int32 pos = key.LastIndexOf('.');
  if (pos == INVALID_INDEX) {
    value = key;
    return String();
  }

  String prefix(key.Mid(0, pos));
  value = key.Mid(pos + 1);
  prefix.Replace(".", "\\");
  return prefix;
}

}  // namespace framework
}  // namespace fun
