#include "fun/json/reader.h"

namespace fun {
namespace json {

namespace {

bool ContainsNewLine(Reader::Location begin, Reader::Location end) {
  for (auto cur = begin; cur != end; ++cur) {
    if (*cur == '\n' || *cur == '\r') {
      return true;
    }
  }
  return false;
}

}  // namespace

Reader::Reader()
    : errors_(),
      begin_(),
      current_(),
      end_(),
      last_value_end_(),
      last_value_(),
      comments_before_(),
      features_(Features_::All()),
      collect_comments_() {}

Reader::Reader(const Features& features)
    : errors_(),
      begin_(),
      current_(),
      end_(),
      last_value_end_(),
      last_value_(),
      comments_before_(),
      features_(features),
      collect_comments_() {}

bool Reader::Parse(const String& document, JValue& root,
                   bool collect_comments) {
  const char* begin = *document;
  const char* end = *document + document.Len();
  return Parse(begin, end, root, collect_comments);
}

bool Reader::Parse(const char* begin, const char* end, JValue& root,
                   bool collect_comments) {
  if (!features_.allow_comments) {
    collect_comments = false;
  }

  collect_comments_ = collect_comments;

  begin_ = begin;
  end_ = end;
  current_ = begin;
  last_value_end_ = nullptr;
  last_value_ = nullptr;
  comments_before_ = "";
  errors_.Clear();
  nodes_.Clear();
  nodes_.Push(&root);

  const bool succeeded = ReadValue();

  Token token;
  SkipCommentTokens(token);

  if (features_.fail_if_extra) {
    if ((features_.strict_root || token.type != TokenType::Error)) &&
        token.type != TokenType::EndOfStream) {
        AddError("Extra non-whitespace after JSON value.", token);
        return false;
      }

    if (collect_comments_ && comments_before_.Len() > 0) {
      root.SetComment(comments_before_, CommentPlacement::After);
    }

    if (features_.string_root) {
      if (!root.IsArray() && !root.IsObject()) {
        // Set error location to start of doc, ideally should be
        // first token found in doc.
        token.type = TokenType::Error;
        token.start = begin_;
        token.end = end_;
        AddError(
            "A valid JSON document must be either an array or an object value.",
            token);
        return false;
      }
    }

    return succeeded;
  }

