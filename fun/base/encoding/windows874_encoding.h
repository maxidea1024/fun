#pragma once

#include "fun/base/base.h"
#include "fun/base/encoding/double_byte_encoding.h"

namespace fun {

/**
 * windows-874 encoding.
 *
 * This text encoding class has been generated from
 * http://www.unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/CP874.TXT.
 */
class FUN_BASE_API Windows874_Encoding : public DoubleByteEncoding {
 public:
  Windows874_Encoding();
  ~Windows874_Encoding();

 private:
  static const char* names_[];
  static const CharacterMap char_map_;
  static const Mapping mapping_table_[];
  static const Mapping reverse_mapping_table_[];
};

}  // namespace fun
