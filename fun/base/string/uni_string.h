#pragma once

//주의 : CASCIIString을 raw로 처리할때 주의해야함. 그렇지 않으면, 불필요한 UTF8
//-> TCHAR의 변환이 수반됨.

#include <initializer_list>  // std::initializer_list
#include <iterator>          // std::iterator
#include <string>            // std::string

#include "fun/base/base.h"
#include "fun/base/string/ascii_string.h"
#include "fun/base/string/byte_string.h"
#include "fun/base/string/byte_string_view.h"
#include "fun/base/string/sharable_array_data.h"
#include "fun/base/string/string_forward_decls.h"
#include "fun/base/string/uni_string_view.h"

namespace fun {

class Regex;
class RegexMatch;

typedef TypedSharableArrayData<UNICHAR> UStringData;

#define UStringLiteral(str)                                       \
  ([]() -> UString {                                              \
    enum { size = sizeof(UTEXT(str)) / sizeof(UNICHAR) - 1 };     \
    static const StaticUStringData<size> literal = {              \
        STATIC_STRING_DATA_HEADER_INITIALIZER(size), UTEXT(str)}; \
    UStringDataPtr holder = {literal.DataPtr()};                  \
    const UString result(holder);                                 \
    return result;                                                \
  }())

#define STATIC_STRING_DATA_HEADER_INITIALIZER_WITH_OFFSET(size, offset) \
  \ { {RefCounter::PERSISTENT_COUNTER_VALUE}, size, 0, 0, offset }

#define STATIC_STRING_DATA_HEADER_INITIALIZER(size) \
  STATIC_STRING_DATA_HEADER_INITIALIZER_WITH_OFFSET(size, sizeof(UStringData))

template <size_t N>
struct StaticUStringData {
  // UStringData Header;
  // POD 타입이어야 하므로, UStringData가 아닌, UntypedSharableArrayData를
  // 사용해야함.
  UntypedSharableArrayData header;
  UNICHAR data[N + 1];

  inline UStringData* DataPtr() const {
    fun_check(header.ref.IsPersistent());
    return const_cast<UStringData*>(static_cast<const UStringData*>(&header));
  }
};

struct UStringDataPtr {
  UStringData* ptr;
};

/*

생성자와 대입연산자(=)에서는 ByteStringView / UStringView로 대체가 안된다.
왜일까...

반면, operator+= 는 동작이 잘되네... 흠...

*/
class FUN_BASE_API UString {
 public:
  typedef UNICHAR CharType;
  typedef UStringData CData;

  class FUN_BASE_API CharRef {
   public:
    inline operator UNICHAR() const {
      return index_ < array_ref_.Len() ? array_ref_.At(index_) : UNICHAR(0);
    }
    inline CharRef& operator=(UNICHAR ch) {
      if (index_ >= array_ref_.Len())
        array_ref_.ExpandAt(index_);
      else
        array_ref_.MutableData()[index_] = ch;
      return *this;
    }
    inline CharRef& operator=(const CharRef& rhs) {
      if (index_ >= array_ref_.Len())
        array_ref_.ExpandAt(index_);
      else
        array_ref_.MutableData()[index_] = rhs.array_ref_.At(rhs.index_);
      return *this;
    }
    inline bool operator==(UNICHAR ch) const {
      return array_ref_.At(index_) == ch;
    }
    inline bool operator!=(UNICHAR ch) const {
      return array_ref_.At(index_) != ch;
    }
    inline bool operator>(UNICHAR ch) const {
      return array_ref_.At(index_) > ch;
    }
    inline bool operator>=(UNICHAR ch) const {
      return array_ref_.At(index_) >= ch;
    }
    inline bool operator<(UNICHAR ch) const {
      return array_ref_.At(index_) < ch;
    }
    inline bool operator<=(UNICHAR ch) const {
      return array_ref_.At(index_) <= ch;
    }

   private:
    UString& array_ref_;
    int32 index_;

    inline CharRef(UString& array_ref, int32 index)
        : array_ref_(array_ref), index_(index) {}

    friend class UString;
  };

  UString();
  UString(UStringDataPtr data_ptr);
  UString(const UString& rhs);
  UString(const UStringRef& ref);
  UString(const UNICHAR* str);
  UString(const UNICHAR* str, int32 len);
  UString(const UNICHAR* begin, const UNICHAR* end);
  // 생성자에서는 이게 모든 UNICHAR 문자열을 받아주질 못하고 있다... 흠...
  // UString(const UNICHAR*)이 여전히 필요하다는 얘긴데...
  UString(const UStringView& str);
  UString(UNICHAR ch);
  UString(int32 count, UNICHAR ch);
  UString(int32 len, NoInit_TAG);
  UString(int32 len, ReservationInit_TAG);

  // Ambiguity problem
  // UString(char ch);
  // UString& operator = (char ch);
  UString(const AsciiString& str);
  UString& operator=(const AsciiString& str);
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  UString(const char* str, int32 len);
  UString(const char* str);
  UString(ByteStringView str);
  UString& operator=(const char* str);
  UString& operator=(ByteStringView str);
#endif

  UString& operator=(const UString& rhs);
  UString& operator=(UNICHAR ch);
  UString& operator=(const UNICHAR* str);
  UString& operator=(UStringView str);
  UString(UString&& rhs);
  UString& operator=(UString&& rhs);
  ~UString();

  void Swap(UString& rhs);

  int32 Len() const;
  int32 NulTermLen() const;
  int32 Capacity() const;
  bool IsEmpty() const;
  void Clear();
  void Clear(int32 initial_capacity);
  void ResizeUninitialized(int32 after_len);
  void ResizeZeroed(int32 after_len);
  void Resize(int32 after_len, UNICHAR filler);
  void Reserve(int32 len);
  void Shrink();

  UString& Fill(UNICHAR filler, int32 len = -1);

  const UNICHAR* operator*() const;
  UNICHAR* MutableData();
  UNICHAR* MutableData(int32 len);
  const UNICHAR* ConstData() const;

  // TODO NUL term을 보장해야함.
  const UNICHAR* c_str() const;

  bool IsNulTerm() const;
  UString& TrimToNulTerminator();
  UString ToNulTerminated() const;

  void Detach();
  bool IsDetached() const;
  bool IsSharedWith(const UString& rhs) const;
  bool IsRawData() const;

  UNICHAR At(int32 index) const;
  UNICHAR operator[](int32 index) const;
  CharRef operator[](int32 index);

  UNICHAR First() const;
  CharRef First();
  UNICHAR Last() const;
  CharRef Last();

  UNICHAR FirstOr(const UNICHAR def = UNICHAR(0)) const;
  UNICHAR LastOr(const UNICHAR def = UNICHAR(0)) const;