  bool Reader::ReadValue() {
    //  To preserve the old behaviour we cast size_t to int.
    if (nodes_.Count() >= features_.stack_limit) {
      throw RangeException(
          StringLiteral("Exceeded StackLimit in ReadValue()."));
    }

    Token token;
    SkipCommentTokens(token);
    bool succeeded = true;

    if (collect_comments_ && comments_before_.Len() > 0) {
      CurrentValue().SetComment(comments_before_, CommentPlacement::Before);
      comments_before_ = "";
    }

    switch (token.type) {
      case TokenType::ObjectBegin:
        succeeded = ReadObject(token);
        CurrentValue().SetOffsetLimit(current_ - begin_);
        break;

      case TokenType::ArrayBegin:
        succeeded = ReadArray(token);
        CurrentValue().SetOffsetLimit(current_ - begin_);
        break;

      case TokenType::Number:
        succeeded = DecodeNumber(token);
        break;

      case TokenType::String:
        succeeded = DecodeString(token);
        break;

      case TokenType::True: {
        JValue true_value(true);
        CurrentValue().SwapPayload(true_value);
        CurrentValue().SetOffsetStart(token.start - begin_);
        CurrentValue().SetOffsetLimit(token.end - begin_);
        break;
      }

      case TokenType::False: {
        JValue false_value(false);
        CurrentValue().SwapPayload(false_value);
        CurrentValue().SetOffsetStart(token.start - begin_);
        CurrentValue().SetOffsetLimit(token.end - begin_);
        break;
      }

      case TokenType::Null: {
        JValue null_value;
        CurrentValue().SwapPayload(null_value);
        CurrentValue().SetOffsetStart(token.start - begin_);
        CurrentValue().SetOffsetLimit(token.end - begin_);
        break;
      }

      case TokenType::NaN: {
        JValue nan_value(JValue::NaN);
        CurrentValue().SwapPayload(nan_value);
        CurrentValue().SetOffsetStart(token.start - begin_);
        CurrentValue().SetOffsetLimit(token.end - begin_);
        break;
      }
      case TokenType::PosInf: {
        JValue pos_inf_value(JValue::PosInf);
        CurrentValue().SwapPayload(pos_inf_value);
        CurrentValue().SetOffsetStart(token.start - begin_);
        CurrentValue().SetOffsetLimit(token.end - begin_);
        break;
      }
      case TokenType::NegInf: {
        JValue neg_inf_value(JValue::NegInf);
        CurrentValue().SwapPayload(neg_inf_value);
        CurrentValue().SetOffsetStart(token.start - begin_);
        CurrentValue().SetOffsetLimit(token.end - begin_);
        break;
      }

      case TokenType::ArraySeparator:
      case TokenType::ObjectEnd:
      case TokenType::ArrayEnd:
        if (features_.allow_dropped_null_placeholders) {
          // "Un-read" the cur token and mark the cur value as a null token.
          current_--;
          JValue null_value;
          CurrentValue().SwapPayload(null_value);
          CurrentValue().SetOffsetStart(current_ - begin_ - 1);
          CurrentValue().SetOffsetLimit(current_ - begin_);
          break;
        }
        //[[fallthrough]]
        FUN_FALLTHROUGH
      default:
        CurrentValue().SetOffsetStart(token.start - begin_);
        CurrentValue().SetOffsetLimit(token.end - begin_);
        return AddError("Syntax error: value, object or array expected.",
                        token);
    }

    if (collect_comments_) {
      last_value_end_ = current_;
      last_value_ = &CurrentValue();
    }

    return succeeded;
  }

  void Reader::SkipCommentTokens(Token & token) {
    if (features_.allow_comments) {
      do {
        ReadToken(token);
      } while (token.type == TokenType::Comment);
    } else {
      ReadToken(token);
    }
  }

  bool Reader::ReadToken(Token & token) {
    SkipSpaces();

    token.start = current_;
    char ch = GetNextChar();
    bool ok = true;
    switch (ch) {
      case '{':
        token.type = TokenType::ObjectBegin;
        break;

      case '}':
        token.type = TokenType::ObjectEnd;
        break;

      case '[':
        token.type = TokenType::ArrayBegin;
        break;

      case ']':
        token.type = TokenType::ArrayEnd;
        break;

      case '"':
        token.type = TokenType::String;
        ok = ReadString();
        break;

      case '\'':
        if (features_.allow_single_quoted_strings) {
          token.type = TokenType::String;
          ok = ReadSingleQuotedString();
          break;
        }
        //[[fallthrough]]
      case '/':
        token.type = TokenType::Comment;
        ok = ReadComment();
        break;

      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      case '-':
        if (ReadNumber()) {
          token.type = TokenType::Number;
        } else {
          token.type = TokenType::NegInf;
          ok = features_.allow_special_floats && Match("nfinity", 7);
        }
        break;

      case 't':
        token.type = TokenType::True;
        ok = Match("rue", 3);
        break;

      case 'f':
        token.type = TokenType::False;
        ok = Match("alse", 4);
        break;

      case 'n':
        token.type = TokenType::Null;
        ok = Match("ull", 3);
        break;

      case 'N':
        if (features_.allow_special_floats) {
          token.type = TokenType::NaN;
          ok = Match("aN", 2);
        } else {
          ok = false;
        }
        break;
      case 'I':
        if (features_.allow_special_floats) {
          token.type = TokenType::PosInf;
          ok = Match("nfinity", 7);
        } else {
          ok = false;
        }
        break;

      case ',':
        token.type = TokenType::ArraySeparator;
        break;

      case ':':
        token.type = TokenType::MemberSeparator;
        break;

      case 0:
        token.type = TokenType::EndOfStream;
        break;

      default:
        ok = false;
        break;
    }

    if (!ok) {
      token.type = TokenType::Error;
    }

    token.end = current_;

    return true;
  }

