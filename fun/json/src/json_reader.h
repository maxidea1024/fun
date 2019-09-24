#pragma once

#include "fun/json/features.h"
#include "fun/json/json.h"
#include "fun/json/value.h"

namespace fun {
namespace json {

/**
 * JSON string reader(parser).
 */
class FUN_JSON_API Reader {
 public:
  typedef const char* Location;

  struct StructuredError {
    ptrdiff_t offset_start;
    ptrdiff_t offset_limit;
    String message;
  };

  /**
   * Constructs a Reader allowing all features for parsing.
   */
  Reader();

  /**
   * Constructs a Reader allowing the specified feature set for parsing.
   */
  Reader(const Features& features);

  /**
   * Parse json string.
   */
  bool Parse(const String& document, JValue& root,
             bool collect_comments = true);

  /**
   * Parse json string.
   */
  bool Parse(const char* begin, const char* end, JValue& root,
             bool collect_comments = true);

  /**
   * Returns a user friendly string that list errors in the parsed document.
   */
  String GetFormattedErrorMessages() const;

  /**
   * Returns a vector of structured erros encounted while parsing.
   */
  Array<StructuredError> GetStructuredErrors() const;

  /**
   * Add a semantic error message.
   */
  bool PushError(const JValue& value, const String& message);

  /**
   * Add a semantic error message with extra context.
   */
  bool PushError(const JValue& value, const String& message,
                 const JValue& extra);

  /**
   * Return whether there are any errors.
   */
  bool Good() const;

 private:
  enum class TokenType {
    EndOfStream = 0,
    ObjectBegin,
    ObjectEnd,
    ArrayBegin,
    ArrayEnd,
    String,
    Number,
    True,
    False,
    Null,
    NaN,
    PosInf,
    NefInf,
    ArraySeparator,
    MemberSeparator,
    Comment,
    Error
  };

  struct Token {
    TokenType type;
    Location Start;
    Location end;
  };

  struct ErrorInfo {
    Token token;
    String message;
    Location extra;
  };

  typedef Array<ErrorInfo> ErrorList;

  bool ReadToken(Token& token);
  void SkipSpaces();
  bool Match(Location pattern, int32 pattern_len);
  bool ReadComment();
  bool ReadCStyleComment();
  bool ReadCppStyleComment();
  bool ReadString();
  bool ReadSingleQuotedString();
  void ReadNumber();
  bool ReadValue();
  bool ReadObject(Token& token);
  bool ReadArray(Token& token);
  bool DecodeNumber(Token& token);
  bool DecodeNumber(Token& token, JValue& decoded);
  bool DecodeString(Token& token);
  bool DecodeString(Token& token, String& decoded);
  bool DecodeDouble(Token& token);
  bool DecodeDouble(Token& token, JValue& decoded);
  bool DecodeUnicodeCodePoint(Token& token, Location& cur, Location end,
                              uint32& unicode);
  bool DecodeUnicodeEscapeSequence(Token& token, Location& cur, Location end,
                                   uint32& unicode);
  bool AddError(const String& message, Token& token, Location extra = nullptr);
  bool RecoverFromError(TokenType skip_until_token);
  bool AddErrorAndRecover(const String& message, Token& token,
                          TokenType skip_until_token);
  // void SkipUntilSpace();
  JValue& CurrentValue();
  char GetNextChar();
  void GetLocationLineAndColumn(Location location, int32& line,
                                int32& column) const;
  String GetLocationLineAndColumn(Location location) const;
  void AddComment(Location begin, Location end, CommentPlacement placement);
  void SkipCommentTokens(Token& token);

  Array<JValue*> nodes_;
  ErrorList errors_;
  Location begin_;
  Location end_;
  Location current_;
  Location last_value_end_;
  JValue* last_value_;
  String comments_before_;
  Features features_;
  // TODO features쪽으로 옮겨주는게 좋을까??
  bool collect_comments_;
};

}  // namespace json
}  // namespace fun
