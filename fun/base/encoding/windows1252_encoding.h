#pragma once

#include "fun/base/base.h"
#include "fun/base/encoding/text_encoding.h"

namespace fun {

class FUN_BASE_API Windows1252Encoding : public TextEncoding {
 public:
  Windows1252Encoding();
  ~Windows1252Encoding();

  // TextEncoding interface
  const char* GetCanonicalName() const override;
  bool IsA(const String& encoding_name) const override;
  const CharacterMap& GetCharacterMap() const override;
  int32 Convert(const uint8* bytes) const override;
  int32 Convert(int32 ch, uint8* bytes, int32 len) const override;
  int32 QueryConvert(const uint8* bytes, int32 len) const override;
  int32 SequenceLength(const uint8* bytes, int32 len) const override;

 private:
  static const char* names_[];
  static const CharacterMap char_map_;
};

} // namespace fun
