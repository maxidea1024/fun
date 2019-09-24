#include "fun/base/string/byte_string_matcher.h"
#include "fun/base/base.h"
#include "fun/base/string/byte_string.h"
#include "fun/base/string/byte_string_lut.h"

#ifdef _MSC_VER
#pragma warning(disable : 4244)  // truncation warning
#endif

namespace fun {

static inline void bm_init_skiptable(const uint8* pattern, int32 pattern_len,
                                     uint8* skip_table,
                                     CaseSensitivity casesense) {
  int32 adjusted_len = MathBase::Min(pattern_len, 255);

  UnsafeMemory::Memset(skip_table, adjusted_len, 256);
  pattern += pattern_len - adjusted_len;

  if (casesense == CaseSensitivity::CaseSensitive) {
    while (adjusted_len--) {
      skip_table[*pattern++] = adjusted_len;
    }
  } else {  // Ignore case
    while (adjusted_len--) {
      skip_table[FoldCase(*pattern++)] = adjusted_len;
    }
  }
}

static inline int32 bm_find(const uint8* haystack, int32 haystack_len,
                            int32 haystack_offset, const uint8* needle,
                            uint32 needle_len, const uint8* skip_table,
                            CaseSensitivity casesense) {
  if (needle_len == 0) {
    return haystack_offset > haystack_len ? INVALID_INDEX : haystack_offset;
  }

  const uint32 needle_len_minus_one = needle_len - 1;

  const uint8* current = haystack + haystack_offset + needle_len_minus_one;
  const uint8* end = haystack + haystack_len;

  if (casesense == CaseSensitivity::CaseSensitive) {
    while (current < end) {
      uint32 skip = skip_table[*current];
      if (skip == 0) {
        // possible match
        while (skip < needle_len) {
          if (*(current - skip) != needle[needle_len_minus_one - skip]) {
            break;
          }
          ++skip;
        }

        if (skip > needle_len_minus_one) {  // we have a match
          return (current - haystack) - skip + 1;
        }

        // in case we don't have a match we are a bit inefficient as we only
        // skip by one when we have the non matching char in the string.
        if (skip_table[*(current - skip)] == needle_len) {
          skip = needle_len - skip;
        } else {
          skip = 1;
        }
      }

      // exit loop if end over.
      if (current > end - skip) {
        break;
      }

      current += skip;
    }
  } else {  // Ignore case
    while (current < end) {
      uint32 skip = skip_table[FoldCase(*current)];
      if (skip == 0) {
        // possible match
        while (skip < needle_len) {
          if (FoldCase(*(current - skip)) !=
              FoldCase(needle[needle_len_minus_one - skip])) {
            break;
          }
          ++skip;
        }

        if (skip > needle_len_minus_one) {  // we have a match
          return (current - haystack) - skip + 1;
        }

        // in case we don't have a match we are a bit inefficient as we only
        // skip by one when we have the non matching char in the string.
        if (skip_table[FoldCase(*(current - skip))] == needle_len) {
          skip = needle_len - skip;
        } else {
          skip = 1;
        }
      }

      // exit loop if end over.
      if (current > end - skip) {
        break;
      }

      current += skip;
    }
  }

  return INVALID_INDEX;  // not found
}

ByteStringMatcher::ByteStringMatcher() {
  UnsafeMemory::Memset(skip_table_, 0x00, sizeof(skip_table_));
}

ByteStringMatcher::ByteStringMatcher(ByteStringView pattern,
                                     CaseSensitivity casesense)
    : pattern_(pattern), casesense_(casesense) {
  UpdateSkipTable();
}

ByteStringMatcher::~ByteStringMatcher() {
  // nothing to do.
}

ByteStringMatcher::ByteStringMatcher(const ByteStringMatcher& rhs)
    : pattern_(rhs.pattern_), casesense_(rhs.casesense_) {
  UpdateSkipTable();
}

ByteStringMatcher& ByteStringMatcher::operator=(const ByteStringMatcher& rhs) {
  if (FUN_LIKELY(&rhs != this)) {
    pattern_ = rhs.pattern_;
    casesense_ = rhs.casesense_;
    UpdateSkipTable();
  }

  return *this;
}

void ByteStringMatcher::SetPattern(ByteStringView pattern) {
  if (pattern != pattern_) {
    pattern_ = pattern;
    UpdateSkipTable();
  }
}

void ByteStringMatcher::SetCaseSensitivity(CaseSensitivity casesense) {
  if (casesense != casesense_) {
    casesense_ = casesense;
    UpdateSkipTable();
  }
}

int32 ByteStringMatcher::IndexIn(ByteStringView str, int32 from) const {
  if (from < 0) {
    from = 0;
  }

  return bm_find(reinterpret_cast<const uint8*>(str.ConstData()), str.Len(),
                 from, reinterpret_cast<const uint8*>(pattern_.ConstData()),
                 pattern_.Len(), skip_table_, casesense_);
}

const ByteString& ByteStringMatcher::GetPattern() const { return pattern_; }

CaseSensitivity ByteStringMatcher::GetCaseSensitivity() const {
  return casesense_;
}

void ByteStringMatcher::UpdateSkipTable() {
  bm_init_skiptable(reinterpret_cast<const uint8*>(pattern_.ConstData()),
                    pattern_.Len(), skip_table_, casesense_);
}

// Standalone utility function

int32 ByteStringMatcher::FastFindChar(const char* str, int32 len, char ch,
                                      int32 from, CaseSensitivity casesense) {
  const uint8* ustr = (const uint8*)str;
  uint8 uc = (uint8)ch;

  if (from < 0) {
    from = MathBase::Max(from + len, 0);
  }

  if (from < len) {
    const uint8* p = ustr + from - 1;
    const uint8* e = ustr + len;

    if (casesense == CaseSensitivity::CaseSensitive) {
      while (++p != e) {
        if (*p == uc) {
          return p - ustr;
        }
      }
    } else {  // Ignore case
      uc = FoldCase(uc);
      while (++p != e) {
        if (FoldCase(*p) == uc) {
          return p - ustr;
        }
      }
    }
  }
  return INVALID_INDEX;  // not found
}

int32 ByteStringMatcher::FastLastFindChar(const char* str, int32 len, char ch,
                                          int32 from,
                                          CaseSensitivity casesense) {
  if (len <= 0) {
    return INVALID_INDEX;
  }

  if (from < 0) {
    from += len;
  } else if (from > len) {
    from = len - 1;
  }

  if (from >= 0) {
    const char* b = str;
    const char* p = str + from + 1;

    if (casesense == CaseSensitivity::CaseSensitive) {
      while (p-- != b) {
        if (*p == ch) {
          return p - b;
        }
      }
    } else {  // Ignore case
      ch = FoldCase(ch);
      while (p-- != b) {
        if (FoldCase(*p) == ch) {
          return p - b;
        }
      }
    }
  }

  return INVALID_INDEX;  // not found
}

static int32 Find_BoyerMoore(const char* haystack, int32 haystack_len,
                             int32 haystack_offset, const char* needle,
                             int32 needle_len, CaseSensitivity casesense) {
  uint8 skip_table[256];
  bm_init_skiptable((const uint8*)needle, needle_len, skip_table, casesense);

  if (haystack_offset < 0) {
    haystack_offset = 0;
  }

  return bm_find((const uint8*)haystack, haystack_len, haystack_offset,
                 (const uint8*)needle, needle_len, skip_table, casesense);
}

#define REHASH(A)                                         \
  if (needle_len_minus_one < sizeof(uint32) * CHAR_BIT) { \
    hash_haystack -= uint32(A) << needle_len_minus_one;   \
  }                                                       \
  hash_haystack <<= 1;

int32 ByteStringMatcher::FastFind(const char* in_haystack, int32 haystack_len,
                                  int32 from, const char* needle,
                                  int32 needle_len, CaseSensitivity casesense) {
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
    return Find_BoyerMoore(in_haystack, haystack_len, from, needle, needle_len,
                           casesense);
  }

