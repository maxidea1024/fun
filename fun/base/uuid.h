#pragma once

#include "fun/base/base.h"
#include "fun/base/cryptographic_hash.h"
#include "fun/base/string/byte_array.h"
#include "fun/base/ftl/template.h"

namespace fun {

enum class UuidVersion {
  MacAddress = 0,
  TimeBased = 1,
  DCE_UID = 2,
  NameBased = 3,
  Random = 4,
  NameBasedSHA1 = 5,
};

enum class UuidFormat {
  /**
   * 32 digits (format Specifier 'N')
   *
   * 00000000000000000000000000000000
   */
  Digits = 0,

  /**
   * 32 digits separated by hyphens (format Specifier 'D')
   *
   * 00000000-0000-0000-0000-000000000000
   */
  DigitsWithHyphens = 1,

  /**
   * 32 digits separated by hyphens, enclosed in braces: (format Specifier 'B')
   *
   * {00000000-0000-0000-0000-000000000000}
   */
  DigitsWithHyphensInBraces = 2,

  /**
   * 32 digits separated by hyphens, enclosed in parentheses: (format Specifier 'P')
   *
   * (00000000-0000-0000-0000-000000000000)
   */
  DigitsWithHyphensInParentheses = 3,

  /**
   * Four hexadecimal values enclosed in braces, where the fourth value is a subset of
   * eight hexadecimal values that is also enclosed in braces: (format Specifier 'X')
   *
   * {0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}}
   */
  HexValuesInBraces = 4,

  /**
   * Unformal format, just used internal usage.: (format Specifier 'O')
   *
   * 00000000-00000000-00000000-00000000
   */
  ObjectId,
};

class FUN_BASE_API Uuid {
 public:
  static const int32 BYTE_LENGTH = 16;

  static const Uuid None;
  static const Uuid DNS;
  static const Uuid URI;
  static const Uuid OID;
  static const Uuid X500;

  Uuid();
  Uuid(const uint8* bytes);
  Uuid(uint32 a, uint16 b, uint16 c, uint8 d, uint8 e, uint8 f, uint8 g, uint8 h, uint8 i, uint8 j, uint8 k);
  Uuid(uint32 a, uint16 b, uint16 c, const uint8* d);
  Uuid(uint32 packed1, uint32 packed2, uint32 packed3, uint32 packed4);
  explicit Uuid(const String& str);

  Uuid(const Uuid& rhs);
  Uuid& operator = (const Uuid& rhs);

  void Swap(Uuid& rhs);

  String ToString() const;
  String ToString(UuidFormat format) const;
  String ToString(const String& format) const;

  static Uuid Parse(const String& str);
  static Uuid Parse(const String& str, UuidFormat format);
  static Uuid Parse(const String& str, const String& format);

  static bool TryParse(const String& str, Uuid& out_uuid);
  static bool TryParse(const String& str, UuidFormat format, Uuid& out_uuid);
  static bool TryParse(const String& str, const String& format, Uuid& out_uuid);

  static Uuid FromBytes(const uint8* bytes);
  static Uuid FromBytes(const ByteArray& bytes);
  void ToBytes(uint8* buffer) const;
  ByteArray ToBytes() const;

  static Uuid FromRFC4122(const uint8* bytes);
  void ToRFC4122(uint8* buffer) const;

  UuidVersion GetVersion() const;
  int32 GetVariant() const;

  int32 Compare(const Uuid& rhs) const;
  bool Equals(const Uuid& rhs) const;

  bool operator == (const Uuid& rhs) const;
  bool operator != (const Uuid& rhs) const;
  bool operator <  (const Uuid& rhs) const;
  bool operator <= (const Uuid& rhs) const;
  bool operator >  (const Uuid& rhs) const;
  bool operator >= (const Uuid& rhs) const;

  bool IsValid() const;
  void Invalidate();

  explicit operator bool() const;
  bool operator !() const;

