#pragma once

#include "fun/base/base.h"
#include "fun/base/encoding/double_byte_encoding.h"

namespace fun {

/**
 * windows-932 encoding.
 *
 * This text encoding class has been generated from
 * http://www.unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/CP932.TXT.
 */
class FUN_BASE_API Windows932_Encoding : public DoubleByteEncoding {
 public:
  Windows932_Encoding();
  ~Windows932_Encoding();

 private:
  static const char* names_[];
  static const CharacterMap char_map_;
  static const Mapping mapping_table_[];
  static const Mapping reverse_mapping_table_[];
};

}  // namespace fun