  // We use some hashing for efficiency's sake. Instead of
  // comparing strings, we compare the hash value of str with that
  // of a part of this QString. Only if that matches, we call memcmp().
  const char* haystack = in_haystack + from;
  const char* end = in_haystack + (haystack_len - needle_len);
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

      if (hash_haystack == hash_needle && *needle == *haystack &&
          UnsafeMemory::Memcmp(needle, haystack, needle_len) == 0) {
        return haystack - in_haystack;
      }

      REHASH(*haystack);

      ++haystack;
    }
  } else {  // Ignore case
    for (int32 i = 0; i < needle_len; ++i) {
      hash_needle = ((hash_needle << 1) + FoldCase(needle[i]));
      hash_haystack = ((hash_haystack << 1) + FoldCase(haystack[i]));
    }

    hash_haystack -= FoldCase(*(haystack + needle_len_minus_one));

    while (haystack <= end) {
      hash_haystack += FoldCase(*(haystack + needle_len_minus_one));

      if (hash_haystack == hash_needle &&
          FoldCase(*needle) == FoldCase(*haystack) &&
          CStringTraitsA::Strnicmp(needle, haystack, needle_len) == 0) {
        return haystack - in_haystack;
      }

      REHASH(FoldCase(*haystack));

      ++haystack;
    }
  }
  return INVALID_INDEX;
}

int32 ByteStringMatcher::FastLastFind(const char* haystack, int32 haystack_len,
                                      int32 from, const char* needle,
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

  const char* end = haystack;
  haystack += from;
  const uint32 needle_len_minus_one = needle_len - 1;
  const char* N = needle + needle_len_minus_one;
  const char* H = haystack + needle_len_minus_one;
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
          UnsafeMemory::Memcmp(needle, haystack, needle_len) == 0) {
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
          CStringTraitsA::Strnicmp(needle, haystack, needle_len) == 0) {
        return haystack - end;
      }

      --haystack;

      REHASH(FoldCase(*(haystack + needle_len)));
    }
  }

  return INVALID_INDEX;  // not found
}

}  // namespace fun
