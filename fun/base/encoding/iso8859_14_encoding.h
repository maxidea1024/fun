﻿#pragma once

#include "fun/base/base.h"
#include "fun/base/encoding/double_byte_encoding.h"

namespace fun {

/**
 * ISO-8859-14 Encoding.
 *
 * This text encoding class has been generated from
 * http://www.unicode.org/Public/MAPPINGS/ISO8859/8859-14.TXT.
 */
class FUN_BASE_API ISO8859_14_Encoding : public DoubleByteEncoding {
 public:
  ISO8859_14_Encoding();
  ~ISO8859_14_Encoding();

 private:
  static const char* names_[];
  static const CharacterMap char_map_;
  static const Mapping mapping_table_[];
  static const Mapping reverse_mapping_table_[];
};

}  // namespace fun