  static Uuid NewUuid();
  static Uuid NewSecuredRandomUuid();
  static Uuid NewRandomUuid();
  static Uuid NewUuidFromName(const Uuid& ns_id, const String& name);
  static Uuid NewUuidFromName(const Uuid& ns_id, const String& name, CryptographicHash& hasher);
  static Uuid NewUuidFromName(const Uuid& ns_id, const String& name, CryptographicHash& hasher, UuidVersion version);

  friend uint32 HashOf(const Uuid& uuid);
  friend Archive& operator & (Archive& ar, Uuid& uuid);

 private:
  alignas(4) union {
    // RFC4122 layout
    struct {
      uint32 time_low;
      uint16 time_mid;
      uint16 time_hi_and_version;
      uint16 clock_seq;
      uint8  node[6];
    };

    // C#(Microsoft) layout
    struct {
      uint32 a;
      uint16 b;
      uint16 c;
      uint8  d;
      uint8  e;
      uint8  f;
      uint8  g;
      uint8  h;
      uint8  i;
      uint8  j;
      uint8  k;
    };

    // Packed layout
    struct {
      uint32 packed1;
      uint32 packed2;
      uint32 packed3;
      uint32 packed4;
    };
  } u;

  friend class UuidGenerator;
  friend class UuidParser;

  Uuid( uint32 time_low,
        uint32 time_mid,
        uint32 time_hi_and_version,
        uint16 clock_seq,
        const uint8* node);

  Uuid(const uint8* bytes, UuidVersion version);

