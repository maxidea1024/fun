// TODO string 객체에 사용되는 메모리 할당을 최소화해야함. compile time string을
// 구분해서 처리하는게 좋을듯 싶은데...
#pragma once

#include "fun/json/forwards.h"
#include "fun/json/json.h"
//#include "fun/json/assertions.h"

#include <initializer_list>
#include <limits>

namespace fun {
namespace json {

// Forward declarations
class JValue;

/**
 * JSON array.
 */
typedef Array<JValue> JArray;

/**
 * JSON object.
 */
typedef Map<String, JValue> JObject;

/**
 * JSON value types.
 */
enum class ValueType {
  Null = 0,
  String = 1,
  Bool = 2,
  Integer = 3,
  UnsignedInteger = 4,
  Double = 5,
  Array = 6,
  Object = 7,
  NumValueTypes = 8,
};

/**
 * JSON comment placements.
 */
enum class CommentPlacement {
  Before = 0,
  AfterOnSameLine,
  After,
  NumCommentPlacements,
};

/**
 * JSON value.
 */
class FUN_JSON_API JValue {
 public:
  static JValue Null;
  static JValue EmptyString;
  static JValue EmptyArray;
  static JValue EmptyObject;
  static JValue True;
  static JValue False;
  static JValue NaN;
  static JValue PosInf;
  static JValue NegInf;

  JValue();
  JValue(ValueType type = ValueType::Null);
  JValue(decltype(nullptr));

  // JValue(bool value);
  template <typename T>
  FUN_ALWAYS_INLINE JValue(
      T value, typename EnableIf<IsSame<T, bool>::Value, int32>::Type = 0) {
    InitBasic(ValueType::Bool);
    bool_value_ = value;
  }

  JValue(int32 value);
  JValue(int64 value);
  JValue(uint32 value);
  JValue(uint64 value);
  JValue(float value);
  JValue(double value);
  // JValue(const char* value);
  // JValue(const char* value, int32 len);
  // JValue(const UNICHAR* value);
  // JValue(const UNICHAR* value, int32 len);
  // JValue(ByteStringView value);
  // JValue(UStringView value);
  // JValue(AsciiString value);
  JValue(const String&
             value);  // String의 사본을 만들지 않기 위해서, 참조로 넘겨줌.

  // TODO
  // JValue(const StaticString& value);

  JValue(const JArray& value);
  JValue(const JObject& value);
  JValue(const JValue& other);
  ~JValue();

  // Assignments

  template <typename T>
  FUN_ALWAYS_INLINE typename EnableIf<IsSame<T, bool>::Value, JValue&>::Type
  operator=(T value) {
    SetBool(value);
    return *this;
  }

  JValue& operator=(int32 value);
  JValue& operator=(int64 value);
  JValue& operator=(uint32 value);
  JValue& operator=(uint64 value);
  JValue& operator=(float value);
  JValue& operator=(double value);
  // JValue& operator = (const char* value);
  // JValue& operator = (ByteStringView value);
  // JValue& operator = (const UNICHAR* value);
  // JValue& operator = (UStringView value);
  // JValue& operator = (AsciiString value);
  JValue& operator=(const String& value);  // CString의 사본을 만들지 않기
                                           // 위해서, 참조로 넘겨줌.
  JValue& operator=(const JArray& value);
  JValue& operator=(const JObject& value);
  JValue& operator=(const JValue& other);

  void Swap(JValue& other);
  void SwapPayload(JValue& other);

  bool IsEmpty() const;
  int32 Count() const;

  // Array manipulation

  bool ContainsIndex(int32 index) const;

  void Resize(int32 new_count);

  JValue& Append();

  template <typename T>
  FUN_ALWAYS_INLINE JValue& Append(
      T value, typename EnableIf<IsSame<T, bool>::Value, int32>::Type = 0) {
    SetArray();
    array_value_->Add(JValue(value));
    return *this;
  }

  JValue& Append(int32 value);
  JValue& Append(int64 value);
  JValue& Append(uint32 value);
  JValue& Append(uint64 value);
  JValue& Append(float value);
  JValue& Append(double value);
  // JValue& Append(const char* value);
  // JValue& Append(const char* value, int32 len);
  // JValue& Append(ByteStringView value);
  // JValue& Append(const UNICHAR* value);
  // JValue& Append(const UNICHAR* value, int32 len);
  // JValue& Append(UStringView value);
  // JValue& Append(AsciiString value);
  JValue& Append(const String& value);  // CString의 사본을 만들지 않기 위해서,
                                        // 참조로 넘겨줌.
  JValue& Append(const JObject& value);
  JValue& Append(const JValue& value);
  JValue& AppendChildArray();
  JValue& AppendChildObject();
  bool RemoveAt(int32 array_index);
  void RemoveAllElements();

  const JValue& operator[](int32 array_index) const;
  JValue& operator[](int32 array_index);

  JValue Get(int32 array_index,
             const JValue& default_value = JValue::Null) const;

  // Object manipulation

  bool ContainsField(const String& field_name) const;

  template <typename T>
  FUN_ALWAYS_INLINE JValue& AddField(
      const String& field_name, T value,
      typename EnableIf<IsSame<T, bool>::Value, int32>::Type = 0) {
    SetObject();
    object_value_->Add(field_name, JValue(value));
    return *this;
  }

  JValue& AddField(const String& field_name, int32 value);
  JValue& AddField(const String& field_name, int64 value);
  JValue& AddField(const String& field_name, uint32 value);
  JValue& AddField(const String& field_name, uint64 value);
  JValue& AddField(const String& field_name, float value);
  JValue& AddField(const String& field_name, double value);
  // JValue& AddField(const String& field_name, const char* value);
  // JValue& AddField(const String& field_name, const char* value, int32 len);
  // JValue& AddField(const String& field_name, ByteStringView value);
  // JValue& AddField(const String& field_name, const UNICHAR* value);
  // JValue& AddField(const String& field_name, const UNICHAR* value, int32
  // len); JValue& AddField(const String& field_name, UStringView value); JValue&
  // AddField(const String& field_name, AsciiString value);
  JValue& AddField(const String& field_name,
                   const String& value);  // String의 사본을 만들지 않기 위해서,
                                          // 참조로 넘겨줌.
  JValue& AddField(const String& field_name, const JArray& value);
  JValue& AddField(const String& field_name, const JObject& value);
  JValue& AddField(const String& field_name, const JValue& value);
  JValue& AddChildArrayField(const String& field_name);
  JValue& AddChildObjectField(const String& field_name);
  bool RemoveField(const String& field_name);
  void RemoveAllFields();

