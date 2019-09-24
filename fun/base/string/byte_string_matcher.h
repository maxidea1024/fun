#pragma once

namespace fun {

class ByteStringMatcher {
 public:
  ByteStringMatcher();
  ByteStringMatcher(ByteStringView pattern, CaseSensitivity casesense = CaseSensitivity::CaseSensitive);
  ~ByteStringMatcher();

  ByteStringMatcher(const ByteStringMatcher& rhs);
  ByteStringMatcher& operator = (const ByteStringMatcher& rhs);

  void SetPattern(ByteStringView pattern);
  const ByteString& GetPattern() const;

  void SetCaseSensitivity(CaseSensitivity casesense);
  CaseSensitivity GetCaseSensitivity() const;

  int32 IndexIn(ByteStringView str, int32 from = 0) const;

  static int32 FastFind(const char* haystack,
                        int32 haystack_len,
                        int32 from,
                        const char* needle,
                        int32 needle_len,
                        CaseSensitivity casesense);
                        
  static int32 FastLastFind(const char* haystack,
                            int32 haystack_len,
                            int32 from,
                            const char* needle,
                            int32 needle_len,
                            CaseSensitivity casesense);

  static int32 FastFindChar(const char* str,
                            int32 len,
                            char ch,
                            int32 from,
                            CaseSensitivity casesense);

  static int32 FastLastFindChar(const char* str,
                                int32 len,
                                char ch,
                                int32 from,
                                CaseSensitivity casesense);

 private:
  ByteString pattern_;
  CaseSensitivity casesense_;

  uint8 skip_table_[256];
  void UpdateSkipTable();
};

} // namespace fun