  int32 IndexOfAny(UStringView chars,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;
  int32 IndexOfAny(AsciiString chars,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;
  int32 IndexOfAny(const Array<UNICHAR>& chars,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;
  inline int32 IndexOfAny(
      std::initializer_list<UNICHAR> chars,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_index = nullptr,
      int32* matched_len = nullptr) const {
    return IndexOfAny(UStringView(chars.begin(), chars.size()), casesense, from,
                      matched_index, matched_len);
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline int32 IndexOfAny(
      ByteStringView chars,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_index = nullptr,
      int32* matched_len = nullptr) const {
    return IndexOfAny(UTF8_AS_UNICHAR(chars).ToView(), casesense, from,
                      matched_index, matched_len);
  }
#endif

  int32 LastIndexOfAny(
      UStringView chars,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = -1, int32* matched_index = nullptr,
      int32* matched_len = nullptr) const;
  int32 LastIndexOfAny(
      AsciiString chars,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = -1, int32* matched_index = nullptr,
      int32* matched_len = nullptr) const;
  int32 LastIndexOfAny(
      const Array<UNICHAR>& chars,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = -1, int32* matched_index = nullptr,
      int32* matched_len = nullptr) const;
  inline int32 LastIndexOfAny(
      std::initializer_list<UNICHAR> chars,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = -1, int32* matched_index = nullptr,
      int32* matched_len = nullptr) const {
    return LastIndexOfAny(UStringView(chars.begin(), chars.size()), casesense,
                          from, matched_index, matched_len);
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline int32 LastIndexOfAny(
      ByteStringView chars,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = -1, int32* matched_index = nullptr,
      int32* matched_len = nullptr) const {
    return LastIndexOfAny(UTF8_AS_UNICHAR(chars).ToView(), casesense, from,
                          matched_index, matched_len);
  }
#endif

  int32 IndexOfAny(const Array<const UNICHAR*>& strings,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;
  int32 IndexOfAny(const Array<UString>& strings,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;
  int32 IndexOfAny(const Array<UStringRef>& strings,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;
  int32 IndexOfAny(const Array<UStringView>& strings,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;
  int32 IndexOfAny(const Array<AsciiString>& strings,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;

  int32 IndexOfAny(std::initializer_list<const UNICHAR*> strings,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;
  int32 IndexOfAny(std::initializer_list<UString> strings,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;
  int32 IndexOfAny(std::initializer_list<UStringRef> strings,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;
  int32 IndexOfAny(std::initializer_list<UStringView> strings,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;
  int32 IndexOfAny(std::initializer_list<AsciiString> strings,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;

  int32 IndexOf(UStringView sub,
                CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                int32 from = 0, int32* matched_len = nullptr) const;

  inline int32 IndexOf(
      UNICHAR ch, CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_len = nullptr) const {
    return IndexOf(UStringView(&ch, 1), casesense, from, matched_len);
  }
  inline int32 IndexOf(
      const UNICHAR* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_len = nullptr) const {
    return IndexOf(UStringView(sub, sub_len), casesense, from, matched_len);
  }
  int32 IndexOf(AsciiString sub,
                CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                int32 from = 0, int32* matched_len = nullptr) const;
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  // Ambiguity problem
  // inline int32 IndexOf(char ch, CaseSensitivity casesense =
  // CaseSensitivity::CaseSensitive, int32 from = 0) const { return
  //IndexOf(UNICHAR(ch), casesense, from, matched_len); }
  inline int32 IndexOf(
      const char* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_len = nullptr) const {
    return IndexOf(UTF8_AS_UNICHAR(sub, sub_len).ToView(), casesense, from,
                   matched_len);
  }
  inline int32 IndexOf(
      ByteStringView sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_len = nullptr) const {
    return IndexOf(UTF8_AS_UNICHAR(sub).ToView(), casesense, from, matched_len);
  }
#endif

  template <typename Predicate>
  int32 IndexOfIf(const Predicate& pred, int32 from = 0) const;

  int32 LastIndexOf(UStringView sub,
                    CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                    int32 from = -1, int32* matched_len = nullptr) const;

  inline int32 LastIndexOf(
      UNICHAR ch, CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = -1, int32* matched_len = nullptr) const {
    return LastIndexOf(UStringView(&ch, 1), casesense, from, matched_len);
  }
  inline int32 LastIndexOf(
      const UNICHAR* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = -1, int32* matched_len = nullptr) const {
    return LastIndexOf(UStringView(sub, sub_len), casesense, from, matched_len);
  }
  int32 LastIndexOf(AsciiString sub,
                    CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                    int32 from = -1, int32* matched_len = nullptr) const;
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  // Ambiguity problem
  // inline int32 LastIndexOf(char ch, CaseSensitivity casesense =
  // CaseSensitivity::CaseSensitive, int32 from = -1, int32* matched_len =
  // nullptr) const { return LastIndexOf(UNICHAR(ch), casesense, from,
  //matched_len); }
  inline int32 LastIndexOf(
      const char* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = -1, int32* matched_len = nullptr) const {
    return LastIndexOf(UTF8_AS_UNICHAR(sub, sub_len).ToView(), casesense, from,
                       matched_len);
  }
  inline int32 LastIndexOf(
      ByteStringView sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = -1, int32* matched_len = nullptr) const {
    return LastIndexOf(UTF8_AS_UNICHAR(sub).ToView(), casesense, from,
                       matched_len);
  }
#endif

  template <typename Predicate>
  int32 LastIndexOfIf(const Predicate& pred, int32 from = -1) const;

  inline bool Contains(
      UNICHAR ch, CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_len = nullptr) const {
    return IndexOf(ch, casesense, from, matched_len) != INVALID_INDEX;
  }
  inline bool Contains(
      const UNICHAR* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_len = nullptr) const {
    return IndexOf(sub, sub_len, casesense, from, matched_len) != INVALID_INDEX;
  }
  inline bool Contains(
      UStringView sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_len = nullptr) const {
    return IndexOf(sub, casesense, from, matched_len) != INVALID_INDEX;
  }
  inline bool Contains(
      AsciiString sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_len = nullptr) const {
    return IndexOf(sub, casesense, from, matched_len) != INVALID_INDEX;
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  // Ambiguity problem
  // inline bool Contains(char ch, CaseSensitivity casesense =
  // CaseSensitivity::CaseSensitive, int32 from = 0, int32* matched_len =
  // nullptr) const { return IndexOf(ch, casesense, from, matched_len) !=
  //INVALID_INDEX; }
  inline bool Contains(
      const char* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_len = nullptr) const {
    return IndexOf(sub, sub_len, casesense, from, matched_len) != INVALID_INDEX;
  }
  inline bool Contains(
      ByteStringView sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_len = nullptr) const {
    return IndexOf(sub, casesense, from, matched_len) != INVALID_INDEX;
  }
#endif

  int32 Count(UStringView sub,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  inline int32 Count(UNICHAR ch, CaseSensitivity casesense =
                                     CaseSensitivity::CaseSensitive) const {
    return Count(UStringView(&ch, 1), casesense);
  }
  inline int32 Count(
      const UNICHAR* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Count(UStringView(sub, sub_len), casesense);
  }
  int32 Count(AsciiString sub,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  // Ambiguity problem
  // inline int32 Count(char ch, CaseSensitivity casesense =
  // CaseSensitivity::CaseSensitive) const { return Count(UNICHAR(ch),
  //casesense); }
  inline int32 Count(
      const char* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Count(UTF8_AS_UNICHAR(sub, sub_len).ToView(), casesense);
  }
  int32 Count(ByteStringView sub, CaseSensitivity casesense =
                                      CaseSensitivity::CaseSensitive) const {
    return Count(UTF8_AS_UNICHAR(sub).ToView(), casesense);
  }
#endif

  inline UString operator()(int32 offset, int32 len) const {
    return Mid(offset, len);
  }

  UString Left(int32 len) const;
  UString Mid(int32 offset, int32 len = int32_MAX) const;
  UString Right(int32 len) const;
  UString LeftChopped(int32 len) const;
  UString RightChopped(int32 len) const;

  UStringRef LeftRef(int32 len) const;
  UStringRef MidRef(int32 offset, int32 len = int32_MAX) const;
  UStringRef RightRef(int32 len) const;
  UStringRef LeftChoppedRef(int32 len) const;
  UStringRef RightChoppedRef(int32 len) const;

  bool StartsWith(UStringView sub, CaseSensitivity casesense =
                                       CaseSensitivity::CaseSensitive) const;

  inline bool StartsWith(UNICHAR ch, CaseSensitivity casesense =
                                         CaseSensitivity::CaseSensitive) const {
    return StartsWith(UStringView(&ch, 1), casesense);
  }
  inline bool StartsWith(
      const UNICHAR* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return StartsWith(UStringView(sub, sub_len), casesense);
  }
  bool StartsWith(AsciiString sub, CaseSensitivity casesense =
                                       CaseSensitivity::CaseSensitive) const;
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  // Ambiguity problem
  // inline bool StartsWith(char ch, CaseSensitivity casesense =
  // CaseSensitivity::CaseSensitive) const { return StartsWith(UNICHAR(ch),
  //casesense); }
  inline bool StartsWith(
      const char* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return StartsWith(UTF8_AS_UNICHAR(sub, sub_len).ToView(), casesense);
  }
  inline bool StartsWith(
      ByteStringView sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return StartsWith(UTF8_AS_UNICHAR(sub).ToView(), casesense);
  }
#endif

  bool EndsWith(UStringView sub, CaseSensitivity casesense =
                                     CaseSensitivity::CaseSensitive) const;

  inline bool EndsWith(UNICHAR ch, CaseSensitivity casesense =
                                       CaseSensitivity::CaseSensitive) const {
    return EndsWith(UStringView(&ch, 1), casesense);
  }
  inline bool EndsWith(
      const UNICHAR* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return EndsWith(UStringView(sub, sub_len), casesense);
  }
  bool EndsWith(AsciiString sub, CaseSensitivity casesense =
                                     CaseSensitivity::CaseSensitive) const;
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  // Ambiguity problem
  // inline bool EndsWith(char ch, CaseSensitivity casesense =
  // CaseSensitivity::CaseSensitive) const { return EndsWith(UNICHAR(ch),
  //casesense); }
  inline bool EndsWith(
      const char* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return EndsWith(UTF8_AS_UNICHAR(sub, sub_len).ToView(), casesense);
  }
  inline bool EndsWith(
      ByteStringView sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return EndsWith(UTF8_AS_UNICHAR(sub).ToView(), casesense);
  }
#endif

  bool GlobMatch(UStringView pattern, CaseSensitivity casesense =
                                          CaseSensitivity::CaseSensitive) const;

  inline bool GlobMatch(
      const UNICHAR* pattern, int32 pattern_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return GlobMatch(UStringView(pattern, pattern_len), casesense);
  }
  bool GlobMatch(AsciiString pattern, CaseSensitivity casesense =
                                          CaseSensitivity::CaseSensitive) const;
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline bool GlobMatch(
      const char* pattern, int32 pattern_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return GlobMatch(UTF8_AS_UNICHAR(pattern, pattern_len).ToView(), casesense);
  }
  inline bool GlobMatch(
      ByteStringView pattern,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return GlobMatch(UTF8_AS_UNICHAR(pattern).ToView(), casesense);
  }
#endif

  UString& Truncate(int32 pos);
  UString& LeftChop(int32 len);
  UString& RightChop(int32 len);

  UString& MakeLower();
  UString& MakeUpper();
  UString ToLower() const;
  UString ToUpper() const;

  UString& TrimLeft();
  UString& TrimRight();
  UString& Trim();
  UString TrimmedLeft() const;
  UString TrimmedRight() const;
  UString Trimmed() const;
  int32 LeftSpaces() const;
  int32 RightSpaces() const;
  int32 SideSpaces() const;

  UStringRef TrimmedLeftRef() const;
  UStringRef TrimmedRightRef() const;
  UStringRef TrimmedRef() const;

  UString& Simplify();
  UString Simplified() const;

  UString Repeated(int32 times) const;  // restricted to max 2GB
  UString operator*(int32 times) const;

  UString LeftJustified(int32 width, UNICHAR filler = UNICHAR(' '),
                        bool truncate = false) const;
  UString RightJustified(int32 width, UNICHAR filler = UNICHAR(' '),
                         bool truncate = false) const;

  UString& Reverse();
  UString Reversed() const;

  UString& PrependUnitialized(int32 len);
  UString& AppendUninitialized(int32 len);
  UString& InsertUninitialized(int32 pos, int32 len);
  UString& PrependZeroed(int32 len);
  UString& AppendZeroed(int32 len);
  UString& InsertZeroed(int32 pos, int32 len);

  UString& Prepend(UStringView str);

  inline UString& Prepend(UNICHAR ch) { return Prepend(UStringView(&ch, 1)); }
  UString& Prepend(int32 count, UNICHAR ch);
  inline UString& Prepend(const UNICHAR* str, int32 len) {
    return Prepend(UStringView(str, len));
  }
  UString& Prepend(AsciiString str);
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  // Ambiguity problem
  // inline UString& Prepend(char ch)
  //{ return Prepend(UNICHAR(ch)); }
  inline UString& Prepend(const char* str, int32 len) {
    return Prepend(UTF8_AS_UNICHAR(str, len).ToView());
  }
  inline UString& Prepend(ByteStringView str) {
    return Prepend(UTF8_AS_UNICHAR(str).ToView());
  }
#endif

  UString& Append(UStringView str);

  inline UString& Append(UNICHAR ch) { return Append(UStringView(&ch, 1)); }
  UString& Append(int32 count, UNICHAR ch);
  inline UString& Append(const UNICHAR* str, int32 len) {
    return Append(UStringView(str, len));
  }
  UString& Append(AsciiString str);
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  // Ambiguity problem
  // inline UString& Append(char ch)
  //{ return Append(UNICHAR(ch)); }
  inline UString& Append(const char* str, int32 len) {
    return Append(UTF8_AS_UNICHAR(str, len).ToView());
  }
  inline UString& Append(ByteStringView str) {
    return Append(UTF8_AS_UNICHAR(str).ToView());
  }
#endif

  template <typename ElementType>
  inline UString& Append(std::initializer_list<ElementType> list) {
    for (auto& element : list) {
      Append(element);
    }
  }

  template <typename ElementType, typename ArrayAllocator>
  inline UString& Append(const Array<ElementType, ArrayAllocator>& list) {
    for (auto& element : list) {
      Append(element);
    }
  }

  // UStringView로 const UNICHAR* / UString 을 구분하지 않고 처리하면,
  // 컴파일러가 제대로 해석을 하지 못하는 문제가 있어서, 각각의 타입을 별개로
  //처리해주어야함.
  inline UString& operator+=(UNICHAR ch) { return Append(ch); }
  inline UString& operator+=(const UNICHAR* str) { return Append(str); }
  inline UString& operator+=(const UString& str) { return Append(str); }
  inline UString& operator+=(const UStringRef& str) { return Append(str); }
  inline UString& operator+=(const UStringView& str) { return Append(str); }
  inline UString& operator+=(const AsciiString& str) { return Append(str); }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  // Ambiguity problem
  // inline UString& operator += (char ch) { return Append(ch); }
  inline UString& operator+=(const char* str) { return Append(str); }
  inline UString& operator+=(const ByteString& str) { return Append(str); }
  inline UString& operator+=(const ByteStringRef& str) { return Append(str); }
  inline UString& operator+=(const ByteStringView& str) { return Append(str); }
#endif

  template <typename ElementType>
  inline UString& operator+=(std::initializer_list<ElementType> list) {
    return Append(list);
  }
  template <typename ElementType, typename ArrayAllocator>
  inline UString& operator+=(const Array<ElementType, ArrayAllocator>& list) {
    return Append(list);
  }  // Allocator를 지정할 수 있어야할까??

  inline UString& operator<<(UNICHAR ch) { return Append(ch); }
  inline UString& operator<<(const UNICHAR* str) { return Append(str); }
  inline UString& operator<<(const UString& str) { return Append(str); }
  inline UString& operator<<(const UStringRef& str) { return Append(str); }
  inline UString& operator<<(const UStringView& str) { return Append(str); }
  inline UString& operator<<(const AsciiString& str) { return Append(str); }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  // Ambiguity problem
  // inline UString& operator += (char ch) { return Append(ch); }
  inline UString& operator<<(const char* str) { return Append(str); }
  inline UString& operator<<(const ByteString& str) { return Append(str); }
  inline UString& operator<<(const ByteStringRef& str) { return Append(str); }
  inline UString& operator<<(const ByteStringView& str) { return Append(str); }
#endif

  template <typename ElementType>
  inline UString& operator<<(std::initializer_list<ElementType> list) {
    return Append(list);
  }
  template <typename ElementType, typename ArrayAllocator>
  inline UString& operator<<(const Array<ElementType, ArrayAllocator>& list) {
    return Append(list);
  }  // Allocator를 지정할 수 있어야할까??

  // UStringView로 const UNICHAR* / UString 을 구분하지 않고 처리하면,
  // 컴파일러가 제대로 해석을 하지 못하는 문제가 있어서, 각각의 타입을 별개로
  //처리해주어야함.
  inline UString operator+(const UString& str) const {
    return UString(*this) += str;
  }

  inline UString operator+(UNICHAR ch) const { return UString(*this) += ch; }
  inline friend UString operator+(UNICHAR ch, const UString& str) {
    return UString(ch) += str;
  }

  inline UString operator+(const UNICHAR* str) const {
    return UString(*this) += str;
  }
  inline friend UString operator+(const UNICHAR* str1, const UString& str2) {
    return UString(str1) += str2;
  }

  inline UString operator+(const UStringRef& str) const {
    return UString(*this) += str;
  }
  inline friend UString operator+(const UStringRef& str1, const UString& str2) {
    return UString(str1) += str2;
  }

  inline UString operator+(const UStringView& str) const {
    return UString(*this) += str;
  }
  inline friend UString operator+(const UStringView& str1,
                                  const UString& str2) {
    return UString(str1) += str2;
  }

  inline UString operator+(const AsciiString& str) const {
    return UString(*this) += str;
  }
  inline friend UString operator+(const AsciiString& str1,
                                  const UString& str2) {
    return UString(str1) += str2;
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  // Ambiguity problem
  // inline UString operator + (char ch) const { return UString(*this) += ch; }
  // inline friend UString operator + (char ch, const UString& str) { return
  // UString(ch) += str; }
  inline UString operator+(const char* str) const {
    return UString(*this) += str;
  }
  inline friend UString operator+(const char* str1, const UString& str2) {
    return UString(str1) += str2;
  }

  inline UString operator+(const ByteString& str) const {
    return UString(*this) += str;
  }
  // inline friend UString operator + (const ByteString& str1, const UString&
  // str2) { return UString(str1) += str2; }

  inline UString operator+(const ByteStringRef& str) const {
    return UString(*this) += str;
  }
  inline friend UString operator+(const ByteStringRef& str1,
                                  const UString& str2) {
    return UString(str1) += str2;
  }

  inline UString operator+(const ByteStringView& str) const {
    return UString(*this) += str;
  }
  inline friend UString operator+(const ByteStringView& str1,
                                  const UString& str2) {
    return UString(str1) += str2;
  }
#endif

  template <typename ElementType>
  inline UString operator+(std::initializer_list<ElementType> list) {
    return UString(*this) += list;
  }
  template <typename ElementType, typename ArrayAllocator>
  inline UString operator+(const Array<ElementType, ArrayAllocator>& list) {
    return UString(*this) += list;
  }  // Allocator를 지정할 수 있어야할까??

  UString& Insert(int32 pos, UStringView str);

  inline UString& Insert(int32 pos, UNICHAR ch) {
    return Insert(pos, UStringView(&ch, 1));
  }
  UString& Insert(int32 pos, int32 count, UNICHAR ch);
  inline UString& Insert(int32 pos, const UNICHAR* str, int32 len) {
    return Insert(pos, UStringView(str, len));
  }
  UString& Insert(int32 pos, AsciiString str);
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  // Ambiguity problem
  // inline UString& Insert(int32 pos, char ch)
  //{ return Insert(pos, UNICHAR(ch)); }
  inline UString& Insert(int32 pos, int32 count, char ch) {
    return Insert(pos, count, UNICHAR(ch));
  }
  inline UString& Insert(int32 pos, const char* str, int32 len) {
    return Insert(pos, UTF8_AS_UNICHAR(str, len).ToView());
  }
  inline UString& Insert(int32 pos, ByteStringView str) {
    return Insert(pos, UTF8_AS_UNICHAR(str).ToView());
  }
#endif

  UString& Overwrite(int32 pos, UStringView str);

  inline UString& Overwrite(int32 pos, UNICHAR ch) {
    return Overwrite(pos, UStringView(&ch, 1));
  }
  UString& Overwrite(int32 pos, int32 count, UNICHAR ch);
  inline UString& Overwrite(int32 pos, const UNICHAR* str, int32 len) {
    return Overwrite(pos, UStringView(str, len));
  }
  UString& Overwrite(int32 pos, AsciiString str);
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  // Ambiguity problem
  // inline UString& Overwrite(int32 pos, char ch)
  //{ return Overwrite(pos, UNICHAR(ch)); }
  inline UString& Overwrite(int32 pos, int32 count, char ch) {
    return Overwrite(pos, count, UNICHAR(ch));
  }
  inline UString& Overwrite(int32 pos, const char* str, int32 len) {
    return Overwrite(pos, UTF8_AS_UNICHAR(str, len).ToView());
  }
  inline UString& Overwrite(int32 pos, ByteStringView str) {
    return Overwrite(pos, UTF8_AS_UNICHAR(str).ToView());
  }
#endif

  UString& Remove(int32 pos, int32 len);

  inline UString& RemoveFirst(int32 len = 1) {
    fun_check(len <= Len());
    return Remove(0, len);
  }
  inline UString RemoveLast(int32 len = 1) {
    fun_check(len <= Len());
    return Remove(Len() - len, len);
  }

  inline UString& FindAndRemove(
      UNICHAR ch, CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryFindAndRemove(ch, casesense);
    return *this;
  }
  inline UString& FindAndRemove(
      const UNICHAR* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryFindAndRemove(sub, sub_len, casesense);
    return *this;
  }
  inline UString& FindAndRemove(
      UStringView sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryFindAndRemove(sub, casesense);
    return *this;
  }
  inline UString& FindAndRemove(
      AsciiString sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryFindAndRemove(sub, casesense);
    return *this;
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  // Ambiguity problem
  // inline UString& FindAndRemove(char ch, CaseSensitivity casesense =
  // CaseSensitivity::CaseSensitive) { TryFindAndRemove(ch, casesense); return
  //*this; }
  inline UString& FindAndRemove(
      const char* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryFindAndRemove(sub, sub_len, casesense);
    return *this;
  }
  inline UString& FindAndRemove(
      ByteStringView sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryFindAndRemove(sub, casesense);
    return *this;
  }
#endif

  int32 TryFindAndRemove(UStringView sub, CaseSensitivity casesense =
                                              CaseSensitivity::CaseSensitive);

  inline int32 TryFindAndRemove(
      UNICHAR ch, CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryFindAndRemove(UStringView(&ch, 1), casesense);
  }
  inline int32 TryFindAndRemove(
      const UNICHAR* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryFindAndRemove(UStringView(sub, sub_len), casesense);
  }
  int32 TryFindAndRemove(AsciiString sub, CaseSensitivity casesense =
                                              CaseSensitivity::CaseSensitive);
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  // Ambiguity problem
  // inline int32 TryFindAndRemove(char ch, CaseSensitivity casesense =
  // CaseSensitivity::CaseSensitive) { return TryFindAndRemove(UNICHAR(ch),
  //casesense); }
  inline int32 TryFindAndRemove(
      const char* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryFindAndRemove(UTF8_AS_UNICHAR(sub, sub_len).ToView(), casesense);
  }
  inline int32 TryFindAndRemove(
      ByteStringView sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryFindAndRemove(UTF8_AS_UNICHAR(sub).ToView(), casesense);
  }
#endif

  UString& Replace(int32 before_pos, int32 before_len, UStringView after);

  inline UString& Replace(int32 before_pos, int32 before_len, UNICHAR after) {
    return Replace(before_pos, before_len, UStringView(&after, 1));
  }
  inline UString& Replace(int32 before_pos, int32 before_len,
                          const UNICHAR* after, int32 after_len) {
    return Replace(before_pos, before_len, UStringView(after, after_len));
  }
  UString& Replace(int32 before_pos, int32 before_len, AsciiString after);
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  // Ambiguity problem
  // inline UString& Replace(int32 before_pos, int32 before_len, char after)
  //{ return Replace(before_pos, before_len, UNICHAR(after)); }
  inline UString& Replace(int32 before_pos, int32 before_len, const char* after,
                          int32 after_len) {
    return Replace(before_pos, before_len,
                   UTF8_AS_UNICHAR(after, after_len).ToView());
  }
  inline UString& Replace(int32 before_pos, int32 before_len,
                          ByteStringView after) {
    return Replace(before_pos, before_len, UTF8_AS_UNICHAR(after).ToView());
  }
#endif

  int32 TryReplace(UStringView before, UStringView after,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive);

  // UNICHAR -> *
  inline int32 TryReplace(
      UNICHAR before, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UStringView(&before, 1), UStringView(&after, 1),
                      casesense);
  }
  inline int32 TryReplace(
      UNICHAR before, const UNICHAR* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UStringView(&before, 1), UStringView(after, after_len),
                      casesense);
  }
  inline int32 TryReplace(
      UNICHAR before, UStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UStringView(&before, 1), after, casesense);
  }
  inline int32 TryReplace(
      UNICHAR before, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UStringView(&before, 1),
                      UStringView(UString::FromAscii(after)), casesense);
  }  // FIXME
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  // Ambiguity problem
  // inline int32 TryReplace(UNICHAR before, char after, CaseSensitivity
  // casesense = CaseSensitivity::CaseSensitive) { return
  //TryReplace(UStringView(&before,1), UNICHAR(after), casesense); }
  inline int32 TryReplace(
      UNICHAR before, const char* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UStringView(&before, 1),
                      UTF8_AS_UNICHAR(after, after_len).ToView(), casesense);
  }
  inline int32 TryReplace(
      UNICHAR before, ByteStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UStringView(&before, 1), UTF8_AS_UNICHAR(after).ToView(),
                      casesense);
  }
#endif

  // const UNICHAR*,Len -> *
  inline int32 TryReplace(
      const UNICHAR* before, int32 before_len, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UStringView(before, before_len), UStringView(&after, 1),
                      casesense);
  }
  inline int32 TryReplace(
      const UNICHAR* before, int32 before_len, const UNICHAR* after,
      int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UStringView(before, before_len),
                      UStringView(after, after_len), casesense);
  }
  inline int32 TryReplace(
      const UNICHAR* before, int32 before_len, UStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UStringView(before, before_len), after, casesense);
  }
  inline int32 TryReplace(
      const UNICHAR* before, int32 before_len, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UStringView(before, before_len),
                      UStringView(UString::FromAscii(after)), casesense);
  }  // FIXME
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  // Ambiguity problem
  // inline int32 TryReplace(const UNICHAR* before, int32 before_len, char
  // after, CaseSensitivity casesense = CaseSensitivity::CaseSensitive) { return
  //TryReplace(UStringView(&before,before_len), UNICHAR(after), casesense); }
  inline int32 TryReplace(
      const UNICHAR* before, int32 before_len, const char* after,
      int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UStringView(before, before_len),
                      UTF8_AS_UNICHAR(after, after_len).ToView(), casesense);
  }
  inline int32 TryReplace(
      const UNICHAR* before, int32 before_len, ByteStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UStringView(before, before_len),
                      UTF8_AS_UNICHAR(after).ToView(), casesense);
  }
#endif

  // UStringView -> *
  inline int32 TryReplace(
      UStringView before, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(before, UStringView(&after, 1), casesense);
  }
  inline int32 TryReplace(
      UStringView before, const UNICHAR* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(before, UStringView(after, after_len), casesense);
  }
  // inline int32 TryReplace(UStringView before, UStringView after,
  // CaseSensitivity casesense = CaseSensitivity::CaseSensitive)
  inline int32 TryReplace(
      UStringView before, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(before, UStringView(UString::FromAscii(after)),
                      casesense);
  }  // FIXME
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  // Ambiguity problem
  // inline int32 TryReplace(UStringView before, char after, CaseSensitivity
  // casesense = CaseSensitivity::CaseSensitive) { return TryReplace(before,
  //UNICHAR(after), casesense); }
  inline int32 TryReplace(
      UStringView before, const char* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(before, UTF8_AS_UNICHAR(after, after_len).ToView(),
                      casesense);
  }
  inline int32 TryReplace(
      UStringView before, ByteStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(before, UTF8_AS_UNICHAR(after).ToView(), casesense);
  }
#endif

  // AsciiString -> *
  inline int32 TryReplace(
      AsciiString before, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UStringView(UString::FromAscii(before)),
                      UStringView(&after, 1), casesense);
  }  // FIXME
  inline int32 TryReplace(
      AsciiString before, const UNICHAR* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UStringView(UString::FromAscii(before)),
                      UStringView(after, after_len), casesense);
  }  // FIXME
  inline int32 TryReplace(
      AsciiString before, UStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UStringView(UString::FromAscii(before)), after,
                      casesense);
  }  // FIXME
  inline int32 TryReplace(
      AsciiString before, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UStringView(UString::FromAscii(before)),
                      UStringView(UString::FromAscii(after)), casesense);
  }  // FIXME
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  // Ambiguity problem
  // inline int32 TryReplace(AsciiString before, char after, CaseSensitivity
  // casesense = CaseSensitivity::CaseSensitive) { return
  //TryReplace(UStringView(UString::FromAscii(before)), UNICHAR(after),
  //casesense); } // FIXME
  inline int32 TryReplace(
      AsciiString before, const char* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UStringView(UString::FromAscii(before)),
                      UTF8_AS_UNICHAR(after, after_len).ToView(), casesense);
  }  // FIXME
  inline int32 TryReplace(
      AsciiString before, ByteStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UStringView(UString::FromAscii(before)),
                      UTF8_AS_UNICHAR(after).ToView(), casesense);
  }  // FIXME
#endif

#if FUN_USE_IMPLICIT_STRING_CONVERSION
  // char* -> *
  inline int32 TryReplace(
      const char* before, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UTF8_AS_UNICHAR(before).ToView(), UStringView(&after, 1),
                      casesense);
  }
  inline int32 TryReplace(
      const char* before, const UNICHAR* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UTF8_AS_UNICHAR(before).ToView(),
                      UStringView(after, after_len), casesense);
  }
  inline int32 TryReplace(
      const char* before, UStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UTF8_AS_UNICHAR(before).ToView(), after, casesense);
  }
  inline int32 TryReplace(
      const char* before, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UTF8_AS_UNICHAR(before).ToView(),
                      UStringView(UString::FromAscii(after)), casesense);
  }  // FIXME
  // Ambiguity problem
  // inline int32 TryReplace(const char* before, char after, CaseSensitivity
  // casesense = CaseSensitivity::CaseSensitive) { return
  //TryReplace(UTF8_AS_UNICHAR(before).ToView(), UNICHAR(after), casesense); }
  inline int32 TryReplace(
      const char* before, const char* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UTF8_AS_UNICHAR(before).ToView(),
                      UTF8_AS_UNICHAR(after, after_len).ToView(), casesense);
  }
  inline int32 TryReplace(
      const char* before, ByteStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UTF8_AS_UNICHAR(before).ToView(),
                      UTF8_AS_UNICHAR(after).ToView(), casesense);
  }

  // const char*,Len -> *
  inline int32 TryReplace(
      const char* before, int32 before_len, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UTF8_AS_UNICHAR(before, before_len).ToView(),
                      UStringView(&after, 1), casesense);
  }
  inline int32 TryReplace(
      const char* before, int32 before_len, const UNICHAR* after,
      int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UTF8_AS_UNICHAR(before, before_len).ToView(),
                      UStringView(after, after_len), casesense);
  }
  inline int32 TryReplace(
      const char* before, int32 before_len, UStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UTF8_AS_UNICHAR(before, before_len).ToView(), after,
                      casesense);
  }
  inline int32 TryReplace(
      const char* before, int32 before_len, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UTF8_AS_UNICHAR(before, before_len).ToView(),
                      UStringView(UString::FromAscii(after)), casesense);
  }  // FIXME
  // Ambiguity problem
  // inline int32 TryReplace(const char* before, int32 before_len, char after,
  // CaseSensitivity casesense = CaseSensitivity::CaseSensitive) { return
  //TryReplace(UTF8_AS_UNICHAR(before,before_len).ToView(), UNICHAR(after),
  //casesense); }
  inline int32 TryReplace(
      const char* before, int32 before_len, const char* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UTF8_AS_UNICHAR(before, before_len).ToView(),
                      UTF8_AS_UNICHAR(after, after_len).ToView(), casesense);
  }
  inline int32 TryReplace(
      const char* before, int32 before_len, ByteStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UTF8_AS_UNICHAR(before, before_len).ToView(),
                      UTF8_AS_UNICHAR(after).ToView(), casesense);
  }

  // ByteStringView -> *
  inline int32 TryReplace(
      ByteStringView before, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UTF8_AS_UNICHAR(before).ToView(), UStringView(&after, 1),
                      casesense);
  }
  inline int32 TryReplace(
      ByteStringView before, const UNICHAR* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UTF8_AS_UNICHAR(before).ToView(),
                      UStringView(after, after_len), casesense);
  }
  inline int32 TryReplace(
      ByteStringView before, UStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UTF8_AS_UNICHAR(before).ToView(), after, casesense);
  }
  inline int32 TryReplace(
      ByteStringView before, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UTF8_AS_UNICHAR(before).ToView(),
                      UStringView(UString::FromAscii(after)), casesense);
  }  // FIXME
  // Ambiguity problem
  // inline int32 TryReplace(ByteStringView before, char after, CaseSensitivity
  // casesense = CaseSensitivity::CaseSensitive) { return
  //TryReplace(UTF8_AS_UNICHAR(before).ToView(), UNICHAR(after), casesense); }
  inline int32 TryReplace(
      ByteStringView before, const char* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UTF8_AS_UNICHAR(before).ToView(),
                      UTF8_AS_UNICHAR(after, after_len).ToView(), casesense);
  }
  inline int32 TryReplace(
      ByteStringView before, ByteStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UTF8_AS_UNICHAR(before).ToView(),
                      UTF8_AS_UNICHAR(after).ToView(), casesense);
  }
