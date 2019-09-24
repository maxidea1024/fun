#include "fun/framework/configuration_base.h"
#include "fun/framework/configuration_view.h"
#include "fun/base/exception.h"
#include "fun/base/str.h"

//TODO
//#include "fun/base/number_parser.h"
//#include "fun/base/number_formatter.h"

using fun::Mutex;
using fun::NotFoundException;
using fun::SyntaxException;
using fun::CircularReferenceException;
//using fun::NumberParser;
//using fun::NumberFormatter;
using fun::icompare;

namespace fun {
namespace framework {

ConfigurationBase::ConfigurationBase()
  : depth_(0), events_enabled_(true) {}

ConfigurationBase::~ConfigurationBase() {}

bool ConfigurationBase::HasProperty(const String& key) const {
  Mutex::ScopedLock guard(mutex_);

  String value;
  return GetRaw(key, value);
}

bool ConfigurationBase::HasOption(const String& key) const {
  return HasProperty(key);
}

bool ConfigurationBase::Has(const String& key) const {
  return HasProperty(key);
}

String ConfigurationBase::GetString(const String& key) const {
  Mutex::ScopedLock guard(mutex_);

  String value;
  if (GetRaw(key, value)) {
    return ExpandInternal(value);
  } else {
    throw NotFoundException(key);
  }
}

String ConfigurationBase::GetString(const String& key,
                                    const String& default_value) const {
  Mutex::ScopedLock guard(mutex_);

  String value;
  if (GetRaw(key, value)) {
    return ExpandInternal(value);
  } else {
    return default_value;
  }
}

String ConfigurationBase::GetRawString(const String& key) const {
  Mutex::ScopedLock guard(mutex_);

  String value;
  if (GetRaw(key, value)) {
    return value;
  } else {
    throw NotFoundException(key);
  }
}

String ConfigurationBase::GetRawString( const String& key,
                                        const String& default_value) const {
  Mutex::ScopedLock guard(mutex_);

  String value;
  if (GetRaw(key, value)) {
    return value;
  } else {
    return default_value;
  }
}

int32 ConfigurationBase::GetInt(const String& key) const {
  Mutex::ScopedLock guard(mutex_);

  String value;
  if (GetRaw(key, value)) {
    return ParseInt(ExpandInternal(value));
  } else {
    throw NotFoundException(key);
  }
}

int32 ConfigurationBase::GetInt(const String& key, int32 default_value) const {
  Mutex::ScopedLock guard(mutex_);

  String value;
  if (GetRaw(key, value)) {
    return ParseInt(ExpandInternal(value));
  } else {
    return default_value;
  }
}

uint32 ConfigurationBase::GetUInt(const String& key) const {
  Mutex::ScopedLock guard(mutex_);

  String value;
  if (GetRaw(key, value)) {
    return ParseUInt(ExpandInternal(value));
  } else {
    throw NotFoundException(key);
  }
}

uint32 ConfigurationBase::GetUInt(const String& key, uint32 default_value) const {
  Mutex::ScopedLock guard(mutex_);

  String value;
  if (GetRaw(key, value)) {
    return ParseUInt(ExpandInternal(value));
  } else {
    return default_value;
  }
}

int64 ConfigurationBase::GetInt64(const String& key) const {
  Mutex::ScopedLock guard(mutex_);

  String value;
  if (GetRaw(key, value)) {
    return ParseInt64(ExpandInternal(value));
  } else {
    throw NotFoundException(key);
  }
}

int64 ConfigurationBase::GetInt64(const String& key, int64 default_value) const {
  Mutex::ScopedLock guard(mutex_);

  String value;
  if (GetRaw(key, value)) {
    return ParseInt64(ExpandInternal(value));
  } else {
    return default_value;
  }
}

uint64 ConfigurationBase::GetUInt64(const String& key) const {
  Mutex::ScopedLock guard(mutex_);

  String value;
  if (GetRaw(key, value)) {
    return ParseUInt64(ExpandInternal(value));
  } else {
    throw NotFoundException(key);
  }
}

uint64 ConfigurationBase::GetUInt64(const String& key,
                                    uint64 default_value) const {
  Mutex::ScopedLock guard(mutex_);

  String value;
  if (GetRaw(key, value)) {
    return ParseUInt64(ExpandInternal(value));
  } else {
    return default_value;
  }
}

float ConfigurationBase::GetFloat(const String& key) const {
  //Mutex::ScopedLock guard(mutex_);

  //String value;
  //if (GetRaw(key, value)) {
  //  return NumberParser::ParseFloat(ExpandInternal(value));
  //} else {
  //  throw NotFoundException(key);
  //}

  //TODO
  return 0.0f;
}

float ConfigurationBase::GetFloat(const String& key, float default_value) const {
  //Mutex::ScopedLock guard(mutex_);

  //String value;
  //if (GetRaw(key, value)) {
  //  return NumberParser::ParseFloat(ExpandInternal(value));
  //} else {
  //  return default_value;
  //}

  //TODO
  return 0.0f;
}

double ConfigurationBase::GetDouble(const String& key) const {
  //Mutex::ScopedLock guard(mutex_);

  //String value;
  //if (GetRaw(key, value)) {
  //  return NumberParser::ParseFloat(ExpandInternal(value));
  //} else {
  //  throw NotFoundException(key);
  //}

  //TODO
  return 0.0;
}

double ConfigurationBase::GetDouble(const String& key, double default_value) const {
  //Mutex::ScopedLock guard(mutex_);

  //String value;
  //if (GetRaw(key, value)) {
  //  return NumberParser::ParseFloat(ExpandInternal(value));
  //} else {
  //  return default_value;
  //}

  //TODO
  return 0.0;
}

bool ConfigurationBase::GetBool(const String& key) const {
  Mutex::ScopedLock guard(mutex_);

  String value;
  if (GetRaw(key, value)) {
    return ParseBool(ExpandInternal(value));
  } else {
    throw NotFoundException(key);
  }
}

bool ConfigurationBase::GetBool(const String& key, bool default_value) const {
  Mutex::ScopedLock guard(mutex_);

  String value;
  if (GetRaw(key, value)) {
    return ParseBool(ExpandInternal(value));
  } else {
    return default_value;
  }
}

void ConfigurationBase::SetString(const String& key, const String& value) {
  SetRawWithEvent(key, value);
}

void ConfigurationBase::SetInt(const String& key, int32 value) {
  SetRawWithEvent(key, String::FromNumber(value));
}

void ConfigurationBase::SetUInt(const String& key, uint32 value) {
  SetRawWithEvent(key, String::FromNumber(value));
}

void ConfigurationBase::SetInt64(const String& key, int64 value) {
  Mutex::ScopedLock guard(mutex_);

  SetRawWithEvent(key, String::FromNumber(value));
}

void ConfigurationBase::SetUInt64(const String& key, uint64 value) {
  Mutex::ScopedLock guard(mutex_);

  SetRawWithEvent(key, String::FromNumber(value));
}

void ConfigurationBase::SetFloat(const String& key, float value) {
  SetRawWithEvent(key, String::FromNumber(value));
}

void ConfigurationBase::SetDouble(const String& key, double value) {
  SetRawWithEvent(key, String::FromNumber(value));
}

void ConfigurationBase::SetBool(const String& key, bool value) {
  SetRawWithEvent(key, value ? "true" : "false");
}

void ConfigurationBase::GetKeys(Keys& range) const {
  Mutex::ScopedLock guard(mutex_);

  String key;
  range.Clear();
  Enumerate(key, range);
}

void ConfigurationBase::GetKeys(const String& key, Keys& range) const {
  Mutex::ScopedLock guard(mutex_);

  range.Clear();
  Enumerate(key, range);
}

const ConfigurationBase::Ptr ConfigurationBase::CreateView(const String& prefix) const {
  return new ConfigurationView(prefix, ConfigurationBase::Ptr(const_cast<ConfigurationBase*>(this), true));
}

ConfigurationBase::Ptr ConfigurationBase::CreateView(const String& prefix) {
  return new ConfigurationView(prefix, ConfigurationBase::Ptr(this, true));
}

namespace {

class AutoCounter {
 public:
  AutoCounter(int32& count) : count_(count) { ++count_; }
  ~AutoCounter() { --count_; }