  const JValue& operator[](const String& field_name) const;
  // TODO
  // JValue& operator [] (const StaticString& field_name);
  JValue& operator[](const String& field_name);

  JValue Get(const String& field_name,
             const JValue& default_value = JValue::Null) const;
  const JValue* Find(const String& field_name) const;
  Array<String> GetFieldNames() const;

  // Types

  ValueType GetType() const;

  const char* GetTypeName() const;
  static const char* GetTypeName(ValueType type);

  explicit operator bool() const;
  // Returns IsNull()
  bool operator!() const;

  bool IsNull() const;
  bool IsBool() const;
  bool IsString() const;

  bool IsNumeric() const;
  bool IsIntegral() const;
  bool IsInteger() const;
  bool IsUnsignedInteger() const;
  bool IsDouble() const;

  bool IsArray() const;
  bool IsObject() const;

  bool IsConvertibleTypeTo(const ValueType type_to) const;

  bool AsBool() const;
  const String AsString() const;
  int64 AsInteger() const;
  uint64 AsUnsignedInteger() const;
  double AsDouble() const;
  const JArray& AsArray() const;
  const JObject& AsObject() const;

  void SetNull();

  void SetBool(bool value);

  void SetInteger(int32 value);
  void SetInteger(int64 value);

  void SetUnsignedInteger(uint32 value);
  void SetUnsignedInteger(uint64 value);

  void SetDouble(float value);
  void SetDouble(double value);

  void SetString();
  // void SetString(ByteStringView value);
  // void SetString(UStringView value);
  // void SetString(AsciiString value);
  void SetString(const String& value);  // CString의 사본을 만들지 않기 위해서,
                                        // 참조로 넘겨줌.

  JValue& SetArray();
  JValue& SetArray(const JArray& value);
  JValue& SetObject();
  JValue& SetObject(const JObject& value);

  //
  // Comparisons
  //

  int32 Compare(const JValue& other) const;
  bool operator==(const JValue& other) const;
  bool operator!=(const JValue& other) const;
  bool operator<(const JValue& other) const;
  bool operator<=(const JValue& other) const;
  bool operator>(const JValue& other) const;
  bool operator>=(const JValue& other) const;

  //
  // Comments
  //

  void SetComment(const char* comment, int32 len, CommentPlacement placement);
  void SetComment(const String& comment, CommentPlacement placement);
  bool HasComment(CommentPlacement placement) const;
  String GetComment(CommentPlacement placement) const;

  //
  // Location(used for reader)
  //

  void SetOffsetStart(ptrdiff_t start);
  void SetOffsetLimit(ptrdiff_t limit);
  ptrdiff_t GetOffsetStart() const;
  ptrdiff_t GetOffsetLimit() const;

  // Serialization with string.

  bool FromString(const String& str, String* out_error = nullptr);
  bool FromString(const char* begin, const char* end,
                  String* out_error = nullptr);
  /** Stringify */
  String ToString(bool pretty = false) const;

  // Serialization with file.

  bool LoadFromFile(const String& filename, String* out_error = nullptr);
  bool SaveToFile(const String& filename, bool pretty = true);

 private:
  friend class CondensedWriter;
  friend class PrettyWriter;

  /**
   * value type
   */
  ValueType type_;

  /**
   * Rainbow value
   */
  union {
    bool bool_value_;
    int64 integer_value_;
    uint64 unsigned_integer_value_;
    double double_value_;
    String* string_value_;
    JArray* array_value_;
    JObject* object_value_;
  };

  // Extra

  /**
   * comments (each CommentPlacement has its own comment array)
   * If there is no comment, it is nullptr.
   */
  String* comments_;

  /**
   * The starting position in this value's document string.
   */
  ptrdiff_t start_;

  /**
   * The end position in this value's document string.
   */
  ptrdiff_t limit_;

  void InitBasic(ValueType type);
  void ChangeType_INTERNAL(const ValueType new_type);
  void FreeValue();

