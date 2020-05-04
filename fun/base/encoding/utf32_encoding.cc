#include "fun/base/encoding/utf32_encoding.h"

#include "fun/base/byte_order.h"
#include "fun/base/string/string.h"

namespace fun {

const char* Utf32Encoding::names_[] = {"UTF-32", "UTF32", nullptr};

const TextEncoding::CharacterMap Utf32Encoding::char_map_ = {
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2,
};

Utf32Encoding::Utf32Encoding(ByteOrderType byte_order) {
  SetByteOrder(byte_order);
}

Utf32Encoding::Utf32Encoding(int32 byte_order_mark) {
  SetByteOrder(byte_order_mark);
}

Utf32Encoding::~Utf32Encoding() {}

Utf32Encoding::ByteOrderType Utf32Encoding::GetByteOrder() const {
#if FUN_ARCH_BIG_ENDIAN
  return flip_bytes_ ? LITTLE_ENDIAN_BYTE_ORDER : BIG_ENDIAN_BYTE_ORDER;
#else
  return flip_bytes_ ? BIG_ENDIAN_BYTE_ORDER : LITTLE_ENDIAN_BYTE_ORDER;
#endif
}

void Utf32Encoding::SetByteOrder(ByteOrderType byte_order) {
#if FUN_ARCH_BIG_ENDIAN
  flip_bytes_ = byte_order == LITTLE_ENDIAN_BYTE_ORDER;
#else
  flip_bytes_ = byte_order == BIG_ENDIAN_BYTE_ORDER;
  ;
#endif
}

void Utf32Encoding::SetByteOrder(int32 byte_order_mark) {
  flip_bytes_ = byte_order_mark != 0xFEFF;
}

const char* Utf32Encoding::GetCanonicalName() const { return names_[0]; }

bool Utf32Encoding::IsA(const String& encoding_name) const {
  for (const char** name = names_; *name; ++name) {
    if (icompare(encoding_name, *name) == 0) {
      return true;
    }
  }
  return false;
}

const TextEncoding::CharacterMap& Utf32Encoding::GetCharacterMap() const {
  return char_map_;
}

int32 Utf32Encoding::Convert(const uint8* bytes) const {
  uint32 uc;
  uint8* p = (uint8*)&uc;
  *p++ = *bytes++;
  *p++ = *bytes++;
  *p++ = *bytes++;
  *p++ = *bytes++;

  if (flip_bytes_) {
    ByteOrder::FlipBytes(uc);
  }

  return uc;
}

int32 Utf32Encoding::Convert(int32 ch, uint8* bytes, int32 len) const {
  if (bytes && len >= 4) {
    uint32 ch1 = flip_bytes_ ? ByteOrder::FlipBytes((uint32)ch) : (uint32)ch;
    uint8* p = (uint8*)&ch1;
    *bytes++ = *p++;
    *bytes++ = *p++;
    *bytes++ = *p++;
    *bytes++ = *p++;
  }
  return 4;
}

int32 Utf32Encoding::QueryConvert(const uint8* bytes, int32 len) const {
  int32 ret = -4;

  if (len >= 4) {
    uint32 uc;
    uint8* p = (uint8*)&uc;
    *p++ = *bytes++;
    *p++ = *bytes++;
    *p++ = *bytes++;
    *p++ = *bytes++;
    if (flip_bytes_) {
      ByteOrder::FlipBytes(uc);
    }
    return uc;
  }

  return ret;
}

int32 Utf32Encoding::SequenceLength(const uint8* /*bytes*/,
                                    int32 /*len*/) const {
  return 4;
}

}  // namespace fun
