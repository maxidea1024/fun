#include "fun/base/variant.h"

namespace fun {

int16 Variant::ToInt16() const {
  switch (type_) {
    case VariantTypes::Empty:  return 0;
    case VariantTypes::Int16:  return (int16)Value<int16>();
    case VariantTypes::UInt16: return (int16)Value<uint16>();
    case VariantTypes::Int32:  return (int16)Value<int32>();
    case VariantTypes::UInt32: return (int16)Value<uint32>();
    case VariantTypes::Int64:  return (int16)Value<int32>();
    case VariantTypes::UInt64: return (int16)Value<uint32>();
    case VariantTypes::Float:  return (int16)Value<float>();
    case VariantTypes::Double: return (int16)Value<double>();
  }
  return 0;
}

uint16 Variant::ToUInt16() const {
  switch (type_) {
    case VariantTypes::Empty:  return 0;
    case VariantTypes::Int16:  return (uint16)Value<int16>();
    case VariantTypes::UInt16: return (uint16)Value<uint16>();
    case VariantTypes::Int32:  return (uint16)Value<int32>();
    case VariantTypes::UInt32: return (uint16)Value<uint32>();
    case VariantTypes::Int64:  return (uint16)Value<int64>();
    case VariantTypes::UInt64: return (uint16)Value<uint64>();
    case VariantTypes::Float:  return (uint16)Value<float>();
    case VariantTypes::Double: return (uint16)Value<double>();
  }
  return 0;
}

int32 Variant::ToInt32() const {
  switch (type_) {
    case VariantTypes::Empty:  return 0;
    case VariantTypes::Int16:  return (int32)Value<int16>();
    case VariantTypes::UInt16: return (int32)Value<uint16>();
    case VariantTypes::Int32:  return (int32)Value<int32>();
    case VariantTypes::UInt32: return (int32)Value<uint32>();
    case VariantTypes::Int64:  return (int32)Value<int64>();
    case VariantTypes::UInt64: return (int32)Value<uint64>();
    case VariantTypes::Float:  return (int32)Value<float>();
    case VariantTypes::Double: return (int32)Value<double>();
  }
  return 0;
}

uint32 Variant::ToUInt32() const {
  switch (type_) {
    case VariantTypes::Empty:  return 0;
    case VariantTypes::Int16:  return (uint32)Value<int16>();
    case VariantTypes::UInt16: return (uint32)Value<uint16>();
    case VariantTypes::Int32:  return (uint32)Value<int32>();
    case VariantTypes::UInt32: return (uint32)Value<uint32>();
    case VariantTypes::Int64:  return (uint32)Value<int64>();
    case VariantTypes::UInt64: return (uint32)Value<uint64>();
    case VariantTypes::Float:  return (uint32)Value<float>();
    case VariantTypes::Double: return (uint32)Value<double>();
  }
  return 0;
}

int64 Variant::ToInt64() const {
  switch (type_) {
    case VariantTypes::Empty:  return 0;
    case VariantTypes::Int16:  return (int64)Value<int16>();
    case VariantTypes::UInt16: return (int64)Value<uint16>();
    case VariantTypes::Int32:  return (int64)Value<int32>();
    case VariantTypes::UInt32: return (int64)Value<uint32>();
    case VariantTypes::Int64:  return (int64)Value<int64>();
    case VariantTypes::UInt64: return (int64)Value<uint64>();
    case VariantTypes::Float:  return (int64)Value<float>();
    case VariantTypes::Double: return (int64)Value<double>();
  }
  return 0;
}

uint64 Variant::ToUInt64() const {
  switch (type_) {
    case VariantTypes::Empty:  return 0;
    case VariantTypes::Int16:  return (uint64)Value<int16>();
    case VariantTypes::UInt16: return (uint64)Value<uint16>();
    case VariantTypes::Int32:  return (uint64)Value<int32>();
    case VariantTypes::UInt32: return (uint64)Value<uint32>();
    case VariantTypes::Int64:  return (uint64)Value<int64>();
    case VariantTypes::UInt64: return (uint64)Value<uint64>();
    case VariantTypes::Float:  return (uint64)Value<float>();
    case VariantTypes::Double: return (uint64)Value<double>();
  }
  return 0;
}

float Variant::ToFloat() const {
  switch (type_) {
    case VariantTypes::Empty:  return 0;
    case VariantTypes::Int16:  return (float)Value<int16>();
    case VariantTypes::UInt16: return (float)Value<uint16>();
    case VariantTypes::Int32:  return (float)Value<int32>();
    case VariantTypes::UInt32: return (float)Value<uint32>();
    case VariantTypes::Int64:  return (float)Value<int64>();
    case VariantTypes::UInt64: return (float)Value<uint64>();
    case VariantTypes::Float:  return (float)Value<float>();
    case VariantTypes::Double: return (float)Value<double>();
  }
  return 0;
}

double Variant::ToDouble() const {
  switch (type_) {
    case VariantTypes::Empty:  return 0;
    case VariantTypes::Int16:  return (double)Value<int16>();
    case VariantTypes::UInt16: return (double)Value<uint16>();
    case VariantTypes::Int32:  return (double)Value<int32>();
    case VariantTypes::UInt32: return (double)Value<uint32>();
    case VariantTypes::Int64:  return (double)Value<int64>();
    case VariantTypes::UInt64: return (double)Value<uint64>();
    case VariantTypes::Float:  return (double)Value<float>();
    case VariantTypes::Double: return (double)Value<double>();
  }
  return 0;
}

bool Variant::ToBool() const {
  return ToInt16() != 0;
}


//
// TODO 인코딩 변환을 처리해주면 좋을듯도...
//


String Variant::ToString() const {
  //TODO 변환관련한 부분은 신경쓰지 않음?
  return (type_ == VariantTypes::String) ? Value<String>() : String();
}

UString Variant::ToUString() const {
  //TODO 변환관련한 부분은 신경쓰지 않음?
  return (type_ == VariantTypes::UString) ? Value<UString>() : UString();
}

} // namespace fun