  void Reader::SkipSpaces() {
    while (current_ != end_) {
      const char ch = *current_;
      if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
        ++current_;
      } else {
        break;
      }
    }
  }

  bool Reader::Match(Location pattern, int32 pattern_len) {
    if ((end_ - current_) < pattern_len) {
      return false;
    }

    int32 index = pattern_len;
    while (index--) {
      if (current_[index] != pattern[index]) {
        return false;
      }
    }
    current_ += pattern_len;
    return true;
  }

  bool Reader::ReadComment() {
    Location comment_begin = current_ - 1;
    const char ch = GetNextChar();
    bool succeeded = false;
    if (ch == '*') {
      succeeded = ReadCStyleComment();  // block comment
    } else if (ch == '/') {
      succeeded = ReadCppStyleComment();  // line comment
    }
    if (!succeeded) {
      return false;
    }

    if (collect_comments_) {
      CommentPlacement placement = CommentPlacement::Before;
      if (last_value_end_ && !ContainsNewLine(last_value_end_, comment_begin)) {
        if (ch != '*' && !ContainsNewLine(comment_begin, current_)) {
          placement = CommentPlacement::AfterOnSameLine;
        }
      }
      AddComment(comment_begin, current_, placement);
    }
    return true;
  }

  namespace {

  String NormalizeEOL(Reader::Location begin, Reader::Location end) {
    String normalized;
    for (auto cur = begin; cur != end;) {
      const char ch = *cur++;
      if (ch == '\r') {
        if (cur != end && *cur == '\n') {
          ++cur;
        }
        normalized += '\n';
      } else {
        normalized += ch;
      }
    }
    return normalized;
  }

  }  // namespace

  void Reader::AddComment(Location begin, Location end,
                          CommentPlacement placement) {
    fun_check(collect_comments_);
    const String& normalized = NormalizeEOL(begin, end);
    if (placement == CommentPlacement::AfterOnSameLine) {
      fun_check_ptr(last_value_);
      last_value_->SetComment(normalized, placement);
    } else {
      comments_before_ += normalized;
    }
  }

  bool Reader::ReadCStyleComment() {
    while ((current_ + 1) < end_) {
      const char ch = GetNextChar();
      if (ch == '*' && *current_ != '/') {
        break;
      }
    }
    return GetNextChar() == '/';
  }

  bool Reader::ReadCppStyleComment() {
    while (current_ != end_) {
      const char ch = GetNextChar();
      if (ch == '\n') {
        break;
      }

      if (ch == '\r') {
        // Consume DOS EOL. It will be normalized in AddComment.
        if (current_ != end_ && *current_ == '\n') {
          GetNextChar();
        }

        // Break on Moc OS 9 EOL.
        break;
      }
    }

    return true;
  }

  void Reader::ReadNumber() {
    const char* ptr = current_;
    char ch = '0';  // Stopgap for already consumed character

    // Integral part
    while (ch >= '0' && ch <= '9') {
      ch = (current_ = ptr) < end_ ? *ptr++ : '\0';
    }

    // Fractional part
    if (ch == '.') {
      ch = (current_ = ptr) < end_ ? *ptr++ : '\0';

      while (ch >= '0' && ch <= '9') {
        ch = (current_ = ptr) < end_ ? *ptr++ : '\0';
      }
    }

    // Exponential part
    if (ch == 'e' || ch == 'E') {
      ch = (current_ = ptr) < end_ ? *ptr++ : '\0';

      if (ch == '+' || ch == '-') {
        ch = (current_ = ptr) < end_ ? *ptr++ : '\0';
      }

      while (ch >= '0' && ch <= '9') {
        ch = (current_ = ptr) < end_ ? *ptr++ : '\0';
      }
    }
  }