  static UuidFormat CharToFormat(char f);
};


//
// Inline codes
//

FUN_ALWAYS_INLINE Uuid::Uuid() {
  u.packed1 = u.packed2 = u.packed3 = u.packed4 = 0;
}

FUN_ALWAYS_INLINE Uuid::Uuid(const uint8* bytes) {
  *this = FromBytes(bytes);
}

FUN_ALWAYS_INLINE Uuid::Uuid(uint32 a,
                  uint16 b,
                  uint16 c,
                  uint8 d,
                  uint8 e,
                  uint8 f,
                  uint8 g,
                  uint8 h,
                  uint8 i,
                  uint8 j,
                  uint8 k) {
  u.a = a;
  u.b = b;
  u.c = c;
  u.d = d;
  u.e = e;
  u.f = f;
  u.g = g;
  u.h = h;
  u.i = i;
  u.j = j;
  u.k = k;
}

FUN_ALWAYS_INLINE Uuid::Uuid(uint32 a, uint16 b, uint16 c, const uint8* d) {
  u.a = a;
  u.b = b;
  u.c = c;
  u.d = d[0];
  u.e = d[1];
  u.f = d[2];
  u.g = d[3];
  u.h = d[4];
  u.i = d[5];
  u.j = d[6];
  u.k = d[7];
}

FUN_ALWAYS_INLINE Uuid::Uuid(uint32 time_low,
                  uint32 time_mid,
                  uint32 time_hi_and_version,
                  uint16 clock_seq,
                  const uint8* node) {
  u.time_low = time_low;
  u.time_mid = time_mid;
  u.time_hi_and_version = time_hi_and_version;
  u.clock_seq = clock_seq;
  UnsafeMemory::Memcpy(u.node, node, 6);
}

FUN_ALWAYS_INLINE Uuid::Uuid(const uint8* bytes, UuidVersion version) {
  *this = FromBytes(bytes);

  u.time_hi_and_version &= 0x0FFF;
  u.time_hi_and_version |= ((uint16)version << 12);
  u.clock_seq &= 0x3FFF;
  u.clock_seq |= 0x8000;
}

FUN_ALWAYS_INLINE Uuid::Uuid( uint32 packed1,
                              uint32 packed2,
                              uint32 packed3,
                              uint32 packed4) {
  u.packed1 = packed1;
  u.packed2 = packed2;
  u.packed3 = packed3;
  u.packed4 = packed4;
}

FUN_ALWAYS_INLINE Uuid::Uuid(const Uuid& rhs) {
  u.packed1 = rhs.u.packed1;
  u.packed2 = rhs.u.packed2;
  u.packed3 = rhs.u.packed3;
  u.packed4 = rhs.u.packed4;
}

FUN_ALWAYS_INLINE Uuid& Uuid::operator = (const Uuid& rhs) {
  if (FUN_LIKELY(&rhs != this)) {
    u.packed1 = rhs.u.packed1;
    u.packed2 = rhs.u.packed2;
    u.packed3 = rhs.u.packed3;
    u.packed4 = rhs.u.packed4;
  }

  return *this;
}

FUN_ALWAYS_INLINE void Uuid::Swap(Uuid& rhs) {
  fun::Swap(u.packed1, rhs.u.packed1);
  fun::Swap(u.packed2, rhs.u.packed2);
  fun::Swap(u.packed3, rhs.u.packed3);
  fun::Swap(u.packed4, rhs.u.packed4);
}

FUN_ALWAYS_INLINE UuidVersion Uuid::GetVersion() const {
  return (UuidVersion)(u.time_hi_and_version >> 12);
}

FUN_ALWAYS_INLINE int32 Uuid::GetVariant() const {
  const int32 v = u.clock_seq >> 13;
  if ((v & 6) == 6) {
    return v;
  } else if (v & 4) {
    return 2;
  } else {
    return 0;
  }
}

FUN_ALWAYS_INLINE bool Uuid::Equals(const Uuid& rhs) const {
  return Compare(rhs) == 0;
}

FUN_ALWAYS_INLINE bool Uuid::operator == (const Uuid& rhs) const {
  return Equals(rhs);
}

FUN_ALWAYS_INLINE bool Uuid::operator != (const Uuid& rhs) const {
  return !Equals(rhs);
}

FUN_ALWAYS_INLINE bool Uuid::operator < (const Uuid& rhs) const {
  return Compare(rhs) < 0;
}

FUN_ALWAYS_INLINE bool Uuid::operator <= (const Uuid& rhs) const {
  return Compare(rhs) <= 0;
}

FUN_ALWAYS_INLINE bool Uuid::operator > (const Uuid& rhs) const {
  return Compare(rhs) > 0;
}

FUN_ALWAYS_INLINE bool Uuid::operator >= (const Uuid& rhs) const {
  return Compare(rhs) >= 0;
}

FUN_ALWAYS_INLINE bool Uuid::IsValid() const {
  return *this != None;
}

FUN_ALWAYS_INLINE Uuid::operator bool() const {
  return IsValid();
}

FUN_ALWAYS_INLINE bool Uuid::operator !() const {
  return !IsValid();
}

FUN_ALWAYS_INLINE void Uuid::Invalidate() {
  *this = None;
}

FUN_ALWAYS_INLINE UuidFormat Uuid::CharToFormat(char f) {
  switch (f) {
    case 'D': case 'd':
      return UuidFormat::DigitsWithHyphens;
    case 'N': case 'n':
      return UuidFormat::Digits;
    case 'B': case 'b':
      return UuidFormat::DigitsWithHyphensInBraces;
    case 'P': case 'p':
      return UuidFormat::DigitsWithHyphensInParentheses;
    case 'X': case 'x':
      return UuidFormat::HexValuesInBraces;
    case 'O': case 'o':
      return UuidFormat::ObjectId;
  }

  fun_unexpected();
  return UuidFormat::DigitsWithHyphens;
}

FUN_ALWAYS_INLINE void Swap(Uuid& lhs, Uuid& rhs) {
  lhs.Swap(rhs);
}

namespace Lex {
FUN_ALWAYS_INLINE String ToString(const Uuid& value) {
  return value.ToString();
}
}

} // namespace fun
