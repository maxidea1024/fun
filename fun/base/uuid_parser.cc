#include "fun/base/uuid_parser.h"

namespace fun {

bool UuidParser::TryParse(const String& str, ParsingResult& result) {
  const String str2 = str.Simplified();

  // {0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}}
  if (str2.Len() == 68) {
    return TryParseWithHexValuesInBraces(str2, result);
  }
  // 00000000000000000000000000000000
  else if (str2.Len() == 32) {
    return TryParseWithDigits(str2, result);
  }
  // 00000000-0000-0000-0000-000000000000
  else if (str2.Len() == 36) {
    return TryParseWithDigitsWithHyphens(str2, result);
  }
  // {00000000-0000-0000-0000-000000000000}
  else if (str2.Len() == 38 && str2[0] == '{') {
    return TryParseWithDigitsWithHyphensInBraces(str2, result);
  }
  // (00000000-0000-0000-0000-000000000000)
  else if (str2.Len() == 38 && str2[0] == '(') {
    return TryParseWithDigitsWithHyphensInParentheses(str2, result);
  }
  // 00000000-00000000-00000000-00000000
  if (str2.Len() == 35) {
    return TryParseWithObjectId(str2, result);
  } else {
    result.SetFailure("Illegal guid str2");
    return false;
  }
}

bool UuidParser::TryParse(const String& str, UuidFormat format,
                          ParsingResult& result) {
  const String str2 = str.Simplified();

  switch (format) {
    case UuidFormat::Digits:
      return TryParseWithDigits(str2, result);
    case UuidFormat::DigitsWithHyphens:
      return TryParseWithDigitsWithHyphens(str2, result);
    case UuidFormat::DigitsWithHyphensInBraces:
      return TryParseWithDigitsWithHyphensInBraces(str2, result);
    case UuidFormat::DigitsWithHyphensInParentheses:
      return TryParseWithDigitsWithHyphensInParentheses(str2, result);
    case UuidFormat::HexValuesInBraces:
      return TryParseWithHexValuesInBraces(str2, result);
    case UuidFormat::ObjectId:
      return TryParseWithObjectId(str2, result);
  }
  fun_unexpected();
  return false;
}

namespace {

inline uint32 ReadHexUInt32_NoFail(const String& str, int32 offset) {
  return (CharTraitsA::HexCharToNibble(str[offset + 0]) << 28) |
         (CharTraitsA::HexCharToNibble(str[offset + 1]) << 24) |
         (CharTraitsA::HexCharToNibble(str[offset + 2]) << 20) |
         (CharTraitsA::HexCharToNibble(str[offset + 3]) << 16) |
         (CharTraitsA::HexCharToNibble(str[offset + 4]) << 12) |
         (CharTraitsA::HexCharToNibble(str[offset + 5]) << 8) |
         (CharTraitsA::HexCharToNibble(str[offset + 6]) << 4) |
         (CharTraitsA::HexCharToNibble(str[offset + 7]));
}

inline uint16 ReadHexUInt16_NoFail(const String& str, int32 offset) {
  return (CharTraitsA::HexCharToNibble(str[offset + 0]) << 12) |
         (CharTraitsA::HexCharToNibble(str[offset + 1]) << 8) |
         (CharTraitsA::HexCharToNibble(str[offset + 2]) << 4) |
         (CharTraitsA::HexCharToNibble(str[offset + 3]));
}

inline uint8 ReadHexUInt8_NoFail(const String& str, int32 offset) {
  return (CharTraitsA::HexCharToNibble(str[offset]) << 4) |
         CharTraitsA::HexCharToNibble(str[offset + 1]);
}

}  // namespace

// 00000000000000000000000000000000
bool UuidParser::TryParseWithDigits(const String& str, ParsingResult& result) {
  if (str.Len() == 0) {
    result.SetFailure("It is an empty guid str");
    return false;
  }

  if (str.Len() != 32) {
    result.SetFailure("Illegal guid str");
    return false;
  }

  for (int32 i = 0; i < str.Len(); ++i) {
    if (!CharTraitsA::IsHexDigit(str[i])) {
      result.SetFailure("Hex digit is expected");
      return false;
    }
  }

  result.parsed_uuid.u.a = ReadHexUInt32_NoFail(str, 0);
  result.parsed_uuid.u.b = ReadHexUInt16_NoFail(str, 8);
  result.parsed_uuid.u.c = ReadHexUInt16_NoFail(str, 12);
  result.parsed_uuid.u.d = ReadHexUInt8_NoFail(str, 16);
  result.parsed_uuid.u.e = ReadHexUInt8_NoFail(str, 18);
  result.parsed_uuid.u.f = ReadHexUInt8_NoFail(str, 20);
  result.parsed_uuid.u.g = ReadHexUInt8_NoFail(str, 22);
  result.parsed_uuid.u.h = ReadHexUInt8_NoFail(str, 24);
  result.parsed_uuid.u.i = ReadHexUInt8_NoFail(str, 26);
  result.parsed_uuid.u.j = ReadHexUInt8_NoFail(str, 28);
  result.parsed_uuid.u.k = ReadHexUInt8_NoFail(str, 30);

  return true;
}

// 00000000-0000-0000-0000-000000000000
bool UuidParser::TryParseWithDigitsWithHyphens(const String& str,
                                               ParsingResult& result) {
  if (str.Len() == 0) {
    result.SetFailure("It is an empty guid str");
    return false;
  }

  if (str.Len() != 36) {
    result.SetFailure("Illegal guid str");
    return false;
  }

  const int32 dash_pos[] = {8, 13, 18, 23};
  for (int32 i = 0; i < countof(dash_pos); ++i) {
    const int32 pos = dash_pos[i];
    if (str[pos] != '-') {
      result.SetFailure("'-' is expected");
      return false;
    }
  }

  for (int32 i = 0; i < str.Len(); ++i) {
    const char c = str[i];
    if (c != '-') {
      if (!CharTraitsA::IsHexDigit(c)) {
        result.SetFailure("Hex digit is expected");
        return false;
      }
    }
  }

  result.parsed_uuid.u.a = ReadHexUInt32_NoFail(str, 0);
  result.parsed_uuid.u.b = ReadHexUInt16_NoFail(str, 9);
  result.parsed_uuid.u.c = ReadHexUInt16_NoFail(str, 14);
  result.parsed_uuid.u.d = ReadHexUInt8_NoFail(str, 19);
  result.parsed_uuid.u.e = ReadHexUInt8_NoFail(str, 21);
  result.parsed_uuid.u.f = ReadHexUInt8_NoFail(str, 24);
  result.parsed_uuid.u.g = ReadHexUInt8_NoFail(str, 26);
  result.parsed_uuid.u.h = ReadHexUInt8_NoFail(str, 28);
  result.parsed_uuid.u.i = ReadHexUInt8_NoFail(str, 30);
  result.parsed_uuid.u.j = ReadHexUInt8_NoFail(str, 32);
  result.parsed_uuid.u.k = ReadHexUInt8_NoFail(str, 34);

  return true;
}

// {00000000-0000-0000-0000-000000000000}
bool UuidParser::TryParseWithDigitsWithHyphensInBraces(const String& str,
                                                       ParsingResult& result) {
  if (str.Len() == 0) {
    result.SetFailure("It is an empty guid str");
    return false;
  }

  if (str.Len() != 38) {
    result.SetFailure("Illegal guid str");
    return false;
  }

  if (str[0] != '{') {
    result.SetFailure("Must be started with '{'");
    return false;
  }

  if (str[37] != '}') {
    result.SetFailure("Must be terminated with '}'");
    return false;
  }

  const int32 dash_pos[] = {9, 14, 19, 24};
  for (int32 i = 0; i < countof(dash_pos); ++i) {
    const int32 pos = dash_pos[i];
    if (str[pos] != '-') {
      result.SetFailure("'-' is expected");
      return false;
    }
  }

  for (int32 i = 1; i < str.Len() - 1; ++i) {
    const char c = str[i];
    if (c != '-') {
      if (!CharTraitsA::IsHexDigit(c)) {
        result.SetFailure("Hex digit is expected");
        return false;
      }
    }
  }

  result.parsed_uuid.u.a = ReadHexUInt32_NoFail(str, 1);
  result.parsed_uuid.u.b = ReadHexUInt16_NoFail(str, 10);
  result.parsed_uuid.u.c = ReadHexUInt16_NoFail(str, 15);
  result.parsed_uuid.u.d = ReadHexUInt8_NoFail(str, 20);
  result.parsed_uuid.u.e = ReadHexUInt8_NoFail(str, 22);
  result.parsed_uuid.u.f = ReadHexUInt8_NoFail(str, 25);
  result.parsed_uuid.u.g = ReadHexUInt8_NoFail(str, 27);
  result.parsed_uuid.u.h = ReadHexUInt8_NoFail(str, 29);
  result.parsed_uuid.u.i = ReadHexUInt8_NoFail(str, 31);
  result.parsed_uuid.u.j = ReadHexUInt8_NoFail(str, 33);
  result.parsed_uuid.u.k = ReadHexUInt8_NoFail(str, 35);

  return true;
}

// (00000000-0000-0000-0000-000000000000)
bool UuidParser::TryParseWithDigitsWithHyphensInParentheses(
    const String& str, ParsingResult& result) {
  if (str.Len() == 0) {
    result.SetFailure("It is an empty guid str");
    return false;
  }

  if (str.Len() != 38) {
    result.SetFailure("Illegal guid str");
    return false;
  }

  if (str[0] != '(') {
    result.SetFailure("Must be started with '('");
    return false;
  }

  if (str[37] != ')') {
    result.SetFailure("Must be terminated with ')'");
    return false;
  }

  const int32 dash_pos[] = {9, 14, 19, 24};
  for (int32 i = 0; i < countof(dash_pos); ++i) {
    const int32 pos = dash_pos[i];
    if (str[pos] != '-') {
      result.SetFailure("'-' is expected");
      return false;
    }
  }

  for (int32 i = 1; i < str.Len() - 1; ++i) {
    const char c = str[i];
    if (c != '-') {
      if (!CharTraitsA::IsHexDigit(c)) {
        result.SetFailure("Hex digit is expected");
        return false;
      }
    }
  }

  result.parsed_uuid.u.a = ReadHexUInt32_NoFail(str, 1);
  result.parsed_uuid.u.b = ReadHexUInt16_NoFail(str, 10);
  result.parsed_uuid.u.c = ReadHexUInt16_NoFail(str, 15);
  result.parsed_uuid.u.d = ReadHexUInt8_NoFail(str, 20);
  result.parsed_uuid.u.e = ReadHexUInt8_NoFail(str, 22);
  result.parsed_uuid.u.f = ReadHexUInt8_NoFail(str, 25);
  result.parsed_uuid.u.g = ReadHexUInt8_NoFail(str, 27);
  result.parsed_uuid.u.h = ReadHexUInt8_NoFail(str, 29);
  result.parsed_uuid.u.i = ReadHexUInt8_NoFail(str, 31);
  result.parsed_uuid.u.j = ReadHexUInt8_NoFail(str, 33);
  result.parsed_uuid.u.k = ReadHexUInt8_NoFail(str, 35);

  return true;
}

// {0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}}
bool UuidParser::TryParseWithHexValuesInBraces(const String& str,
                                               ParsingResult& result) {
  if (str.Len() == 0) {
    result.SetFailure("It is an empty guid str");
    return false;
  }

  if (str.Len() != 68) {
    result.SetFailure("Illegal guid str");
    return false;
  }

  if (str[0] != '{' || str[26] != '{') {
    result.SetFailure("'{' is expected");
    return false;
  }

  if (str[66] != '}' || str[67] != '}') {
    result.SetFailure("Must be terminated with '}}'");
    return false;
  }

  const int32 comma_pos[] = {11, 18, 25, 31, 36, 41, 46, 51, 56, 61};
  for (int32 i = 0; i < countof(comma_pos); ++i) {
    const int32 pos = comma_pos[i];
    if (str[pos] != ',') {
      result.SetFailure("',' is expected");
      return false;
    }
  }

  const int32 zx_pos[] = {1, 12, 19, 27, 32, 37, 42, 47, 52, 57, 62};
  for (int32 i = 0; i < countof(zx_pos); ++i) {
    const int32 pos = zx_pos[i];
    if (str[pos] != '0' || (str[pos + 1] != 'x' && str[pos + 1] != 'X')) {
      result.SetFailure("'0x' is expected");
      return false;
    }
  }

  const int32 digit_pos_and_len[] = {3, 8,  14, 4,  21, 4,  29, 2,  34, 2,  39,
                                     2, 44, 2,  49, 2,  54, 2,  59, 2,  64, 2};
  for (int32 i = 0; i < countof(digit_pos_and_len); i += 2) {
    const int32 pos = digit_pos_and_len[i];
    const int32 len = digit_pos_and_len[i + 1];
    for (int32 digit_idx = pos; digit_idx < pos + len; ++digit_idx) {
      if (!CharTraitsA::IsHexDigit(str[digit_idx])) {
        result.SetFailure("Hex digit is expected");
        return false;
      }
    }
  }

  result.parsed_uuid.u.a = ReadHexUInt32_NoFail(str, 3);
  result.parsed_uuid.u.b = ReadHexUInt16_NoFail(str, 14);
  result.parsed_uuid.u.c = ReadHexUInt16_NoFail(str, 21);
  result.parsed_uuid.u.d = ReadHexUInt8_NoFail(str, 29);
  result.parsed_uuid.u.e = ReadHexUInt8_NoFail(str, 34);
  result.parsed_uuid.u.f = ReadHexUInt8_NoFail(str, 39);
  result.parsed_uuid.u.g = ReadHexUInt8_NoFail(str, 44);
  result.parsed_uuid.u.h = ReadHexUInt8_NoFail(str, 49);
  result.parsed_uuid.u.i = ReadHexUInt8_NoFail(str, 54);
  result.parsed_uuid.u.j = ReadHexUInt8_NoFail(str, 59);
  result.parsed_uuid.u.k = ReadHexUInt8_NoFail(str, 64);

  return true;
}

// 바이트 인코딩에 대해서 한번더 생각해봐야함.
// 00000000-00000000-00000000-00000000
bool UuidParser::TryParseWithObjectId(const String& str,
                                      ParsingResult& result) {
  if (str.Len() == 0) {
    result.SetFailure("It is an empty guid str");
    return false;
  }

  if (str.Len() != 35) {
    result.SetFailure("Illegal guid str");
    return false;
  }

  const int32 dash_pos[] = {8, 17, 26};
  for (int32 i = 0; i < countof(dash_pos); ++i) {
    const int32 pos = dash_pos[i];
    if (str[pos] != '-') {
      result.SetFailure("'-' is expected");
      return false;
    }
  }

  for (int32 i = 1; i < str.Len() - 1; ++i) {
    const char c = str[i];
    if (c != '-') {
      if (!CharTraitsA::IsHexDigit(c)) {
        result.SetFailure("Hex digit is expected");
        return false;
      }
    }
  }

  // 00000000-00000000-00000000-00000000
  result.parsed_uuid.u.packed1 = ReadHexUInt32_NoFail(str, 0);
  result.parsed_uuid.u.packed2 = ReadHexUInt32_NoFail(str, 9);
  result.parsed_uuid.u.packed3 = ReadHexUInt32_NoFail(str, 18);
  result.parsed_uuid.u.packed4 = ReadHexUInt32_NoFail(str, 27);

  return true;
}

}  // namespace fun
