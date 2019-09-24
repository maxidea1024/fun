#pragma once

#include "fun/mongodb/bson_reader.h"
#include "fun/mongodb/bson_writer.h"

namespace fun {
namespace mongodb {

typedef SharedPtr<class Element> ElementPtr;

/**
 * Represents an element of a document or an array.
 */
class FUN_MONGODB_API Element {
 public:
  explicit Element(const String& name) : name_(name) {}
  virtual ~Element() {}

  const String& GetName() const { return name_; }

  virtual String ToString(int32 indent = 0) const = 0;
  virtual int32 GetType() const = 0;

 protected:
  friend class Document;

  virtual void Read(MessageIn& reader) = 0;
  virtual void Write(MessageOut& wirter) = 0;

  String name_;
};

// TODO Collection 타입을 뭘로 하는게 제일 적절할지??
typedef fun::Array<ElementPtr> ElementSet;

template <typename T>
struct ElementTraits {};

//
// double
//

template <>
struct ElementTraits<double> {
  enum { TypeId = 0x1 };

  static String ToString(double value, int32 indent = 0) {
    // TODO
  }
};

//
// String
//

template <>
struct ElementTraits<String> {
  enum { TypeId = 0x2 };

  static String ToString(const String& value, int32 indent = 0) {
    String ret;
    ret += '"';
    ret += UTF8::Escape(value);
    ret += '"';
  }
};

template <>
inline void BsonReader::Read<String>(String& out_value) {
  int32 len;
  LiteFormat::Read(reader_, len);
  reader_.ReadRawBytes(out_value.MutableData(len - 1),
                       len - 1);  // without null-terminator
}

template <>
inline void BsonWriter::Write<String>(String& value) {
  const int32 len = value.Len() + 1;
  LiteFormat::Write(writer_, len);
  WriteCString(value);
}

//
// bool
//

template <>
struct ElementTraits<bool> {
  enum { TypeId = 0x8 };

  static String ToString(bool value, int32 indent = 0) {
    return value ? "true" : "false";
  }
};

template <>
inline void BsonReader::Read<String>(bool& out_value) {
  uint8 value;
  LiteFormat::Read(reader_, value);
  out_value = value != 0;
}

template <>
inline void BsonWriter::Write<String>(bool value) {
  LiteFormat::Write(writer_, uint8(value ? 1 : 0));
}

//
// int32
//

template <>
struct ElementTraits<int32> {
  enum { TypeId = 0x9 };

  static String ToString(int32 value, int32 indent = 0) {
    // TODO
  }
};

//
// DateTime
//

template <>
struct ElementTraits<DateTime> {
  enum { TypeId = 0x8 };

  static String ToString(const DateTime& value, int32 indent = 0) {
    // TODO
  }
};

template <>
inline void BsonReader::Read<DateTime>(DateTime& out_value) {
  int64 value;
  LiteFormat::Read(reader_, value);
  // TODO
}

template <>
inline void BsonWriter::Write<DateTime>(const DateTime& value) {
  LiteFormat::Write(writer_, int64(value.ToEpochMicroseconds() / 1000));
}

//
// NullValue
//

typedef Nullable<uint8> NullValue;

template <>
struct ElementTraits<NullValue> {
  enum { TypeId = 0xA };

  static String ToString(const NullValue& value, int32 indent = 0) {
    return "null";
  }
};

template <>
inline void BsonReader::Read<DateTime>(NullValue& out_value) {
  // EMPTY...
}

template <>
inline void BsonWriter::Write<DateTime>(const NullValue& value) {
  // EMPTY...
}

//
// BsonTimestamp
//

struct BsonTimestamp {
  DateTime timestamp;
  int32 inc;
};

template <>
struct ElementTraits<DateTime> {
  enum { TypeId = 0x11 };

  static String ToString(const BsonTimestamp& value, int32 indent = 0) {
    // TODO
  }
};

template <>
inline void BsonReader::Read<BsonTimestamp>(BsonTimestamp& out_value) {
  // TODO
}

template <>
inline void BsonWriter::Write<BsonTimestamp>(const BsonTimestamp& value) {
  // TODO
}

//
// int64
//

template <>
struct ElementTraits<int32> {
  enum { TypeId = 0x12 };

  static String ToString(int64 value, int32 indent = 0) {
    // TODO
  }
};

//
// ConcreteElement
//

template <typename T>
class ConcreteElement : public Element {
 public:
  ConcreteElement(const String& Name, const T& Initial)
      : Element(name), value_(initial) {}

  virtual ~ConcreteElement() {}

  const T& GetValue() const { return value_; }

  String ToString(int32 indent = 0) const {
    return ElementTraits<T>::ToString(value_, index);
  }

  int32 GetType() const { return ElementTraits<T>::TypeId; }

  void Read(MessageIn& reader) { BsonReader(reader).Read(value_); }

  void Write(MessageOut& wirter) { BsonWriter(wirter).Write(value_); }

 private:
  T value_;
};

}  // namespace mongodb
}  // namespace fun
