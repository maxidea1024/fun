//TODO 16bit 문자열에 대해서만 동작함. 전반적으로 문자열 타입에 대해서 정리가 필요함.
#include "fun/base/string/uni_string_matcher.h"
#include "fun/base/math/math_base.h"
#include "fun/base/memory.h"

#ifdef _MSC_VER
#pragma warning(disable : 4244) // truncation warning
#endif

namespace fun {

namespace {

FUN_ALWAYS_INLINE uint16 FoldCase(uint16 c) {
  return CharTraitsU::IsLower((UNICHAR)c) ? uint16(CharTraitsU::ToUpper((UNICHAR)c)) : uint16(c);
}

FUN_ALWAYS_INLINE void bm_init_skiptable(
    const uint16* pattern,
    int32 pattern_len,
    uint8* skip_table,
    CaseSensitivity casesense) {
  int32 adjusted_len = MathBase::Min(pattern_len, 255);

  UnsafeMemory::Memset(skip_table, adjusted_len, 256);
  pattern += pattern_len - adjusted_len;

  if (casesense == CaseSensitivity::CaseSensitive) {
    while (adjusted_len--) {
      skip_table[*pattern++ & 0xFF] = adjusted_len;
    }
  } else { // Ignore case
    while (adjusted_len--) {
      skip_table[FoldCase(*pattern++) & 0xFF] = adjusted_len;
    }
  }
}

FUN_ALWAYS_INLINE int32 bm_find(
    const uint16* haystack,
    int32 haystack_len,
    int32 haystack_offset,
    const uint16* needle,
    uint32 needle_len,
    const uint8* skip_table,
    CaseSensitivity casesense) {
  if (needle_len == 0) {
    return haystack_offset > haystack_len ? INVALID_INDEX : haystack_offset;
  }

  const uint32 needle_len_minus_one = needle_len - 1;

  const uint16* current = haystack + haystack_offset + needle_len_minus_one;
  const uint16* end = haystack + haystack_len;

  if (casesense == CaseSensitivity::CaseSensitive) {
    while (current < end) {
      uint32 skip = skip_table[*current & 0xFF];
      if (skip == 0) {
        // possible match
        while (skip < needle_len) {
          if (*(current - skip) != needle[needle_len_minus_one - skip]) {
            break;
          }
          ++skip;
        }

        if (skip > needle_len_minus_one) { // we have a match
          return (current - haystack) - skip + 1;
        }

        // in case we don't have a match we are a bit inefficient as we only skip by one
        // when we have the non matching UNICHAR in the string.
        if (skip_table[*(current - skip) & 0xFF] == needle_len) {
          skip = needle_len - skip;
        } else {
          skip = 1;
        }
      }

      // exit loop if end over
      if (current > end - skip) {
        break;
      }

      current += skip;
    }
  } else { // Ignore case
    while (current < end) {
      uint32 skip = skip_table[FoldCase(*current) & 0xFF];
      if (skip == 0) {
        // possible match
        while (skip < needle_len) {
          if (FoldCase(*(current - skip)) != FoldCase(needle[needle_len_minus_one - skip])) {
            break;
          }
          ++skip;
        }

        if (skip > needle_len_minus_one) { // we have a match
          return (current - haystack) - skip + 1;
        }

        // in case we don't have a match we are a bit inefficient as we only skip by one
        // when we have the non matching UNICHAR in the string.
        if (skip_table[FoldCase(*(current - skip)) & 0xFF] == needle_len) {
          skip = needle_len - skip;
        } else {
          skip = 1;
        }
      }

      // exit loop if end over
      if (current > end - skip) {
        break;
      }

      current += skip;
    }
  }

  return INVALID_INDEX; // not found
}

} // namespace

UStringMatcher::UStringMatcher() {
  UnsafeMemory::Memset(skip_table_, 0x00, sizeof(skip_table_));
}

UStringMatcher::UStringMatcher(UStringView pattern, CaseSensitivity casesense)
  : pattern_(pattern), casesense_(casesense) {
  UpdateSkipTable();
}

UStringMatcher::~UStringMatcher() {
  // nothing to do.
}

UStringMatcher::UStringMatcher(const UStringMatcher& rhs)
  : pattern_(rhs.pattern_), casesense_(rhs.casesense_) {
  UpdateSkipTable();
}

UStringMatcher& UStringMatcher::operator = (const UStringMatcher& rhs) {
  if (FUN_LIKELY(&rhs != this)) {
    pattern_ = rhs.pattern_;
    casesense_ = rhs.casesense_;

    UpdateSkipTable();
  }

  return *this;
}

void UStringMatcher::SetPattern(UStringView pattern) {
  if (pattern != pattern_) {
    pattern_ = pattern;

    UpdateSkipTable();
  }
}

void UStringMatcher::SetCaseSensitivity(CaseSensitivity casesense) {
  if (casesense != casesense_) {
    casesense_ = casesense;

    UpdateSkipTable();
  }
}

int32 UStringMatcher::IndexIn(UStringView str, int32 from) const {
  if (from < 0) {
    from = 0;
  }

  return bm_find( reinterpret_cast<const uint16*>(str.ConstData()), str.Len(), from,
                  reinterpret_cast<const uint16*>(pattern_.ConstData()), pattern_.Len(), skip_table_, casesense_);
}

const UString& UStringMatcher::GetPattern() const {
  return pattern_;
}

CaseSensitivity UStringMatcher::GetCaseSensitivity() const {
  return casesense_;
}

void UStringMatcher::UpdateSkipTable() {
  bm_init_skiptable(reinterpret_cast<const uint16*>(pattern_.ConstData()), pattern_.Len(), skip_table_, casesense_);
}

// Standalone utility function

int32 UStringMatcher::FastFindChar( const UNICHAR* str,
                                    int32 length,
                                    UNICHAR ch,
                                    int32 from,
                                    CaseSensitivity casesense) {
  const uint16* ustr = (const uint16*)str;
  uint16 uc = (uint16)ch;

  if (from < 0) {
    from = MathBase::Max(from + length, 0);
  }

  if (from < length) {
    const uint16* p = ustr + from - 1;
    const uint16* e = ustr + length;

    if (casesense == CaseSensitivity::CaseSensitive) {
      while (++p != e) {
        if (*p == uc) {
          return p - ustr;
        }
      }
    } else { // Ignore case
      uc = FoldCase(uc);
      while (++p != e) {
        if (FoldCase(*p) == uc) {
          return p - ustr;
        }
      }
    }
  }
  return INVALID_INDEX; // not found
}


int32 UStringMatcher::FastLastFindChar( const UNICHAR* str,
                                        int32 length,
                                        UNICHAR ch,
                                        int32 from,
                                        CaseSensitivity casesense) {
  if (length <= 0) {
    return INVALID_INDEX;
  }

  if (from < 0) {
    from += length;
  } else if (from > length) {
    from = length - 1;
  }

  if (from >= 0) {
    const UNICHAR* b = str;
    const UNICHAR* p = str + from + 1;

    if (casesense == CaseSensitivity::CaseSensitive) {
      while (p-- != b) {
        if (*p == ch) {
          return p - b;
        }
      }
    } else { // Ignore case
      ch = FoldCase(ch);
      while (p-- != b) {
        if (FoldCase(*p) == ch) {
          return p - b;
        }
      }
    }
  }

  return INVALID_INDEX; // not found
}

static int32 Find_BoyerMoore( const UNICHAR* haystack,
                              int32 haystack_len,
                              int32 haystack_offset,
                              const UNICHAR* needle,
                              int32 needle_len,
                              CaseSensitivity casesense) {
  uint8 skip_table[256];
  bm_init_skiptable((const uint16*)needle, needle_len, skip_table, casesense);

  if (haystack_offset < 0) {
    haystack_offset = 0;
  }

  return bm_find( (const uint16*)haystack, haystack_len, haystack_offset,
                  (const uint16*)needle, needle_len, skip_table, casesense);
}


#define REHASH(A) \
  if (needle_len_minus_one < sizeof(uint32) * CHAR_BIT) { \
    hash_haystack -= uint32(A) << needle_len_minus_one; \
  } \
  hash_haystack <<= 1;

int32 UStringMatcher::FastFind( const UNICHAR* in_haystack,
                                int32 haystack_len,
                                int32 from,
                                const UNICHAR* needle,
                                int32 needle_len,
                                CaseSensitivity casesense) {
  if (from < 0) {
    from += haystack_len;
  }

  if (uint32(needle_len + from) > uint32(haystack_len)) {
    return INVALID_INDEX;
  }

  if (!needle_len) {
    return from;
  }

  if (haystack_len == 0) {
    return INVALID_INDEX;
  }

  if (needle_len == 1) {
    return FastFindChar(in_haystack, haystack_len, needle[0], from, casesense);
  }

  // We use the Boyer-Moore algorithm in cases where the overhead
  // for the skip table should pay off, otherwise we use a simple
  // hash function.
  if (haystack_len > 500 && needle_len > 5) {
    return Find_BoyerMoore(in_haystack, haystack_len, from, needle, needle_len, casesense);
  }

  // We use some hashing for efficiency's sake. Instead of
  // comparing strings, we compare the hash value of str with that
  // of a part of this QString. Only if that matches, we call memcmp().
  const UNICHAR* haystack = in_haystack + from;
  const UNICHAR* end = in_haystack + (haystack_len - needle_len);
  const uint32 needle_len_minus_one = needle_len - 1;
  uint32 hash_needle = 0;
  uint32 hash_haystack = 0;
  if (casesense == CaseSensitivity::CaseSensitive) {
    for (int32 i = 0; i < needle_len; ++i) {
      hash_needle = ((hash_needle << 1) + needle[i]);
      hash_haystack = ((hash_haystack << 1) + haystack[i]);
    }

    hash_haystack -= *(haystack + needle_len_minus_one);

    while (haystack <= end) {
      hash_haystack += *(haystack + needle_len_minus_one);

      if (hash_haystack == hash_needle &&
          *needle == *haystack &&
          CStringTraitsU::Strncmp(needle, haystack, needle_len) == 0) {
        return haystack - in_haystack;
      }

      REHASH(*haystack);

      ++haystack;
    }
  } else { // Ignore case
    for (int32 i = 0; i < needle_len; ++i) {
      hash_needle = ((hash_needle << 1) + FoldCase(needle[i]));
      hash_haystack = ((hash_haystack << 1) + FoldCase(haystack[i]));
    }

    hash_haystack -= FoldCase(*(haystack + needle_len_minus_one));

    while (haystack <= end) {
      hash_haystack += FoldCase(*(haystack + needle_len_minus_one));

      if (hash_haystack == hash_needle &&
          FoldCase(*needle) == FoldCase(*haystack) &&
          CStringTraitsU::Strnicmp(needle, haystack, needle_len) == 0) {
        return haystack - in_haystack;
      }

      REHASH(FoldCase(*haystack));

      ++haystack;
    }
  }
  return INVALID_INDEX;
}

int32 UStringMatcher::FastLastFind( const UNICHAR* haystack,
                                    int32 haystack_len,
                                    int32 from,
                                    const UNICHAR* needle,
                                    int32 needle_len,
                                    CaseSensitivity casesense) {
  const int32 delta = haystack_len - needle_len;

  if (from < 0) {
    from = delta;
  }

  if (from < 0 || from > haystack_len) {
    return INVALID_INDEX;
  }

  if (from > delta) {
    from = delta;
  }

  const UNICHAR* end = haystack;
  haystack += from;
  const uint32 needle_len_minus_one = needle_len - 1;
  const UNICHAR* N = needle + needle_len_minus_one;
  const UNICHAR* H = haystack + needle_len_minus_one;
  uint32 hash_needle = 0;
  uint32 hash_haystack = 0;
  if (casesense == CaseSensitivity::CaseSensitive) {
    for (int32 i = 0; i < needle_len; ++i) {
      hash_needle = ((hash_needle << 1) + *(N - i));
      hash_haystack = ((hash_haystack << 1) + *(H - i));
    }

    hash_haystack -= *haystack;

    while (haystack >= end) {
      hash_haystack += *haystack;

      if (hash_haystack == hash_needle &&
          CStringTraitsU::Strncmp(needle, haystack, needle_len) == 0) {
        return haystack - end;
      }

      --haystack;

      REHASH(*(haystack + needle_len));
    }
  } else {
    for (int32 i = 0; i < needle_len; ++i) {
      hash_needle = ((hash_needle << 1) + FoldCase(*(N - i)));
      hash_haystack = ((hash_haystack << 1) + FoldCase(*(H - i)));
    }

    hash_haystack -= FoldCase(*haystack);

    while (haystack >= end) {
      hash_haystack += FoldCase(*haystack);

      if (hash_haystack == hash_needle &&
          CStringTraitsU::Strnicmp(needle, haystack, needle_len) == 0) {
        return haystack - end;
      }

      --haystack;

      REHASH(FoldCase(*(haystack + needle_len)));
    }
  }

  return INVALID_INDEX; // not found
}

} // namespace fun
