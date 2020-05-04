#pragma once

#include "fun/base/base.h"
#include "fun/base/encoding/text_encoding.h"

namespace fun {

class FUN_BASE_API Utf32Encoding : public TextEncoding {
 public:
  enum ByteOrderType {
    BIG_ENDIAN_BYTE_ORDER,
    LITTLE_ENDIAN_BYTE_ORDER,
    NATIVE_BYTE_ORDER
  };

  Utf32Encoding(ByteOrderType byte_order = NATIVE_BYTE_ORDER);
  Utf32Encoding(int32 byte_order_mark);
  ~Utf32Encoding();

  ByteOrderType GetByteOrder() const;
  void SetByteOrder(ByteOrderType byte_order = NATIVE_BYTE_ORDER);
  void SetByteOrder(int32 byte_order_mark);

  // TextEncoding interface
  const char* GetCanonicalName() const override;
  bool IsA(const String& encoding_name) const override;
  const CharacterMap& GetCharacterMap() const override;
  int32 Convert(const uint8* bytes) const override;
  int32 Convert(int32 ch, uint8* bytes, int32 len) const override;
  int32 QueryConvert(const uint8* bytes, int32 len) const override;
  int32 SequenceLength(const uint8* bytes, int32 len) const override;

 private:
  bool flip_bytes_;

  static const char* names_[];
  static const CharacterMap char_map_;
};

}  // namespace fun
