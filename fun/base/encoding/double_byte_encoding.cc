#include "fun/base/encoding/double_byte_encoding.h"

#include <algorithm>

#include "fun/base/string/string.h"

namespace fun {

DoubleByteEncoding::DoubleByteEncoding(
    const char* names[], const TextEncoding::CharacterMap& char_map,
    const Mapping mapping_table[], size_t mapping_table_size,
    const Mapping reverse_mapping_table[], size_t reverse_mapping_table_size)
    : names_(names),
      char_map_(char_map),
      mapping_table_(mapping_table),
      mapping_table_size_(mapping_table_size),
      reverse_mapping_table_(reverse_mapping_table),
      reverse_mapping_table_size_(reverse_mapping_table_size) {}

DoubleByteEncoding::~DoubleByteEncoding() {}

const char* DoubleByteEncoding::GetCanonicalName() const { return names_[0]; }

bool DoubleByteEncoding::IsA(const String& encoding_name) const {
  for (const char** name = names_; *name; ++name) {
    if (icompare(encoding_name, *name) == 0) {
      return true;
    }
  }
  return false;
}

const TextEncoding::CharacterMap& DoubleByteEncoding::GetCharacterMap() const {
  return char_map_;
}

int32 DoubleByteEncoding::Convert(const uint8* bytes) const {
  int32 n = char_map_[*bytes];
  switch (n) {
    case -1:
      return -1;
    case -2:
      return map(static_cast<uint16>(bytes[0] << 8) | bytes[1]);
    default:
      return n;
  }
}

int32 DoubleByteEncoding::Convert(int32 ch, uint8* bytes, int32 len) const {
  int32 n = ReverseMap(ch);
  if (n < 0) {
    return 0;
  }

  if (!bytes || !len) {
    return n > 0xFF ? 2 : 1;
  }
  if (n > 0xFF && len < 2) {
    return 0;
  }

  if (n > 0xFF) {
    bytes[0] = static_cast<uint8>(n >> 8);
    bytes[1] = static_cast<uint8>(n & 0xFF);
    return 2;
  } else {
    bytes[0] = static_cast<uint8>(n);
    return 1;
  }
}

int32 DoubleByteEncoding::QueryConvert(const uint8* bytes, int32 len) const {
  int32 n = char_map_[*bytes];
  switch (n) {
    case -1:
      return -1;
    case -2:
      if (len >= 2) {
        return map((bytes[0] << 8) | bytes[1]);
      } else {
        return -2;
      }
    default:
      return n;
  }
}

int32 DoubleByteEncoding::SequenceLength(const uint8* bytes, int32 len) const {
  if (1 <= len) {
    int32 cc = char_map_[*bytes];
    if (cc >= 0) {
      return 1;
    } else if (cc < -1) {
      return -cc;
    } else {
      return -1;
    }
  } else {
    return -1;
  }
}

struct MappingLessThan {
  bool operator()(const DoubleByteEncoding::Mapping& mapping,
                  const uint16& key) const {
    return mapping.from < key;
  }
};

int32 DoubleByteEncoding::Map(uint16 encoded) const {
  const Mapping* begin = mapping_table_;
  const Mapping* end = begin + mapping_table_size_;
  const Mapping* it = std::lower_bound(begin, end, encoded, MappingLessThan());
  if (it != end && it->from == encoded) {
    return it->to;
  } else {
    return -1;
  }
}

int32 DoubleByteEncoding::ReverseMap(int32 cp) const {
  const Mapping* begin = reverse_mapping_table_;
  const Mapping* end = begin + reverse_mapping_table_size_;
  const Mapping* it =
      std::lower_bound(begin, end, static_cast<uint16>(cp), MappingLessThan());
  if (it != end && it->from == cp) {
    return it->to;
  } else {
    return -1;
  }
}

}  // namespace fun
