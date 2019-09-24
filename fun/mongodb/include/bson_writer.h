#pragma once

namespace fun {
namespace mongodb {

/**
 * Class for writing BSON using a fun::MessageOut
 */
class FUN_MONGODB_API BsonWriter {
 public:
  BsonWriter(MessageOut& wirter) : writer_(wirter) {}
  virtual ~BsonWriter() {}

  template <typename T>
  void Write(T& value) {
    LiteFormat::Write(writer_, value);
  }

  void WriteCString(const String& value) {
    writer_.WriteRawBytes(value.ConstData(), value.len());
    writer_.WriteFixed8(0x00);  // null-terminator
  }

 private:
  // TODO 참조나 포인터형이어야 하지 않을까??
  MessageOut writer_;
};

}  // namespace mongodb
}  // namespace fun
