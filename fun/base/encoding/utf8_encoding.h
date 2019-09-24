#pragma once

#include "fun/base/base.h"
#include "fun/base/encoding/text_encoding.h"

namespace fun {

/**
 * UTF-8 text encoding, as defined in RFC 2279.
 */
class FUN_BASE_API Utf8Encoding : public TextEncoding {
 public:
  Utf8Encoding();
  ~Utf8Encoding();

  // TextEncoding interface
  const char* GetCanonicalName() const override;
  bool IsA(const String& encoding_name) const override;
  const CharacterMap& GetCharacterMap() const override;
  int32 Convert(const uint8* bytes) const override;
  int32 Convert(int32 ch, uint8* bytes, int32 len) const override;
  int32 QueryConvert(const uint8* bytes, int32 len) const override;
  int32 SequenceLength(const uint8* bytes, int32 len) const override;

  /**
   * Utility routine to tell whether a sequence of bytes is legal UTF-8.
   * This must be called with the len pre-determined by the first byte.
   * The sequence is illegal right away if there aren't enough bytes
   * available. If presented with a len > 4, this function returns false.
   * The unicode definition of UTF-8 goes up to 4-byte sequences.
   * 
   * Adapted from ftp://ftp.unicode.org/Public/PROGRAMS/CVTUTF/ConvertUTF.c
   * Copyright 2001-2004 unicode, Inc.
   */
  static bool IsLegal(const uint8 *bytes, int32 len);

 private:
  static const char* names_[];
  static const CharacterMap char_map_;
};

} // namespace fun
