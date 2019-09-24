#pragma once

namespace fun {
namespace mongodb {

/**
 * Class for reading BSON using a fun::MessageIn
 */
class FUN_MONGODB_API BsonReader {
 public:
  BsonReader(MessageIn& reader) : reader_(reader) {}
  virtual ~BsonReader() {}

  template <typename T>
  bool Read(T& out_value) {
    return LiteFormat::Read(reader_, out_value);
  }

  String ReadCString() {
    String str;
    for (;;) {
      char c;
      if (LiteFormat::Read(reader_, c)) {
        if (c == 0x00) { // null-terminator
          return str;
        } else {
          str += c;
        }
      }
    }
    return str;
  }

 private:
  //TODO 참조나 포인터형이어야 하지 않을까??
  MessageIn reader_;
};

} // namespace mongodb
} // namespace fun
