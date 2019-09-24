#include "fun/framework/map_configuration.h"

namespace fun {
namespace framework {

// FUN_IMPLEMENT_RTCLASS(MapConfiguration)

MapConfiguration::MapConfiguration() {}

void MapConfiguration::CopyTo(ConfigurationBase& config) {
  for (const auto& pair : map_) {
    config.SetString(pair.key, pair.value);
  }
}

void MapConfiguration::Clear() { map_.Clear(); }

bool MapConfiguration::GetRaw(const String& key, String& out_value) const {
  return map_.TryGetValue(key, out_value);
}

void MapConfiguration::SetRaw(const String& key, const String& value) {
  map_.Add(key, value);
}

void MapConfiguration::Enumerate(const String& key,
                                 Array<String>& out_keys) const {
  Set<String> key_set;
  String prefix = key;
  if (!prefix.IsEmpty()) {
    prefix += ".";
  }

  int32 prefix_len = prefix.Len();
  for (const auto& pair : map_) {
    if (pair.key.StartsWith(prefix)) {
      String sub_key;
      int32 pos =
          pair.key.IndexOf('.', CaseSensitivity::CaseSensitive, prefix_len);
      if (pos == INVALID_INDEX) {
        sub_key = pair.key.Mid(prefix_len);
      } else {
        sub_key = pair.key.Mid(prefix_len, pos - prefix_len);
      }

      if (!key_set.Contains(sub_key)) {
        out_keys.Add(sub_key);
        key_set.Add(sub_key);
      }
    }
  }
}

void MapConfiguration::RemoveRaw(const String& key) {
  String prefix = key;
  if (!prefix.IsEmpty()) {
    prefix += '.';
  }
  int32 prefix_len = prefix.Len();

  decltype(map_)::Iterator it = map_.CreateIterator();
  while (it) {
    if (it->key == key || it->key.StartsWith(prefix.c_str(), prefix_len)) {
      it.RemoveCurrent();
    }
  }
}

}  // namespace framework
}  // namespace fun