#endif

  // UNICHAR -> *
  inline UString& Replace(
      UNICHAR before, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
  inline UString& Replace(
      UNICHAR before, const UNICHAR* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, after_len, casesense);
    return *this;
  }
  inline UString& Replace(
      UNICHAR before, UStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
  inline UString& Replace(
      UNICHAR before, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline UString& Replace(
      UNICHAR before, char after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
  inline UString& Replace(
      UNICHAR before, const char* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, after_len, casesense);
    return *this;
  }
  inline UString& Replace(
      UNICHAR before, ByteStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
#endif

  // const UNICHAR*,Len -> *
  inline UString& Replace(
      const UNICHAR* before, int32 before_len, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, before_len, after, casesense);
    return *this;
  }
  inline UString& Replace(
      const UNICHAR* before, int32 before_len, const UNICHAR* after,
      int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, before_len, after, after_len, casesense);
    return *this;
  }
  inline UString& Replace(
      const UNICHAR* before, int32 before_len, UStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, before_len, after, casesense);
    return *this;
  }
  inline UString& Replace(
      const UNICHAR* before, int32 before_len, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, before_len, after, casesense);
    return *this;
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline UString& Replace(
      const UNICHAR* before, int32 before_len, char after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, before_len, after, casesense);
    return *this;
  }
  inline UString& Replace(
      const UNICHAR* before, int32 before_len, const char* after,
      int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, before_len, after, after_len, casesense);
    return *this;
  }
  inline UString& Replace(
      const UNICHAR* before, int32 before_len, ByteStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, before_len, after, casesense);
    return *this;
  }
#endif

  // UStringView -> *
  inline UString& Replace(
      UStringView before, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
  inline UString& Replace(
      UStringView before, const UNICHAR* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, after_len, casesense);
    return *this;
  }
  inline UString& Replace(
      UStringView before, UStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
  inline UString& Replace(
      UStringView before, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline UString& Replace(
      UStringView before, char after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
  inline UString& Replace(
      UStringView before, const char* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, after_len, casesense);
    return *this;
  }
  inline UString& Replace(
      UStringView before, ByteStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
#endif

  // AsciiString -> *
  inline UString& Replace(
      AsciiString before, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
  inline UString& Replace(
      AsciiString before, const UNICHAR* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, after_len, casesense);
    return *this;
  }
  inline UString& Replace(
      AsciiString before, UStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
  inline UString& Replace(
      AsciiString before, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline UString& Replace(
      AsciiString before, char after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
  inline UString& Replace(
      AsciiString before, const char* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, after_len, casesense);
    return *this;
  }
  inline UString& Replace(
      AsciiString before, ByteStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
#endif

#if FUN_USE_IMPLICIT_STRING_CONVERSION
  // char* -> *
  inline UString& Replace(
      const char* before, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
  inline UString& Replace(
      const char* before, const UNICHAR* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, after_len, casesense);
    return *this;
  }
  inline UString& Replace(
      const char* before, UStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
  inline UString& Replace(
      const char* before, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
  inline UString& Replace(
      const char* before, char after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
  inline UString& Replace(
      const char* before, const char* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, after_len, casesense);
    return *this;
  }
  inline UString& Replace(
      const char* before, ByteStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }

  // const char*,Len -> *
  inline UString& Replace(
      const char* before, int32 before_len, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, before_len, after, casesense);
    return *this;
  }
  inline UString& Replace(
      const char* before, int32 before_len, const UNICHAR* after,
      int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, before_len, after, after_len, casesense);
    return *this;
  }
  inline UString& Replace(
      const char* before, int32 before_len, UStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, before_len, after, casesense);
    return *this;
  }
  inline UString& Replace(
      const char* before, int32 before_len, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, before_len, after, casesense);
    return *this;
  }
  inline UString& Replace(
      const char* before, int32 before_len, char after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, before_len, after, casesense);
    return *this;
  }
  inline UString& Replace(
      const char* before, int32 before_len, const char* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, before_len, after, after_len, casesense);
    return *this;
  }
  inline UString& Replace(
      const char* before, int32 before_len, ByteStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, before_len, after, casesense);
    return *this;
  }

  // ByteStringView -> *
  inline UString& Replace(
      ByteStringView before, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
  inline UString& Replace(
      ByteStringView before, const UNICHAR* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, after_len, casesense);
    return *this;
  }
  inline UString& Replace(
      ByteStringView before, UStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
  inline UString& Replace(
      ByteStringView before, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
  inline UString& Replace(
      ByteStringView before, char after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
  inline UString& Replace(
      ByteStringView before, const char* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, after_len, casesense);
    return *this;
  }
  inline UString& Replace(
      ByteStringView before, ByteStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
#endif

  // UNICHAR -> *
  inline UString Replaced(
      UNICHAR before, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, casesense);
  }
  inline UString Replaced(
      UNICHAR before, const UNICHAR* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, after_len, casesense);
  }
  inline UString Replaced(
      UNICHAR before, UStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, casesense);
  }
  inline UString Replaced(
      UNICHAR before, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, casesense);
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline UString Replaced(
      UNICHAR before, char after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, casesense);
  }
  inline UString Replaced(
      UNICHAR before, const char* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, after_len, casesense);
  }
  inline UString Replaced(
      UNICHAR before, ByteStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, casesense);
  }
#endif

  // const UNICHAR*,Len -> *
  inline UString Replaced(
      const UNICHAR* before, int32 before_len, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, casesense);
  }
  inline UString Replaced(
      const UNICHAR* before, int32 before_len, const UNICHAR* after,
      int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, after_len, casesense);
  }
  inline UString Replaced(
      const UNICHAR* before, int32 before_len, UStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, casesense);
  }
  inline UString Replaced(
      const UNICHAR* before, int32 before_len, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, casesense);
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline UString Replaced(
      const UNICHAR* before, int32 before_len, char after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, casesense);
  }
  inline UString Replaced(
      const UNICHAR* before, int32 before_len, const char* after,
      int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, after_len, casesense);
  }
  inline UString Replaced(
      const UNICHAR* before, int32 before_len, ByteStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, casesense);
  }
#endif

  // UStringView -> *
  inline UString Replaced(
      UStringView before, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, casesense);
  }
  inline UString Replaced(
      UStringView before, const UNICHAR* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, after_len, casesense);
  }
  inline UString Replaced(
      UStringView before, UStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, casesense);
  }
  inline UString Replaced(
      UStringView before, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, casesense);
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline UString Replaced(
      UStringView before, char after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, casesense);
  }
  inline UString Replaced(
      UStringView before, const char* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, after_len, casesense);
  }
  inline UString Replaced(
      UStringView before, ByteStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, casesense);
  }
#endif

  // AsciiString -> *
  inline UString Replaced(
      AsciiString before, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, casesense);
  }
  inline UString Replaced(
      AsciiString before, const UNICHAR* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, after_len, casesense);
  }
  inline UString Replaced(
      AsciiString before, UStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, casesense);
  }
  inline UString Replaced(
      AsciiString before, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, casesense);
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline UString Replaced(
      AsciiString before, char after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, casesense);
  }
  inline UString Replaced(
      AsciiString before, const char* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, after_len, casesense);
  }
  inline UString Replaced(
      AsciiString before, ByteStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, casesense);
  }
#endif

