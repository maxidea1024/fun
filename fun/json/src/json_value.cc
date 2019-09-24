//TODO HashOf
//TODO Serialization

#include "fun/json/value.h"
#include "fun/json/reader.h"
#include "fun/json/writer.h"
#include "fun/json/file_helper.h"

namespace fun {
namespace json {

JValue JValue::Null(nullptr);
JValue JValue::EmptyString(ValueType::String);
JValue JValue::EmptyArray(ValueType::Array);
JValue JValue::EmptyObject(ValueType::Object);
JValue JValue::True(true);
JValue JValue::False(false);
JValue JValue::NaN(std::numeric_limits<double>::quiet_NaN());
JValue JValue::PosInf(std::numeric_limits<double>::infinity());
JValue JValue::NegInf(-std::numeric_limits<double>::infinity());

bool JValue::FromString(const String& str, String* out_error) {
  return FromString(*str, *str + str.Len(), out_error);
}

bool JValue::FromString(const char* begin, const char* end, String* out_error) {
  SetNull();

  Reader reader;
  if (!reader.Parse(begin, end, *this, true)) {
    if (out_error) {
      *out_error = reader.GetFormattedErrorMessages();
    }
    return false;
  }
  return true;
}

bool JValue::LoadFromFile(const String& filename, String* out_error) {
  String text;
  if (FileHelper::ReadAllText(text, *filename)) {
    if (out_error) {
      *out_error = String::Format("Could not open text file '{}'.", *filename);
    }
    return false;
  }

  return FromString(text, out_error);
}

bool JValue::SaveToFile(const String& filename, bool pretty) {
  String text = ToString(pretty);
  return FileHelper::WriteAllText(text, *filename);
}

String JValue::ToString(bool pretty) const {
  if (pretty) {
    PrettyWriter writer;
    return writer.Write(*this);
  } else {
    CondensedWriter writer;
    return writer.Write(*this);
  }
}

} // namespace json
} // namespace fun







static inline char* DuplicateStringValue(const char* value, size_t length) {
  if (length >= static_cast<size_t>(JValue::MaxInt)) {
    length = JValue::MaxInt - 1;
  }

  char* new_string = static_cast<char*>(UnsafeMemory::Malloc(length + 1));
  if (new_string == nullptr) {
    ThrowRuntimeError("in JValue::DuplicateStringValue(): "
                      "Failed to allocate string value buffer");
  }

  UnsafeMemory::Memcpy(new_string, value, length);
  new_string[length] = '\0';
  return new_string;
}

static inline char* DuplicateAndPrefixStringValue(const char* value, uint32 length) {
  //TODO
}




JValue::CZString(int32 index) : cstr_(nullptr), index_(index) {}

JValue::CZString::CZString( const char* str,
                            uint32 length,
                            DuplicationPolicy allocation_policy)
    : cstr_(str) {
  storage_.policy_ = allocation_policy & 3;
  storage_.length_ = length & 0x3FFFFFFF;
}

//TODO copy constructor.

JValue::CZString::~CZString() {
  if (cstr_ && storage_.policy_ == DUPLICATE) {
    ReleaseStringValue( const_cast<char*>(cstr_),
                        storage_.length_ + 1);
  }
}

void JValue::CZString::Swap(CZString& other) {
  fun::Swap(cstr_, other.cstr_);
  fun::Swap(index_, other.index_);
}

JValue::CZString& JValue::CZString::operator = (const CZString& other) {
  cstr_ = other.cstr_;
  index_ = other.index_;
  return *this;
}

JValue::CZString& JValue::CZString::operator = (CZString&& other) {
  cstr_ = other.cstr_;
  index_ = other.index_;
  other.cstr_ = nullptr;
  return *this;
}



int32 JValue::CZString::Index() const { return index_; }

const char* JValue::CZString::Data() const { return cstr_; }

uint32 JValue::CZString::Length() const { return storage_.length_; }

bool JValue::CZString::IsStaticString() const {
  return storage_.policy_ == NO_DUPLICATION;
}



bool JValue::TryRemoveField(const char* begin,
                            const char* end,
                            JValue* out_removed) {
  if (GetType() != Type::ObjectValue) {
    return false;
  }

  CZString actual_key(begin, end, CZString::NO_DUPLICATION);
  auto it = value_.map_.find(actual_key);
  if (it == value_.map_.end()) {
    return false;
  }

  if (out_removed) {
    *out_removed = MoveRValue(it->second);
  }

  value_.map_.erase(it);
  return true;
}

