// TODO 내부에서만 사용되므로, export할 필요는 없어보임!

#pragma once

#include "fun/json/json.h"
#include "fun/json/value.h"

namespace fun {
namespace json {

class FUN_JSON_API IWriter {
 public:
  virtual ~IWriter() {}

  virtual String Write(const JValue& value) = 0;
};

class FUN_JSON_API CondensedWriter : public IWriter {
 public:
  CondensedWriter();

  void EnableYAMLCompatibility();
  void DropNullPlaceholders();
  void OmitEndingLineFeed();

  // IWriter interface
  String Write(const JValue& root) override;

 private:
  void WriteValue(const JValue& value);

  String document_;
  bool yaml_compatibility_enabled_;
  bool drop_null_placeholders_;
  bool omit_ending_linefeed_;
};

class FUN_JSON_API PrettyWriter : public IWriter {
 public:
  PrettyWriter();

  // IWriter interface
  String Write(const JValue& root) override;

  void WriteValue(const JValue& value);
  void WriteArrayValue(const JValue& value);
  void WriteObjectValue(const JValue& value);
  bool IsMultineArray(const JValue& value);
  void PushValue(const String& value);
  void WriteIndent();
  void WriteWithIndent(const String& value);
  void Indent();
  void Outdent();
  void WriteCommentBeforeValue(const JValue& root);
  void WriteCommentAfterValueOnSameLine(const JValue& root);
  bool HasCommentForValue(const JValue& value) const;

  typedef Array<String> ChildValues;

  ChildValues child_values_;
  String doument_;
  String indent_string_;
  int32 right_margin_;
  int32 indent_size_;
  bool add_child_values_;
};

}  // namespace json
}  // namespace fun
