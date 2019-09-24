//TODO C++ 14 형태로 지원하는게 좋을듯 싶은데...

#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/type_traits.h"
#include "fun/base/container/array.h"
#include "fun/base/container/map.h"
#include "fun/base/container/set.h"
#include "fun/base/string/string.h"
#include "fun/base/serialization/archives.h"

namespace fun {

class Box;
class BoxSphereBounds;
class Vector2;
class Vector;
class Vector4;
class Quat;
class Rotator;
class IntPoint;
class IntVector;
class IntRect;
class TwoVectors;
class LinearColor;
class Transform;
class Matrix;
class Plane;
class Color;
class Time;
class Date;
class DateTime;
class Timespan;
class Timestamp;
class Uuid;
class Uri;

namespace VariantTypes {
  const int32 Empty = 0;
  //const int32 AnsiChar = 1;
  const int32 Bool = 2;
  const int32 Box = 3;
  const int32 BoxSphereBounds = 4;
  const int32 ByteArray = 5;
  const int32 Color = 6;
  const int32 DateTime = 7;
  const int32 Double = 8;
  const int32 Enum = 9;
  const int32 Float = 10;
  const int32 Uuid = 11;
  const int32 Int8 = 12;
  const int32 Int16 = 13;
  const int32 Int32 = 14;
  const int32 Int64 = 15;
  const int32 IntRect = 16;
  const int32 LinearColor = 17;
  const int32 Matrix = 18;
  const int32 Name = 19;
  const int32 Plane = 20;
  const int32 Quat = 21;
  const int32 RandomStream = 22;
  const int32 Rotator = 23;
  const int32 String = 24;
  const int32 UString = 25;
  const int32 Char = 26;
  const int32 UniChar = 27;
  const int32 Timespan = 28;
  const int32 Transform = 29;
  const int32 TwoVectors = 30;
  const int32 UInt8 = 31;
  const int32 UInt16 = 32;
  const int32 UInt32 = 33;
  const int32 UInt64 = 34;
  const int32 Vector = 35;
  const int32 Vector2d = 36;
  const int32 Vector4 = 37;
  const int32 IntPoint = 38;
  const int32 IntVector = 39;
  const int32 NetworkUUID = 40;

  const int32 Date = 41;
  const int32 Time = 42;

  const int32 Uri = 43;

  const int32 CharArray = 44;
  const int32 UniCharArray = 45;

  const int32 Int8Array = 46;
  const int32 Int16Array = 47;
  const int32 Int32Array = 48;
  const int32 Int64Array = 49;

  //const int32 UInt8Array = 50;
  const int32 UInt16Array = 51;
  const int32 UInt32Array = 52;
  const int32 UInt64Array = 53;

  const int32 FloatArray = 54;
  const int32 DoubleArray = 55;

  const int32 BoolArray = 56;

  const int32 StringArray = 57;
  const int32 UStringArray = 58;

  const int32 VariantArray = 59;
  const int32 Map = 60;

  const int32 Custom = 0x50;
} // namespace VariantTypes

template <typename T>
struct VariantTraits {
  static const int32 Type = -1;
};

template <typename T>
struct VariantTraitsCall {
  static int32 Type() {
    const int32 resolved_type = VariantTraits<T>::Type;
    static_assert(resolved_type != -1, "Variant trait must be specialized for this type.");
    return resolved_type;
  }
};

class FUN_BASE_API Variant {
 public:
  Variant() : type_(VariantTypes::Empty) {}

  template <typename T>
  Variant(T value, typename EnableIf<!IsEnum<T>::Value,T>::Type* = nullptr) {
    MemoryWriter writer(value_, true);
    writer & value;
    type_ = VariantTraitsCall<T>::Type();
  }

  template <typename T>
  Variant(T value, typename EnableIf<IsEnum<T>::Value,T>::Type* = nullptr) {
    MemoryWriter writer(value_, true);
    int32 i = (int32)value;
    writer & i;
    type_ = VariantTypes::Enum;
  }

  Variant(const Array<uint8>& byte_array)
    : value_(byte_array),
      type_(VariantTypes::ByteArray) {}

  Variant(const Variant& rhs)
    : type_(rhs.type_),
      value_(rhs.value_) {}

  Variant& operator = (const Variant& rhs) {
    if (FUN_LIKELY(&rhs != this)) {
      value_ = rhs.value_;
      type_ = rhs.type_;
    }

    return *this;
  }