  bool Reader::ReadString() {
    char ch = '\0';
    while (current_ != end_) {
      ch = GetNextChar();
      if (ch == '\\') {
        GetNextChar();
      } else if (ch == '"') {
        break;
      }
    }
    return ch == '"';
  }

  bool Reader::ReadSingleQuotedString() {
    char ch = '\0';
    while (current_ != end_) {
      ch = GetNextChar();
      if (ch == '\\') {
        GetNextChar();
      } else if (ch == '\'') {
        break;
      }
    }
    return ch == '\'';
  }

  bool Reader::ReadObject(Token & token_start) {
    Token token_name;
    String name;

    JValue init(JValue::EmptyObject);
    CurrentValue().SwapPayload(init);
    CurrentValue().SetOffsetStart(token_start.start - begin_);
    while (ReadToken(token_name)) {
      bool initial_token_ok = true;
      while (token_name.type == TokenType::Comment && initial_token_ok) {
        initial_token_ok = ReadToken(token_name);
      }
      if (!initial_token_ok) {
        break;
      }

      if (token_name.type == TokenType::ObjectEnd &&
          name.IsEmpty()) {  // empty object
        return true;
      }

      name = "";
      if (token_name.type == TokenType::String) {
        if (!DecodeString(token_name, name)) {
          return RecoverFromError(TokenType::ObjectEnd);
        }
      } else if (token_name.type == TokenType::Number &&
                 features_.allow_numeric_keys) {
        JValue number_name;
        if (!DecodeNumber(token_name, number_name)) {
          return RecoverFromError(TokenType::ObjectEnd);
        }
        name = number_name.AsString();
      } else {
        break;
      }

      Token colon;
      if (!ReadToken(colon) || colon.type != TokenType::MemberSeparator) {
        return AddErrorAndRecover("Missing ':' after object member name", colon,
                                  TokenType::ObjectEnd);
      }

      if (name.Len() >= (1 << 30)) {
        throw RangeException(StringLiteral("key length >= 2^30"));
      }

      if (features_.reject_dup_keys && CurrentValue().ContainsField(name)) {
        String msg = String::Format("Duplicate key: '{0}'", name);
        return AddErrorAndRecover(msg, token_name, TokenType::ObjectEnd);
      }

      JValue& value = CurrentValue()[name];
      nodes_.Push(&value);
      bool ok = ReadValue();
      nodes_.Pop();
      if (!ok) {  // error already set
        return RecoverFromError(TokenType::ObjectEnd);
      }

      Token comma;
      if (!ReadToken(comma) || (comma.type != TokenType::ObjectEnd &&
                                comma.type != TokenType::ArraySeparator &&
                                comma.type != TokenType::Comment)) {
        return AddErrorAndRecover("Missing ',' or '}' in object declaration",
                                  comma, TokenType::ObjectEnd);
      }

      bool finalize_token_ok = true;
      while (comma.type == TokenType::Comment && finalize_token_ok) {
        finalize_token_ok = ReadToken(comma);
      }
      if (comma.type == TokenType::ObjectEnd) {
        return true;
      }
    }

    return AddErrorAndRecover("Missing '}' or object member name", token_name,
                              TokenType::ObjectEnd);
  }

  bool Reader::ReadArray(Token & token_start) {
    JValue initial(JValue::EmptyArray);
    CurrentValue().SwapPayload(initial);
    CurrentValue().SetOffsetStart(token_start.start - begin_);
    SkipSpaces();
    if (current_ != end_ && *current_ == ']') {  // empty array
      Token end_array;
      ReadToken(end_array);
      return true;
    }

    int32 index = 0;
    for (;;) {
      JValue& value = CurrentValue()[index++];
      nodes_.Push(&value);
      bool ok = ReadValue();
      nodes_.Pop();
      if (!ok) {  // error already set
        return RecoverFromError(TokenType::ArrayEnd);
      }

      Token current_token;
      // Accept comment after last item in the array.
      ok = ReadToken(current_token);
      while (current_token.type == TokenType::Comment && ok) {
        ok = ReadToken(current_token);
      }

      const bool is_bad_token_type =
          (current_token.type != TokenType::ArraySeparator &&
           current_token.type != TokenType::ArrayEnd);
      if (!ok || is_bad_token_type) {
        return AddErrorAndRecover("Missing ',' or ']' in array declaration",
                                  current_token, TokenType::ArrayEnd);
      }

      if (current_token.type == TokenType::ArrayEnd) {
        break;
      }
    }
    return true;
  }

