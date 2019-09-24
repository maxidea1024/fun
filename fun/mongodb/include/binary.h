#pragma once

#include "fun/mongodb/element.h"

namespace fun {
namespace mongodb {

typedef SharedPtr<class Binary> BinaryPtr;

/**
 * Implements BSON Binary.
 *
 * A Binary stores its data in a fun::Array<uint8>.
 */
class FUN_MONGODB_API Binary {
 public:
  Binary() : buffer_(), sub_type_(0) {}

  Binary(int32 size, uint8 sub_type) : buffer_(size, NoInit), sub_type_(sub_type) {}

  Binary(const Uuid& uuid) : buffer_(16, NoInit), sub_type_(0x4) {
    //TODO
  }

  Binary(const String& data, uint8 sub_type = 0)
    : buffer_(data.ConstData(), data.len()),
      sub_type_(sub_type) {
  }

  Binary(const void* data, int32 size, uint8 sub_type = 0)
    : buffer_(data, size),
      sub_type_(sub_type) {
  }

  virtual ~Binary() {}

  fun::Array<uint8>& GetBuffer() {
    return buffer_;
  }

  uint8 GetSubType() const {
    return sub_type_;
  }

  void SetSubType(uint8 sub_type) {
    sub_type_ = sub_type;
  }

  String ToString(int32 indent = 0) const {
    //TODO base64 string으로 변환하면 됨...
  }

  String ToRawString() const {
    //TODO hex string으로 변환하면 됨...
  }

  Uuid ToUuid() const {
    if (sub_type == 0x4 && buffer_.Count() == 16) {
      //TODO
    }

    throw BadCastException("invalid subtype");
  }

 private:
  fun::Array<uint8> buffer_;
  uint8 sub_type_;
};


template <>
struct ElementTraits<BinaryPtr> {
  enum { TypeId = 0x5 };

  static String ToString(const BinaryPtr& value, int32 indent = 0) {
    return value.IsValid() ? value->ToString() : "";
  }
};


template <>
inline void BsonReader::Read<BinaryPtr>(BinaryPtr& out_value) {
  int32 size;
  LiteFormat::Read(reader_, size);

  out_value->GetBuffer().ResizeUninitialized(size);

  uint8 sub_type;
  LiteFormat::Read(reader_, sub_type);
  out_value->SetSubType(sub_type);

  reader_.ReadRawBytes(out_value->GetBuffer().MutableData(), size);
}


template <>
inline void BsonWriter::Write<BinaryPtr>(BinaryPtr& value) {
  LiteFormat::Write(writer_, value->GetBuffer().Count());
  LiteFormat::Write(writer_, value->GetSubType());
  writer_.WriteRawBytes(value->GetBuffer().ConstData(), value->GetBuffer().Count());
}

} // namespace mongodb
} // namespace fun