  template <typename T>
  Variant& operator = (T value) {
    MemoryWriter writer(value_, true);
    writer & value;
    type_ = VariantTraitsCall<T>::Type();
    return *this;
  }

  Variant& operator = (const Array<uint8>& byte_array) {
    value_ = byte_array;
    type_ = VariantTypes::ByteArray;
    return *this;
  }

  Variant& operator = (const char* str) {
    *this = String(str);
    return *this;
  }

  template <typename T>
  operator T() const {
    return Value<T>();
  }

  bool operator == (const Variant& rhs) const {
    return type_ == rhs.type_ && value_ == rhs.value_;
  }

  bool operator != (const Variant& rhs) const {
    return type_ != rhs.type_ || value_ != rhs.value_;
  }

  void SetNull() {
    type_ = VariantTypes::Empty;
    value_.Clear();
  }

  bool IsNull() const {
    return type_ == VariantTypes::Empty;
  }

  const Array<uint8>& ByteArray() const {
    return value_;
  }

  int32 Len() const {
    return value_.Count();
  }

  int32 Type() const {
    return type_;
  }

  template <typename T>
  typename EnableIf<IsEnum<T>::Value, T>::Type Value() const {
    fun_check(type_ == VariantTypes::Enum);

    int32 tmp;
    MemoryReader reader(value_, true);
    reader & tmp;
    return (T)tmp;
  }

  template <typename T>
  typename EnableIf<!IsEnum<T>::Value, T>::Type Value() const {
    //fun_check((type_ == VariantTraitsCall<T>::Type()) || ((VariantTraitsCall<T>::Type() == VariantTypes::UInt8) && (type_ == VariantTypes::Enum)));
    fun_check(type_ == VariantTraitsCall<T>::Type());

    T ret;
    MemoryReader reader(value_, true);
    reader & ret;
    return ret;
  }

  int16 ToInt16() const;
  uint16 ToUInt16() const;
  int32 ToInt32() const;
  uint32 ToUInt32() const;
  int64 ToInt64() const;
  uint64 ToUInt64() const;
  float ToFloat() const;
  double ToDouble() const;
  bool ToBool() const;
  String ToString() const;
  UString ToUString() const;

  friend Archive& operator & (Archive& ar, Variant& v) {
    return ar & v.type_ & v.value_;
  }