  void EnsureValidCommentPlacement(const CommentPlacement placement) const;
};

//
// inlines
//

FUN_ALWAYS_INLINE void JValue::InitBasic(ValueType type) {
  type_ = type;
  unsigned_integer_value_ = 0;

  start_ = 0;
  limit_ = 0;
  comments_ = nullptr;
}

FUN_ALWAYS_INLINE JValue::JValue() { InitBasic(ValueType::Null); }

FUN_ALWAYS_INLINE JValue::JValue(ValueType type) {
  InitBasic(type);

  switch (type_) {
    case ValueType::String:
      string_value_ = new String();
      break;
    case ValueType::Array:
      array_value_ = new JArray();
      break;
    case ValueType::Object:
      object_value_ = new JObject();
      break;
    default:
      object_value_ = nullptr;
      break;  // largest
  }
}

FUN_ALWAYS_INLINE JValue::JValue(decltype(nullptr)) {
  InitBasic(ValueType::Null);
}

// FUN_ALWAYS_INLINE JValue::JValue(bool value) {
//  InitBasic(ValueType::Bool);
//  bool_value_ = value;
//}

FUN_ALWAYS_INLINE JValue::JValue(int32 value) {
  InitBasic(ValueType::Integer);
  integer_value_ = value;
}

FUN_ALWAYS_INLINE JValue::JValue(int64 value) {
  InitBasic(ValueType::Integer);
  integer_value_ = value;
}

FUN_ALWAYS_INLINE JValue::JValue(uint32 value) {
  InitBasic(ValueType::UnsignedInteger);
  unsigned_integer_value_ = value;
}

FUN_ALWAYS_INLINE JValue::JValue(uint64 value) {
  InitBasic(ValueType::UnsignedInteger);
  unsigned_integer_value_ = value;
}

FUN_ALWAYS_INLINE JValue::JValue(float value) {
  InitBasic(ValueType::Double);
  double_value_ = value;
}

FUN_ALWAYS_INLINE JValue::JValue(double value) {
  InitBasic(ValueType::Double);
  double_value_ = value;
}

// FUN_ALWAYS_INLINE JValue::JValue(const char* value) {
//  InitBasic(ValueType::String);
//  string_value_ = new String(ByteStringView(value));
//}
//
// FUN_ALWAYS_INLINE JValue::JValue(const char* value, int32 len) {
//  InitBasic(ValueType::String);
//  string_value_ = new String(ByteStringView(value, len));
//}
//
// FUN_ALWAYS_INLINE JValue::JValue(const UNICHAR* value) {
//  InitBasic(ValueType::String);
//  string_value_ = new String(value);
//}
//
// FUN_ALWAYS_INLINE JValue::JValue(const UNICHAR* value, int32 len) {
//  InitBasic(ValueType::String);
//  string_value_ = new String(UStringView(value, len));
//}
//
// FUN_ALWAYS_INLINE JValue::JValue(ByteStringView value) {
//  InitBasic(ValueType::String);
//  string_value_ = new String(value);
//}
//
// FUN_ALWAYS_INLINE JValue::JValue(UStringView value) {
//  InitBasic(ValueType::String);
//  string_value_ = new String(value);
//}
//
// FUN_ALWAYS_INLINE JValue::JValue(AsciiString value) {
//  InitBasic(ValueType::String);
//  string_value_ = new String(value);
//}

FUN_ALWAYS_INLINE JValue::JValue(const String& value) {
  InitBasic(ValueType::String);
  string_value_ = new String(value);
}

FUN_ALWAYS_INLINE JValue::JValue(const JArray& value) {
  InitBasic(ValueType::Array);
  array_value_ = new JArray(value);
}

FUN_ALWAYS_INLINE JValue::JValue(const JObject& value) {
  InitBasic(ValueType::Object);
  object_value_ = new JObject(value);
}

FUN_ALWAYS_INLINE JValue::JValue(const JValue& other) : type_(other.type_) {
  type_ = other.type_;

  if (other.comments_) {
    const int32 N = (int32)CommentPlacement::NumCommentPlacements;
    comments_ = new String[N];
    for (int32 i = 0; i < N; ++i) {
      comments_[i] = other.comments_[i];
    }
  } else {
    comments_ = nullptr;
  }

  start_ = other.start_;
  limit_ = other.limit_;

  switch (type_) {
    case ValueType::Null:
      break;
    case ValueType::Integer:
      integer_value_ = other.integer_value_;
      break;
    case ValueType::UnsignedInteger:
      unsigned_integer_value_ = other.unsigned_integer_value_;
      break;
    case ValueType::Double:
      double_value_ = other.double_value_;
      break;
    case ValueType::Bool:
      bool_value_ = other.bool_value_;
      break;
    case ValueType::String:
      string_value_ = new String(*other.string_value_);
      break;
    case ValueType::Array:
      array_value_ = new JArray(*other.array_value_);
      break;
    case ValueType::Object:
      object_value_ = new JObject(*other.object_value_);
      break;
    default:
      fun_unexpected();
      break;
  }
}

FUN_ALWAYS_INLINE JValue::~JValue() {
  FreeValue();
  delete[] comments_;
  ;
}

FUN_ALWAYS_INLINE void JValue::FreeValue() {
  switch (type_) {
    case ValueType::String:
      delete string_value_;
      string_value_ = nullptr;
      break;
    case ValueType::Array:
      delete array_value_;
      array_value_ = nullptr;
      break;
    case ValueType::Object:
      delete object_value_;
      object_value_ = nullptr;
      break;
  }
}

// FUN_ALWAYS_INLINE JValue& JValue::operator = (bool value) {
//  SetBool(value);
//  return *this;
//}

FUN_ALWAYS_INLINE JValue& JValue::operator=(int32 value) {
  SetInteger(value);
  return *this;
}

FUN_ALWAYS_INLINE JValue& JValue::operator=(int64 value) {
  SetInteger(value);
  return *this;
}

FUN_ALWAYS_INLINE JValue& JValue::operator=(uint32 value) {
  SetUnsignedInteger(value);
  return *this;
}

FUN_ALWAYS_INLINE JValue& JValue::operator=(uint64 value) {
  SetUnsignedInteger(value);
  return *this;
}

FUN_ALWAYS_INLINE JValue& JValue::operator=(float value) {
  SetDouble(value);
  return *this;
}

FUN_ALWAYS_INLINE JValue& JValue::operator=(double value) {
  SetDouble(value);
  return *this;
}

// FUN_ALWAYS_INLINE JValue& JValue::operator = (const char* value) {
//  return (*this = ByteStringView(value));
//}
//
// FUN_ALWAYS_INLINE JValue& JValue::operator = (ByteStringView value) {
//  SetString(value);
//  return *this;
//}
//
// FUN_ALWAYS_INLINE JValue& JValue::operator = (const UNICHAR* value) {
//  return (*this = UStringView(value));
//}
//
// FUN_ALWAYS_INLINE JValue& JValue::operator = (UStringView value) {
//  SetString(value);
//  return *this;
//}
//
// FUN_ALWAYS_INLINE JValue& JValue::operator = (AsciiString value) {
//  SetString(value);
//  return *this;
//}

FUN_ALWAYS_INLINE JValue& JValue::operator=(const String& value) {
  SetString(value);
  return *this;
}

FUN_ALWAYS_INLINE JValue& JValue::operator=(const JArray& value) {
  SetArray(value);
  return *this;
}

FUN_ALWAYS_INLINE JValue& JValue::operator=(const JObject& value) {
  SetObject(value);
  return *this;
}

FUN_ALWAYS_INLINE JValue& JValue::operator=(const JValue& other) {
  if (FUN_LIKELY(&other != this)) {
    ChangeType_INTERNAL(other.type_);

    switch (type_) {
      case ValueType::Null:
        break;
      case ValueType::Bool:
        bool_value_ = other.bool_value_;
        break;
      case ValueType::Integer:
        integer_value_ = other.integer_value_;
        break;
      case ValueType::UnsignedInteger:
        unsigned_integer_value_ = other.unsigned_integer_value_;
        break;
      case ValueType::Double:
        double_value_ = other.double_value_;
        break;
      case ValueType::String:
        *string_value_ = *other.string_value_;
        break;
      case ValueType::Array:
        *array_value_ = *other.array_value_;
        break;
      case ValueType::Object:
        *object_value_ = *other.object_value_;
        break;
      default:
        fun_unreachable();
        break;
    }

    if (other.comments_) {
      const int32 N = (int32)CommentPlacement::NumCommentPlacements;
      if (comments_ == nullptr) {
        comments_ = new String[N];
      }

      for (int32 i = 0; i < N; ++i) {
        comments_[i] = other.comments_[i];
      }
    } else {
      delete[] comments_;
      comments_ = nullptr;
    }

    start_ = other.start_;
    limit_ = other.limit_;
  }
  return *this;
}

FUN_ALWAYS_INLINE void JValue::Swap(JValue& other) {
  SwapPayload(other);

  fun::Swap(comments_, other.comments_);
  fun::Swap(start_, other.start_);
  fun::Swap(limit_, other.limit_);
}

FUN_ALWAYS_INLINE void JValue::SwapPayload(JValue& other) {
  fun::Swap(type_, other.type_);
  // UnsignedIntegerValue이게 제일큰가?
  fun::Swap(unsigned_integer_value_, other.unsigned_integer_value_);
}

FUN_ALWAYS_INLINE bool JValue::IsEmpty() const {
  switch (type_) {
    case ValueType::Null:
      return true;
    case ValueType::Array:
      return array_value_->Count() == 0;
    case ValueType::Object:
      return object_value_->Count() == 0;
  }
  return false;
}

FUN_ALWAYS_INLINE int32 JValue::Count() const {
  switch (type_) {
    case ValueType::Array:
      return array_value_->Count();
    case ValueType::Object:
      return object_value_->Count();
  }
  return 0;
}

FUN_ALWAYS_INLINE bool JValue::ContainsIndex(int32 index) const {
  return (type_ == ValueType::Array) &&
         (index >= 0 && index < array_value_->Count());
}

FUN_ALWAYS_INLINE void JValue::Resize(int32 new_count) {
  if (type_ == ValueType::Array) {
    array_value_->Resize(new_count);
  }
}

FUN_ALWAYS_INLINE JValue& JValue::Append() {
  SetArray();
  array_value_->Add(JValue::Null);
  return *this;
}

// FUN_ALWAYS_INLINE JValue& JValue::Append(bool value) {
//  SetArray();
//  array_value_->Add(JValue(value));
//  return *this;
//}

FUN_ALWAYS_INLINE JValue& JValue::Append(int32 value) {
  SetArray();
  array_value_->Add(JValue(value));
  return *this;
}

FUN_ALWAYS_INLINE JValue& JValue::Append(int64 value) {
  SetArray();
  array_value_->Add(JValue(value));
  return *this;
}

FUN_ALWAYS_INLINE JValue& JValue::Append(uint32 value) {
  SetArray();
  array_value_->Add(JValue(value));
  return *this;
}

FUN_ALWAYS_INLINE JValue& JValue::Append(uint64 value) {
  SetArray();
  array_value_->Add(JValue(value));
  return *this;
}

FUN_ALWAYS_INLINE JValue& JValue::Append(float value) {
  SetArray();
  array_value_->Add(JValue(value));
  return *this;
}

FUN_ALWAYS_INLINE JValue& JValue::Append(double value) {
  SetArray();
  array_value_->Add(JValue(value));
  return *this;
}

// FUN_ALWAYS_INLINE JValue& JValue::Append(const char* value) {
//  return Append(ByteStringView(value));
//}
//
// FUN_ALWAYS_INLINE JValue& JValue::Append(const char* value, int32 len) {
//  return Append(ByteStringView(value, len));
//}
//
// FUN_ALWAYS_INLINE JValue& JValue::Append(ByteStringView value) {
//  SetArray();
//  array_value_->Add(JValue(value));
//  return *this;
//}
//
// FUN_ALWAYS_INLINE JValue& JValue::Append(const UNICHAR* value) {
//  return Append(UStringView(value));
//}
//
// FUN_ALWAYS_INLINE JValue& JValue::Append(const UNICHAR* value, int32 len) {
//  return Append(UStringView(value, len));
//}
//
// FUN_ALWAYS_INLINE JValue& JValue::Append(UStringView value) {
//  SetArray();
//  array_value_->Add(JValue(value));
//  return *this;
//}
//
// FUN_ALWAYS_INLINE JValue& JValue::Append(AsciiString value) {
//  SetArray();
//  array_value_->Add(JValue(value));
//  return *this;
//}

FUN_ALWAYS_INLINE JValue& JValue::Append(const String& value) {
  SetArray();
  array_value_->Add(JValue(value));
  return *this;
}

FUN_ALWAYS_INLINE JValue& JValue::Append(const JObject& value) {
  SetArray();
  array_value_->Add(JValue(value));
  return *this;
}

FUN_ALWAYS_INLINE JValue& JValue::Append(const JValue& value) {
  SetArray();
  array_value_->Add(JValue(value));
  return *this;
}

FUN_ALWAYS_INLINE JValue& JValue::AppendChildArray() {
  SetArray();
  array_value_->Add(JArray());
  return (*array_value_).Last();
}

FUN_ALWAYS_INLINE JValue& JValue::AppendChildObject() {
  SetArray();
  array_value_->Add(JObject());
  return (*array_value_).Last();
}

FUN_ALWAYS_INLINE bool JValue::RemoveAt(int32 array_index) {
  if (type_ == ValueType::Array) {
    if (array_index < 0 || array_index >= array_value_->Count()) {
      return false;
    }

    array_value_->RemoveAt(array_index);
    return true;
  }

  return false;
}

FUN_ALWAYS_INLINE void JValue::RemoveAllElements() {
  if (type_ == ValueType::Array) {
    array_value_->Clear();
  }
}

FUN_ALWAYS_INLINE const JValue& JValue::operator[](int32 array_index) const {
  fun_check(type_ == ValueType::Null || type_ == ValueType::Array);

  if (array_index < 0) {
    throw IndexOutOfBoundsException,
        StringLiteral("array index must be non-negative.");
  }

  if (array_index >= std::numeric_limits<int32>::max() / 2) {
    throw IndexOutOfBoundsException, StringLiteral("array index too biggest.");
  }

  if (type_ == ValueType::Null) {
    return JValue::Null;
  }

  if (array_index >= array_value_->Count()) {
    return JValue::Null;
  }

  return (*array_value_)[array_index];
}

FUN_ALWAYS_INLINE JValue& JValue::operator[](int32 array_index) {
  fun_check(type_ == ValueType::Null || type_ == ValueType::Array);

  if (array_index < 0) {
    throw IndexOutOfBoundsException,
        StringLiteral("array index must be non-negative.");
  }

  if (array_index >= std::numeric_limits<int32>::max() / 2) {
    throw IndexOutOfBoundsException, StringLiteral("array index too biggest.");
  }

  if (type_ == ValueType::Null) {
    JValue empty_array(EmptyArray);
    this->SwapPayload(empty_array);

    array_value_->Resize(array_index +
                         1);  // auto expand with JValue(ValueType::Null);
  } else {
    if (array_index >= array_value_->Count()) {
      array_value_->Resize(array_index +
                           1);  // auto expand with JValue(ValueType::Null);
    }
  }

  return (*array_value_)[array_index];
}

FUN_ALWAYS_INLINE JValue JValue::Get(int32 array_index,
                                     const JValue& default_value) const {
  if (type_ != ValueType::Array) {
    return default_value;
  }

  if (array_index < 0 || array_index >= array_value_->Count()) {
    return default_value;
  }

  return (*array_value_)[array_index];
}

FUN_ALWAYS_INLINE bool JValue::ContainsField(const String& field_name) const {
  return type_ == ValueType::Object && object_value_->Contains(field_name);
}

// FUN_ALWAYS_INLINE JValue& JValue::AddField(const String& field_name, bool
// value) {
//  SetObject();
//  object_value_->Add(field_name, JValue(value));
//  return *this;
//}

FUN_ALWAYS_INLINE JValue& JValue::AddField(const String& field_name,
                                           int32 value) {
  SetObject();
  object_value_->Add(field_name, JValue(value));
  return *this;
}

FUN_ALWAYS_INLINE JValue& JValue::AddField(const String& field_name,
                                           int64 value) {
  SetObject();
  object_value_->Add(field_name, JValue(value));
  return *this;
}

FUN_ALWAYS_INLINE JValue& JValue::AddField(const String& field_name,
                                           uint32 value) {
  SetObject();
  object_value_->Add(field_name, JValue(value));
  return *this;
}

FUN_ALWAYS_INLINE JValue& JValue::AddField(const String& field_name,
                                           uint64 value) {
  SetObject();
  object_value_->Add(field_name, JValue(value));
  return *this;
}

FUN_ALWAYS_INLINE JValue& JValue::AddField(const String& field_name,
                                           float value) {
  SetObject();
  object_value_->Add(field_name, JValue(value));
  return *this;
}

FUN_ALWAYS_INLINE JValue& JValue::AddField(const String& field_name,
                                           double value) {
  SetObject();
  object_value_->Add(field_name, JValue(value));
  return *this;
}

// FUN_ALWAYS_INLINE JValue& JValue::AddField(const String& field_name, const
// char* value) {
//  return AddField(field_name, ByteStringView(value));
//}
//
// FUN_ALWAYS_INLINE JValue& JValue::AddField(const String& field_name, const
// char* value, int32 len) {
//  return AddField(field_name, ByteStringView(value, len));
//}
//
// FUN_ALWAYS_INLINE JValue& JValue::AddField(const String& field_name,
// ByteStringView value) {
//  SetObject();
//  object_value_->Add(field_name, JValue(value));
//  return *this;
//}
//
// FUN_ALWAYS_INLINE JValue& JValue::AddField(const String& field_name, const
// UNICHAR* value) {
//  return AddField(field_name, UStringView(value));
//}
//
// FUN_ALWAYS_INLINE JValue& JValue::AddField(const String& field_name, const
// UNICHAR* value, int32 len) {
//  return AddField(field_name, UStringView(value, len));
//}
//
// FUN_ALWAYS_INLINE JValue& JValue::AddField(const String& field_name,
// UStringView value) {
//  SetObject();
//  object_value_->Add(field_name, JValue(value));
//  return *this;
//}
//
// FUN_ALWAYS_INLINE JValue& JValue::AddField(const String& field_name,
// AsciiString value) {
//  SetObject();
//  object_value_->Add(field_name, JValue(value));
//  return *this;
//}

FUN_ALWAYS_INLINE JValue& JValue::AddField(const String& field_name,
                                           const String& value) {
  SetObject();
  object_value_->Add(field_name, JValue(value));
  return *this;
}

FUN_ALWAYS_INLINE JValue& JValue::AddField(const String& field_name,
                                           const JArray& value) {
  SetObject();
  object_value_->Add(field_name, JValue(value));
  return *this;
}

FUN_ALWAYS_INLINE JValue& JValue::AddField(const String& field_name,
                                           const JObject& value) {
  SetObject();
  object_value_->Add(field_name, JValue(value));
  return *this;
}

FUN_ALWAYS_INLINE JValue& JValue::AddField(const String& field_name,
                                           const JValue& value) {
  SetObject();
  object_value_->Add(field_name, JValue(value));
  return *this;
}

FUN_ALWAYS_INLINE JValue& JValue::AddChildArrayField(const String& field_name) {
  SetObject();
  return object_value_->Add(field_name, JArray());
}

FUN_ALWAYS_INLINE JValue& JValue::AddChildObjectField(
    const String& field_name) {
  SetObject();
  return object_value_->Add(field_name, JObject());
}

FUN_ALWAYS_INLINE bool JValue::RemoveField(const String& field_name) {
  if (type_ == ValueType::Object) {
    return object_value_->Remove(field_name) != 0;
  }
  return false;
}

FUN_ALWAYS_INLINE void JValue::RemoveAllFields() {
  if (type_ == ValueType::Object) {
    object_value_->Clear();
  }
}

FUN_ALWAYS_INLINE const JValue& JValue::operator[](
    const String& field_name) const {
  fun_check(type_ == ValueType::Null || type_ == ValueType::Object);

  if (type_ == ValueType::Null) {
    return JValue::Null;
  }

  const JValue* found = Find(field_name);
  return found ? *found : JValue::Null;
}

FUN_ALWAYS_INLINE JValue& JValue::operator[](const String& field_name) {
  fun_check(type_ == ValueType::Null || type_ == ValueType::Object);

  if (type_ == ValueType::Null) {
    JValue empty_object(EmptyObject);
    this->SwapPayload(empty_object);

    return object_value_->Add(field_name, JValue::Null);
  }

  if (auto* found = object_value_->Find(field_name)) {
    return *found;
  } else {
    return object_value_->Add(field_name, JValue::Null);
  }
}

FUN_ALWAYS_INLINE JValue JValue::Get(const String& field_name,
                                     const JValue& default_value) const {
  const JValue* found = Find(field_name);
  return found ? *found : default_value;
}

FUN_ALWAYS_INLINE const JValue* JValue::Find(const String& field_name) const {
  if (type_ != ValueType::Object) {
    return nullptr;
  }

  return object_value_->Find(field_name);
}

FUN_ALWAYS_INLINE Array<String> JValue::GetFieldNames() const {
  Array<String> result;
  if (type_ == ValueType::Object) {
    result.Reserve(object_value_->Count());
    for (const auto& pair : *object_value_) {
      result.Add(pair.Key);
    }
  }
  return result;
}

FUN_ALWAYS_INLINE ValueType JValue::GetType() const { return type_; }

FUN_ALWAYS_INLINE const char* JValue::TypeName() const {
  return TypeName(type_);
}

FUN_ALWAYS_INLINE const char* JValue::TypeName(ValueType type) {
  switch (type) {
    case ValueType::Null:
      return "null";

    case ValueType::Bool:
      return "bool";

    case ValueType::Integer:
      return "integer";

    case ValueType::UnsignedInteger:
      return "unsigned_integer";

    case ValueType::Double:
      return "double";

    case ValueType::String:
      return "string";

    case ValueType::Array:
      return "array";

    case ValueType::Object:
      return "object";

    default:
      fun_unexpected();
  }
}

FUN_ALWAYS_INLINE JValue::operator bool() const { return !IsNull(); }

FUN_ALWAYS_INLINE bool JValue::operator!() const { return IsNull(); }

FUN_ALWAYS_INLINE bool JValue::IsNull() const {
  return type_ == ValueType::Null;
}

FUN_ALWAYS_INLINE bool JValue::IsBool() const {
  return type_ == ValueType::Bool;
}

FUN_ALWAYS_INLINE bool JValue::IsString() const {
  return type_ == ValueType::String;
}

FUN_ALWAYS_INLINE bool JValue::IsNumeric() const {
  return type_ == ValueType::Double || type_ == ValueType::Integer ||
         type_ == ValueType::UnsignedInteger;
}

FUN_ALWAYS_INLINE bool JValue::IsIntegral() const {
  return IsInteger() || IsUnsignedInteger();
}

FUN_ALWAYS_INLINE bool JValue::IsInteger() const {
  switch (type_) {
    case ValueType::Integer:
      return true;

    case ValueType::UnsignedInteger:
      return unsigned_integer_value_ <
             (uint64)std::numeric_limits<int64>::max();

    case ValueType::Double: {
      if (double_value_ < std::numeric_limits<int64>::min() ||
          double_value_ > std::numeric_limits<int64>::max()) {
        return false;
      }

      double integral_part;
      return modf(double_value_, &integral_part) == 0.0;
    }
  }

  return false;
}

FUN_ALWAYS_INLINE bool JValue::IsUnsignedInteger() const {
  switch (type_) {
    case ValueType::Integer:
      return integer_value_ >= 0;

    case ValueType::UnsignedInteger:
      return true;

    case ValueType::Double: {
      if (double_value_ < 0.0 ||
          double_value_ > std::numeric_limits<uint64>::max()) {
        return false;
      }

      double integral_part;
      return modf(double_value_, &integral_part) == 0.0;
    }
  }

  return false;
}

FUN_ALWAYS_INLINE bool JValue::IsDouble() const { return IsNumeric(); }

FUN_ALWAYS_INLINE bool JValue::IsArray() const {
  return type_ == ValueType::Array;
}

FUN_ALWAYS_INLINE bool JValue::IsObject() const {
  return type_ == ValueType::Object;
}

FUN_ALWAYS_INLINE bool JValue::IsConvertibleTypeTo(
    const ValueType type_to) const {
  switch (type_to) {
    case ValueType::Null:
      return (IsNumeric() && AsDouble() == 0.0) ||
             (type_ == ValueType::Bool && bool_value_ == false) ||
             (type_ == ValueType::String && *string_value_ == String()) ||
             (type_ == ValueType::Array && array_value_->Count() == 0) ||
             (type_ == ValueType::Object && object_value_->Count() == 0) ||
             (type_ == ValueType::Null);

    case ValueType::Integer:
      return IsInteger() ||
             (type_ == ValueType::Bool || type_ == ValueType::Null);

    case ValueType::UnsignedInteger:
      return IsUnsignedInteger() ||
             (type_ == ValueType::Bool || type_ == ValueType::Null);

    case ValueType::Double:
      return IsNumeric() || type_ == ValueType::Bool ||
             type_ == ValueType::Null;

    case ValueType::Bool:
      return IsNumeric() || type_ == ValueType::Bool ||
             type_ == ValueType::Null;

    case ValueType::String:
      return IsNumeric() || type_ == ValueType::Bool ||
             type_ == ValueType::String || type_ == ValueType::Null;

    case ValueType::Array:
      return type_ == ValueType::Array || type_ == ValueType::Null;

    case ValueType::Object:
      return type_ == ValueType::Object || type_ == ValueType::Null;
  }

  fun_unexpected();
}

FUN_ALWAYS_INLINE bool JValue::AsBool() const {
  switch (type_) {
    case ValueType::Bool:
      return bool_value_;

    case ValueType::Integer:
      return !!integer_value_;

    case ValueType::UnsignedInteger:
      return !!integer_value_;

    case ValueType::Double:
      return !!double_value_;

    case ValueType::Null:
      return false;

    default:
      throw TypeIncompatibleException(
          String::Format("could not cast type {0} to bool.", TypeName()));
  }
}

FUN_ALWAYS_INLINE const String JValue::AsString() const {
  switch (type_) {
    case ValueType::Null:
      return String();  // "(nil)" ??

    case ValueType::String:
      return *string_value_;

    case ValueType::Bool:
      return bool_value_ ? StringLiteral("true") : StringLiteral("false");

    case ValueType::Integer:
      return String::FromNumber(integer_value_);

    case ValueType::UnsignedInteger:
      return String::FromNumber(unsigned_integer_value_);

    case ValueType::Double:
      return String::FromNumber(double_value_);

    default:
      throw TypeIncompatibleException(
          String::Format("could not cast type {0} to string.", TypeName()));
  }
}

FUN_ALWAYS_INLINE int64 JValue::AsInteger() const {
  switch (type_) {
    case ValueType::Integer:
      return integer_value_;

    case ValueType::UnsignedInteger:
      fun_check(IsInteger());
      return unsigned_integer_value_;

    case ValueType::Double:
      fun_check(IsInteger());
      return (int64)double_value_;

    case ValueType::Null:
      return 0;

    case ValueType::Bool:
      return bool_value_ ? 1 : 0;

    default:
      throw TypeIncompatibleException(
          String::Format("could not cast type {0} to integer.", TypeName()));
  }
}

FUN_ALWAYS_INLINE uint64 JValue::AsUnsignedInteger() const {
  switch (type_) {
    case ValueType::Integer:
      fun_check(IsUnsignedInteger());
      return integer_value_;

    case ValueType::UnsignedInteger:
      return unsigned_integer_value_;

    case ValueType::Double:
      fun_check(IsUnsignedInteger());
      return (uint64)double_value_;

    case ValueType::Null:
      return 0;

    case ValueType::Bool:
      return bool_value_ ? 1 : 0;

    default:
      throw TypeIncompatibleException(String::Format(
          "could not cast type {0} to unsigned-integer.", TypeName()));
  }
}

FUN_ALWAYS_INLINE double JValue::AsDouble() const {
  switch (type_) {
    case ValueType::Integer:
      return (double)integer_value_;

    case ValueType::UnsignedInteger:
      return (double)unsigned_integer_value_;

    case ValueType::Double:
      return double_value_;

    case ValueType::Null:
      return 0.0;

    case ValueType::Bool:
      return bool_value_ ? 1.0 : 0.0;

    default:
      throw TypeIncompatibleException(
          String::Format("could not cast type {0} to double.", TypeName()));
  }
}

FUN_ALWAYS_INLINE const JArray& JValue::AsArray() const {
  if (type_ != ValueType::Array) {
    throw TypeIncompatibleException(
        String::Format("could not cast type {0} to array.", TypeName()));
  }

  return *array_value_;
}

FUN_ALWAYS_INLINE const JObject& JValue::AsObject() const {
  if (type_ != ValueType::Object) {
    throw TypeIncompatibleException(
        String::Format("could not cast type {0} to object.", TypeName()));
  }

  return *object_value_;
}

FUN_ALWAYS_INLINE void JValue::SetNull() {
  ChangeType_INTERNAL(ValueType::Null);
}

FUN_ALWAYS_INLINE void JValue::SetBool(bool value) {
  fun_check_msg(type_ == ValueType::Null || type_ == ValueType::Bool,
                "It can be changed to bool type only if it is null type.");
  ChangeType_INTERNAL(ValueType::Bool);
  bool_value_ = value;
}

FUN_ALWAYS_INLINE void JValue::SetInteger(int32 value) {
  fun_check_msg(type_ == ValueType::Null || type_ == ValueType::Integer,
                "It can be changed to integer type only if it is null type.");
  ChangeType_INTERNAL(ValueType::Integer);
  integer_value_ = value;
}

FUN_ALWAYS_INLINE void JValue::SetInteger(int64 value) {
  fun_check_msg(type_ == ValueType::Null || type_ == ValueType::Integer,
                "It can be changed to integer type only if it is null type.");
  ChangeType_INTERNAL(ValueType::Integer);
  integer_value_ = value;
}

FUN_ALWAYS_INLINE void JValue::SetUnsignedInteger(uint32 value) {
  fun_check_msg(
      type_ == ValueType::Null || type_ == ValueType::UnsignedInteger,
      "It can be changed to unsigned integer type only if it is null type.");
  ChangeType_INTERNAL(ValueType::UnsignedInteger);
  unsigned_integer_value_ = value;
}

FUN_ALWAYS_INLINE void JValue::SetUnsignedInteger(uint64 value) {
  fun_check_msg(
      type_ == ValueType::Null || type_ == ValueType::UnsignedInteger,
      "It can be changed to unsigned integer type only if it is null type.");
  ChangeType_INTERNAL(ValueType::UnsignedInteger);
  unsigned_integer_value_ = value;
}

FUN_ALWAYS_INLINE void JValue::SetDouble(float value) {
  fun_check_msg(type_ == ValueType::Null || type_ == ValueType::Double,
                "It can be changed to double type only if it is null type.");
  ChangeType_INTERNAL(ValueType::Double);
  double_value_ = value;
}

FUN_ALWAYS_INLINE void JValue::SetDouble(double value) {
  fun_check_msg(type_ == ValueType::Null || type_ == ValueType::Double,
                "It can be changed to double type only if it is null type.");
  ChangeType_INTERNAL(ValueType::Double);
  double_value_ = value;
}

FUN_ALWAYS_INLINE void JValue::SetString() {
  fun_check_msg(type_ == ValueType::Null || type_ == ValueType::String,
                "It can be changed to string type only if it is null type.");
  ChangeType_INTERNAL(ValueType::String);
}

// FUN_ALWAYS_INLINE void JValue::SetString(ByteStringView value) {
//  fun_check_msg(type_ == ValueType::Null || type_ == ValueType::String,
//    "It can be changed to string type only if it is null type.");
//  ChangeType_INTERNAL(ValueType::String);
//  *string_value_ = value;
//}
//
// FUN_ALWAYS_INLINE void JValue::SetString(UStringView value) {
//  fun_check_msg(type_ == ValueType::Null || type_ == ValueType::String,
//    "It can be changed to string type only if it is null type.");
//  ChangeType_INTERNAL(ValueType::String);
//  *string_value_ = value;
//}
//
// FUN_ALWAYS_INLINE void JValue::SetString(AsciiString value) {
//  fun_check_msg(type_ == ValueType::Null || type_ == ValueType::String,
//    "It can be changed to string type only if it is null type.");
//  ChangeType_INTERNAL(ValueType::String);
//  *string_value_ = value;
//}

FUN_ALWAYS_INLINE void JValue::SetString(const String& value) {
  fun_check_msg(type_ == ValueType::Null || type_ == ValueType::String,
                "It can be changed to string type only if it is null type.");
  ChangeType_INTERNAL(ValueType::String);
  *string_value_ = value;
}

FUN_ALWAYS_INLINE JValue& JValue::SetArray() {
  fun_check_msg(type_ == ValueType::Null || type_ == ValueType::Array,
                "It can be changed to array type only if it is null type.");
  ChangeType_INTERNAL(ValueType::Array);
  return *this;
}

FUN_ALWAYS_INLINE JValue& JValue::SetArray(const JArray& value) {
  fun_check_msg(type_ == ValueType::Null || type_ == ValueType::Array,
                "It can be changed to array type only if it is null type.");
  ChangeType_INTERNAL(ValueType::Array);
  *array_value_ = value;
  return *this;
}

FUN_ALWAYS_INLINE JValue& JValue::SetObject() {
  fun_check_msg(type_ == ValueType::Null || type_ == ValueType::Object,
                "It can be changed to object type only if it is null type.");
  ChangeType_INTERNAL(ValueType::Object);
  return *this;
}

FUN_ALWAYS_INLINE JValue& JValue::SetObject(const JObject& value) {
  fun_check_msg(type_ == ValueType::Null || type_ == ValueType::Object,
                "It can be changed to object type only if it is null type.");
  ChangeType_INTERNAL(ValueType::Object);
  *object_value_ = value;
  return *this;
}

FUN_ALWAYS_INLINE void JValue::ChangeType_INTERNAL(ValueType new_type) {
  const ValueType old_type = type_;
  if (new_type != old_type) {
    FreeValue();

    type_ = new_type;

    switch (type_) {
      case ValueType::String:
        string_value_ = new String();
        break;
      case ValueType::Array:
        array_value_ = new JArray();
        break;
      case ValueType::Object:
        object_value_ = new JObject();
        break;
      default:
        object_value_ = nullptr;
        break;  // largest
    }
  }
}

FUN_ALWAYS_INLINE int32 JValue::Compare(const JValue& other) const {
  if (*this < other) {
    return -1;
  } else if (*this > other) {
    return 1;
  } else {
    return 0;
  }
}

FUN_ALWAYS_INLINE bool JValue::operator==(const JValue& other) const {
  if (type_ != other.type_) {
    return false;
  }

  switch (type_) {
    case ValueType::Null:
      return true;
    case ValueType::Integer:
      return integer_value_ == other.integer_value_;
    case ValueType::UnsignedInteger:
      return unsigned_integer_value_ == other.unsigned_integer_value_;
    case ValueType::Double:
      return double_value_ == other.double_value_;
    case ValueType::Bool:
      return bool_value_ == other.bool_value_;
    case ValueType::String:
      return *string_value_ == *other.string_value_;
    case ValueType::Array:
      return *array_value_ == *other.array_value_;
    case ValueType::Object:
      // TODO
      fun_check(0);
      return false;
      // return *object_value_ == *other.object_value_;
    default:
      fun_unexpected();
      return false;
  }
}

FUN_ALWAYS_INLINE bool JValue::operator!=(const JValue& other) const {
  return !(*this == other);
}

FUN_ALWAYS_INLINE bool JValue::operator<(const JValue& other) const {
  const int32 type_diff = (int32)type_ - (int32)other.type_;
  if (type_diff != 0) {
    return type_diff < 0;
  }

  switch (type_) {
    case ValueType::Null:
      return false;
    case ValueType::Integer:
      return integer_value_ < other.integer_value_;
    case ValueType::UnsignedInteger:
      return unsigned_integer_value_ < other.unsigned_integer_value_;
    case ValueType::Double:
      return double_value_ < other.double_value_;
    case ValueType::Bool:
      return bool_value_ < other.bool_value_;
    case ValueType::String:
      return *string_value_ < *other.string_value_;
    case ValueType::Array:
      // TODO
      // return *array_value_ < *other.array_value_;
      fun_check(0);
      return false;
    case ValueType::Object:
      // TODO
      // return *object_value_ < *other.object_value_;
      fun_check(0);
      return false;
  }

  fun_unexpected();
  return false;
}

FUN_ALWAYS_INLINE bool JValue::operator<=(const JValue& other) const {
  return !(other < *this);
}

FUN_ALWAYS_INLINE bool JValue::operator>(const JValue& other) const {
  return (other < *this);
}

FUN_ALWAYS_INLINE bool JValue::operator>=(const JValue& other) const {
  return !(*this < other);
}

FUN_ALWAYS_INLINE void JValue::SetComment(const char* comment, int32 len,
                                          CommentPlacement placement) {
  EnsureValidCommentPlacement(placement);

  if (comments_ == nullptr) {
    comments_ = new String[(int32)CommentPlacement::NumCommentPlacements];
  }
  comments_[(int32)placement] = String(comment, len);
}

FUN_ALWAYS_INLINE void JValue::SetComment(const String& comment,
                                          CommentPlacement placement) {
  EnsureValidCommentPlacement(placement);

  if (comments_ == nullptr) {
    comments_ = new String[(int32)CommentPlacement::NumCommentPlacements];
  }
  comments_[(int32)placement] = comment;
}

FUN_ALWAYS_INLINE bool JValue::HasComment(CommentPlacement placement) const {
  EnsureValidCommentPlacement(placement);
  return (comments_ && comments_[(int32)placement].Len() > 0);
}

FUN_ALWAYS_INLINE String JValue::GetComment(CommentPlacement placement) const {
  EnsureValidCommentPlacement(placement);
  return comments_ ? comments_[(int32)placement] : String();
}

FUN_ALWAYS_INLINE void JValue::EnsureValidCommentPlacement(
    const CommentPlacement placement) const {
  const int32 index = (int32)placement;
  fun_check(index >= 0 &&
            index < (int32)CommentPlacement::NumCommentPlacements);
}

FUN_ALWAYS_INLINE void JValue::SetOffsetStart(ptrdiff_t start) {
  start_ = start;
}

FUN_ALWAYS_INLINE void JValue::SetOffsetLimit(ptrdiff_t limit) {
  limit_ = limit;
}

FUN_ALWAYS_INLINE ptrdiff_t JValue::GetOffsetStart() const { return start_; }

FUN_ALWAYS_INLINE ptrdiff_t JValue::GetOffsetLimit() const { return limit_; }

}  // namespace json
}  // namespace fun