#if FUN_USE_IMPLICIT_STRING_CONVERSION
  // char* -> *
  inline UString Replaced(
      const char* before, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, casesense);
  }
  inline UString Replaced(
      const char* before, const UNICHAR* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, casesense);
  }
  inline UString Replaced(
      const char* before, UStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, casesense);
  }
  inline UString Replaced(
      const char* before, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, casesense);
  }
  inline UString Replaced(
      const char* before, char after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, casesense);
  }
  inline UString Replaced(
      const char* before, const char* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, after_len, casesense);
  }
  inline UString Replaced(
      const char* before, ByteStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, casesense);
  }

  // const char*,Len -> *
  inline UString Replaced(
      const char* before, int32 before_len, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, casesense);
  }
  inline UString Replaced(
      const char* before, int32 before_len, const UNICHAR* after,
      int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, after_len, casesense);
  }
  inline UString Replaced(
      const char* before, int32 before_len, UStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, casesense);
  }
  inline UString Replaced(
      const char* before, int32 before_len, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, casesense);
  }
  inline UString Replaced(
      const char* before, int32 before_len, char after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, casesense);
  }
  inline UString Replaced(
      const char* before, int32 before_len, const char* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, after_len, casesense);
  }
  inline UString Replaced(
      const char* before, int32 before_len, ByteStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, casesense);
  }

  // ByteStringView -> *
  inline UString Replaced(
      ByteStringView before, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, casesense);
  }
  inline UString Replaced(
      ByteStringView before, const UNICHAR* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, casesense);
  }
  inline UString Replaced(
      ByteStringView before, UStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, casesense);
  }
  inline UString Replaced(
      ByteStringView before, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, casesense);
  }
  inline UString Replaced(
      ByteStringView before, char after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, casesense);
  }
  inline UString Replaced(
      ByteStringView before, const char* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, after_len, casesense);
  }
  inline UString Replaced(
      ByteStringView before, ByteStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return UString(*this).Replace(before, after, casesense);
  }