  bool Reader::DecodeNumber(Token & token) {
    JValue decoded;
    if (!DecodeNumber(token, decoded)) {
      return false;
    }

    CurrentValue().SwapPayload(decoded);
    CurrentValue().SetOffsetStart(token.start - begin_);
    CurrentValue().SetOffsetLimit(token.end - begin_);
    return true;
  }

  bool Reader::DecodeNumber(Token & token, JValue & decoded) {
    // Attempts to parse the number as an integer. If the number is
    // larger than the maximum supported value of an integer then
    // we decode the number as a double.
    Location cur = token.start;
    const bool is_neg = *cur == '-';
    if (is_neg) {
      ++cur;
    }

    const uint64 max_integer_value = is_neg
                                         ? std::numeric_limits<int64>::max() + 1
                                         : std::numeric_limits<uint64>::max();

    const uint64 threshold = max_integer_value / 10;
    uint64 value = 0;

    while (cur != token.end) {
      const char ch = *cur++;

      if (ch < '0' || ch > '9') {
        return DecodeDouble(token, decoded);
      }

      const uint32 digit = static_cast<uint32>(ch - '0');
      if (value >= threshold) {
        if (value > threshold || cur != token.end ||
            digit > (max_integer_value % 10)) {
          return DecodeDouble(token, decoded);
        }
      }

      value = value * 10 + digit;
    }

    /*
    if (is_neg) {
      decoded = -Value::LargestInt(value);
    } else if (value <= Value::LargestUInt(Value::maxInt)) {
      decoded = Value::LargestInt(value);
    } else {
      decoded = value;
    }
    */

    if (is_neg && value == max_integer_value) {  // hmm...
      decoded = std::numeric_limits<int64>::min();
    } else if (is_neg) {  // negative int64
      decoded = -(int64)(value);
    } else if (value <=
               (uint64)std::numeric_limits<int32>::max()) {  // positive int64
      decoded = (int64)(value);
    } else {  // unsigned int64
      decoded = value;
    }

    return true;
  }

  bool Reader::DecodeDouble(Token & token) {
    JValue decoded;
    if (!DecodeDouble(token, decoded)) {
      return false;
    }

    CurrentValue().SwapPayload(decoded);
    CurrentValue().SetOffsetStart(token.start - begin_);
    CurrentValue().SetOffsetLimit(token.end - begin_);
    return true;
  }

  bool Reader::DecodeDouble(Token & token, JValue & decoded) {
    double value = 0;
    String buffer(token.start, (int32)(token.end - token.start));

    bool ok = false;
    value = buffer.ToDouble(&ok);
    if (!ok) {
      return AddError("'" + buffer + "' is not a number.", token);
    }

    decoded = value;
    return true;
  }

  bool Reader::DecodeString(Token & token) {
    String decoded_str;
    if (!DecodeString(token, decoded_str)) {
      return false;
    }

    JValue decoded(decoded_str);
    CurrentValue().SwapPayload(decoded);
    CurrentValue().SetOffsetStart(token.start - begin_);
    CurrentValue().SetOffsetLimit(token.end - begin_);
    return true;
  }

  namespace {

