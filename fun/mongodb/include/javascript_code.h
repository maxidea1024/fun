#pragma once

#include "fun/mongodb/element.h"

namespace fun {
namespace mongodb {

typedef SharedPtr<class JavascriptCode> JavascriptCodePtr;

/**
 * Represents JavaScript type in BSON.
 */
class FUN_MONGODB_API JavascriptCode {
 public:
  JavascriptCode() : code_() {}
  JavascriptCode(const String& code) : code_(code) {}
  virtual ~JavascriptCode() {}

  const String& GetCode() const { return code_; }
  void SetCode(const String& code) { code_ = code; }

 private:
  String code_;
};

template <>
struct ElementTraits<JavascriptCodePtr> {
  enum { TypeId = 0xD };

  static String ToString(const JavascriptCodePtr& value, int32 indent = 0) {
    return value.IsValid() ? value->GetCode() : "";
  }
};

template <>
inline void BsonReader::Read<JavascriptCodePtr>(JavascriptCodePtr& out_value) {
  out_value->Read(reader_);

  String code;
  BsonReader(reader_).Read(code);
  out_value = new JavascriptCode(code);
}

template <>
inline void BsonWriter::Write<JavascriptCodePtr>(JavascriptCodePtr& value) {
  BsonWriter(writer_).Write(value->GetCode());
}

}  // namespace mongodb
}  // namespace fun