#endif

  Array<UString> Split(
      UNICHAR separator, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UString> Split(
      const Array<UNICHAR>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UString> Split(
      std::initializer_list<UNICHAR> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UString> Split(
      UStringView separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UString> Split(
      AsciiString separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  Array<UString> Split(
      const Array<const UNICHAR*>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UString> Split(
      const Array<UString>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UString> Split(
      const Array<UStringRef>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UString> Split(
      const Array<UStringView>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UString> Split(
      const Array<AsciiString>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  Array<UString> Split(
      std::initializer_list<const UNICHAR*> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UString> Split(
      std::initializer_list<UString> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UString> Split(
      std::initializer_list<UStringRef> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UString> Split(
      std::initializer_list<UStringView> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UString> Split(
      std::initializer_list<AsciiString> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  Array<UString> SplitByWhitespaces(
      UString extra_separator = UString(), int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UString> SplitLines(
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

#if FUN_USE_IMPLICIT_STRING_CONVERSION
  Array<UString> Split(
      ByteStringView separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  Array<UString> Split(
      const Array<const char*>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UString> Split(
      const Array<ByteString>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UString> Split(
      const Array<ByteStringRef>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UString> Split(
      const Array<ByteStringView>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  Array<UString> Split(
      const std::initializer_list<const char*> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UString> Split(
      const std::initializer_list<ByteString> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UString> Split(
      const std::initializer_list<ByteStringRef> separators,
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UString> Split(
      const std::initializer_list<ByteStringView> separators,
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
#endif

  int32 Split(Array<UString>& out_tokens, UNICHAR separator,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<UString>& out_tokens, const Array<UNICHAR>& separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<UString>& out_tokens,
              std::initializer_list<UNICHAR> separators, int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<UString>& out_tokens, UStringView separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<UString>& out_tokens, AsciiString separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  int32 Split(Array<UString>& out_tokens,
              const Array<const UNICHAR*>& separators, int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<UString>& out_tokens, const Array<UString>& separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<UString>& out_tokens, const Array<UStringRef>& separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<UString>& out_tokens, const Array<UStringView>& separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<UString>& out_tokens, const Array<AsciiString>& separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  int32 Split(Array<UString>& out_tokens,
              std::initializer_list<const UNICHAR*> separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<UString>& out_tokens,
              std::initializer_list<UString> separators, int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<UString>& out_tokens,
              std::initializer_list<UStringRef> separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<UString>& out_tokens,
              std::initializer_list<UStringView> separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<UString>& out_tokens,
              std::initializer_list<AsciiString> separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  int32 SplitByWhitespaces(
      Array<UString>& out_tokens, UString extra_separator = UString(),
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitLines(
      Array<UString>& out_lines,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

#if FUN_USE_IMPLICIT_STRING_CONVERSION
  int32 Split(Array<UString>& out_tokens, ByteStringView separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  int32 Split(Array<UString>& out_tokens, const Array<const char*>& separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<UString>& out_tokens, const Array<ByteString>& separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<UString>& out_tokens,
              const Array<ByteStringRef>& separators, int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<UString>& out_tokens,
              const Array<ByteStringView>& separators, int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  int32 Split(Array<UString>& out_tokens,
              std::initializer_list<const char*> separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<UString>& out_tokens,
              std::initializer_list<ByteString> separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<UString>& out_tokens,
              std::initializer_list<ByteStringRef> separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<UString>& out_tokens,
              std::initializer_list<ByteStringView> separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
#endif

  // Reference based version

  Array<UStringRef> SplitRef(
      UNICHAR separator, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UStringRef> SplitRef(
      const Array<UNICHAR>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UStringRef> SplitRef(
      std::initializer_list<UNICHAR> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UStringRef> SplitRef(
      UStringView separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UStringRef> SplitRef(
      AsciiString separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  Array<UStringRef> SplitRef(
      const Array<const UNICHAR*>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UStringRef> SplitRef(
      const Array<UString>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UStringRef> SplitRef(
      const Array<UStringRef>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UStringRef> SplitRef(
      const Array<UStringView>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UStringRef> SplitRef(
      const Array<AsciiString>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  Array<UStringRef> SplitRef(
      std::initializer_list<const UNICHAR*> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UStringRef> SplitRef(
      std::initializer_list<UString> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UStringRef> SplitRef(
      std::initializer_list<UStringRef> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UStringRef> SplitRef(
      std::initializer_list<UStringView> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UStringRef> SplitRef(
      std::initializer_list<AsciiString> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  Array<UStringRef> SplitByWhitespacesRef(
      UString extra_separator = UString(), int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UStringRef> SplitLinesRef(
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

#if FUN_USE_IMPLICIT_STRING_CONVERSION
  Array<UStringRef> SplitRef(
      ByteStringView separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  Array<UStringRef> SplitRef(
      const Array<const char*>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UStringRef> SplitRef(
      const Array<ByteString>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UStringRef> SplitRef(
      const Array<ByteStringRef>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UStringRef> SplitRef(
      const Array<ByteStringView>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  Array<UStringRef> SplitRef(
      const std::initializer_list<const char*> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UStringRef> SplitRef(
      const std::initializer_list<ByteString> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UStringRef> SplitRef(
      const std::initializer_list<ByteStringRef> separators,
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UStringRef> SplitRef(
      const std::initializer_list<ByteStringView> separators,
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
#endif

  int32 SplitRef(
      Array<UStringRef>& out_tokens, UNICHAR separator, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitRef(
      Array<UStringRef>& out_tokens, const Array<UNICHAR>& separators,
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitRef(
      Array<UStringRef>& out_tokens, std::initializer_list<UNICHAR> separators,
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitRef(
      Array<UStringRef>& out_tokens, UStringView separators,
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitRef(
      Array<UStringRef>& out_tokens, AsciiString separators,
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  int32 SplitRef(
      Array<UStringRef>& out_tokens, const Array<const UNICHAR*>& separators,
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitRef(
      Array<UStringRef>& out_tokens, const Array<UString>& separators,
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitRef(
      Array<UStringRef>& out_tokens, const Array<UStringRef>& separators,
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitRef(
      Array<UStringRef>& out_tokens, const Array<UStringView>& separators,
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitRef(
      Array<UStringRef>& out_tokens, const Array<AsciiString>& separators,
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  int32 SplitRef(
      Array<UStringRef>& out_tokens,
      std::initializer_list<const UNICHAR*> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitRef(
      Array<UStringRef>& out_tokens, std::initializer_list<UString> separators,
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitRef(
      Array<UStringRef>& out_tokens,
      std::initializer_list<UStringRef> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitRef(
      Array<UStringRef>& out_tokens,
      std::initializer_list<UStringView> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitRef(
      Array<UStringRef>& out_tokens,
      std::initializer_list<AsciiString> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  int32 SplitByWhitespacesRef(
      Array<UStringRef>& out_tokens, UString extra_separator = UString(),
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitLinesRef(
      Array<UStringRef>& out_lines,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

#if FUN_USE_IMPLICIT_STRING_CONVERSION
  int32 SplitRef(
      Array<UStringRef>& out_tokens, ByteStringView separators,
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  int32 SplitRef(
      Array<UStringRef>& out_tokens, const Array<const char*>& separators,
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitRef(
      Array<UStringRef>& out_tokens, const Array<ByteString>& separators,
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitRef(
      Array<UStringRef>& out_tokens, const Array<ByteStringRef>& separators,
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitRef(
      Array<UStringRef>& out_tokens, const Array<ByteStringView>& separators,
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  int32 SplitRef(
      Array<UStringRef>& out_tokens,
      std::initializer_list<const char*> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitRef(
      Array<UStringRef>& out_tokens,
      std::initializer_list<ByteString> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitRef(
      Array<UStringRef>& out_tokens,
      std::initializer_list<ByteStringRef> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitRef(
      Array<UStringRef>& out_tokens,
      std::initializer_list<ByteStringView> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
#endif

  bool Divide(UStringView delim, UString* out_left, UString* out_right,
              bool trimming = false,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  inline bool Divide(
      UNICHAR delim_char, UString* out_left, UString* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Divide(UStringView(&delim_char, 1), out_left, out_right, trimming,
                  casesense);
  }
  inline bool Divide(
      const UNICHAR* delim, int32 delim_len, UString* out_left,
      UString* out_right, bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Divide(UStringView(delim, delim_len), out_left, out_right, trimming,
                  casesense);
  }
  bool Divide(AsciiString delim, UString* out_left, UString* out_right,
              bool trimming = false,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  // Ambiguity problem
  // inline bool Divide(char delim_char, UString* out_left, UString* out_right,
  // bool trimming = false, CaseSensitivity casesense =
  // CaseSensitivity::CaseSensitive) const { return Divide(UNICHAR(delim_char),
  //out_left, out_right, trimming, casesense); }
  inline bool Divide(
      const char* delim, int32 delim_len, UString* out_left, UString* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Divide(UTF8_AS_UNICHAR(delim, delim_len).ToView(), out_left,
                  out_right, trimming, casesense);
  }
  inline bool Divide(
      ByteStringView delim, UString* out_left, UString* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Divide(UTF8_AS_UNICHAR(delim).ToView(), out_left, out_right,
                  trimming, casesense);
  }
#endif

  bool Divide(UStringView delim, UStringRef* out_left, UStringRef* out_right,
              bool trimming = false,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  inline bool Divide(
      UNICHAR delim_char, UStringRef* out_left, UStringRef* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Divide(UStringView(&delim_char, 1), out_left, out_right, trimming,
                  casesense);
  }
  inline bool Divide(
      const UNICHAR* delim, int32 delim_len, UStringRef* out_left,
      UStringRef* out_right, bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Divide(UStringView(delim, delim_len), out_left, out_right, trimming,
                  casesense);
  }
  bool Divide(AsciiString delim, UStringRef* out_left, UStringRef* out_right,
              bool trimming = false,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  // Ambiguity problem
  // inline bool Divide(char delim_char, UStringRef* out_left, UStringRef*
  // out_right, bool trimming = false, CaseSensitivity casesense =
  // CaseSensitivity::CaseSensitive) const { return Divide(UNICHAR(delim_char),
  //out_left, out_right, trimming, casesense); }
  inline bool Divide(
      const char* delim, int32 delim_len, UStringRef* out_left,
      UStringRef* out_right, bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Divide(UTF8_AS_UNICHAR(delim, delim_len).ToView(), out_left,
                  out_right, trimming, casesense);
  }
  inline bool Divide(
      ByteStringView delim, UStringRef* out_left, UStringRef* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Divide(UTF8_AS_UNICHAR(delim).ToView(), out_left, out_right,
                  trimming, casesense);
  }
#endif

  bool LastDivide(
      UStringView delim, UString* out_left, UString* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  inline bool LastDivide(
      UNICHAR delim_char, UString* out_left, UString* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return LastDivide(UStringView(&delim_char, 1), out_left, out_right,
                      trimming, casesense);
  }
  inline bool LastDivide(
      const UNICHAR* delim, int32 delim_len, UString* out_left,
      UString* out_right, bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return LastDivide(UStringView(delim, delim_len), out_left, out_right,
                      trimming, casesense);
  }
  bool LastDivide(
      AsciiString delim, UString* out_left, UString* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  // Ambiguity problem
  // inline bool LastDivide(char delim_char, UString* out_left, UString*
  // out_right, bool trimming = false, CaseSensitivity casesense =
  // CaseSensitivity::CaseSensitive) const { return
  //LastDivide(UNICHAR(delim_char), out_left, out_right, trimming, casesense); }
  inline bool LastDivide(
      const char* delim, int32 delim_len, UString* out_left, UString* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return LastDivide(UTF8_AS_UNICHAR(delim, delim_len).ToView(), out_left,
                      out_right, trimming, casesense);
  }
  inline bool LastDivide(
      ByteStringView delim, UString* out_left, UString* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return LastDivide(UTF8_AS_UNICHAR(delim).ToView(), out_left, out_right,
                      trimming, casesense);
  }
#endif

  bool LastDivide(
      UStringView delim, UStringRef* out_left, UStringRef* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  inline bool LastDivide(
      UNICHAR delim_char, UStringRef* out_left, UStringRef* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return LastDivide(UStringView(&delim_char, 1), out_left, out_right,
                      trimming, casesense);
  }
  inline bool LastDivide(
      const UNICHAR* delim, int32 delim_len, UStringRef* out_left,
      UStringRef* out_right, bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return LastDivide(UStringView(delim, delim_len), out_left, out_right,
                      trimming, casesense);
  }
  bool LastDivide(
      AsciiString delim, UStringRef* out_left, UStringRef* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  // Ambiguity problem
  // inline bool LastDivide(char delim_char, UStringRef* out_left, UStringRef*
  // out_right, bool trimming = false, CaseSensitivity casesense =
  // CaseSensitivity::CaseSensitive) const { return
  //LastDivide(UNICHAR(delim_char), out_left, out_right, trimming, casesense); }
  inline bool LastDivide(
      const char* delim, int32 delim_len, UStringRef* out_left,
      UStringRef* out_right, bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return LastDivide(UTF8_AS_UNICHAR(delim, delim_len).ToView(), out_left,
                      out_right, trimming, casesense);
  }
  inline bool LastDivide(
      ByteStringView delim, UStringRef* out_left, UStringRef* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return LastDivide(UTF8_AS_UNICHAR(delim).ToView(), out_left, out_right,
                      trimming, casesense);
  }
#endif

  static UString Join(const Array<const UNICHAR*>& list, UStringView separator);
  static UString Join(const Array<UString>& list, UStringView separator);
  static UString Join(const Array<UStringRef>& list, UStringView separator);
  static UString Join(const Array<UStringView>& list, UStringView separator);
  static UString Join(const Array<AsciiString>& list, UStringView separator);
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  static UString Join(const Array<const char*>& list, UStringView separator);
  static UString Join(const Array<ByteString>& list, UStringView separator);
  static UString Join(const Array<ByteStringRef>& list, UStringView separator);
  static UString Join(const Array<ByteStringView>& list, UStringView separator);
#endif

  static UString Join(std::initializer_list<const UNICHAR*> list,
                      UStringView separator);
  static UString Join(std::initializer_list<UString> list,
                      UStringView separator);
  static UString Join(std::initializer_list<UStringRef> list,
                      UStringView separator);
  static UString Join(std::initializer_list<UStringView> list,
                      UStringView separator);
  static UString Join(std::initializer_list<AsciiString> list,
                      UStringView separator);
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  static UString Join(std::initializer_list<const char*> list,
                      UStringView separator);
  static UString Join(std::initializer_list<ByteString> list,
                      UStringView separator);
  static UString Join(std::initializer_list<ByteStringRef> list,
                      UStringView separator);
  static UString Join(std::initializer_list<ByteStringView> list,
                      UStringView separator);
#endif

  static UString Join(const Array<const UNICHAR*>& list, AsciiString separator);
  static UString Join(const Array<UString>& list, AsciiString separator);
  static UString Join(const Array<UStringRef>& list, AsciiString separator);
  static UString Join(const Array<UStringView>& list, AsciiString separator);
  static UString Join(const Array<AsciiString>& list, AsciiString separator);
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  static UString Join(const Array<const char*>& list, AsciiString separator);
  static UString Join(const Array<ByteString>& list, AsciiString separator);
  static UString Join(const Array<ByteStringRef>& list, AsciiString separator);
  static UString Join(const Array<ByteStringView>& list, AsciiString separator);
#endif

  static UString Join(std::initializer_list<const UNICHAR*> list,
                      AsciiString separator);
  static UString Join(std::initializer_list<UString> list,
                      AsciiString separator);
  static UString Join(std::initializer_list<UStringRef> list,
                      AsciiString separator);
  static UString Join(std::initializer_list<UStringView> list,
                      AsciiString separator);
  static UString Join(std::initializer_list<AsciiString> list,
                      AsciiString separator);
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  static UString Join(std::initializer_list<const char*> list,
                      AsciiString separator);
  static UString Join(std::initializer_list<ByteString> list,
                      AsciiString separator);
  static UString Join(std::initializer_list<ByteStringRef> list,
                      AsciiString separator);
  static UString Join(std::initializer_list<ByteStringView> list,
                      AsciiString separator);
#endif

#if FUN_USE_IMPLICIT_STRING_CONVERSION
  static UString Join(const Array<const UNICHAR*>& list,
                      ByteStringView separator);
  static UString Join(const Array<UString>& list, ByteStringView separator);
  static UString Join(const Array<UStringRef>& list, ByteStringView separator);
  static UString Join(const Array<UStringView>& list, ByteStringView separator);
  static UString Join(const Array<AsciiString>& list, ByteStringView separator);
  static UString Join(const Array<const char*>& list, ByteStringView separator);
  static UString Join(const Array<ByteString>& list, ByteStringView separator);
  static UString Join(const Array<ByteStringRef>& list,
                      ByteStringView separator);
  static UString Join(const Array<ByteStringView>& list,
                      ByteStringView separator);

  static UString Join(std::initializer_list<const UNICHAR*> list,
                      ByteStringView separator);
  static UString Join(std::initializer_list<UString> list,
                      ByteStringView separator);
  static UString Join(std::initializer_list<UStringRef> list,
                      ByteStringView separator);
  static UString Join(std::initializer_list<UStringView> list,
                      ByteStringView separator);
  static UString Join(std::initializer_list<AsciiString> list,
                      ByteStringView separator);
  static UString Join(std::initializer_list<const char*> list,
                      ByteStringView separator);
  static UString Join(std::initializer_list<ByteString> list,
                      ByteStringView separator);
  static UString Join(std::initializer_list<ByteStringRef> list,
                      ByteStringView separator);
  static UString Join(std::initializer_list<ByteStringView> list,
                      ByteStringView separator);
#endif

#if FUN_USE_REGULAR_EXPRESSION
  inline int32 IndexOf(const Regex& regex, int32 from = 0) const {
    return IndexOf(regex, from, nullptr);
  }
  int32 IndexOf(const Regex& regex, int32 from, RegexMatch* out_match) const;

  inline int32 LastIndexOf(const Regex& regex, int32 from = 0) const {
    return LastIndexOf(regex, from, nullptr);
  }
  int32 LastIndexOf(const Regex& regex, int32 from,
                    RegexMatch* out_match) const;

  inline bool Contains(const Regex& regex) const {
    return Contains(regex, nullptr);
  }
  bool Contains(const Regex& regex, RegexMatch* out_match) const;

  int32 Count(const Regex& regex) const;

  int32 TryFindAndRemove(const Regex& regex);
  inline UString& FindAndRemove(const Regex& regex) {
    TryFindAndRemove(regex);
    return *this;
  }

  int32 TryReplace(const Regex& regex, UStringView after);
  inline UString& Replace(const Regex& regex, UStringView after) {
    TryReplace(regex, after);
    return *this;
  }
  inline UString Replaced(const Regex& regex, UStringView after) {
    UString result(*this);
    return result.Replace(regex, after);
  }

  int32 TryReplace(const Regex& regex, AsciiString after);
  inline UString& Replace(const Regex& regex, AsciiString after) {
    TryReplace(regex, after);
    return *this;
  }
  inline UString Replaced(const Regex& regex, AsciiString after) {
    UString result(*this);
    return result.Replace(regex, after);
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  int32 TryReplace(const Regex& regex, ByteStringView after);
  inline UString& Replace(const Regex& regex, ByteStringView after) {
    TryReplace(regex, after);
    return *this;
  }
  inline UString Replaced(const Regex& regex, ByteStringView after) {
    UString result(*this);
    return result.Replace(regex, after);
  }
#endif

  // TODO 최대 split 횟수를 지정할 수 있도록 하는게 좋을듯...
  Array<UString> Split(
      const Regex& regex, int32 max_splits = 0,
      StringSplitOptions options = StringSplitOption::None) const;
  // TODO 최대 split 횟수를 지정할 수 있도록 하는게 좋을듯...
  Array<UStringRef> SplitRef(
      const Regex& regex, int32 max_splits = 0,
      StringSplitOptions options = StringSplitOption::None) const;
#endif  // FUN_USE_REGULAR_EXPRESSION

  // UString and UString
  int32 Compare(const UString& str, CaseSensitivity casesense =
                                        CaseSensitivity::CaseSensitive) const;
  bool Equals(const UString& str,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool operator==(const UString& str) const;
  bool operator!=(const UString& str) const;
  bool operator<(const UString& str) const;
  bool operator<=(const UString& str) const;
  bool operator>(const UString& str) const;
  bool operator>=(const UString& str) const;

  // UString and UStringRef
  int32 Compare(
      const UStringRef& str,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool Equals(const UStringRef& str,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool operator==(const UStringRef& str) const;
  bool operator!=(const UStringRef& str) const;
  bool operator<(const UStringRef& str) const;
  bool operator<=(const UStringRef& str) const;
  bool operator>(const UStringRef& str) const;
  bool operator>=(const UStringRef& str) const;

  // UString and UStringView
  int32 Compare(
      const UStringView& str,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool Equals(const UStringView& str,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool operator==(const UStringView& str) const;
  bool operator!=(const UStringView& str) const;
  bool operator<(const UStringView& str) const;
  bool operator<=(const UStringView& str) const;
  bool operator>(const UStringView& str) const;
  bool operator>=(const UStringView& str) const;

  // UString and AsciiString
  int32 Compare(
      const AsciiString& str,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool Equals(const AsciiString& str,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool operator==(const AsciiString& str) const;
  bool operator!=(const AsciiString& str) const;
  bool operator<(const AsciiString& str) const;
  bool operator<=(const AsciiString& str) const;
  bool operator>(const AsciiString& str) const;
  bool operator>=(const AsciiString& str) const;

  // UString and ByteString
  int32 Compare(
      const ByteString& str,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool Equals(const ByteString& str,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool operator==(const ByteString& str) const;
  bool operator!=(const ByteString& str) const;
  bool operator<(const ByteString& str) const;
  bool operator<=(const ByteString& str) const;
  bool operator>(const ByteString& str) const;
  bool operator>=(const ByteString& str) const;

  // UString and ByteStringRef
  int32 Compare(
      const ByteStringRef& str,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool Equals(const ByteStringRef& str,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool operator==(const ByteStringRef& str) const;
  bool operator!=(const ByteStringRef& str) const;
  bool operator<(const ByteStringRef& str) const;
  bool operator<=(const ByteStringRef& str) const;
  bool operator>(const ByteStringRef& str) const;
  bool operator>=(const ByteStringRef& str) const;

  // UString and ByteStringView
  int32 Compare(
      const ByteStringView& str,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool Equals(const ByteStringView& str,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool operator==(const ByteStringView& str) const;
  bool operator!=(const ByteStringView& str) const;
  bool operator<(const ByteStringView& str) const;
  bool operator<=(const ByteStringView& str) const;
  bool operator>(const ByteStringView& str) const;
  bool operator>=(const ByteStringView& str) const;

  // UString and char
  int32 Compare(char ch, CaseSensitivity casesense =
                             CaseSensitivity::CaseSensitive) const;
  bool Equals(char ch,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool operator==(char ch) const;
  bool operator!=(char ch) const;
  bool operator<(char ch) const;
  bool operator<=(char ch) const;
  bool operator>(char ch) const;
  bool operator>=(char ch) const;
  friend bool operator==(char ch, const UString& str);
  friend bool operator!=(char ch, const UString& str);
  friend bool operator<(char ch, const UString& str);
  friend bool operator<=(char ch, const UString& str);
  friend bool operator>(char ch, const UString& str);
  friend bool operator>=(char ch, const UString& str);

  // UString and const char*
  int32 Compare(const char* str, CaseSensitivity casesense =
                                     CaseSensitivity::CaseSensitive) const;
  bool Equals(const char* str,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool operator==(const char* str) const;
  bool operator!=(const char* str) const;
  bool operator<(const char* str) const;
  bool operator<=(const char* str) const;
  bool operator>(const char* str) const;
  bool operator>=(const char* str) const;
  friend bool operator==(const char* str1, const UString& str2);
  friend bool operator!=(const char* str1, const UString& str2);
  friend bool operator<(const char* str1, const UString& str2);
  friend bool operator<=(const char* str1, const UString& str2);
  friend bool operator>(const char* str1, const UString& str2);
  friend bool operator>=(const char* str1, const UString& str2);

  // UString and UNICHAR
  int32 Compare(UNICHAR ch, CaseSensitivity casesense =
                                CaseSensitivity::CaseSensitive) const;
  bool Equals(UNICHAR ch,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool operator==(UNICHAR ch) const;
  bool operator!=(UNICHAR ch) const;
  bool operator<(UNICHAR ch) const;
  bool operator<=(UNICHAR ch) const;
  bool operator>(UNICHAR ch) const;
  bool operator>=(UNICHAR ch) const;
  friend bool operator==(UNICHAR ch, const UString& str);
  friend bool operator!=(UNICHAR ch, const UString& str);
  friend bool operator<(UNICHAR ch, const UString& str);
  friend bool operator<=(UNICHAR ch, const UString& str);
  friend bool operator>(UNICHAR ch, const UString& str);
  friend bool operator>=(UNICHAR ch, const UString& str);

  // UString and const UNICHAR*
  int32 Compare(const UNICHAR* str, CaseSensitivity casesense =
                                        CaseSensitivity::CaseSensitive) const;
  bool Equals(const UNICHAR* str,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool operator==(const UNICHAR* str) const;
  bool operator!=(const UNICHAR* str) const;
  bool operator<(const UNICHAR* str) const;
  bool operator<=(const UNICHAR* str) const;
  bool operator>(const UNICHAR* str) const;
  bool operator>=(const UNICHAR* str) const;
  friend bool operator==(const UNICHAR* str1, const UString& str2);
  friend bool operator!=(const UNICHAR* str1, const UString& str2);
  friend bool operator<(const UNICHAR* str1, const UString& str2);
  friend bool operator<=(const UNICHAR* str1, const UString& str2);
  friend bool operator>(const UNICHAR* str1, const UString& str2);
  friend bool operator>=(const UNICHAR* str1, const UString& str2);

  int16 ToInt16(bool* ok = nullptr, int32 base = 10) const;
  uint16 ToUInt16(bool* ok = nullptr, int32 base = 10) const;
  int32 ToInt32(bool* ok = nullptr, int32 base = 10) const;
  uint32 ToUInt32(bool* ok = nullptr, int32 base = 10) const;
  int64 ToInt64(bool* ok = nullptr, int32 base = 10) const;
  uint64 ToUInt64(bool* ok = nullptr, int32 base = 10) const;
  float ToFloat(bool* ok = nullptr) const;
  double ToDouble(bool* ok = nullptr) const;

  UString& SetNumber(int16 value, int32 base = 10);
  UString& SetNumber(uint16 value, int32 base = 10);
  UString& SetNumber(int32 value, int32 base = 10);
  UString& SetNumber(uint32 value, int32 base = 10);
  UString& SetNumber(int64 value, int32 base = 10);
  UString& SetNumber(uint64 value, int32 base = 10);
  UString& SetNumber(float value, UNICHAR f = UNICHAR('g'),
                     int32 precision = 6);
  UString& SetNumber(double value, UNICHAR f = UNICHAR('g'),
                     int32 precision = 6);

  // TODO 좀더 최적화가 가능하지 않을까 싶은데...
  inline UString& AppendNumber(int16 value, int32 base = 10) {
    return (*this << UString().SetNumber(value, base));
  }
  inline UString& AppendNumber(uint16 value, int32 base = 10) {
    return (*this << UString().SetNumber(value, base));
  }
  inline UString& AppendNumber(int32 value, int32 base = 10) {
    return (*this << UString().SetNumber(value, base));
  }
  inline UString& AppendNumber(uint32 value, int32 base = 10) {
    return (*this << UString().SetNumber(value, base));
  }
  inline UString& AppendNumber(int64 value, int32 base = 10) {
    return (*this << UString().SetNumber(value, base));
  }
  inline UString& AppendNumber(uint64 value, int32 base = 10) {
    return (*this << UString().SetNumber(value, base));
  }
  inline UString& AppendNumber(float value, UNICHAR f = UNICHAR('g'),
                               int32 precision = 6) {
    return (*this << UString().SetNumber(value, f, precision));
  }
  inline UString& AppendNumber(double value, UNICHAR f = UNICHAR('g'),
                               int32 precision = 6) {
    return (*this << UString().SetNumber(value, f, precision));
  }

  static UString FromNumber(int16 value, int32 base = 10);
  static UString FromNumber(uint16 value, int32 base = 10);
  static UString FromNumber(int32 value, int32 base = 10);
  static UString FromNumber(uint32 value, int32 base = 10);
  static UString FromNumber(int64 value, int32 base = 10);
  static UString FromNumber(uint64 value, int32 base = 10);
  static UString FromNumber(float value, UNICHAR f = UNICHAR('g'),
                            int32 precision = 6);
  static UString FromNumber(double value, UNICHAR f = UNICHAR('g'),
                            int32 precision = 6);

  UString& PathAppend(UStringView str);

  inline UString& PathAppend(UNICHAR ch) {
    return PathAppend(UStringView(&ch, 1));
  }
  inline UString& PathAppend(const UNICHAR* str, int32 len) {
    return PathAppend(UStringView(str, len));
  }
  UString& PathAppend(AsciiString str);
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline UString& PathAppend(ByteStringView str) {
    return PathAppend(UTF8_AS_UNICHAR(str).ToView());
  }
  inline UString& PathAppend(const char* str, int32 len) {
    return PathAppend(ByteStringView(str, len));
  }
#endif

  bool IsPathTerminated() const;
  UString& MakePathTerminated();
  UString ToPathTerminated() const;

  // UStringView로 const UNICHAR* / UString 을 구분하지 않고 처리하면,
  // 컴파일러가 제대로 해석을 하지 못하는 문제가 있어서, 각각의 타입을 별개로
  //처리해주어야함.
  inline UString& operator/=(UNICHAR ch) { return PathAppend(ch); }
  inline UString& operator/=(const UNICHAR* str) { return PathAppend(str); }
  inline UString& operator/=(const UString& str) { return PathAppend(str); }
  inline UString& operator/=(const UStringRef& str) { return PathAppend(str); }
  inline UString& operator/=(UStringView str) { return PathAppend(str); }
  inline UString& operator/=(AsciiString str) { return PathAppend(str); }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  // Ambiguity problem
  // inline UString& operator /= (char ch) { return PathAppend(ch); }
  inline UString& operator/=(const char* str) { return PathAppend(str); }
  inline UString& operator/=(const ByteString& str) { return PathAppend(str); }
  inline UString& operator/=(const ByteStringRef& str) {
    return PathAppend(str);
  }
  inline UString& operator/=(ByteStringView str) { return PathAppend(str); }
#endif

  // UStringView로 const UNICHAR* / UString 을 구분하지 않고 처리하면,
  // 컴파일러가 제대로 해석을 하지 못하는 문제가 있어서, 각각의 타입을 별개로
  //처리해주어야함.
  inline UString operator/(const UString& str) const {
    return UString(*this) /= str;
  }

  inline UString operator/(UNICHAR ch) const { return UString(*this) /= ch; }
  inline friend UString operator/(UNICHAR ch, const UString& str) {
    return UString(ch) /= str;
  }

  inline UString operator/(const UNICHAR* str) const {
    return UString(*this) /= str;
  }
  inline friend UString operator/(const UNICHAR* str1, const UString& str2) {
    return UString(str1) /= str2;
  }

  inline UString operator/(const UStringRef& str) const {
    return UString(*this) /= str;
  }
  inline friend UString operator/(const UStringRef& str1, const UString& str2) {
    return UString(str1) /= str2;
  }

  inline UString operator/(UStringView str) const {
    return UString(*this) /= str;
  }
  inline friend UString operator/(UStringView str1, const UString& str2) {
    return UString(str1) /= str2;
  }

  inline UString operator/(AsciiString str) const {
    return UString(*this) /= str;
  }
  inline friend UString operator/(AsciiString str1, const UString& str2) {
    return UString(str1) /= str2;
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  // Ambiguity problem
  // inline UString operator / (char ch) const { return UString(*this) /= ch; }
  // inline friend UString operator / (char ch, const UString& str) { return
  // UString(ch) /= str; }
  inline UString operator/(const char* str) const {
    return UString(*this) /= str;
  }
  inline friend UString operator/(const char* str1, const UString& str2) {
    return UString(str1) /= str2;
  }

  inline UString operator/(const ByteString& str) const {
    return UString(*this) /= str;
  }
  inline friend UString operator/(const ByteString& str1, const UString& str2) {
    return UString(str1) /= str2;
  }

  inline UString operator/(const ByteStringRef& str) const {
    return UString(*this) /= str;
  }
  inline friend UString operator/(const ByteStringRef& str1,
                                  const UString& str2) {
    return UString(str1) /= str2;
  }

  inline UString operator/(ByteStringView str) const {
    return UString(*this) /= str;
  }
  inline friend UString operator/(ByteStringView str1, const UString& str2) {
    return UString(str1) /= str2;
  }
#endif

  UString ToHtmlEscaped() const;

  bool IsNumeric() const;
  bool IsIdentifier() const;

  bool IsQuoted() const;
  static bool IsQuoted(UStringView str);
  UString Unquoted(bool* out_quotes_removed = nullptr) const;
  UString& Unquotes(bool* out_quotes_removed = nullptr);
  UStringRef UnquotedRef(bool* out_quotes_removed = nullptr) const;

  UString ReplaceQuotesWithEscapedQuotes() const;
  UString ReplaceCharWithEscapedChar(
      const Array<UNICHAR>* chars = nullptr) const;
  UString ReplaceEscapedCharWithChar(
      const Array<UNICHAR>* chars = nullptr) const;

  UString ConvertTabsToSpaces(int32 spaces_per_tab) const;

  void SerializeAsASCIICharArray(Archive& ar, int32 min_characters = 0) const;

  static UString Chr(UNICHAR ch) { return UString(ch); }
  static UString ChrN(UNICHAR ch, int32 count) { return UString(count, ch); }

  UString& SetRawData(const UNICHAR* raw, int32 len);
  static UString FromRawData(const UNICHAR* raw, int32 len);

  // VARARG_DECL(static UString, static UString, return, Printf, VARARG_NONE,
  // const UNICHAR*, VARARG_NONE, VARARG_NONE);

  // TODO
  // template <typename Args...>
  // static UString Format(Args args...)
  static UString Format(const UNICHAR* fmt, ...) {
    fun_check(!"TODO");
    return UString();
  }

  template <typename... Args>
  UString& Appendf(const Args&... args) {
    // TODO
    return *this;
  }

  static UStringData* FromUNICHARArray_helper(const UNICHAR* str,
                                              int32 len = -1);
  static UStringData* FromASCII_helper(const char* str, int32 len = -1);
  static UStringData* FromAsciiOrUtf8_helper(const char* str, int32 len = -1);
  static UString FromUTF8_helper(const char* str, int32 len);
  static UString FromLocal8Bit_helper(const char* str, int32 len);

  static UString FromAscii(const char* str, int32 len = -1);
  static UString FromAscii(const ByteString& str);
  static UString FromAscii(ByteStringRef str);
  static UString FromLocal8Bit(const char* str, int32 len = -1);
  static UString FromLocal8Bit(const ByteString& str);
  static UString FromLocal8Bit(ByteStringRef str);
  static UString FromStdString(const std::string& str);
  static UString FromStdU16String(const std::u16string& str);
  static UString FromStdU32String(const std::u32string& str);
  static UString FromStdWString(const std::wstring& str);
  static UString FromUtf32(const uint32* unicode, int32 len = -1);
  static UString FromUtf32(const char32_t* unicode, int32 len = -1);
  static UString FromUtf8(const char* str, int32 len = -1);
  static UString FromUtf8(const ByteString& str);
  static UString FromUtf8(ByteStringRef str);
  static UString FromUtf16(const uint16* unicode, int32 len = -1);
  static UString FromUtf16(const char16_t* unicode, int32 len = -1);
  static UString FromWCharArray(const wchar_t* str, int32 len = -1);

  bool IsAscii() const;

  // static UString FromCFString(CFStringRef str);
  // static UString FromNSString(const NSString* str);

  static ByteString ToAscii_helper(UStringView str);
  static ByteString ToAscii_helper_inplace(UString& str);
  static ByteString ToUtf8_helper(UStringView str);
  static ByteString ToLocal8Bit_helper(UStringView str);
  static int32 ToUtf32_helper(UStringView str, uint32* out);

  ByteString ToAscii() const;
  ByteString ToLocal8Bit() const;
  ByteString ToUtf8() const;
  std::string ToStdString() const;
  std::u16string ToStdU16String() const;
  std::u32string ToStdU32String() const;
  std::wstring ToStdWString() const;
  Array<uint32> ToUtf32() const;
  int32 ToWCharArray(wchar_t* array) const;
  const UNICHAR* Unicode() const;
  const uint16* ToUtf16() const;

  // CFStringRef ToCFString() const;
  // NSString* ToNSString() const;

  // STL compatibilities

  typedef int32 size_type;
  typedef intptr_t difference_type;
  typedef const UNICHAR& const_reference;
  typedef UNICHAR& reference;
  typedef UNICHAR* pointer;
  typedef const UNICHAR* const_pointer;
  typedef UNICHAR value_type;

  typedef UNICHAR* iterator;
  typedef const UNICHAR* const_iterator;
  typedef iterator Iterator;
  typedef const_iterator ConstIterator;
  typedef std::reverse_iterator<iterator> reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

  inline iterator begin() {
    Detach();
    return data_->MutableData();
  }
  inline const_iterator begin() const { return data_->ConstData(); }
  inline const_iterator cbegin() const { return data_->ConstData(); }
  inline iterator end() {
    Detach();
    return data_->MutableData() + data_->length;
  }
  inline const_iterator end() const {
    return data_->ConstData() + data_->length;
  }
  inline const_iterator cend() const {
    return data_->ConstData() + data_->length;
  }
  inline reverse_iterator rbegin() { return reverse_iterator(end()); }
  inline reverse_iterator rend() { return reverse_iterator(begin()); }
  inline const_reverse_iterator rbegin() const {
    return const_reverse_iterator(end());
  }
  inline const_reverse_iterator rend() const {
    return const_reverse_iterator(begin());
  }
  inline const_reverse_iterator crbegin() const {
    return const_reverse_iterator(end());
  }
  inline const_reverse_iterator crend() const {
    return const_reverse_iterator(begin());
  }

  inline void push_back(UNICHAR ch) { Append(ch); }
  inline void push_back(UStringView str) { Append(str); }
  inline void push_back(AsciiString str) { Append(str); }

  inline void push_front(UNICHAR ch) { Prepend(ch); }
  inline void push_front(UStringView str) { Prepend(str); }
  inline void push_front(AsciiString str) { Prepend(str); }

#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline void push_back(ByteStringView str) { Append(str); }
  inline void push_front(ByteStringView str) { Prepend(str); }
#endif

  inline void shrink_to_fit() { Shrink(); }

  // static UString FromStdString(const std::string& StdStr);
  // std::string ToStdString() const;

 private:
  // sharable data block. with COW (Copy On Write)
  UStringData* data_;

  void ReallocateData(int32 new_alloc, UStringData::AllocationOptions options);
  void ExpandAt(int32 pos);

 public:
  FUN_BASE_API friend uint32 HashOf(const UString&);
  FUN_BASE_API friend Archive& operator&(Archive& ar, UString&);

  friend class UStringRef;
  friend class UString;
};

class FUN_BASE_API UStringRef {
 public:
  typedef UNICHAR CharType;

  UStringRef();
  UStringRef(const UString* str, int32 pos, int32 len);
  explicit UStringRef(const UString* str);
  UStringRef(const UStringRef& rhs);
  UStringRef& operator=(const UStringRef& rhs);
  UStringRef(UStringRef&& rhs);
  UStringRef& operator=(UStringRef&& rhs);
  UStringRef& operator=(const UString* str);

  void Swap(UStringRef& rhs);

  const UString* Str() const;
  int32 Position() const;
  int32 Len() const;
  bool IsEmpty() const;
  void Clear();
  bool IsNull() const;

  const UNICHAR* operator*() const;
  const UNICHAR* ConstData() const;

  UString ToString() const;
  UStringRef AppendTo(UString* str) const;

  UNICHAR At(int32 index) const;
  UNICHAR operator[](int32 index) const;

  UNICHAR First() const;
  UNICHAR Last() const;

  UNICHAR FirstOr(const UNICHAR def = UNICHAR('\0')) const;
  UNICHAR LastOr(const UNICHAR def = UNICHAR('\0')) const;

  int32 IndexOfAny(UStringView chars,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;
  int32 IndexOfAny(AsciiString chars,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;
  int32 IndexOfAny(const Array<UNICHAR>& chars,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;
  inline int32 IndexOfAny(
      std::initializer_list<UNICHAR> chars,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_index = nullptr,
      int32* matched_len = nullptr) const {
    return IndexOfAny(UStringView(chars.begin(), chars.size()), casesense, from,
                      matched_index, matched_len);
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  // TODO Array<char>, std::initializer_list<char>
  inline int32 IndexOfAny(
      ByteStringView chars,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_index = nullptr,
      int32* matched_len = nullptr) const {
    return IndexOfAny(UTF8_AS_UNICHAR(chars).ToView(), casesense, from,
                      matched_index, matched_len);
  }
#endif

  int32 LastIndexOfAny(
      UStringView chars,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = -1, int32* matched_index = nullptr,
      int32* matched_len = nullptr) const;
  int32 LastIndexOfAny(
      AsciiString chars,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = -1, int32* matched_index = nullptr,
      int32* matched_len = nullptr) const;
  int32 LastIndexOfAny(
      const Array<UNICHAR>& chars,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = -1, int32* matched_index = nullptr,
      int32* matched_len = nullptr) const;
  inline int32 LastIndexOfAny(
      std::initializer_list<UNICHAR> chars,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = -1, int32* matched_index = nullptr,
      int32* matched_len = nullptr) const {
    return LastIndexOfAny(UStringView(chars.begin(), chars.size()), casesense,
                          from, matched_index, matched_len);
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  // TODO Array<char>, std::initializer_list<char>
  inline int32 LastIndexOfAny(
      ByteStringView chars,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = -1, int32* matched_index = nullptr,
      int32* matched_len = nullptr) const {
    return LastIndexOfAny(UTF8_AS_UNICHAR(chars).ToView(), casesense, from,
                          matched_index, matched_len);
  }
#endif

  int32 IndexOfAny(const Array<const UNICHAR*>& strings,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;
  int32 IndexOfAny(const Array<UString>& strings,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;
  int32 IndexOfAny(const Array<UStringRef>& strings,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;
  int32 IndexOfAny(const Array<UStringView>& strings,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;
  int32 IndexOfAny(const Array<AsciiString>& strings,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;

  int32 IndexOfAny(std::initializer_list<const UNICHAR*> strings,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;
  int32 IndexOfAny(std::initializer_list<UString> strings,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;
  int32 IndexOfAny(std::initializer_list<UStringRef> strings,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;
  int32 IndexOfAny(std::initializer_list<UStringView> strings,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;
  int32 IndexOfAny(std::initializer_list<AsciiString> strings,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;

#if FUN_USE_IMPLICIT_STRING_CONVERSION
  int32 IndexOfAny(const Array<const char*>& strings,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;
  int32 IndexOfAny(const Array<ByteString>& strings,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;
  int32 IndexOfAny(const Array<ByteStringRef>& strings,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;
  int32 IndexOfAny(const Array<ByteStringView>& strings,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;

  int32 IndexOfAny(std::initializer_list<const char*> strings,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;
  int32 IndexOfAny(std::initializer_list<ByteString> strings,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;
  int32 IndexOfAny(std::initializer_list<ByteStringRef> strings,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;
  int32 IndexOfAny(std::initializer_list<ByteStringView> strings,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;
#endif

  int32 IndexOf(UStringView sub,
                CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                int32 from = 0, int32* matched_len = nullptr) const;

  inline int32 IndexOf(
      UNICHAR ch, CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_len = nullptr) const {
    return IndexOf(UStringView(&ch, 1), casesense, from, matched_len);
  }
  inline int32 IndexOf(
      const UNICHAR* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_len = nullptr) const {
    return IndexOf(UStringView(sub, sub_len), casesense, from, matched_len);
  }
  int32 IndexOf(AsciiString sub,
                CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                int32 from = 0, int32* matched_len = nullptr) const;
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline int32 IndexOf(
      const char* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_len = nullptr) const {
    return IndexOf(UTF8_AS_UNICHAR(sub, sub_len).ToView(), casesense, from,
                   matched_len);
  }
  inline int32 IndexOf(
      ByteStringView sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_len = nullptr) const {
    return IndexOf(UTF8_AS_UNICHAR(sub).ToView(), casesense, from, matched_len);
  }
#endif

  template <typename Predicate>
  int32 IndexOfIf(const Predicate& pred, int32 from = 0) const;

  int32 LastIndexOf(UStringView sub,
                    CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                    int32 from = -1, int32* matched_len = nullptr) const;

  inline int32 LastIndexOf(
      UNICHAR ch, CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = -1, int32* matched_len = nullptr) const {
    return LastIndexOf(UStringView(&ch, 1), casesense, from, matched_len);
  }
  inline int32 LastIndexOf(
      const UNICHAR* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = -1, int32* matched_len = nullptr) const {
    return LastIndexOf(UStringView(sub, sub_len), casesense, from, matched_len);
  }
  int32 LastIndexOf(AsciiString sub,
                    CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                    int32 from = -1, int32* matched_len = nullptr) const;
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline int32 LastIndexOf(
      const char* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = -1, int32* matched_len = nullptr) const {
    return LastIndexOf(UTF8_AS_UNICHAR(sub, sub_len).ToView(), casesense, from,
                       matched_len);
  }
  inline int32 LastIndexOf(
      ByteStringView sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = -1, int32* matched_len = nullptr) const {
    return LastIndexOf(UTF8_AS_UNICHAR(sub).ToView(), casesense, from,
                       matched_len);
  }
#endif

  template <typename Predicate>
  int32 LastIndexOfIf(const Predicate& pred, int32 from = -1) const;

  inline bool Contains(
      UNICHAR ch, CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_len = nullptr) const {
    return IndexOf(ch, casesense, from, matched_len) != INVALID_INDEX;
  }
  inline bool Contains(
      const UNICHAR* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_len = nullptr) const {
    return IndexOf(sub, sub_len, casesense, from, matched_len) != INVALID_INDEX;
  }
  inline bool Contains(
      UStringView sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_len = nullptr) const {
    return IndexOf(sub, casesense, from, matched_len) != INVALID_INDEX;
  }
  inline bool Contains(
      AsciiString sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_len = nullptr) const {
    return IndexOf(sub, casesense, from, matched_len) != INVALID_INDEX;
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline bool Contains(
      const char* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_len = nullptr) const {
    return IndexOf(sub, sub_len, casesense, from, matched_len) != INVALID_INDEX;
  }
  inline bool Contains(
      ByteStringView sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_len = nullptr) const {
    return IndexOf(sub, casesense, from, matched_len) != INVALID_INDEX;
  }
#endif

  int32 Count(UStringView sub,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  inline int32 Count(UNICHAR ch, CaseSensitivity casesense =
                                     CaseSensitivity::CaseSensitive) const {
    return Count(UStringView(&ch, 1), casesense);
  }
  inline int32 Count(
      const UNICHAR* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Count(UStringView(sub, sub_len), casesense);
  }
  int32 Count(AsciiString sub,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline int32 Count(
      const char* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Count(UTF8_AS_UNICHAR(sub, sub_len).ToView(), casesense);
  }
  inline int32 Count(
      ByteStringView sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Count(UTF8_AS_UNICHAR(sub).ToView(), casesense);
  }
#endif

  UStringRef Left(int32 len) const;
  UStringRef Mid(int32 offset, int32 len = int32_MAX) const;
  UStringRef Right(int32 len) const;
  UStringRef LeftChopped(int32 len) const;
  UStringRef RightChopped(int32 len) const;

  bool StartsWith(UStringView sub, CaseSensitivity casesense =
                                       CaseSensitivity::CaseSensitive) const;

  inline bool StartsWith(UNICHAR ch, CaseSensitivity casesense =
                                         CaseSensitivity::CaseSensitive) const {
    return StartsWith(UStringView(&ch, 1), casesense);
  }
  inline bool StartsWith(
      const UNICHAR* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return StartsWith(UStringView(sub, sub_len), casesense);
  }
  bool StartsWith(AsciiString sub, CaseSensitivity casesense =
                                       CaseSensitivity::CaseSensitive) const;
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  bool StartsWith(
      const char* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return StartsWith(UTF8_AS_UNICHAR(sub, sub_len).ToView(), casesense);
  }
  inline bool StartsWith(
      ByteStringView sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return StartsWith(UTF8_AS_UNICHAR(sub).ToView(), casesense);
  }
#endif

  bool EndsWith(UStringView sub, CaseSensitivity casesense =
                                     CaseSensitivity::CaseSensitive) const;

  inline bool EndsWith(UNICHAR ch, CaseSensitivity casesense =
                                       CaseSensitivity::CaseSensitive) const {
    return EndsWith(UStringView(&ch, 1), casesense);
  }
  inline bool EndsWith(
      const UNICHAR* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return EndsWith(UStringView(sub, sub_len), casesense);
  }
  bool EndsWith(AsciiString sub, CaseSensitivity casesense =
                                     CaseSensitivity::CaseSensitive) const;
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline bool EndsWith(
      const char* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return EndsWith(UTF8_AS_UNICHAR(sub, sub_len).ToView(), casesense);
  }
  inline bool EndsWith(
      ByteStringView sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return EndsWith(UTF8_AS_UNICHAR(sub).ToView(), casesense);
  }
#endif

  bool GlobMatch(UStringView pattern, CaseSensitivity casesense =
                                          CaseSensitivity::CaseSensitive) const;

  inline bool GlobMatch(
      const UNICHAR* pattern, int32 pattern_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return GlobMatch(UStringView(pattern, pattern_len), casesense);
  }
  bool GlobMatch(AsciiString pattern, CaseSensitivity casesense =
                                          CaseSensitivity::CaseSensitive) const;
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline bool GlobMatch(
      const char* pattern, int32 pattern_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return GlobMatch(UTF8_AS_UNICHAR(pattern, pattern_len).ToView(), casesense);
  }
  inline bool GlobMatch(
      ByteStringView pattern,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return GlobMatch(UTF8_AS_UNICHAR(pattern).ToView(), casesense);
  }
#endif

  UStringRef& Truncate(int32 pos);
  UStringRef& LeftChop(int32 len);
  UStringRef& RightChop(int32 len);

  UStringRef& TrimLeft();
  UStringRef& TrimRight();
  UStringRef& Trim();
  UStringRef TrimmedLeft() const;
  UStringRef TrimmedRight() const;
  UStringRef Trimmed() const;
  int32 LeftSpaces() const;
  int32 RightSpaces() const;
  int32 SideSpaces() const;

  Array<UStringRef> Split(
      UNICHAR separator, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UStringRef> Split(
      const Array<UNICHAR>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UStringRef> Split(
      std::initializer_list<UNICHAR> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UStringRef> Split(
      UStringView separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UStringRef> Split(
      AsciiString separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  Array<UStringRef> Split(
      const Array<const UNICHAR*>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UStringRef> Split(
      const Array<UString>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UStringRef> Split(
      const Array<UStringRef>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UStringRef> Split(
      const Array<UStringView>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UStringRef> Split(
      const Array<AsciiString>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  Array<UStringRef> Split(
      std::initializer_list<const UNICHAR*> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UStringRef> Split(
      std::initializer_list<UString> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UStringRef> Split(
      std::initializer_list<UStringRef> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UStringRef> Split(
      std::initializer_list<UStringView> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UStringRef> Split(
      std::initializer_list<AsciiString> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  Array<UStringRef> SplitByWhitespaces(
      UString extra_separator = UString(), int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UStringRef> SplitLines(
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

#if FUN_USE_IMPLICIT_STRING_CONVERSION
  Array<UStringRef> Split(
      ByteStringView separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  Array<UStringRef> Split(
      const Array<const char*>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UStringRef> Split(
      const Array<ByteString>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UStringRef> Split(
      const Array<ByteStringRef>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UStringRef> Split(
      const Array<ByteStringView>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  Array<UStringRef> Split(
      std::initializer_list<const char*> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UStringRef> Split(
      std::initializer_list<ByteString> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UStringRef> Split(
      std::initializer_list<ByteStringRef> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<UStringRef> Split(
      std::initializer_list<ByteStringView> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
#endif

  int32 Split(Array<UStringRef>& out_tokens, UNICHAR separator,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<UStringRef>& out_tokens, const Array<UNICHAR>& separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<UStringRef>& out_tokens,
              std::initializer_list<UNICHAR> separators, int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<UStringRef>& out_tokens, UStringView separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<UStringRef>& out_tokens, AsciiString separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  int32 Split(Array<UStringRef>& out_tokens,
              const Array<const UNICHAR*>& separators, int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<UStringRef>& out_tokens, const Array<UString>& separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<UStringRef>& out_tokens,
              const Array<UStringRef>& separators, int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<UStringRef>& out_tokens,
              const Array<UStringView>& separators, int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<UStringRef>& out_tokens,
              const Array<AsciiString>& separators, int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  int32 Split(Array<UStringRef>& out_tokens,
              std::initializer_list<const UNICHAR*> separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<UStringRef>& out_tokens,
              std::initializer_list<UString> separators, int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<UStringRef>& out_tokens,
              std::initializer_list<UStringRef> separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<UStringRef>& out_tokens,
              std::initializer_list<UStringView> separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<UStringRef>& out_tokens,
              std::initializer_list<AsciiString> separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  int32 SplitByWhitespaces(
      Array<UStringRef>& out_tokens, UString extra_separator = UString(),
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitLines(
      Array<UStringRef>& out_lines,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

#if FUN_USE_IMPLICIT_STRING_CONVERSION
  int32 Split(Array<UStringRef>& out_tokens, ByteStringView separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  int32 Split(Array<UStringRef>& out_tokens,
              const Array<const char*>& separators, int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<UStringRef>& out_tokens,
              const Array<ByteString>& separators, int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<UStringRef>& out_tokens,
              const Array<ByteStringRef>& separators, int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<UStringRef>& out_tokens,
              const Array<ByteStringView>& separators, int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  int32 Split(Array<UStringRef>& out_tokens,
              std::initializer_list<const char*> separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<UStringRef>& out_tokens,
              std::initializer_list<ByteString> separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<UStringRef>& out_tokens,
              std::initializer_list<ByteStringRef> separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<UStringRef>& out_tokens,
              std::initializer_list<ByteStringView> separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
#endif

  bool Divide(UStringView delim, UStringRef* out_left, UStringRef* out_right,
              bool trimming = false,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  inline bool Divide(
      UNICHAR delim_char, UStringRef* out_left, UStringRef* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Divide(UStringView(&delim_char, 1), out_left, out_right, trimming,
                  casesense);
  }
  inline bool Divide(
      const UNICHAR* delim, int32 delim_len, UStringRef* out_left,
      UStringRef* out_right, bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Divide(UStringView(delim, delim_len), out_left, out_right, trimming,
                  casesense);
  }
  bool Divide(AsciiString delim, UStringRef* out_left, UStringRef* out_right,
              bool trimming = false,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline bool Divide(
      const char* delim, int32 delim_len, UStringRef* out_left,
      UStringRef* out_right, bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Divide(UTF8_AS_UNICHAR(delim, delim_len).ToView(), out_left,
                  out_right, trimming, casesense);
  }
  inline bool Divide(
      ByteStringView delim, UStringRef* out_left, UStringRef* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Divide(UTF8_AS_UNICHAR(delim).ToView(), out_left, out_right,
                  trimming, casesense);
  }
#endif

  bool LastDivide(
      UStringView delim, UStringRef* out_left, UStringRef* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  inline bool LastDivide(
      UNICHAR delim_char, UStringRef* out_left, UStringRef* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return LastDivide(UStringView(&delim_char, 1), out_left, out_right,
                      trimming, casesense);
  }
  inline bool LastDivide(
      const UNICHAR* delim, int32 delim_len, UStringRef* out_left,
      UStringRef* out_right, bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return LastDivide(UStringView(delim, delim_len), out_left, out_right,
                      trimming, casesense);
  }
  bool LastDivide(
      AsciiString delim, UStringRef* out_left, UStringRef* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline bool LastDivide(
      const char* delim, int32 delim_len, UStringRef* out_left,
      UStringRef* out_right, bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return LastDivide(UTF8_AS_UNICHAR(delim, delim_len).ToView(), out_left,
                      out_right, trimming, casesense);
  }
  inline bool LastDivide(
      ByteStringView delim, UStringRef* out_left, UStringRef* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return LastDivide(UTF8_AS_UNICHAR(delim).ToView(), out_left, out_right,
                      trimming, casesense);
  }
#endif

  // UStringRef and UStringRef
  int32 Compare(
      const UStringRef& str,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool Equals(const UStringRef& str,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool operator==(const UStringRef& str) const;
  bool operator!=(const UStringRef& str) const;
  bool operator<(const UStringRef& str) const;
  bool operator<=(const UStringRef& str) const;
  bool operator>(const UStringRef& str) const;
  bool operator>=(const UStringRef& str) const;

  // UStringRef and UString
  int32 Compare(const UString& str, CaseSensitivity casesense =
                                        CaseSensitivity::CaseSensitive) const;
  bool Equals(const UString& str,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool operator==(const UString& str) const;
  bool operator!=(const UString& str) const;
  bool operator<(const UString& str) const;
  bool operator<=(const UString& str) const;
  bool operator>(const UString& str) const;
  bool operator>=(const UString& str) const;

  // UStringRef and UStringView
  int32 Compare(
      const UStringView& str,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool Equals(const UStringView& str,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool operator==(const UStringView& str) const;
  bool operator!=(const UStringView& str) const;
  bool operator<(const UStringView& str) const;
  bool operator<=(const UStringView& str) const;
  bool operator>(const UStringView& str) const;
  bool operator>=(const UStringView& str) const;

  // UStringRef and AsciiString
  int32 Compare(
      const AsciiString& str,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool Equals(const AsciiString& str,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool operator==(const AsciiString& str) const;
  bool operator!=(const AsciiString& str) const;
  bool operator<(const AsciiString& str) const;
  bool operator<=(const AsciiString& str) const;
  bool operator>(const AsciiString& str) const;
  bool operator>=(const AsciiString& str) const;

  // UStringRef and ByteString
  int32 Compare(
      const ByteString& str,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool Equals(const ByteString& str,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool operator==(const ByteString& str) const;
  bool operator!=(const ByteString& str) const;
  bool operator<(const ByteString& str) const;
  bool operator<=(const ByteString& str) const;
  bool operator>(const ByteString& str) const;
  bool operator>=(const ByteString& str) const;

  // UStringRef and ByteStringRef
  int32 Compare(
      const ByteStringRef& str,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool Equals(const ByteStringRef& str,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool operator==(const ByteStringRef& str) const;
  bool operator!=(const ByteStringRef& str) const;
  bool operator<(const ByteStringRef& str) const;
  bool operator<=(const ByteStringRef& str) const;
  bool operator>(const ByteStringRef& str) const;
  bool operator>=(const ByteStringRef& str) const;

  // UStringRef and ByteStringView
  int32 Compare(
      const ByteStringView& str,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool Equals(const ByteStringView& str,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool operator==(const ByteStringView& str) const;
  bool operator!=(const ByteStringView& str) const;
  bool operator<(const ByteStringView& str) const;
  bool operator<=(const ByteStringView& str) const;
  bool operator>(const ByteStringView& str) const;
  bool operator>=(const ByteStringView& str) const;

  // UStringRef and char
  int32 Compare(char ch, CaseSensitivity casesense =
                             CaseSensitivity::CaseSensitive) const;
  bool Equals(char ch,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool operator==(char ch) const;
  bool operator!=(char ch) const;
  bool operator<(char ch) const;
  bool operator<=(char ch) const;
  bool operator>(char ch) const;
  bool operator>=(char ch) const;
  friend bool operator==(char ch, const UStringRef& str);
  friend bool operator!=(char ch, const UStringRef& str);
  friend bool operator<(char ch, const UStringRef& str);
  friend bool operator<=(char ch, const UStringRef& str);
  friend bool operator>(char ch, const UStringRef& str);
  friend bool operator>=(char ch, const UStringRef& str);

  // UStringRef and const char*
  int32 Compare(const char* str, CaseSensitivity casesense =
                                     CaseSensitivity::CaseSensitive) const;
  bool Equals(const char* str,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool operator==(const char* str) const;
  bool operator!=(const char* str) const;
  bool operator<(const char* str) const;
  bool operator<=(const char* str) const;
  bool operator>(const char* str) const;
  bool operator>=(const char* str) const;
  friend bool operator==(const char* str1, const UStringRef& str2);
  friend bool operator!=(const char* str1, const UStringRef& str2);
  friend bool operator<(const char* str1, const UStringRef& str2);
  friend bool operator<=(const char* str1, const UStringRef& str2);
  friend bool operator>(const char* str1, const UStringRef& str2);
  friend bool operator>=(const char* str1, const UStringRef& str2);

  // UStringRef and UNICHAR
  int32 Compare(UNICHAR ch, CaseSensitivity casesense =
                                CaseSensitivity::CaseSensitive) const;
  bool Equals(UNICHAR ch,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool operator==(UNICHAR ch) const;
  bool operator!=(UNICHAR ch) const;
  bool operator<(UNICHAR ch) const;
  bool operator<=(UNICHAR ch) const;
  bool operator>(UNICHAR ch) const;
  bool operator>=(UNICHAR ch) const;
  friend bool operator==(UNICHAR ch, const UStringRef& str);
  friend bool operator!=(UNICHAR ch, const UStringRef& str);
  friend bool operator<(UNICHAR ch, const UStringRef& str);
  friend bool operator<=(UNICHAR ch, const UStringRef& str);
  friend bool operator>(UNICHAR ch, const UStringRef& str);
  friend bool operator>=(UNICHAR ch, const UStringRef& str);

  // UStringRef and const UNICHAR*
  int32 Compare(const UNICHAR* str, CaseSensitivity casesense =
                                        CaseSensitivity::CaseSensitive) const;
  bool Equals(const UNICHAR* str,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool operator==(const UNICHAR* str) const;
  bool operator!=(const UNICHAR* str) const;
  bool operator<(const UNICHAR* str) const;
  bool operator<=(const UNICHAR* str) const;
  bool operator>(const UNICHAR* str) const;
  bool operator>=(const UNICHAR* str) const;
  friend bool operator==(const UNICHAR* str1, const UStringRef& str2);
  friend bool operator!=(const UNICHAR* str1, const UStringRef& str2);
  friend bool operator<(const UNICHAR* str1, const UStringRef& str2);
  friend bool operator<=(const UNICHAR* str1, const UStringRef& str2);
  friend bool operator>(const UNICHAR* str1, const UStringRef& str2);
  friend bool operator>=(const UNICHAR* str1, const UStringRef& str2);

  int16 ToInt16(bool* ok = nullptr, int32 base = 10) const;
  uint16 ToUInt16(bool* ok = nullptr, int32 base = 10) const;
  int32 ToInt32(bool* ok = nullptr, int32 base = 10) const;
  uint32 ToUInt32(bool* ok = nullptr, int32 base = 10) const;
  int64 ToInt64(bool* ok = nullptr, int32 base = 10) const;
  uint64 ToUInt64(bool* ok = nullptr, int32 base = 10) const;
  float ToFloat(bool* ok = nullptr) const;
  double ToDouble(bool* ok = nullptr) const;

  bool IsNumeric() const;
  bool IsIdentifier() const;

  bool IsQuoted() const;
  UStringRef Unquoted(bool* out_quotes_removed = nullptr) const;
  UStringRef& Unquotes(bool* out_quotes_removed = nullptr);

  bool IsAscii() const;

  // TODO encoding conversions

  // STL compatibilities

  typedef int32 size_type;
  typedef intptr_t difference_type;
  typedef const UNICHAR& const_reference;
  typedef UNICHAR& reference;
  typedef UNICHAR* pointer;
  typedef const UNICHAR* const_pointer;
  typedef UNICHAR value_type;

  typedef const UNICHAR* const_iterator;
  typedef const_iterator ConstIterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

  inline const_iterator begin() const { return ConstData(); }
  inline const_iterator cbegin() const { return ConstData(); }
  inline const_iterator end() const { return ConstData() + Len(); }
  inline const_iterator cend() const { return ConstData() + Len(); }
  inline const_reverse_iterator rbegin() const {
    return const_reverse_iterator(end());
  }
  inline const_reverse_iterator rend() const {
    return const_reverse_iterator(begin());
  }
  inline const_reverse_iterator crbegin() const {
    return const_reverse_iterator(end());
  }
  inline const_reverse_iterator crend() const {
    return const_reverse_iterator(begin());
  }

  // TODO 필요한가??
  // static UStringRef FromStdString(const std::string& StdStr);
  // std::string ToStdString() const;

 private:
  FUN_BASE_API friend uint32 HashOf(const UStringRef&);
  // FUN_BASE_API friend Archive& operator & (Archive& ar, UStringRef&);

 private:
  friend class UString;

  const UString* string_;
  int32 position_;
  int32 length_;

  void CheckInvariants();
};

int32 CullArray(Array<UString>* array, bool trimming = false);
int32 CullArray(Array<UStringRef>* array, bool trimming = false);

//중요: zero construct이면 안됨.
// template <> struct IsZeroConstructible<UString> { enum { value = true }; };

// TODO?
// Expose_TNameOf(UString)

///**
// * Convert an array of bytes to a UNICHAR
// *
// * \param in - byte array values to convert
// * \param count - number of bytes to convert
// *
// * \return Valid string representing bytes.
//*/
// inline UString BytesToString(const uint8* in, int32 count) {
//  UString result(count, NoInit);
//  UNICHAR* dst = result.MutableData();
//
//  while (count > 0) {
//    // Put the byte into an int16 and add 1 to it, this keeps anything from
//    being put into the string as a null terminator int16 value = *in; value +=
//    1;
//
//    *dst++ += UNICHAR(value);
//
//    ++in;
//    --count;
//  }
//  return result;
//}

/**
 * Convert UString of bytes into the byte array.
 *
 * \param str - The string of byte values
 * \param out_bytes - ptr to memory must be preallocated large enough
 * \param max_buffer_size - Max buffer size of the out_bytes array, to prevent
 * overflow
 *
 * \return The number of bytes copied
 */
// inline int32 StringToBytes(const UString& str, uint8* out_bytes, int32
// max_buffer_size) {
//  int32 byte_count = 0;
//  const UNICHAR* char_pos = *str;
//
//  while (*char_pos && byte_count < max_buffer_size) {
//    out_bytes[byte_count] = (int8)(*char_pos - 1);
//    ++char_pos;
//    ++byte_count;
//  }
//  return byte_count - 1;
//}
//
///** \return Char value of Nibble */
// inline UNICHAR NibbleToChar(uint8 value) {
//  if (value > 9) {
//    return 'A' + UNICHAR(value - 10);
//  }
//  return '0' + UNICHAR(value);
//}
//
///**
// * Convert a byte to hex
// *
// * \param In - byte value to convert
// * \param result - out hex value output
// */
// inline void ByteToHex(uint8 in, UString& result) {
//  result += NibbleToChar(in >> 4);
//  result += NibbleToChar(in & 15);
//}
//
///**
// * Convert an array of bytes to hex
// *
// * \param in - byte array values to convert
// * \param count - number of bytes to convert
// *
// * \return Hex value in UString.
// */
// inline UString BytesToHex(const uint8* in, int32 count) {
//  UString result;
//  result.Reserve(count * 2);
//  while (count--) {
//    ByteToHex(*in++, result);
//  }
//  return result;
//}
//
///**
// * Checks if the CharTraits is a valid hex ch
// *
// * \param ch - The ch
// *
// * \return True if in 0-9 and A-f ranges
// */
// inline const bool CheckTCharIsHex(const UNICHAR ch) {
//  return (ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'f') || (ch >= 'a' &&
//  ch <= 'f');
//}
//
// inline const int32 CharToNibble_NonChecked(const UNICHAR ch, int32 def = -1)
// {
//  if (ch >= '0' && ch <= '9') {
//    return ch - '0';
//  } else if (ch >= 'A' && ch <= 'f') {
//    return (ch - 'A') + 10;
//  } else if (ch >= 'a' && ch <= 'f') {
//    return (ch - 'a') + 10;
//  }
//  return def;
//}
//
///**
// * Convert a CharTraits to equivalent hex value as a int32
// *
// * \param ch - The ch
// *
// * \return The int32 value of a hex ch
// */
// inline const int32 CharToNibble(const UNICHAR ch) {
//  fun_check(CheckTCharIsHex(ch));
//  if (ch >= '0' && ch <= '9') {
//    return ch - '0';
//  } else if (ch >= 'A' && ch <= 'f') {
//    return (ch - 'A') + 10;
//  }
//  return (ch - 'a') + 10;
//}
//
///**
// * Convert UString of Hex digits into the byte array.
// *
// * \param hex_string - The string of Hex values
// * \param out_bytes - ptr to memory must be preallocated large enough
// *
// * \return The number of bytes copied
// */
// inline int32 HexToBytes(const UString& hex_string, uint8* out_bytes) {
//  int32 byte_count = 0;
//  const bool pad_nibble = (hex_string.Len() % 2) == 1;
//  const UNICHAR* char_pos = *hex_string;
//  if (pad_nibble) {
//    out_bytes[byte_count++] = CharToNibble(*char_pos++);
//  }
//
//  while (*char_pos) {
//    out_bytes[byte_count] = CharToNibble(*char_pos++) << 4;
//    out_bytes[byte_count] += CharToNibble(*char_pos++);
//    ++byte_count;
//  }
//
//  return byte_count;
//}
//
// TODO
// ToString을 별도로 잘 정리해서 적용하는게 바람직해보임!
///**
// * Namespace that houses lexical conversion for various types.
// * User defined conversions can be implemented externally
// */
// namespace lex {
//
//  /**
//   * Expected functions in this namespace are as follows:
//   *     static bool     TryParseString(T& out_value, const UNICHAR* buffer);
//   *     static void     FromString(T& out_value, const UNICHAR* buffer);
//   *     static UString  ToString(const T& out_value);
//   *
//   * Implement custom functionality externally.
//   */
//
//  /** Covert a string buffer to intrinsic types */
//  inline void FromString(int8& out_value, const UNICHAR* buffer) { out_value =
//  CStringTraitsU::Atoi(buffer); } inline void FromString(int16& out_value,
//  const UNICHAR* buffer) { out_value = CStringTraitsU::Atoi(buffer); } inline
//  void FromString(int32& out_value, const UNICHAR* buffer) { out_value =
//  CStringTraitsU::Atoi(buffer); } inline void FromString(int64& out_value,
//  const UNICHAR* buffer) { out_value = CStringTraitsU::Atoi64(buffer); }
//  inline void FromString(uint8& out_value, const UNICHAR* buffer) { out_value
//  = CStringTraitsU::Atoi(buffer); } inline void FromString(uint16& out_value,
//  const UNICHAR* buffer) { out_value = CStringTraitsU::Atoi(buffer); } inline
//  void FromString(uint32& out_value, const UNICHAR* buffer) { out_value =
//  CStringTraitsU::Atoi64(buffer); }   //64 because this unsigned and so Atoi
//  might overflow inline void FromString(uint64& out_value, const UNICHAR*
//  buffer) { out_value = CStringTraitsU::Strtoui64(buffer, buffer +
//  CStringTraitsU::Strlen(buffer), nullptr, 0); } inline void FromString(float&
//  out_value, const UNICHAR* buffer) { out_value =
//  CStringTraitsU::Atof(buffer); } inline void FromString(double& out_value,
//  const UNICHAR* buffer) { out_value = CStringTraitsU::Atod(buffer); } inline
//  void FromString(bool& out_value, const UNICHAR* buffer) { out_value =
//  CStringTraitsU::ToBool(buffer); }
//
//  /** Convert numeric types to a string */
//  template <typename T>
//  typename EnableIf<IsArithmetic<T>::value, UString>::Type
//  ToString(const T& value) {
//    return UString::Format(TFormatSpecifier<T>::GetFormatSpecifier(), value);
//  }
//
//  //CHECKME UString만 별도로 처리해주는게 좋을까?
//  inline UString ToString(const UString& value) {
//    return value;
//  }
//
//  /**
//   * Helper template to convert to sanitized strings
//   */
//  template <typename T>
//  UString ToSanitizedString(const T& value) {
//    return ToString(value);
//  }
//
//  /**
//   * Specialized for floats
//   */
//  template <>
//  inline UString ToSanitizedString<float>(const float& value) {
//    //return UString::SanitizeFloat(value);
//    return UString::FromNumber(value);
//  }
//
//  /** Parse a UString into this type, returning whether it was successful */
//  /** Specialization for arithmetic types */
//  template <typename T>
//  static typename EnableIf<IsArithmetic<T>::Value, bool>::Type
//    TryParseString(T& out_value, const UNICHAR* buffer) {
//    if (CharTraitsU::IsNumeric(buffer)) {
//      FromString(out_value, buffer);
//      return true;
//    }
//    return false;
//  }
//
//  /** Try and parse a bool - always returns true */
//  static bool TryParseString(bool& out_value, const UNICHAR* buffer) {
//    FromString(out_value, buffer);
//    return true;
//  }
//}
//
//
///** Shorthand legacy use for Lex functions */
// template <typename T>
// struct TypeToString {
//  static UString ToString(const T& value) { return Lex::ToString(value); }
//  static UString ToSanitizedString(const T& value) { return
//  Lex::ToSanitizedString(value); }
//};
//
// template <typename T>
// struct TypeFromString {
//  static void FromString(T& value, const UNICHAR* buffer) {
//  Lex::FromString(value, buffer); }
//};
//

//
// Special printers.
//

// TODO?
///**
// * str printer.
// */
// class UStringPrinter : public UString, public Printer {
// public:
//  UStringPrinter(const UNICHAR* printer_name = "")
//    : UString(printer_name) {
//    auto_emit_line_terminator = false;
//  }
//
//  void Serialize(const UNICHAR* data, ELogVerbosity::Type verbosity, const
//  CName& category) override {
//    UString::operator += ((const UNICHAR*)data);
//
//    if (auto_emit_line_terminator) {
//      *this += UString(FUN_LINE_TERMINATOR);
//    }
//  }
//
//  UStringPrinter(UStringPrinter&&) = default;
//  UStringPrinter(const UStringPrinter&) = default;
//  UStringPrinter& operator = (UStringPrinter&&) = default;
//  UStringPrinter& operator = (const UStringPrinter&) = default;
//
//  // Make += operator virtual.
//  virtual UString& operator += (const UString& other) {
//    return UString::operator += (other);
//  }
//};

// TODO?
///**
// * str printer.
// */
// class UStringPrinterCountLines : public UStringPrinter {
//  typedef UStringPrinter Super;
//
//  int32 line_count_;
//
// public:
//  UStringPrinterCountLines(const UNICHAR* printer_name = "")
//    : Super(printer_name)
//    , line_count_(0) {}
//
//  void Serialize(const UNICHAR* data, ELogVerbosity::Type verbosity, const
//  CName& category) override {
//    Super::Serialize(data, verbosity, category);
//
//    const int32 line_terminator_len =
//    CStringTraitsU::Strlen(FUN_LINE_TERMINATOR); for (;;) {
//      data = CStringTraitsU::Strstr(data, FUN_LINE_TERMINATOR);
//      if (data == nullptr) {
//        break;
//      }
//      line_count_++;
//      data += line_terminator_len;
//    }
//
//    if (auto_emit_line_terminator) {
//      line_count_++;
//    }
//  }
//
//  virtual UStringPrinterCountLines& operator += (const
//  UStringPrinterCountLines& other) {
//    UString::operator += ((const UString&)other);
//
//    line_count_ += other.GetLineCount();
//
//    return *this;
//  }
//
//  virtual UString& operator += (const UString& other) override {
//    Print(*other);
//    return *this;
//  }
//
//  int32 GetLineCount() const {
//    return line_count_;
//  }
//
//  UStringPrinterCountLines(const UStringPrinterCountLines&) = default;
//  UStringPrinterCountLines& operator = (const UStringPrinterCountLines&) =
//  default;
//
//  inline UStringPrinterCountLines(UStringPrinterCountLines&& other)
//    : Super((Super&&)other), line_count_(other.line_count_) {
//    other.line_count_ = 0;
//  }
//
//  inline UStringPrinterCountLines& operator = (UStringPrinterCountLines&&
//  other) {
//    if (FUN_LIKELY(&other != this)) {
//      (Super&)*this = (Super&&)other;
//      line_count_ = other.line_count_;
//
//      other.line_count_ = 0;
//    }
//    return *this;
//  }
//};

#include "fun/base/string/string_inline.h"

// TODO
// ToString을 잘 정리해서 적용하는게 바람직해보임!
// namespace Lex {
//
//  inline UString ToString(const ByteString& value) { return UString(value); }
//  //inline UString ToString(const UString& value) { return value; }
//
//  template <typename T, typename ArrayAllocator>
//  inline UString ToString(const Array<T, ArrayAllocator>& value) {
//    UString result;
//    result += AsciiString("[");
//    for (int32 i = 0; i < value.Count(); ++i) {
//      if (i != 0) {
//        result += AsciiString(",");
//      }
//      result += ToString(value[i]);
//    }
//    result += AsciiString("]");
//    return result;
//  }
//
//  template <typename KeyType, typename ValueType, typename SetAllocator,
//  typename KeyFuncs> inline UString ToString(const
//  Map<KeyType,ValueType,SetAllocator,KeyFuncs>& value) {
//    UString result;
//    result += AsciiString("{");
//    bool first = true;
//    for (const auto& pair : value) {
//      if (!first) {
//        result += AsciiString(",");
//      }
//      first = false;
//
//      result += AsciiString("{");
//      result += ToString(pair.Key);
//      result += AsciiString(",");
//      result += ToString(pair.value);
//      result += AsciiString("}");
//    }
//    result += AsciiString("}");
//    return result;
//  }
//
//  template <typename KeyType, typename ValueType, typename SetAllocator,
//  typename KeyFuncs> inline UString ToString(const
//  MultiMap<KeyType,ValueType,SetAllocator,KeyFuncs>& value) {
//    UString result;
//    result += AsciiString("{");
//    bool first = true;
//    for (const auto& pair : value) {
//      if (!first) {
//        result += AsciiString(",");
//      }
//      first = false;
//
//      result += AsciiString("{");
//      result += ToString(pair.Key);
//      result += AsciiString(",");
//      result += ToString(pair.value);
//      result += AsciiString("}");
//    }
//    result += AsciiString("}");
//    return result;
//  }
//
//  //TODO
//  //template <typename T, typename KeyFuncs, typename Allocator>
//  //inline UString ToString(const Set<T,KeyFuncs,Allocator>& value)
//  //{
//  //  UString result;
//  //  result += AsciiString("[");
//  //  for (int32 i = 0; i < value.Count(); ++i) {
//  //    if (i != 0) result += AsciiString(",");
//  //    result += ToString(value[i]);
//  //  }
//  //  result += AsciiString("]");
//  //  return result;
//  //}
//}

FUN_BASE_API uint32 HashOf(const UString&);
FUN_BASE_API uint32 HashOf(const UStringRef&);

}  // namespace fun
