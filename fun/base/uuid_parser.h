#pragma once

#include "fun/base/base.h"
#include "fun/base/uuid.h"

namespace fun {

class UuidParser {
 public:
  struct ParsingResult {
    Uuid parsed_uuid;
    String failure_message;

    void SetFailure(const String& message) { failure_message = message; }
  };

  static bool TryParse(const String& str, ParsingResult& result);
  static bool TryParse(const String& str, UuidFormat format,
                       ParsingResult& result);

 private:
  static bool TryParseWithDigits(const String& str, ParsingResult& result);
  static bool TryParseWithDigitsWithHyphens(const String& str,
                                            ParsingResult& result);
  static bool TryParseWithDigitsWithHyphensInBraces(const String& str,
                                                    ParsingResult& result);
  static bool TryParseWithDigitsWithHyphensInParentheses(const String& str,
                                                         ParsingResult& result);
  static bool TryParseWithHexValuesInBraces(const String& str,
                                            ParsingResult& result);
  static bool TryParseWithObjectId(const String& str,
                                   ParsingResult& result);  // ADDED
};

}  // namespace fun