 private:
  int32& count_;
};

} // namespace

String ConfigurationBase::Expand(const String& value) const {
  Mutex::ScopedLock guard(mutex_);
  return ExpandInternal(value);
}

void ConfigurationBase::Remove(const String& key) {
  if (events_enabled_) {
    property_removing(this, key);
  }

  {
    Mutex::ScopedLock guard(mutex_);
    RemoveRaw(key);
  }

  if (events_enabled_) {
    property_removed(this, key);
  }
}

void ConfigurationBase::SetEventsEnabled(bool enable) {
  events_enabled_ = enable;
}

bool ConfigurationBase::GetEventsEnabled() const {
  return events_enabled_;
}

void ConfigurationBase::RemoveRaw(const String& /*key*/) {
  throw fun::NotImplementedException("RemoveRaw()");
}

String ConfigurationBase::ExpandInternal(const String& value) const {
  AutoCounter counter(depth_);
  if (depth_ > 10) {
    throw CircularReferenceException("Too many property references encountered");
  }
  return UncheckedExpand(value);
}

String ConfigurationBase::UncheckedExpand(const String& value) const {
  String result;
  String::const_iterator it  = value.begin();
  String::const_iterator end = value.end();
  while (it != end) {
    if (*it == '$') {
      ++it;
      if (it != end && *it == '{') {
        ++it;
        String prop;
        while (it != end && *it != '}') {
          prop += *it++;
        }
        if (it != end) {
          ++it;
        }
        String raw_value;
        if (GetRaw(prop, raw_value)) {
          result.Append(ExpandInternal(raw_value));
        } else {
          result.Append("${");
          result.Append(prop);
          result.Append("}");
        }
      } else {
        result += '$';
      }
    } else {
      result += *it++;
    }
  }
  return result;
}

int32 ConfigurationBase::ParseInt(const String& value) {
  //if ((value.compare(0, 2, "0x") == 0) || (value.compare(0, 2, "0X") == 0)) {
  //  return static_cast<int32>(NumberParser::ParseHex(value));
  //} else {
  //  return NumberParser::Parse(value);
  //}
  
  //TODO
  return 0;
}

uint32 ConfigurationBase::ParseUInt(const String& value) {
  //if ((value.compare(0, 2, "0x") == 0) || (value.compare(0, 2, "0X") == 0)) {
  //  return NumberParser::ParseHex(value);
  //} else {
  //  return NumberParser::ParseUnsigned(value);
  //}
  
  //TODO
  return 0;
}

int64 ConfigurationBase::ParseInt64(const String& value) {
  //if ((value.compare(0, 2, "0x") == 0) || (value.compare(0, 2, "0X") == 0)) {
  //  return static_cast<int64>(NumberParser::ParseHex64(value));
  //} else {
  //  return NumberParser::Parse64(value);
  //}
  
  //TODO
  return 0;
}

uint64 ConfigurationBase::ParseUInt64(const String& value) {
  //if ((value.compare(0, 2, "0x") == 0) || (value.compare(0, 2, "0X") == 0)) {
  //  return NumberParser::ParseHex64(value);
  //} else {
  //  return NumberParser::ParseUnsigned64(value);
  //}
  
  //TODO
  return 0;
}

bool ConfigurationBase::ParseBool(const String& value) {
  int32 n;
  bool ok;
  n = value.ToInt32(&ok);
  if (ok) {
    return n != 0;
  } else if (icompare(value, "true") == 0) {
    return true;
  } else if (icompare(value, "yes") == 0) {
    return true;
  } else if (icompare(value, "on") == 0) {
    return true;
  } else if (icompare(value, "false") == 0) {
    return false;
  } else if (icompare(value, "no") == 0) {
    return false;
  } else if (icompare(value, "off") == 0) {
    return false;
  } else {
    throw SyntaxException("Cannot convert to boolean", value);
  }
}

void ConfigurationBase::SetRawWithEvent(const String& key, const String& value) {
  KeyValue kv(key, value);

  if (events_enabled_) {
    property_changing(this, kv);
  }

  {
    Mutex::ScopedLock guard(mutex_);
    SetRaw(key, value);
  }

  if (events_enabled_) {
    property_changed(this, kv);
  }
}

} // namespace framework
} // namespace fun
