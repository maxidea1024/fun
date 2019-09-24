#include "fun/base/encoding/utf16_encoding.h"
#include "fun/base/byte_order.h"
#include "fun/base/string/string.h"

namespace fun {

const char* Utf16Encoding::names_[] = {
  "UTF-16",
  "UTF16",
  nullptr
};

const TextEncoding::CharacterMap Utf16Encoding::char_map_ = {
  /* 00 */  -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
  /* 10 */  -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
  /* 20 */  -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
  /* 30 */  -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
  /* 40 */  -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
  /* 50 */  -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
  /* 60 */  -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
  /* 70 */  -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
  /* 80 */  -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
  /* 90 */  -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
  /* a0 */  -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
  /* b0 */  -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
  /* c0 */  -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
  /* d0 */  -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
  /* e0 */  -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
  /* f0 */  -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
};

Utf16Encoding::Utf16Encoding(ByteOrderType byte_order) {
  SetByteOrder(byte_order);
}

Utf16Encoding::Utf16Encoding(int32 byte_order_mark) {
  SetByteOrder(byte_order_mark);
}

Utf16Encoding::~Utf16Encoding() {}

Utf16Encoding::ByteOrderType Utf16Encoding::GetByteOrder() const {
#if FUN_ARCH_BIG_ENDIAN
  return flip_bytes_ ? LITTLE_ENDIAN_BYTE_ORDER : BIG_ENDIAN_BYTE_ORDER;
#else
  return flip_bytes_ ? BIG_ENDIAN_BYTE_ORDER : LITTLE_ENDIAN_BYTE_ORDER;
#endif
}

void Utf16Encoding::SetByteOrder(ByteOrderType byte_order) {
#if FUN_ARCH_BIG_ENDIAN
  flip_bytes_ = byte_order == LITTLE_ENDIAN_BYTE_ORDER;
#else
  flip_bytes_ = byte_order == BIG_ENDIAN_BYTE_ORDER;;
#endif
}

void Utf16Encoding::SetByteOrder(int32 byte_order_mark) {
  flip_bytes_ = byte_order_mark != 0xFEFF;
}

const char* Utf16Encoding::GetCanonicalName() const {
  return names_[0];
}

bool Utf16Encoding::IsA(const String& encoding_name) const {
  for (const char** name = names_; *name; ++name) {
    if (icompare(encoding_name, *name) == 0) {
      return true;
    }
  }
  return false;
}

const TextEncoding::CharacterMap& Utf16Encoding::GetCharacterMap() const {
  return char_map_;
}

int32 Utf16Encoding::Convert(const uint8* bytes) const {
  uint16 uc;
  uint8* p = (uint8*)&uc;
  *p++ = *bytes++;
  *p++ = *bytes++;

  if (flip_bytes_) {
    ByteOrder::FlipBytes(uc);
  }

  if (uc >= 0xd800 && uc < 0xdc00) {
    uint16 uc2;
    p = (uint8*)&uc2;
    *p++ = *bytes++;
    *p++ = *bytes++;

    if (flip_bytes_) {
      ByteOrder::FlipBytes(uc2);
    }
    if (uc2 >= 0xdc00 && uc2 < 0xe000) {
      return ((uc & 0x3ff) << 10) + (uc2 & 0x3ff) + 0x10000;
    } else {
      return -1;
    }
  } else {
    return uc;
  }
}

int32 Utf16Encoding::Convert(int32 ch, uint8* bytes, int32 len) const {
  if (ch <= 0xFFFF) {
    if (bytes && len >= 2) {
      uint16 ch1 = flip_bytes_ ? ByteOrder::FlipBytes((uint16) ch) : (uint16) ch;
      uint8* p = (uint8*)&ch1;
      *bytes++ = *p++;
      *bytes++ = *p++;
    }
    return 2;
  } else {
    if (bytes && len >= 4) {
      int32 ch1 = ch - 0x10000;
      uint16 w1 = 0xD800 + ((ch1 >> 10) & 0x3FF);
      uint16 w2 = 0xDC00 + (ch1 & 0x3FF);
      if (flip_bytes_) {
        w1 = ByteOrder::FlipBytes(w1);
        w2 = ByteOrder::FlipBytes(w2);
      }
      uint8* p = (uint8*)&w1;
      *bytes++ = *p++;
      *bytes++ = *p++;
      p = (uint8*)&w2;
      *bytes++ = *p++;
      *bytes++ = *p++;
    }
    return 4;
  }
}

int32 Utf16Encoding::QueryConvert(const uint8* bytes, int32 len) const {
  int32 ret = -2;

  if (len >= 2) {
    uint16 uc;
    uint8* p = (uint8*)&uc;
    *p++ = *bytes++;
    *p++ = *bytes++;
    if (flip_bytes_) {
      ByteOrder::FlipBytes(uc);
    }
    if (uc >= 0xd800 && uc < 0xdc00) {
      if (len >= 4) {
        uint16 uc2;
        p = (uint8*)&uc2;
        *p++ = *bytes++;
        *p++ = *bytes++;
        if (flip_bytes_) {
          ByteOrder::FlipBytes(uc2);
        }
        if (uc2 >= 0xdc00 && uc < 0xe000) {
          ret = ((uc & 0x3ff) << 10) + (uc2 & 0x3ff) + 0x10000;
        } else {
          ret = -1; // Malformed sequence
        }
      } else {
        ret = -4; // surrogate pair, four bytes needed
      }
    } else {
      ret = uc;
    }
  }

  return ret;
}

int32 Utf16Encoding::SequenceLength(const uint8* bytes, int32 len) const {
  int32 ret = -2;

  if (flip_bytes_) {
    if (len >= 1) {
      uint8 c = *bytes;
      if (c >= 0xd8 && c < 0xdc) {
        ret = 4;
      } else {
        ret = 2;
      }
    }
  } else {
    if (len >= 2) {
      uint16 uc;
      uint8* p = (uint8*)&uc;
      *p++ = *bytes++;
      *p++ = *bytes++;
      if (uc >= 0xd800 && uc < 0xdc00) {
        ret = 4;
      } else {
        ret = 2;
      }
    }
  }
  return ret;
}

} // namespace fun