  FUN_ALWAYS_INLINE String CodepointToUtf8(uint32 cp) {
    // based on description from http://en.wikipedia.org/wiki/UTF-8

    char result[5] = {0};

    if (cp <= 0x7F) {
      result[0] = static_cast<char>(cp);
    } else if (cp <= 0x7FF) {
      result[1] = static_cast<char>(0x80 | (0x3F & cp));
      result[0] = static_cast<char>(0xc0 | (0x1F & (cp >> 6)));
    } else if (cp <= 0xFFFF) {
      result[2] = static_cast<char>(0x80 | (0x3F & cp));
      result[1] = static_cast<char>(0x80 | (0x3F & (cp >> 6)));
      result[0] = static_cast<char>(0xe0 | (0x0F & (cp >> 12)));
    } else if (cp <= 0x10FFFF) {
      result[3] = static_cast<char>(0x80 | (0x3F & cp));
      result[2] = static_cast<char>(0x80 | (0x3F & (cp >> 6)));
      result[1] = static_cast<char>(0x80 | (0x3F & (cp >> 12)));
      result[0] = static_cast<char>(0xF0 | (0x07 & (cp >> 18)));
    }

    return result;
  }

  }  // namespace

  bool Reader::DecodeString(Token & token, String & decoded) {
    decoded.Reserve(static_cast<int32>(token.end - token.start - 2));

    Location cur = token.start + 1;  // skip '"'
    Location end = token.end - 1;    // do not include '"'
    while (cur != end) {
      const char ch = *cur++;

      if (ch == '"') {
        break;
      }

      if (ch != '\\') {
        decoded += ch;
        continue;
      }

      if (cur == end) {
        return AddError("Empty escape sequence in string", token, cur);
      }

      char escape = *cur++;
      switch (escape) {
        case '"':
          decoded += '"';
          break;
        case '/':
          decoded += '/';
          break;
        case '\\':
          decoded += '\\';
          break;
        case 'b':
          decoded += '\b';
          break;
        case 'f':
          decoded += '\f';
          break;
        case 'n':
          decoded += '\n';
          break;
        case 'r':
          decoded += '\r';
          break;
        case 't':
          decoded += '\t';
          break;

        case 'u': {
          uint32 unicode;
          if (!DecodeUnicodeCodepoint(token, cur, end, unicode)) {
            return false;
          }

          decoded += CodepointToUtf8(unicode);
          break;
        }

        default:
          return AddError("Bad escape sequence in string", token, cur);
      }
    }

    return true;
  }

  bool Reader::DecodeUnicodeCodepoint(Token & token, Location & cur,
                                      Location end, uint32 & unicode) {
    if (!DecodeUnicodeEscapeSequence(token, cur, end, unicode)) {
      return false;
    }

    if (unicode >= 0xD800 && unicode <= 0xDBFF) {
      // surrogate pairs
      if ((end - cur) < 6) {
        return AddError(
            "additional six characters expected to parse unicode surrogate "
            "pair.",
            token, cur);
      }

      if (*cur == '\\' && *(cur + 1) == 'u') {
        cur += 2;

        uint32 surrogate_pair;
        if (DecodeUnicodeEscapeSequence(token, cur, end, surrogate_pair)) {
          unicode =
              0x10000 + ((unicode & 0x3FF) << 10) + (surrogate_pair & 0x3FF);
        } else {
          return false;
        }
      } else {
        return AddError(
            "expecting another \\u token to begin the second half of a unicode "
            "surrogate pair",
            token, cur);
      }
    }

    return true;
  }

  bool Reader::DecodeUnicodeEscapeSequence(Token & token, Location & cur,
                                           Location end, uint32 & out_unicode) {
    if (end - cur < 4) {
      return AddError(
          "Bad unicode escape sequence in string: four digits expected.", token,
          cur);
    }

    int32 unicode = 0;
    for (int32 index = 0; index < 4; ++index) {
      const uint32 ch = *cur++;
      unicode <<= 4;
      if (ch >= '0' && ch <= '9') {
        unicode += ch - '0';
      } else if (ch >= 'a' && ch <= 'f') {
        unicode += ch - 'a' + 10;
      } else if (ch >= 'A' && ch <= 'F') {
        unicode += ch - 'A' + 10;
      } else {
        return AddError(
            "Bad unicode escape sequence in string: hexadecimal digit "
            "expected.",
            token, cur);
      }
    }

    // Assign result.
    out_unicode = unicode;
    return true;
  }