 private:
  int32 type_;
  Array<uint8> value_;
};


//
// inlines
//

template <>
inline Array<uint8> Variant::Value<Array<uint8>>() const {
  fun_check(type_ == VariantTypes::ByteArray);
  return value_;
}

template <> struct VariantTraits<bool> {
  static const int32 Type = VariantTypes::Bool;
};

template <> struct VariantTraits<Box> {
  static const int32 Type = VariantTypes::Box;
};

template <> struct VariantTraits<BoxSphereBounds> {
  static const int32 Type = VariantTypes::BoxSphereBounds;
};

template <> struct VariantTraits<Color> {
  static const int32 Type = VariantTypes::Color;
};

template <> struct VariantTraits<DateTime> {
  static const int32 Type = VariantTypes::DateTime;
};

template <> struct VariantTraits<Date> {
  static const int32 Type = VariantTypes::Date;
};

template <> struct VariantTraits<Time> {
  static const int32 Type = VariantTypes::Time;
};

template <> struct VariantTraits<double> {
  static const int32 Type = VariantTypes::Double;
};

//template <typename EnumType>
//struct VariantTraits<EnumAsByte<EnumType>>
//{
//  static const int32 Type = VariantTypes::Enum;
//};

template <> struct VariantTraits<float> {
  static const int32 Type = VariantTypes::Float;
};

template <> struct VariantTraits<Uuid> {
  static const int32 Type = VariantTypes::Uuid;
};

template <> struct VariantTraits<int8> {
  static const int32 Type = VariantTypes::Int8;
};

template <> struct VariantTraits<int16> {
  static const int32 Type = VariantTypes::Int16;
};

template <> struct VariantTraits<int32> {
  static const int32 Type = VariantTypes::Int32;
};

template <> struct VariantTraits<int64> {
  static const int32 Type = VariantTypes::Int64;
};

template <> struct VariantTraits<IntPoint> {
  static const int32 Type = VariantTypes::IntPoint;
};

template <> struct VariantTraits<IntVector> {
  static const int32 Type = VariantTypes::IntVector;
};

template <> struct VariantTraits<IntRect> {
  static const int32 Type = VariantTypes::IntRect;
};

template <> struct VariantTraits<LinearColor> {
  static const int32 Type = VariantTypes::LinearColor;
};

template <> struct VariantTraits<Matrix> {
  static const int32 Type = VariantTypes::Matrix;
};

template <> struct VariantTraits<Plane> {
  static const int32 Type = VariantTypes::Plane;
};

template <> struct VariantTraits<Quat> {
  static const int32 Type = VariantTypes::Quat;
};

//template <> struct VariantTraits<Name> {
//  static const int32 Type = VariantTypes::Name;
//};

//template <> struct VariantTraits<RandomStream> {
//  static const int32 Type = VariantTypes::RandomStream;
//};

template <> struct VariantTraits<Rotator> {
  static const int32 Type = VariantTypes::Rotator;
};

template <> struct VariantTraits<String> {
  static const int32 Type = VariantTypes::String;
};

template <> struct VariantTraits<char> {
  static const int32 Type = VariantTypes::Char;
};

template <> struct VariantTraits<UString> {
  static const int32 Type = VariantTypes::UString;
};

template <> struct VariantTraits<UNICHAR> {
  static const int32 Type = VariantTypes::UniChar;
};

template <> struct VariantTraits<Timespan> {
  static const int32 Type = VariantTypes::Timespan;
};

template <> struct VariantTraits<Transform> {
  static const int32 Type = VariantTypes::Transform;
};

template <> struct VariantTraits<TwoVectors> {
  static const int32 Type = VariantTypes::TwoVectors;
};

template <> struct VariantTraits<uint8> {
  static const int32 Type = VariantTypes::UInt8;
};

template <> struct VariantTraits<uint16> {
  static const int32 Type = VariantTypes::UInt16;
};

template <> struct VariantTraits<uint32> {
  static const int32 Type = VariantTypes::UInt32;
};

template <> struct VariantTraits<uint64> {
  static const int32 Type = VariantTypes::UInt64;
};

template <> struct VariantTraits<Vector> {
  static const int32 Type = VariantTypes::Vector;
};

template <> struct VariantTraits<Vector2> {
  static const int32 Type = VariantTypes::Vector2d;
};

template <> struct VariantTraits<Vector4> {
  static const int32 Type = VariantTypes::Vector4;
};

//template <> struct VariantTraits<NetworkUUID> {
//  static const int32 Type = VariantTypes::NetworkUUID;
//};

template <> struct VariantTraits<Uri> {
  static const int32 Type = VariantTypes::Uri;
};

template <> struct VariantTraits<Array<char>> {
  static const int32 Type = VariantTypes::CharArray;
};

template <> struct VariantTraits<Array<UNICHAR>> {
  static const int32 Type = VariantTypes::UniCharArray;
};

template <> struct VariantTraits<Array<int8>> {
  static const int32 Type = VariantTypes::Int8Array;
};

template <> struct VariantTraits<Array<int16>> {
  static const int32 Type = VariantTypes::Int16Array;
};

template <> struct VariantTraits<Array<int32>> {
  static const int32 Type = VariantTypes::Int32Array;
};

template <> struct VariantTraits<Array<int64>> {
  static const int32 Type = VariantTypes::Int64Array;
};

template <> struct VariantTraits<Array<uint8>> {
  static const int32 Type = VariantTypes::ByteArray;
};

template <> struct VariantTraits<Array<uint16>> {
  static const int32 Type = VariantTypes::UInt16Array;
};

template <> struct VariantTraits<Array<uint32>> {
  static const int32 Type = VariantTypes::UInt32Array;
};

template <> struct VariantTraits<Array<uint64>> {
  static const int32 Type = VariantTypes::UInt64Array;
};

template <> struct VariantTraits<Array<float>> {
  static const int32 Type = VariantTypes::FloatArray;
};

template <> struct VariantTraits<Array<double>> {
  static const int32 Type = VariantTypes::DoubleArray;
};

template <> struct VariantTraits<Array<bool>> {
  static const int32 Type = VariantTypes::BoolArray;
};

template <> struct VariantTraits<Array<String>> {
  static const int32 Type = VariantTypes::StringArray;
};

template <> struct VariantTraits<Array<UString>> {
  static const int32 Type = VariantTypes::UStringArray;
};

template <> struct VariantTraits<Array<Variant>> {
  static const int32 Type = VariantTypes::VariantArray;
};

template <> struct VariantTraits<Map<UString, Variant>> {
  static const int32 Type = VariantTypes::Map;
};

} // namespace fun
