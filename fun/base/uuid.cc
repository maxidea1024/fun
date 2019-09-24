#include "fun/base/uuid.h"
#include "fun/base/byte_order.h"
#include "fun/base/exception.h"
#include "fun/base/serialization/archive.h"
#include "fun/base/uuid_generator.h"
#include "fun/base/uuid_parser.h"

namespace fun {

/*! \class Uuid
  \brief Uuid 클래스는 GUID(Globally Unique IDentifier)를 저장하고 표현하는데
  사용합니다.

  GUID는 분산 컴퓨팅 환경에서 객체를 식별하는 유일한 표준 방법입니다. GUID는
  몇몇 알고리즘(방식)에 의해서 생성된 128비트(16바이트) 번호로 분산 컴퓨팅
  환경에서 공유하게 사용됩니다. GUID는 UUID(Universal Unique IDentifier)와 같은
  말입니다. \n GUID 레이아웃은 아래와 같습니다. \code 0                   1 2 3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                          time_low                             |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |       time_mid                |         time_hi_and_version   |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |clk_seq_hi_res |  clk_seq_low  |         node (0-1)            |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                         node (2-5)                            |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  RFC4122
  \endcode
*/

const Uuid Uuid::None;
const Uuid Uuid::DNS(AsciiString("6ba7b810-9dad-11d1-80b4-00c04fd430c8"));
const Uuid Uuid::URI(AsciiString("6ba7b811-9dad-11d1-80b4-00c04fd430c8"));
const Uuid Uuid::OID(AsciiString("6ba7b812-9dad-11d1-80b4-00c04fd430c8"));
const Uuid Uuid::X500(AsciiString("6ba7b814-9dad-11d1-80b4-00c04fd430c8"));

Uuid::Uuid(const String& str) {
  try {
    Parse(str);
  } catch (Exception&) {
    *this = None;

    // TODO
    // fun_log(Warning, "Can not parse guid from string '%s'. Replace with the
    // value None.", *str);
  }
}

Uuid Uuid::FromBytes(const uint8* bytes) {
  fun_check_ptr(bytes);

  Uuid ret;
  ret.u.a = ((uint32)bytes[3] << 24) | ((uint32)bytes[2] << 16) |
            ((uint32)bytes[1] << 8) | bytes[0];
  ret.u.b = (uint16)(((uint32)bytes[5] << 8) | bytes[4]);
  ret.u.c = (uint16)(((uint32)bytes[7] << 8) | bytes[6]);
  ret.u.d = bytes[8];
  ret.u.e = bytes[9];
  ret.u.f = bytes[10];
  ret.u.g = bytes[11];
  ret.u.h = bytes[12];
  ret.u.i = bytes[13];
  ret.u.j = bytes[14];
  ret.u.k = bytes[15];
  return ret;
}

Uuid Uuid::FromBytes(const ByteArray& bytes) {
  fun_check(bytes.Len() == 16);
  if (bytes.Len() != 16) {
    return Uuid::None;
  }

  return FromBytes((const uint8*)bytes.ConstData());
}

void Uuid::ToBytes(uint8* buffer) const {
  fun_check_ptr(buffer);

  buffer[0] = uint8(u.a);
  buffer[1] = uint8(u.a >> 8);
  buffer[2] = uint8(u.a >> 16);
  buffer[3] = uint8(u.a >> 24);
  buffer[4] = uint8(u.b);
  buffer[5] = uint8(u.b >> 8);
  buffer[6] = uint8(u.c);
  buffer[7] = uint8(u.c >> 8);
  buffer[8] = u.d;
  buffer[9] = u.e;
  buffer[10] = u.f;
  buffer[11] = u.g;
  buffer[12] = u.h;
  buffer[13] = u.i;
  buffer[14] = u.j;
  buffer[15] = u.k;
}

ByteArray Uuid::ToBytes() const {
  ByteArray result(16, NoInit);
  ToBytes((uint8*)result.MutableData());
  return result;
}

Uuid Uuid::FromRFC4122(const uint8* bytes) {
  fun_check_ptr(bytes);

  Uuid ret;

  ret.u.a = (uint32(bytes[0]) << 24) | (uint32(bytes[1]) << 16) |
            (uint32(bytes[2]) << 8) | bytes[3];

  ret.u.b = uint16((uint32(bytes[4]) << 8) | bytes[5]);

  ret.u.c = uint16((uint32(bytes[6]) << 8) | bytes[7]);

  ret.u.d = bytes[9];
  ret.u.e = bytes[8];

  ret.u.f = bytes[10];
  ret.u.g = bytes[11];
  ret.u.h = bytes[12];
  ret.u.i = bytes[13];
  ret.u.j = bytes[14];
  ret.u.k = bytes[15];

  return ret;
}

void Uuid::ToRFC4122(uint8* buffer) const {
  fun_check_ptr(buffer);

  buffer[0] = uint8(u.a >> 24);
  buffer[1] = uint8(u.a >> 16);
  buffer[2] = uint8(u.a >> 8);
  buffer[3] = uint8(u.a);

  buffer[4] = uint8(u.b >> 8);
  buffer[5] = uint8(u.b);

  buffer[6] = uint8(u.c >> 8);
  buffer[7] = uint8(u.c);

  buffer[8] = u.e;
  buffer[9] = u.d;

  buffer[10] = u.f;
  buffer[11] = u.g;
  buffer[12] = u.h;
  buffer[13] = u.i;
  buffer[14] = u.j;
  buffer[15] = u.k;
}

namespace {

inline char* HexsToChars(char* dst, uint32 a, uint32 b, bool hex = false) {
  if (hex) {
    *dst++ = '0';
    *dst++ = 'x';
  }
  *dst++ = CharTraitsA::NibbleToHexChar(a >> 4);
  *dst++ = CharTraitsA::NibbleToHexChar(a & 0xF);

  if (hex) {
    *dst++ = ',';
    *dst++ = '0';
    *dst++ = 'x';
  }
  *dst++ = CharTraitsA::NibbleToHexChar(b >> 4);
  *dst++ = CharTraitsA::NibbleToHexChar(b & 0xF);
  return dst;
}

}  // namespace

String Uuid::ToString(UuidFormat format) const {
  char buffer[68 + 1];
  char* dst = buffer;

  // ObjectId는 일반 GUID 표기 형식이 아니므로, 별도 처리해야함.
  // 00000000-00000000-00000000-00000000
  if (format == UuidFormat::ObjectId) {
    dst = HexsToChars(dst, u.packed1 >> 24, u.packed1 >> 16);
    dst = HexsToChars(dst, u.packed1 >> 8, u.packed1);
    *dst++ = '-';
    dst = HexsToChars(dst, u.packed2 >> 24, u.packed2 >> 16);
    dst = HexsToChars(dst, u.packed2 >> 8, u.packed2);
    *dst++ = '-';
    dst = HexsToChars(dst, u.packed3 >> 24, u.packed3 >> 16);
    dst = HexsToChars(dst, u.packed3 >> 8, u.packed3);
    *dst++ = '-';
    dst = HexsToChars(dst, u.packed4 >> 24, u.packed4 >> 16);
    dst = HexsToChars(dst, u.packed4 >> 8, u.packed4);
    return String(buffer, 35);
  }

  bool dash = true;
  bool hex = false;
  int32 len = 0;

  switch (format) {
    // 00000000-0000-0000-0000-000000000000
    case UuidFormat::DigitsWithHyphens:
      len = 36;
      break;

    // 00000000000000000000000000000000
    case UuidFormat::Digits:
      len = 32;
      dash = false;
      break;

    // {00000000-0000-0000-0000-000000000000}
    case UuidFormat::DigitsWithHyphensInBraces:
      len = 38;

      *dst++ = '{';      // begin '{'
      buffer[37] = '}';  // end   '}'
      break;

    // (00000000-0000-0000-0000-000000000000)
    case UuidFormat::DigitsWithHyphensInParentheses:
      len = 38;

      *dst++ = '(';      // begin '('
      buffer[37] = ')';  // end   ')'
      break;

    // {0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}}
    case UuidFormat::HexValuesInBraces:
      len = 68;
      dash = false;  // meaningless, just clarity
      hex = true;

      *dst++ = '{';      // begin '{'
      buffer[67] = '}';  // end   '}'
      break;

    // 위에서 별도로 처리하므로, 이곳에 도달할 수 없음.
    case UuidFormat::ObjectId:
      fun_unexpected();
      break;

    default:
      // TODO error illegal guid format specifier.
      fun_unexpected();
      return String();
  }

  if (hex) {
    // {0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}}
    *dst++ = '0';
    *dst++ = 'x';
    dst = HexsToChars(dst, u.a >> 24, u.a >> 16);
    dst = HexsToChars(dst, u.a >> 8, u.a);
    *dst++ = ',';
    *dst++ = '0';
    *dst++ = 'x';
    dst = HexsToChars(dst, u.b >> 8, u.b);
    *dst++ = ',';
    *dst++ = '0';
    *dst++ = 'x';
    dst = HexsToChars(dst, u.c >> 8, u.c);
    *dst++ = ',';
    *dst++ = '{';
    dst = HexsToChars(dst, u.d, u.e, true);
    *dst++ = ',';
    dst = HexsToChars(dst, u.f, u.g, true);
    *dst++ = ',';
    dst = HexsToChars(dst, u.h, u.i, true);
    *dst++ = ',';
    dst = HexsToChars(dst, u.j, u.k, true);
    *dst++ = '}';
  } else {
    // [{|(]00000000[-]0000[-]0000[-]0000[-]000000000000[}|)]
    dst = HexsToChars(dst, u.a >> 24, u.a >> 16);
    dst = HexsToChars(dst, u.a >> 8, u.a);
    if (dash) *dst++ = '-';
    dst = HexsToChars(dst, u.b >> 8, u.b);
    if (dash) *dst++ = '-';
    dst = HexsToChars(dst, u.c >> 8, u.c);
    if (dash) *dst++ = '-';
    dst = HexsToChars(dst, u.d, u.e);
    if (dash) *dst++ = '-';
    dst = HexsToChars(dst, u.f, u.g);
    dst = HexsToChars(dst, u.h, u.i);
    dst = HexsToChars(dst, u.j, u.k);
  }

  return String(buffer, len);
}

String Uuid::ToString(const String& format) const {
  if (format.Len() != 1) {
    throw InvalidArgumentException(AsciiString("Illegal format specifier"));
  }

  const char f = format[0];
  return ToString(CharToFormat(f));
}

String Uuid::ToString() const {
  return ToString(UuidFormat::DigitsWithHyphens);
}

Uuid Uuid::Parse(const String& str) {
  UuidParser::ParsingResult result;
  if (!UuidParser::TryParse(str, result)) {
    throw SyntaxException(result.failure_message);
  }

  return result.parsed_uuid;
}

Uuid Uuid::Parse(const String& str, const String& format) {
  if (format.Len() != 1) {
    throw InvalidArgumentException(AsciiString("Illegal format specifier"));
  }

  const char f = format[0];
  UuidParser::ParsingResult result;
  if (!UuidParser::TryParse(str, CharToFormat(f), result)) {
    throw SyntaxException(result.failure_message);
  }

  return result.parsed_uuid;
}

Uuid Uuid::Parse(const String& str, UuidFormat format) {
  UuidParser::ParsingResult result;
  if (!UuidParser::TryParse(str, format, result)) {
    throw SyntaxException(result.failure_message);
  }

  return result.parsed_uuid;
}

bool Uuid::TryParse(const String& str, Uuid& out_uuid) {
  UuidParser::ParsingResult result;
  if (UuidParser::TryParse(str, result)) {
    out_uuid = result.parsed_uuid;
    return true;
  } else {
    out_uuid = None;
    return false;
  }
}

bool Uuid::TryParse(const String& str, UuidFormat format, Uuid& out_uuid) {
  UuidParser::ParsingResult result;
  if (UuidParser::TryParse(str, format, result)) {
    out_uuid = result.parsed_uuid;
    return true;
  } else {
    out_uuid = None;
    return false;
  }
}

bool Uuid::TryParse(const String& str, const String& format, Uuid& out_uuid) {
  if (format.Len() != 1) {
    // throw InvalidArgumentException(AsciiString("Illegal format specifier"));
    return false;
  }

  const char f = format[0];
  UuidParser::ParsingResult result;
  const bool ok = UuidParser::TryParse(str, CharToFormat(f), result);
  if (ok) {
    out_uuid = result.parsed_uuid;
  } else {
    out_uuid = None;
  }

  return ok;
}

int32 Uuid::Compare(const Uuid& rhs) const {
#if 0
  if (u.time_low != rhs.u.time_low) {
    return u.time_low < rhs.u.time_low ? -1 : 1;
  }

  if (u.time_mid != rhs.u.time_mid) {
    return u.time_mid < rhs.u.time_mid ? -1 : 1;
  }

  if (u.time_hi_and_version != rhs.u.time_hi_and_version) {
    return u.time_hi_and_version < rhs.u.time_hi_and_version ? -1 : 1;
  }

  if (u.clock_seq != rhs.u.clock_seq) {
    return u.clock_seq < rhs.u.clock_seq ? -1 : 1;
  }

  for (int32 i = 0; i < countof(u.node); ++i) {
    if (u.node[i] < rhs.u.node[i]) {
      return -1;
    } else if (u.node[i] > rhs.u.node[i]) {
      return 1;
    }
  }
#else
  //일반 GUID포맷은 순서가 의미 없지만(?) ObjectId일 경우에는 버젼으로
  //사용될수도 있으므로, 순서가 중요할수도 있다.

  // if (u.a != rhs.u.a) { return u.a < rhs.u.a ? -1 : 1; }
  // if (u.b != rhs.u.b) { return u.b < rhs.u.b ? -1 : 1; }
  // if (u.c != rhs.u.c) { return u.c < rhs.u.c ? -1 : 1; }
  // if (u.d != rhs.u.d) { return u.d < rhs.u.d ? -1 : 1; }
  // if (u.e != rhs.u.e) { return u.e < rhs.u.e ? -1 : 1; }
  // if (u.f != rhs.u.f) { return u.f < rhs.u.f ? -1 : 1; }
  // if (u.g != rhs.u.g) { return u.g < rhs.u.g ? -1 : 1; }
  // if (u.h != rhs.u.h) { return u.h < rhs.u.h ? -1 : 1; }
  // if (u.i != rhs.u.i) { return u.i < rhs.u.i ? -1 : 1; }
  // if (u.j != rhs.u.j) { return u.j < rhs.u.j ? -1 : 1; }
  // if (u.k != rhs.u.k) { return u.k < rhs.u.k ? -1 : 1; }

  if (u.packed1 != rhs.u.packed1) {
    return u.packed1 < rhs.u.packed1 ? -1 : 1;
  }
  if (u.packed2 != rhs.u.packed2) {
    return u.packed2 < rhs.u.packed2 ? -1 : 1;
  }
  if (u.packed3 != rhs.u.packed3) {
    return u.packed3 < rhs.u.packed3 ? -1 : 1;
  }
  if (u.packed4 != rhs.u.packed4) {
    return u.packed4 < rhs.u.packed4 ? -1 : 1;
  }
#endif

  return 0;
}

Uuid Uuid::NewUuid() { return UuidGenerator::DefaultGenerator().NewUuid(); }

Uuid Uuid::NewSecuredRandomUuid() {
  return UuidGenerator::DefaultGenerator().NewSecuredRandomUuid();
}

Uuid Uuid::NewRandomUuid() {
  return UuidGenerator::DefaultGenerator().NewRandomUuid();
}

Uuid Uuid::NewUuidFromName(const Uuid& ns_id, const String& name) {
  return UuidGenerator::DefaultGenerator().NewUuidFromName(ns_id, name);
}

Uuid Uuid::NewUuidFromName(const Uuid& ns_id, const String& name,
                           CryptographicHash& hasher) {
  return UuidGenerator::DefaultGenerator().NewUuidFromName(ns_id, name, hasher);
}

Uuid Uuid::NewUuidFromName(const Uuid& ns_id, const String& name,
                           CryptographicHash& hasher, UuidVersion version) {
  return UuidGenerator::DefaultGenerator().NewUuidFromName(ns_id, name, hasher,
                                                           version);
}

uint32 HashOf(const Uuid& uuid) {
#if 0
  return Crc::MemCrc32(&uuid, sizeof(uuid));
#else
  return uuid.u.a ^ (((uint32)uuid.u.b << 16) | (uint32)uuid.u.c) ^
         (((uint32)uuid.u.f << 24) | uuid.u.k);  // from C# reference source
#endif
}

Archive& operator&(Archive& ar, Uuid& uuid) {
  static_assert(sizeof(Uuid) == Uuid::BYTE_LENGTH, "sizeof(Uuid) == 16");

#if 0
  return ar & uuid.u.a & uuid.u.b & uuid.u.c & uuid.u.d;
#else
  uint8 bytes[Uuid::BYTE_LENGTH];
  if (ar.IsLoading()) {
    ar.Serialize(bytes, Uuid::BYTE_LENGTH);
    uuid = Uuid::FromBytes(bytes);
  } else {
    uint8 bytes[Uuid::BYTE_LENGTH];
    uuid.ToBytes(bytes);
    ar.Serialize(bytes, Uuid::BYTE_LENGTH);
  }
  return ar;
#endif
}

}  // namespace fun
