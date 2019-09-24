#pragma once

#include "fun/base/base.h"
#include "fun/base/encoding/encodings.h"
#include "fun/base/encoding/text_encoding.h"

namespace fun {

/**
 * This abstract class is a base class for various double-byte character
 * set (DBCS) encodings.
 *
 * Double-byte encodings are variants of multi-byte encodings
 * where (unicode) each code point is represented by one or
 * two bytes. unicode code points are restricted to the
 * Basic Multilingual Plane.
 *
 * Subclasses must provide encoding names, a static CharacterMap, as well
 * as static Mapping and reverse Mapping tables, and provide these to the
 * DoubleByteEncoding constructor.
 */
class FUN_BASE_API DoubleByteEncoding : public TextEncoding {
 public:
  struct Mapping {
    uint16 from;
    uint16 to;
  };

  // TextEncoding
  const char* GetCanonicalName() const;
  bool IsA(const String& encoding_name) const;
  const CharacterMap& GetCharacterMap() const;
  int32 Convert(const uint8* bytes) const;
  int32 Convert(int32 ch, uint8* bytes, int32 len) const;
  int32 QueryConvert(const uint8* bytes, int32 len) const;
  int32 SequenceLength(const uint8* bytes, int32 len) const;

 protected:
  /**
   * Creates a DoubleByteEncoding using the given mapping and reverse-mapping
   * tables.
   *
   * names must be a static array declared in the derived class,
   * containing the names of this encoding, declared as:
   *
   *   const char* MyEncoding::names_[] = {
   *     "myencoding",
   *     "MyEncoding",
   *     nullptr
   *   };
   *
   * The first entry in names must be the canonical name.
   *
   * char_map must be a static CharacterMap giving information about double-byte
   * character sequences.
   *
   * For each mapping_table item, from must be a value in range 0x0100 to
   * 0xFFFF for double-byte mappings, which the most significant (upper) byte
   * representing the first character in the sequence and the lower byte
   * representing the second character in the sequence.
   *
   * For each reverse_mapping_table item, from must be unicode code point from
   * the Basic Multilingual Plane, and to is a one-byte or two-byte sequence. As
   * with mapping_table, a one-byte sequence is in range 0x00 to 0xFF, and a
   * two-byte sequence is in range 0x0100 to 0xFFFF.
   *
   * unicode code points are restricted to the Basic Multilingual Plane
   * (code points 0x0000 to 0xFFFF).
   *
   * Items in both tables must be sorted by from, in ascending order.
   */
  DoubleByteEncoding(const char** names,
                     const TextEncoding::CharacterMap& char_map,
                     const Mapping mapping_table[], size_t mapping_table_size,
                     const Mapping reverse_mapping_table[],
                     size_t reverse_mapping_table_size);

  /**
   * Destroys the DoubleByteEncoding.
   */
  ~DoubleByteEncoding();

  /**
   * Maps a double-byte encoded character to its unicode code point.
   *
   * Returns the unicode code point, or -1 if the encoded character is bad
   * and cannot be mapped.
   */
  int32 Map(uint16 encoded) const;

  /**
   * Maps a unicode code point to its double-byte representation.
   *
   * Returns -1 if the code point cannot be mapped, otherwise
   * a value in range 0 to 0xFF for single-byte mappings, or
   * 0x0100 to 0xFFFF for double-byte mappings.
   */
  int32 ReverseMap(int32 cp) const;

 private:
  DoubleByteEncoding();

  const char** names_;
  const TextEncoding::CharacterMap& char_map_;
  const Mapping* mapping_table_;
  const size_t mapping_table_size_;
  const Mapping* reverse_mapping_table_;
  const size_t reverse_mapping_table_size_;
};

}  // namespace fun