bool JValue::TryRemoveField(const char* key, JValue* out_removed) {
  return TryRemoveField(key, key + CharTraitsA::Strlen(key), out_removed);
}

bool JValue::TryRemoveField(const String& key, JValue* out_removed) {
  return TryRemoveField(key.cbegin(), key.cend(), out_removed);
}

void JValue::RemoveField(const char* key) {
  if (GetType() == Type::Null) {
    return;
  }

  CZString actual_key(key, key + CharTraitsA::Strlen(key), CZString::NO_DUPLICATION);
  value_.map_->erase(actual_key);
}

void JValue::RemoveField(const String& key) {
  //TODO optimize (remove Strlen)
  RemoveField(key.c_str());
}


bool JValue::RemoveIndex(int32 index, JValue* out_removed) {
  if (!IsArray()) {
    return false;
  }

  // object/array 모두를 std::map으로 퉁쳐서 처리하는데
  // 배열 같은 경우 순서가 중요할텐데, std::map은 ordered_map이기 때문에
  // 문제가 되지 않는다.
  CZString key(index);
  auto it = value_.map_->find(key);
  if (it == value_.map_.end()) {
    return false;
  }

  if (out_removed) {
    *out_removed = MoveRValue(it->second);
  }

  int32 old_count = Count();
  for (int32 i = index; i < (old_count - 1); ++i) {
    CZString index_key(i);
    (*value_.map_)[index_key] = (*this)[i + 1];
  }

  // Erase the last one ("leftover")
  CZString key_last(old_count - 1);
  auto it_last = value_.map_.find(key_last);
  value_.map_->erase(it_last);
  return true;
}




static bool IsIntegral(double d) {
  double integral_part;
  return modf(d, &integral_part) == 0.0;
}

bool JValue::IsNull() const {
  return type_ == Type::Null;
}

bool JValue::IsBool() const {
  return type_ == Type::Bool;
}

bool JValue::IsInt32() const {
  switch (type_) {
    case Type::Int:
      return value_.int_ >= int32_MIN && value_.int_ <= int32_MAX;

    case Type::UInt:
      return value_.uint_ <= uint32(int32_MAX);

    case Type::Real:
      return  value_.real_ >= int32_MIN && value_.real_ <= int32_MAX &&
              IsIntegral(value_.real_);

    default:
      return false;
  }
}

bool JValue::IsUInt32() const {
  switch (type_) {
    case Type::Int:
      return value_.int_ >= 0 && LargestUInt(value_.int_) <= LargestUInt(uint32_MAX);

    case Type::UInt:
      return value_.uint_ <= uint32_MAX;

    case Type::Real:
      return  value_.real_ >= 0 && value_.real_ <= uint32_MAX &&
              IsIntegral(value_.real_);

    default:
      return false;
  }
}

bool JValue::IsInt64() const {
  switch (type_) {
    case Type::Int:
      return true;

    case Type::UInt:
      return value_.uint_ <= uint64(MAX_INT64);

    case Type::Real:
      return  value_.real_ >= double(int64_MIN) &&
              value_.real_ < double(int64_MAX) && IsIntegral(value_.real_);

    default:
      return false;
  }
}

bool JValue::IsUInt64() const {
  switch (type_) {
    case Type::Int:
      return value_.int_ >= 0;

    case Type::UInt:
      return true;

    case Type::Real:
      return  value_.real_ >= 0 && value_.real_ < MAX_UINT64_AS_DOUBLE &&
              IsIntegral(value_.real_);

    default:
      return false;
  }
}

bool JValue::IsIntegral() const {
  switch (GetType()) {
    case Type::Int:
    case Type::UInt:
      return true;

    case Type::Real:
      return  value_.real_ >= double(MIN_INT64) &&
              value_.real_ < MAX_UINT64_AS_DOUBLE && IsIntegral(value_.real_);

    default:
      return false;
  }
}

bool JValue::IsDouble() const {
  return GetType() == Type::Int || GetType() == Type::UInt || GetType() == Type::Real;
}

bool JValue::IsNumeric() const {
  return IsDouble();
}

bool JValue::IsString() const {
  return GetType() == Type::String;
}

bool JValue::IsArray() const {
  return GetType() == Type::Array;
}

bool JValue::IsObject() const {
  return GetType() == Type::Object;
}







