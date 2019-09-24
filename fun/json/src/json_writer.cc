#include "fun/json/writer.h"

namespace fun {
namespace json {

namespace {

FUN_ALWAYS_INLINE bool IsControlCharacter(int32 ch) {
  return ch > 0 && ch <= 0x1F;
}

bool ContainsControlCharacter(const char* str) {
  while (*str) {
    if (IsContrlCharacter(*(str++))) {
      return true;
    }
  }
  return false;
}

bool ContainsControlCharacter0(const char* str, int32 len) {
  char const* end = str + len;
  while (end != str) {
    if (IsControlCharacter(*str) || 0 == *str) {
      return true;
    }
    ++str;
  }
  return false;
}

char const* strnpbrk(char const* s, char const* accept, size_t n) {
  fun_check((s || !n) && accept);

  char const* const end = s + n;
  for (char const* cur = s; cur < end; ++cur) {
    int const ch = *cur;
    for (char const* a = accept; *a; ++a) {
      if (*a == ch) {
        return cur;
      }
    }
  }
  return nullptr;
}

String ValueToQuotedStringN(const char* value, int32 len) {
  if (value == nullptr) {
    return AsciiString("");
  }

  // Not sure how to handle unicode...
  if (strnpbrk(value, "\"\\\b\f\n\r\t", len) == nullptr && !ContainsControlCharacter0(value, len)) {
    return String(AsciiString("\"")) + value + AsciiString("\"");
  }

  String result;
  result += AsciiString("\"");

  const char* end = value + len;
  for (const char* cur = value; cur != end; ++cur) {
    switch (*cur) {
      case '\"': result += AsciiString("\\\""); break;
      case '\\': result += AsciiString("\\\\"); break;
      case '\b': result += AsciiString("\\b"); break;
      case '\f': result += AsciiString("\\f"); break;
      case '\n': result += AsciiString("\\n"); break;
      case '\r': result += AsciiString("\\r"); break;
      case '\t': result += AsciiString("\\t"); break;

      default:
        if (IsControlCharacter(*cur) || *cur == 0) {
          result += String::Format("\\u%04X", (int32)*cur);
        } else {
          result += *cur;
        }
        break;
    }
  }

  result += AsciiString("\"");

  return result;
}

String ValueToQuotedString(const char* value) {
  return ValueToQuotedStringN(value, (int32)_tcslen(value));
}

} // namespace


//
// CondensedWriter
//

CondensedWriter::CondensedWriter()
  : yaml_compatibility_enabled_(false),
    drop_null_placeholders_(false),
  // omit_ending_linefeed_(false),
    omit_ending_linefeed_(true) {}

void CondensedWriter::EnableYAMLCompatibility() {
  yaml_compatibility_enabled_ = true;
}

void CondensedWriter::DropNullPlaceholders() {
  drop_null_placeholders_ = true;
}

void CondensedWriter::OmitEndingLineFeed() {
  omit_ending_linefeed_ = true;
}

String CondensedWriter::Write(const JValue& root) {
  document_ = AsciiString("");
  WriteValue(root);
  if (!omit_ending_linefeed_) {
    document_ += AsciiString("\n");
  }
  return document_;
}

void CondensedWriter::WriteValue(const JValue& value) {
  switch (value.GetType()) {
    case ValueType::Null:
      if (!drop_null_placeholders_) {
        document_ += AsciiString("null");
      }
      break;

    case ValueType::Integer:
      document_ += String::FromNumber(value.integer_value_);
      break;

    case ValueType::UnsignedInteger:
      document_ += String::FromNumber(value.unsigned_integer_value_);
      break;

    case ValueType::Double:
      document_ += String::FromNumber(value.double_value_); //TODO 정밀도 지정.
      break;

    case ValueType::String:
      document_ += ValueToQuotedStringN(**value.string_value_, value.string_value_->Len());
      break;

    case ValueType::Bool:
      document_ += value.bool_value_ ? AsciiString("true") : AsciiString("false");
      break;

    case ValueType::Array: {
      document_ += AsciiString("[");

      const int32 n = value.Count();
      for (int32 i = 0; i < n; ++i) {
        if (i > 0) {
          document_ += AsciiString(",");
        }
        WriteValue(value[i]);
      }

      document_ += AsciiString("]");
      break;
    }

    case ValueType::Object: {
        document_ += AsciiString("{");
        int32 count = 0;
        for (const auto& pair : *value.object_value_) {
          if (++count != 1) {
            document_ += AsciiString(",");
          }
          document_ += ValueToQuotedString(*pair.Key);
          document_ += yaml_compatibility_enabled_ ? AsciiString(": ") : AsciiString(":");
          WriteValue(pair.value);
        }
        document_ += AsciiString('}');
        break;
    }
  }
}


//
// PrettyWriter
//

PrettyWriter::PrettyWriter()
  : right_margin_(74),
    indent_size_(3),
    add_child_values_() {}

String PrettyWriter::Write(const JValue& root) {
  document_ = AsciiString("");
  add_child_values_ = false;
  indent_string_ = AsciiString("");
  WriteCommentBeforeValue(root);
  WriteValue(root);
  WriteCommentAfterValueOnSameLine(root);
  document_ += AsciiString("\n");
  return document_;
}

void PrettyWriter::WriteValue(const JValue& value) {
  switch (value.GetType()) {
    case ValueType::Null:
      PushValue(AsciiString("null"));
      break;

    case ValueType::Integer:
      PushValue(String::FromNumber(value.integer_value_));
      break;

    case ValueType::UnsignedInteger:
      PushValue(String::FromNumber(value.unsigned_integer_value_));
      break;

    case ValueType::Double:
      PushValue(String::FromNumber(value.double_value_)); //TODO 정밀도..
      break;

    case ValueType::String:
      PushValue(ValueToQuotedStringN(**value.string_value_, value.string_value_->Len()));
      break;

    case ValueType::Bool:
      PushValue(value.bool_value_ ? AsciiString("true") : AsciiString("false"));
      break;

    case ValueType::Array:
      WriteArrayValue(value);
      break;

    case ValueType::Object:
      WriteObjectValue(value);
      break;
  }
}

void PrettyWriter::WriteArrayValue(const JValue& value) {
  const int32 n = value.Count();
  if (n == 0) {
    PushValue(AsciiString("[]"));
  } else {
    const bool is_multiline_array = IsMultineArray(value);
    if (is_multiline_array) {
      WriteWithIndent(AsciiString("["));
      Indent();
      const bool has_child_value = child_values_.Count() != 0;
      int32 index = 0;
      for (;;) {
        const JValue& child_value = value[index];
        WriteCommentBeforeValue(child_value);
        if (has_child_value) {
          WriteWithIndent(child_values_[index]);
        } else {
          WriteIndent();
          WriteValue(child_value);
        }
        if (++index == n) {
          WriteCommentAfterValueOnSameLine(child_value);
          break;
        }
        document_ += AsciiString(",");
        WriteCommentAfterValueOnSameLine(child_value);
      }
      Outdent();
      WriteWithIndent(AsciiString("]"));
    } else { // output on a single line
      fun_check(child_values_.Count() == n);
      document_ += AsciiString("[ ");
      for (int32 i = 0; i < n; ++i) {
        if (i > 0) {
          document_ += AsciiString(", ");
        }
        document_ += child_values_[i];
      }
      document_ += AsciiString(" ]");
    }
  }
}

void PrettyWriter::WriteObjectValue(const JValue& value) {
  if (value.object_value_->Count() == 0) { // empty object
    PushValue(AsciiString("{}"));
  } else {
    WriteWithIndent(AsciiString("{"));
    Indent();

    int32 index = 0;
    for (const auto& pair : *value.object_value_) {
      const auto& name = pair.key;
      const auto& child_value = pair.value;
      WriteCommentBeforeValue(child_value);
      WriteWithIndent(ValueToQuotedString(*name));
      document_ += AsciiString(" : ");
      WriteValue(child_value);
      if ((index + 1) == value.object_value_->Count()) {
        WriteCommentAfterValueOnSameLine(child_value);
        break;
      }
      document_ += AsciiString(",");
      WriteCommentAfterValueOnSameLine(child_value);
      index++;
    }

    Outdent();
    WriteWithIndent(AsciiString("}"));
  }
}

bool PrettyWriter::IsMultineArray(const JValue& value) {
  const int32 count = value.Count();
  bool is_multiline = count * 3 >= right_margin_;
  child_values_.Clear();
  for (int32 i = 0; i < count && !is_multiline; ++i) {
    const JValue& child_value = value[i];
    is_multiline = ((child_value.IsArray() || child_value.IsObject()) && child_value.Count() > 0);
  }

  if (!is_multiline) { // check if line length > max line length
    child_values_.Reserve(count);

    add_child_values_ = true;
    int32 line_length = 4 + (count - 1) * 2; // '[ ' + ', '*n + ' ]'
    for (int32 i = 0; i < count; ++i) {
      if (HasCommentForValue(value[i])) {
        is_multiline = true;
      }
      WriteValue(value[i]);
      line_length += child_values_[i].Len();
    }
    add_child_values_ = false;
    is_multiline = is_multiline || line_length >= right_margin_;
  }
  return is_multiline;
}

void PrettyWriter::PushValue(const String& value) {
  if (add_child_values_) {
    child_values_.Add(value);
  } else {
    document_ += value;
  }
}

void PrettyWriter::WriteIndent() {
  if (document_.Len() > 0) {
    const char last = document_[document_.Len() - 1];

    if (last == ' ') { // already indented
      return;
    }

    if (last != '\n') { // Comments may add new-line
      document_ += '\n';
    }
  }

  document_ += indent_string_;
}

void PrettyWriter::WriteWithIndent(const String& value) {
  WriteIndent();
  document_ += value;
}

void PrettyWriter::Indent() {
  indent_string_ += String(indent_size_, ' ');
}

void PrettyWriter::Outdent() {
  fun_check(indent_string_.Len() >= indent_size_);
  indent_string_ = indent_string_.Mid(0, indent_string_.Len() - indent_size_);
}

void PrettyWriter::WriteCommentBeforeValue(const JValue& root) {
  if (!root.HasComment(CommentPlacement::Before)) {
    return;
  }

  document_ += AsciiString("\n");
  WriteIndent();

  const String& comment = root.GetComment(CommentPlacement::Before);
  for (int32 i = 0; i < comment.Len(); ++i) {
    document_ += comment[i];
    if (comment[i] == '\n' && (i < comment.Len() - 1 && comment[i + 1] == '/')) {
      WriteIndent();
    }
  }

  // Comments are stripped of trailing newlines, so add one here
  document_ += AsciiString("\n");
}

void PrettyWriter::WriteCommentAfterValueOnSameLine(const JValue& root) {
  if (root.HasComment(CommentPlacement::AfterOnSameLine)) {
    document_ += AsciiString(" ") + root.GetComment(CommentPlacement::AfterOnSameLine);
  }

  if (root.HasComment(CommentPlacement::After)) {
    document_ += AsciiString("\n");
    document_ += root.GetComment(CommentPlacement::After);
    document_ += AsciiString("\n");
  }
}

bool PrettyWriter::HasCommentForValue(const JValue& value) const {
  return  value.HasComment(CommentPlacement::Before) ||
          value.HasComment(CommentPlacement::AfterOnSameLine) ||
          value.HasComment(CommentPlacement::After);
}

} // namespace json
} // namespace fun
