#include "fun/framework/ini_file_configuration.h"

#if !defined(FUN_NO_INI_CONFIGURATION)

#include <set>

#include "fun/base/exception.h"
#include "fun/base/file_stream.h"
#include "fun/base/path.h"
#include "fun/base/str.h"

using fun::icompare;
using fun::Path;
using fun::Trim;

namespace fun {
namespace framework {

IniFileConfiguration::IniFileConfiguration() {}

IniFileConfiguration::IniFileConfiguration(std::istream& istr) { Load(istr); }

IniFileConfiguration::IniFileConfiguration(const String& path) { Load(path); }

IniFileConfiguration::~IniFileConfiguration() {}

void IniFileConfiguration::Load(std::istream& istr) {
  map_.clear();
  section_key_.clear();
  while (!istr.eof()) {
    ParseLine(istr);
  }
}

void IniFileConfiguration::Load(const String& path) {
  fun::FileInputStream istr(path);
  if (istr.good()) {
    Load(istr);
  } else {
    throw fun::OpenFileException(path);
  }
}

bool IniFileConfiguration::GetRaw(const String& key, String& value) const {
  IStringMap::const_iterator it = map_.Find(key);
  if (it != map_.end()) {
    value = it->second;
    return true;
  } else {
    return false;
  }
}

void IniFileConfiguration::SetRaw(const String& key, const String& value) {
  map_[key] = value;
}

void IniFileConfiguration::Enumerate(const String& key, Keys& range) const {
  std::set<String> key_set;
  String prefix = key;
  if (!prefix.IsEmpty()) {
    prefix += '.';
  }

  String::size_type psize = prefix.size();
  for (IStringMap::const_iterator it = map_.begin(); it != map_.end(); ++it) {
    if (icompare(it->first, psize, prefix) == 0) {
      String sub_key;
      String::size_type end = it->first.Find('.', psize);
      if (end == String::npos) {
        sub_key = it->first.Mid(psize);
      } else {
        sub_key = it->first.Mid(psize, end - psize);
      }

      if (key_set.Find(sub_key) == key_set.end()) {
        range.push_back(sub_key);
        key_set.Insert(sub_key);
      }
    }
  }
}

void IniFileConfiguration::RemoveRaw(const String& key) {
  String prefix = key;
  if (!prefix.IsEmpty()) {
    prefix += '.';
  }

  String::size_type psize = prefix.size();
  IStringMap::iterator it = map_.begin();
  IStringMap::iterator itCur;
  while (it != map_.end()) {
    itCur = it++;
    if ((icompare(itCur->first, key) == 0) ||
        (icompare(itCur->first, psize, prefix) == 0)) {
      map_.erase(itCur);
    }
  }
}

bool IniFileConfiguration::ICompare::operator()(const String& s1,
                                                const String& s2) const {
  return icompare(s1, s2) < 0;
}

void IniFileConfiguration::ParseLine(std::istream& istr) {
  static const int32 eof = std::char_traits<char>::eof();

  int32 c = istr.get();
  while (c != eof && fun::Ascii::IsSpace(c)) {
    c = istr.get();
  }
  if (c != eof) {
    if (c == ';') {
      while (c != eof && c != '\n') {
        c = istr.get();
      }
    } else if (c == '[') {
      String key;
      c = istr.get();
      while (c != eof && c != ']' && c != '\n') {
        key += (char)c;
        c = istr.get();
      }
      section_key_ = Trim(key);
    } else {
      String key;
      while (c != eof && c != '=' && c != '\n') {
        key += (char)c;
        c = istr.get();
      }
      String value;
      if (c == '=') {
        c = istr.get();
        while (c != eof && c != '\n') {
          value += (char)c;
          c = istr.get();
        }
      }
      String full_key = section_key_;
      if (!full_key.IsEmpty()) {
        full_key += '.';
      }
      full_key.Append(Trim(key));
      map_[full_key] = Trim(value);
    }
  }
}

}  // namespace framework
}  // namespace fun

#endif  // !defined(FUN_NO_INI_CONFIGURATION)
