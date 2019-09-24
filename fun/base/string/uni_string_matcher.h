#pragma once

#include "fun/base/base.h"
#include "fun/base/string/char_traits.h"
#include "fun/base/string/string.h"

namespace fun {

class UStringMatcher {
 public:
  UStringMatcher();
  UStringMatcher(UStringView pattern,
                 CaseSensitivity casesense = CaseSensitivity::CaseSensitive);
  ~UStringMatcher();

  UStringMatcher(const UStringMatcher& rhs);
  UStringMatcher& operator=(const UStringMatcher& rhs);

 public:
  void SetPattern(UStringView pattern);
  const UString& GetPattern() const;

  void SetCaseSensitivity(CaseSensitivity casesense);
  CaseSensitivity GetCaseSensitivity() const;

  int32 IndexIn(UStringView str, int32 from = 0) const;

  static int32 FastFind(const UNICHAR* haystack, int32 haystack_len, int32 from,
                        const UNICHAR* needle, int32 needle_len,
                        CaseSensitivity casesense);

  static int32 FastLastFind(const UNICHAR* haystack, int32 haystack_len,
                            int32 from, const UNICHAR* needle, int32 needle_len,
                            CaseSensitivity casesense);

  static int32 FastFindChar(const UNICHAR* str, int32 length, UNICHAR ch,
                            int32 from, CaseSensitivity casesense);

  static int32 FastLastFindChar(const UNICHAR* str, int32 length, UNICHAR ch,
                                int32 from, CaseSensitivity casesense);

 private:
  UString pattern_;
  CaseSensitivity casesense_;
  uint8 skip_table_[256];

  void UpdateSkipTable();
};

}  // namespace fun