  bool Reader::AddError(const String& message, Token& token, Location extra) {
    ErrorInfo info;
    info.token = token;
    info.message = message;
    info.extra = extra;
    errors_.Add(info);
    return false;
  }

  bool Reader::RecoverFromError(TokenType skip_until_token) {
    const int32 encountered_error_count = errors_.Count();

    Token skip;
    for (;;) {
      if (!ReadToken(skip)) {
        errors_.Resize(
            encountered_error_count);  // discard errors caused by recovery
      }

      if (skip.type == skip_until_token ||
          skip.type == TokenType::EndOfStream) {
        break;
      }
    }
    errors_.Resize(encountered_error_count);
    return false;
  }

  bool Reader::AddErrorAndRecover(const String& message, Token& token,
                                  TokenType skip_until_token) {
    AddError(message, token);
    return RecoverFromError(skip_until_token);
  }

  JValue& Reader::CurrentValue() { return *(nodes_.Top()); }

  char Reader::GetNextChar() {
    if (current_ == end_) {
      return 0;
    }

    return *current_++;
  }

  void Reader::GetLocationLineAndColumn(Location location, int32 & line,
                                        int32 & column) const {
    Location cur = begin_;
    Location last_line_start = cur;
    line = 0;
    while (cur < location && cur != end_) {
      const char ch = *cur++;

      if (ch == '\r') {
        if (*cur == '\n') {
          ++cur;
        }
        last_line_start = cur;
        ++line;
      } else if (ch == '\n') {
        last_line_start = cur;
        ++line;
      }
    }

    // column & line start at 1
    column = int32(location - last_line_start) + 1;
    ++line;
  }

  String Reader::GetLocationLineAndColumn(Location location) const {
    int32 line, column;
    GetLocationLineAndColumn(location, line, column);
    return String::Format("line {}, column {}", line, column);
  }

  String Reader::GetFormattedErrorMessages() const {
    String formatted_message;
    for (const auto& error : errors_) {
      formatted_message +=
          "* " + GetLocationLineAndColumn(error.token.start) + "\n";
      formatted_message += "  " + error.message + "\n";
      if (error.extra) {
        formatted_message +=
            "See " + GetLocationLineAndColumn(error.extra) + " for detail.\n";
      }
    }
    return formatted_message;
  }

  fun::Array<Reader::StructuredError> Reader::GetStructuredErrors() const {
    fun::Array<Reader::StructuredError> all_errors;
    for (const auto& error : errors_) {
      Reader::StructuredError structured;
      structured.offset_start = error.token.start - begin_;
      structured.offset_limit = error.token.end - begin_;
      structured.message = error.message;
      all_errors.Add(structured);
    }
    return all_errors;
  }

  bool Reader::PushError(const JValue& value, const String& message) {
    const ptrdiff_t len = end_ - begin_;
    if (value.GetOffsetStart() > len || value.GetOffsetLimit() > len) {
      return false;
    }

    Token token;
    token.type = TokenType::Error;
    token.start = begin_ + value.GetOffsetStart();
    token.end = end_ + value.GetOffsetLimit();

    ErrorInfo info;
    info.token = token;
    info.message = message;
    info.extra = nullptr;
    errors_.Add(info);
    return true;
  }

  bool Reader::PushError(const JValue& value, const String& message,
                         const JValue& extra) {
    const ptrdiff_t len = end_ - begin_;

    if (value.GetOffsetStart() > len || value.GetOffsetLimit() > len ||
        extra.GetOffsetLimit() > len) {
      return false;
    }

    Token token;
    token.type = TokenType::Error;
    token.start = begin_ + value.GetOffsetStart();
    token.end = begin_ + value.GetOffsetLimit();

    ErrorInfo info;
    info.token = token;
    info.message = message;
    info.extra = begin_ + extra.GetOffsetStart();
    errors_.Add(info);
    return true;
  }

  bool Reader::Good() const { return errors_.IsEmpty(); }

}  // namespace json
}  // namespace json
