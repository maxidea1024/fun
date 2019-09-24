#pragma once

#include "fun/mongodb/element.h"

namespace fun {
namespace mongodb {

typedef SharedPtr<class ObjectId> ObjectIdPtr;

/**
 * ObjectId is a 12-byte BSON type, constructed using:
 *
 *   - a 4-byte timestamp,
 *   - a 3-byte machine identifier,
 *   - a 2-byte process id, and
 *   - a 3-byte counter, starting with a random value.
 */
class FUN_MONGODB_API ObjectId {
 public:
  explicit ObjectId(const String& id) {
    fun_check(id.Len() == 24);

    const uint8* src = id.ConstData();
    for (int32 i = 0; i < 12; ++i) {
      id_[i] = FromHex(src);
      src += 2;
    }
  }

  ObjectId(const ObjectId& rhs) {
    UnsafeMemory::Memcpy(id_, rhs.id_, sizeof(id_));
  }

  virtual ~ObjectId() {}

  DateTime GetTimestamp() const {
    int32 time;
    char* bytes = (char*)&time;
    bytes[0] = id_[3];
    bytes[1] = id_[2];
    bytes[2] = id_[1];
    bytes[3] = id_[0];
    // TODO
    return DateTime::FromUtcEpoch(time_t)time);
  }

  String ToString(const String& fmt = "%02x") const {
    String ret;
    for (int32 i = 0; i < 12; ++i) {
      ret += String::Format(fmt, (uint32)id_[i]);
    }
    return ret;
  }

 private:
  ObjectId() { UnsafeMemory::Memset(id_, 0x00, sizeof(id_)); }

  static int32 FromHex(char c) {
    if ('0' <= c && c <= '9') {
      return c - '0';
    } else if ('a' <= c && c <= 'f') {
      return c - 'a' + 10;
    } else if ('A' <= c && c <= 'F') {
      return c - 'A' + 10;
    } else {
      return 0xff;
    }
  }

  static char FromHex(const char* c) {
    return (char)((FromHex(c[0]) << 4) | FromHex(c[1]));
  }

  uint8 id_[12];

  friend class BsonWriter;
  friend class BsonReader;
  friend class Document;
};

template <>
struct ElementTraits<ObjectIdPtr> {
  enum { TypeId = 0x7 };

  static String ToString(const ObjectIdPtr& id, int32 indent = 0,
                         const String& fmt = "%02x") {
    return id->ToString(fmt);
  }
};

template <>
inline void BsonReader::Read<ObjectIdPtr>(ObjectIdPtr& out_value) {
  reader_.ReadRawBytes((char*)out_value.id_, 12);
}

template <>
inline void BsonWriter::Write<ObjectIdPtr>(ObjectIdPtr& value) {
  writer_.WriteRawBytes((const char*)value.id_, 12);
}

}  // namespace mongodb
}  // namespace fun
