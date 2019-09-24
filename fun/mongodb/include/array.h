#pragma once

#include "fun/mongodb/document.h"

namespace fun {
namespace mongodb {

typedef SharedPtr<class Array> ArrayPtr;

/**
 * This class represents a BSON Array.
 */
class FUN_MONGODB_API Array : public Document {
 public:
  Array() : Document() {}

  //virtual ~Array()
  //{
  //}

  template <typename T>
  T Get(int32 index) const {
    return Document::Get<T>(String::FromUInt32(index));
  }

  template <typename T>
  T Get(int32 index, const T& default_value) const {
    return Document::Get<T>(String::FromUInt32(index), default_value);
  }

  ElementPtr Get(int32 index) const {
    return Document::Get(String::FromUInt32(index));
  }

  int64 GetInteger(int32 index) const {
    return Document::GetInteger(String::FromUInt32(index));
  }

  template <typename T>
  bool IsType(int32 index) const {
    return Document::IsType<T>(String::FromUInt32(index));
  }

  String ToString(int32 indent = 0) const {
    //TODO
    String ret;
    return ret;
  }
};


//
// inlines
//

template <>
inline void BsonReader::Read<ArrayPtr>(ArrayPtr& out_value) {
  out_value->Read(reader_);
}

template <>
inline void BsonWriter::Write<ArrayPtr>(ArrayPtr& value) {
  value->Write(writer_);
}

} // namespace mongodb
} // namespace fun
