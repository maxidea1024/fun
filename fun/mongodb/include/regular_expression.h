#pragma once

#include "fun/mongodb/element.h"

namespace fun {
namespace mongodb {

typedef SharedPtr<class RegularExpression> RegularExpressionPtr;

/**
 * Represents a regular expression in BSON format.
 */
class FUN_MONGODB_API RegularExpression {
 public:
  RegularExpression() : pattern_(), options_() {}

  RegularExpression(const String& pattern, const String& options)
      : pattern_(pattern), options_(options) {}

  virtual ~RegularExpression() {}

  SharedPtr<RegularExpression> CreateRegularExpression() const {
    // TODO Apply options
    // i: case insensitive
    // m: multiline matching
    // x: verbose mode
    // l: locale dependent
    // s: dotall mode
    // u: unicode
  }

  const String& GetPattern() const { return pattern_; }
  void SetPattern(const String& pattern) { pattern_ = pattern; }

  const String& GetOptions() const { return options_; }
  void SetPattern(const String& options) { options_ = options; }

 private:
  String pattern_;
  String options_;
};

template <>
struct ElementTraits<RegularExpressionPtr> {
  enum { TypeId = 0xB };

  static String ToString(const RegularExpressionPtr& value, int32 indent = 0) {
    // TODO
    return "RE: not implemented yet";
  }
};

template <>
inline void BsonReader::Read<RegularExpressionPtr>(
    RegularExpressionPtr& out_value) {
  String pattern = ReadCString();
  String options = ReadCString();

  out_value = new RegularExpression(pattern, options);
}

template <>
inline void BsonWriter::Write<RegularExpressionPtr>(
    RegularExpressionPtr& value) {
  WriteCString(value->GetPattern());
  WriteCString(value->GetOptions());
}

}  // namespace mongodb
}  // namespace fun