JValue::ConstIterator JValue::begin() const {
  switch (type_) {
    case Type::Array:
    case Type::Object:
      if (value_.map_) {
        return ConstIterator(value_.map_->begin());
      }
      break;

    default:
      return {};
  }
}

JValue::ConstIterator JValue::end() const {
  switch (type_) {
    case Type::Array:
    case Type::Object:
      if (value_.map_) {
        return ConstIterator(value_.map_->end());
      }
      break;

    default:
      return {};
  }
}


JValue::Iterator JValue::begin() {
  switch (type_) {
    case Type::Array:
    case Type::Object:
      if (value_.map_) {
        return Iterator(value_.map_->begin());
      }
      break;

    default:
      return {};
  }
}

JValue::Iterator JValue::end() {
  switch (type_) {
    case Type::Array:
    case Type::Object:
      if (value_.map_) {
        return Iterator(value_.map_->end());
      }
      break;

    default:
      return {};
  }
}






int32 JValue::Compare(const JValue& other) const {
  if (*this < other) {
    return -1;
  }
  if (*this > other) {
    return +1;
  }
  return 0;
}

bool JValue::operator < (const JValue& other) const {
  int32 type_delta = GetType() - other.GetType();
  if (type_delta != 0) {
    return type_delta < 0;
  }

  switch (GetType()) {
    case Type::Null:
      return false;

    case Type::Int:
      return value_.int_ < other.value_.int_;
    case Type::UInt:
      return value_.uint_ < other.value_.uint_;
    case Type::Real:
      return value_.real_ < other.value_.real_;
    case Type::Bool:
      return value_.bool_ < other.value_.bool_;
    case Type::String:
      //...

    case Type::Array:
    case Type::Object: {
      int32 delta = int32(value_.map_.size() - other.value_.map_.size());
      if (delta != 0) {
        return delta < 0;
      }

      return (*value_.map_) < (*other.value_.map);
    }

    default:
      fun_unexpected();
      return false;
  }
}

bool JValue::operator <= (const JValue& other) const {
  return !(other < *this);
}

bool JValue::operator >= (const JValue& other) const {
  return !(*this < other);
}

bool JValue::operator > (const JValue& other) const {
  return other < *this;
}

bool JValue::operator == (const JValue& other) const {
  if (GetType() != other.GetType()) {
    return false;
  }

  //TODO
}

bool JValue::operator != (const JValue& other) const {
  return !(*this == other);
}


const char* JValue::AsCString() const {
  fun_check_msg(IsString(), "in JValue::AsCString(): requires string");

  if (value_.string_ == null) {
    return nullptr;
  }

  uint32 this_len;
  const char* this_str;
  DecodePrefixedString( this->IsAllocated(), this->value_.string_, &this_len,
                        &this_str);
  return this_str;
}

#if FUN_JSON_USING_SECURE_MEMORY
uint32 JValue::GetCStringLength() const {
  fun_check_msg(IsString(), "in JValue::GetCStringLength(): requires string");

  if (value_.string_ == null) {
    return nullptr;
  }

  uint32 this_len;
  const char* this_str;
  DecodePrefixedString( this->IsAllocated(), this->value_.string_, &this_len,
                        &this_str);
  return this_len;
}
#endif

bool JValue::GetString(const char** begin, const char** end) const {
  if (GetType() != Type::String) {
    return false;
  }

  if (value_.string_ == nullptr) {
    return false;
  }

  uint32 this_len;
  DecodePrefixedString( this->IsAllocated(), this->value_.string_, &this_len,
                        begin);
  *end = *begin + this_len;
  return true;
}

String JValue::AsString() const {
  switch (type_) {
    case Type::Null:
      return "";

    case Type::String:
      if (!value_.string_) {
        return "";
      } else {
        uint32 this_len;
        const char* this_str;
        DecodePrefixedString( this->IsAllocated(), this->value_.string_, &this_len,
                              &this_str);
        return String(this_str, this_len);
      }

    case Type::Bool:
      return value_.bool_ ? "true" : "false";

    case Type::Int:
      return ValueToString(value_.int_);

    case Type::UInt:
      return ValueToString(value_.uint_);

    case Type::Real:
      return ValueToString(value_.real_);

    default:
      JSON_FAIL_MESSAGE("Type is not convertible to string");
  }
}

int32 JValue::AsInt() const {
}

uint32 JValue::AsUInt() const {
}

int64 JValue::AsInt64() const {
}

uint64 JValue::AsUInt64() const {
}

//TODO LargestInt / LargestUInt ?

double JValue::AsDouble() const {
}

float JValue::AsFloat() const {
}

bool JValue::AsBool() const {
}

bool JValue::IsConvertibleTo(ValueType type_to) const {
}

int32 JValue::Count() const {
  switch (type_) {
    case Type::Null:
    case Type::Int:
    case Type::UInt:
    case Type::Int64:
    case Type::UInt64:
    case Type::String:
      return 0;

    case Type::Array:
      //TODO
      return 0;

    case Type::Object:
      return int32(value_.map_->size());

    default:
      fun_unexpected();
      return 0;
  }
}

bool JValue::IsEmpty() const {
  if (IsNull() || IsArray() || IsObject()) {
    return Count() == 0;
  } else {
    return false;
  }
}

JValue::operator bool() const { return !IsNull(); }

void JValue::Clear() {
  fun_check_msg(type_ == Type::Null || type_ == Type::Array || type_ == Type::Object,
                "in JValue::Clear(): requires complex value");

  start_ = 0;
  limit_ = 0;

  switch (type_) {
    case Type::Array:
    case Type::Object:
      value_.map_->clear();
      break;
    default:
      fun_unexpected();
  }
}

void JValue::Resize(int32 new_count) {
  fun_check_msg(type_ == Type::Null || type_ == Type::Array,
                "in JValue::Resize(): requires array");

  if (type_ == Type::Null) {
    *this = JValue(Type::Array);
  }

  int32 old_count = Count();
  if (new_count == 0) {
    Clear();
  } else if (new_count > old_count) {
    this->operator[](new_count - 1);
  } else {
    for (int32 i = new_count; i < old_count; ++i) {
      value_.map_->erase(i);
    }
    fun_check(Count() == new_count);
  }
}

JValue& JValue::operator[](int32 index) {
  fun_check_msg(type_ == Type::Null || type_ == Type::Array,
                "in JValue::operator[](int32 index): requires array");

  if (type_ == Type::Null) {
    *this = JValue(Type::Array);
  }

  CZString key(index);
  auto it = value_.map_->lower_bound(key);
  if (it != value_.map_->end() && (*it).first == key) {
    return (*it).second;
  }

  ObjectValues::value_type default_value(key, NullSingleton());
  it = values_.map_->insert(it, default_value);
  return (*it).second;
}





void JValue::DuplicatePaylod(const JValue& other) {
  SetType(other.GetType());
  SetIsAllocated(false);

  switch (type_) {
    case Type::Null:
    case Type::Int:
    case Type::UInt:
    case Type::Real:
    case Type::Bool:
        value_ = other.value_;
        break;

    case Type::String:
      if (other.value_.string_ && other.IsAllocated()) {
        uint32 len;
        const char* str;
        DecodePrefixedString(other.IsAllocated(), other.value_.string_, &len, &str);
        value_.string_ = DuplicateAndPrefixStringValue(str, len);
        SetIsAllocated(true);
      } else {
        value_.string_ = other.value_.string_;
      }
      break;

    case Type::Array:
    case Type::Object:
      value_.map_ = new ObjectValues(*other.value_.map);
      break;

    default:
      fun_unexpected();
  }
}

void JValue::ReleasePayload() {
  switch (type_) {
    case Type::Null:
    case Type::Int:
    case Type::UInt:
    case Type::Real:
    case Type::Bool:
        break;

    case Type::String:
      if (IsAllocated()) {
        ReleasePrefixedStringValue(value_.string_);
      }
      break;

    case Type::Array:
    case Type::Object:
      delete value_.map_;
      break;

    default:
      fun_unexpected();
  }
}

void JValue::DuplicateMeta(const JValue& other) {
  comment_ = other.comment_;
  start_ = other.start_;
  limit_ = other.limit_;
}

JValue& JValue::ResolveReference(const char* key) {
}

JValue& JValue::ResolveReference(const char* key, const char* end) {
}


JValue JValue::Get(int32 index, const JValue& default_value) const {
  const JValue* value = &((*this)[index]);
  return value == &NullSingleton() ? default_value : *value;
}

bool JValue::IsValidIndex(int32 index) const { return index < Count(); }





JValue* JValue::Demand(const char* begin, const char* end) {
}



const JValue& JValue::operator[](const char* key) const {
  const JValue* found = Find(key, key + CharTraitsA::Strlen(key));
  return found ? *found : __null__;
}
