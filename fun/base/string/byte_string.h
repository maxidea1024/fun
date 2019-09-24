#pragma once

#include <initializer_list>  // std::initializer_list
#include <iterator>          // std::iterator
#include <string>            // std::string

#include "fun/base/base.h"
#include "fun/base/flags.h"
#include "fun/base/string/ascii_string.h"
#include "fun/base/string/byte_string_view.h"
#include "fun/base/string/sharable_array_data.h"
#include "fun/base/string/string_conversion.h"
#include "fun/base/string/string_forward_decls.h"
#include "fun/base/string/uni_string_view.h"  // 먼저 인클루드를 해주어야, 문법오류(생성자등에서 모호하다는...)가 안생김.

namespace fun {

typedef UntypedSharableArrayData ByteStringData;

template <size_t N>
struct StaticByteStringData {
  ByteStringData header;
  char data[N + 1];

  ByteStringData* DataPtr() const {
    fun_check(header.ref.IsPersistent());
    return const_cast<ByteStringData*>(&header);
  }
};

struct ByteStringDataPtr {
  ByteStringData* ptr;
};

#define STATIC_BYTE_STRING_DATA_HEADER_INITIALIZER_WITH_OFFSET(size, offset) \
  STATIC_SHARABLE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(size, offset)

#define STATIC_BYTE_STRING_DATA_HEADER_INITIALIZER(size)  \
  STATIC_BYTE_STRING_DATA_HEADER_INITIALIZER_WITH_OFFSET( \
      size, sizeof(ByteStringData))

#define ByteStringLiteral(str)                                  \
  ([]() -> ByteString {                                         \
    enum { size = sizeof(str) - 1 };                            \
    static const StaticByteStringData<size> literal = {         \
        STATIC_BYTE_STRING_DATA_HEADER_INITIALIZER(size), str}; \
    ByteStringDataPtr holder = {literal.DataPtr()};             \
    const ByteString result(holder);                            \
    return result;                                              \
  }())

class FUN_BASE_API ByteString {
 public:
  typedef char CharType;
  typedef TypedSharableArrayData<char> Data;

  class FUN_BASE_API CharRef {
   public:
    inline operator char() const {
      return index_ < array_ref_.Len() ? array_ref_.At(index_) : char(0);
    }
    inline CharRef& operator=(char ch) {
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
    inline bool operator==(char ch) const {
      return array_ref_.At(index_) == ch;
    }
    inline bool operator!=(char ch) const {
      return array_ref_.At(index_) != ch;
    }
    inline bool operator>(char ch) const { return array_ref_.At(index_) > ch; }
    inline bool operator>=(char ch) const {
      return array_ref_.At(index_) >= ch;
    }
    inline bool operator<(char ch) const { return array_ref_.At(index_) < ch; }
    inline bool operator<=(char ch) const {
      return array_ref_.At(index_) <= ch;
    }

   private:
    ByteString& array_ref_;
    int32 index_;

    inline CharRef(ByteString& array_ref, int32 index)
        : array_ref_(array_ref), index_(index) {}

    friend class ByteString;
  };

  enum Base64Option {
    Base64Encoding = 0,
    Base64UrlEncoding = 1,

    KeepTrailingEquals = 0,
    OmitTrailingEquals = 2
  };
  FUN_DECLARE_FLAGS_IN_CLASS(Base64Options, Base64Option)

  ByteString();
  ByteString(ByteStringDataPtr data_ptr);
  ByteString(const char* str);
  ByteString(const char* str, int32 len);
  ByteString(const char* begin, const char* end);
  ByteString(char ch);
  ByteString(int32 count, char ch);
  ByteString(int32 len, NoInit_TAG);
  ByteString(int32 len, ReservationInit_TAG);
  ByteString(const ByteString& rhs);
  ByteString(ByteStringView str);
  ByteString& operator=(char ch);
  ByteString& operator=(const char* str);
  ByteString& operator=(ByteStringView str);
  ByteString& operator=(const ByteString& rhs);
  ByteString(ByteString&& rhs);
  ByteString& operator=(ByteString&& rhs);
  ~ByteString();

  ByteString(AsciiString str);
  ByteString& operator=(AsciiString str);
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  ByteString(const UNICHAR* str, int32 len);
  ByteString(UNICHAR ch);
  ByteString(int32 count, UNICHAR ch);
  ByteString(UStringView str);
  ByteString& operator=(UNICHAR ch);
  ByteString& operator=(UStringView str);
#endif

  void Swap(ByteString& rhs);

  int32 Len() const;
  int32 NulTermLen() const;
  int32 Capacity() const;
  bool IsEmpty() const;
  void Clear();
  void Clear(int32 initial_capacity);
  void ResizeUninitialized(int32 after_len);
  void ResizeZeroed(int32 after_len);
  void Resize(int32 after_len, char filler);
  void Reserve(int32 len);
  void Shrink();

  ByteString& Fill(char filler, int32 len = -1);

  const char* operator*() const;

  char* MutableData();
  char* MutableData(int32 len);
  const char* ConstData() const;

  // TODO NUL term을 보장해야함.
  const char* c_str() const;

  bool IsNulTerm() const;
  ByteString& TrimToNulTerminator();
  ByteString ToNulTerminated() const;

  void Detach();
  bool IsDetached() const;
  bool IsSharedWith(const ByteString& rhs) const;
  bool IsRawData() const;

  char At(int32 index) const;
  char operator[](int32 index) const;
  CharRef operator[](int32 index);

  char First() const;
  CharRef First();
  char Last() const;
  CharRef Last();

  char FirstOr(const char def = char('\0')) const;
  char LastOr(const char def = char('\0')) const;

  int32 IndexOfAny(ByteStringView chars,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;
  int32 IndexOfAny(AsciiString chars,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;
  int32 IndexOfAny(const Array<char>& chars,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;
  inline int32 IndexOfAny(
      std::initializer_list<char> chars,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_index = nullptr,
      int32* matched_len = nullptr) const {
    return IndexOfAny(ByteStringView(chars.begin(), chars.size()), casesense,
                      from, matched_index, matched_len);
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline int32 IndexOfAny(
      UStringView chars,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_index = nullptr,
      int32* matched_len = nullptr) const {
    return IndexOfAny(UNICHAR_TO_UTF8_BUFFER(chars).ToView(), casesense, from,
                      matched_index, matched_len);
  }
#endif

  int32 LastIndexOfAny(
      ByteStringView chars,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = -1, int32* matched_index = nullptr,
      int32* matched_len = nullptr) const;
  int32 LastIndexOfAny(
      AsciiString chars,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = -1, int32* matched_index = nullptr,
      int32* matched_len = nullptr) const;
  int32 LastIndexOfAny(
      const Array<char>& chars,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = -1, int32* matched_index = nullptr,
      int32* matched_len = nullptr) const;
  inline int32 LastIndexOfAny(
      std::initializer_list<char> chars,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = -1, int32* matched_index = nullptr,
      int32* matched_len = nullptr) const {
    return LastIndexOfAny(ByteStringView(chars.begin(), chars.size()),
                          casesense, from, matched_index, matched_len);
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  // TODO Array<UNICHAR>, std::initializer_list<UNICHAR>
  inline int32 LastIndexOfAny(
      UStringView chars,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = -1, int32* matched_index = nullptr,
      int32* matched_len = nullptr) const {
    return LastIndexOfAny(UNICHAR_TO_UTF8_BUFFER(chars).ToView(), casesense,
                          from, matched_index, matched_len);
  }
#endif

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
  int32 IndexOfAny(const Array<AsciiString>& strings,
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
  int32 IndexOfAny(std::initializer_list<AsciiString> strings,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;
#if FUN_USE_IMPLICIT_STRING_CONVERSION
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
#endif

  int32 IndexOf(ByteStringView sub,
                CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                int32 from = 0, int32* matched_len = nullptr) const;

  inline int32 IndexOf(
      char ch, CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_len = nullptr) const {
    return IndexOf(ByteStringView(&ch, 1), casesense, from, matched_len);
  }
  inline int32 IndexOf(
      const char* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_len = nullptr) const {
    return IndexOf(ByteStringView(sub, sub_len), casesense, from, matched_len);
  }
  inline int32 IndexOf(
      AsciiString sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_len = nullptr) const {
    return IndexOf(ByteStringView(sub.ConstData(), sub.Len()), casesense, from,
                   matched_len);
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline int32 IndexOf(
      UNICHAR ch, CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_len = nullptr) const {
    return IndexOf(UNICHAR_TO_UTF8_BUFFER(&ch, 1).ToView(), casesense, from,
                   matched_len);
  }
  inline int32 IndexOf(
      const UNICHAR* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_len = nullptr) const {
    return IndexOf(UNICHAR_TO_UTF8_BUFFER(sub, sub_len).ToView(), casesense,
                   from, matched_len);
  }
  inline int32 IndexOf(
      UStringView sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_len = nullptr) const {
    return IndexOf(UNICHAR_TO_UTF8_BUFFER(sub).ToView(), casesense, from,
                   matched_len);
  }
#endif

  template <typename Predicate>
  int32 IndexOfIf(const Predicate& pred, int32 from = 0) const;

  int32 LastIndexOf(ByteStringView sub,
                    CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                    int32 from = -1, int32* matched_len = nullptr) const;

  inline int32 LastIndexOf(
      char ch, CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = -1, int32* matched_len = nullptr) const {
    return LastIndexOf(ByteStringView(&ch, 1), casesense, from, matched_len);
  }
  inline int32 LastIndexOf(
      const char* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = -1, int32* matched_len = nullptr) const {
    return LastIndexOf(ByteStringView(sub, sub_len), casesense, from,
                       matched_len);
  }
  inline int32 LastIndexOf(
      AsciiString sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = -1, int32* matched_len = nullptr) const {
    return LastIndexOf(ByteStringView(sub.ConstData(), sub.Len()), casesense,
                       from, matched_len);
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline int32 LastIndexOf(
      UNICHAR ch, CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = -1, int32* matched_len = nullptr) const {
    return LastIndexOf(UNICHAR_TO_UTF8_BUFFER(&ch, 1).ToView(), casesense, from,
                       matched_len);
  }
  inline int32 LastIndexOf(
      const UNICHAR* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = -1, int32* matched_len = nullptr) const {
    return LastIndexOf(UNICHAR_TO_UTF8_BUFFER(sub, sub_len).ToView(), casesense,
                       from, matched_len);
  }
  inline int32 LastIndexOf(
      UStringView sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = -1, int32* matched_len = nullptr) const {
    return LastIndexOf(UNICHAR_TO_UTF8_BUFFER(sub).ToView(), casesense, from,
                       matched_len);
  }
#endif

  template <typename Predicate>
  int32 LastIndexOfIf(const Predicate& pred, int32 from = -1) const;

  inline bool Contains(
      char ch, CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_len = nullptr) const {
    return IndexOf(ch, casesense, from, matched_len) != INVALID_INDEX;
  }
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
  inline bool Contains(
      AsciiString sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_len = nullptr) const {
    return IndexOf(sub, casesense, from, matched_len) != INVALID_INDEX;
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
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
#endif

  int32 Count(ByteStringView sub,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  inline int32 Count(char ch, CaseSensitivity casesense =
                                  CaseSensitivity::CaseSensitive) const {
    return Count(ByteStringView(&ch, 1), casesense);
  }
  inline int32 Count(
      const char* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Count(ByteStringView(sub, sub_len), casesense);
  }
  inline int32 Count(
      AsciiString sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Count(ByteStringView(sub.ConstData(), sub.Len()), casesense);
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline int32 Count(UNICHAR ch, CaseSensitivity casesense =
                                     CaseSensitivity::CaseSensitive) const {
    return Count(UNICHAR_TO_UTF8_BUFFER(&ch, 1).ToView(), casesense);
  }
  inline int32 Count(
      const UNICHAR* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Count(UNICHAR_TO_UTF8_BUFFER(sub, sub_len).ToView(), casesense);
  }
  inline int32 Count(
      UStringView sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Count(UNICHAR_TO_UTF8_BUFFER(sub).ToView(), casesense);
  }
#endif

  inline ByteString operator()(int32 offset, int32 len) const {
    return Mid(offset, len);
  }

  ByteString Left(int32 len) const;
  ByteString Mid(int32 offset, int32 len = int32_MAX) const;
  ByteString Right(int32 len) const;
  ByteString LeftChopped(int32 len) const;
  ByteString RightChopped(int32 len) const;

  ByteStringRef LeftRef(int32 len) const;
  ByteStringRef MidRef(int32 offset, int32 len = int32_MAX) const;
  ByteStringRef RightRef(int32 len) const;
  ByteStringRef LeftChoppedRef(int32 len) const;
  ByteStringRef RightChoppedRef(int32 len) const;

  bool StartsWith(ByteStringView sub, CaseSensitivity casesense =
                                          CaseSensitivity::CaseSensitive) const;

  inline bool StartsWith(char ch, CaseSensitivity casesense =
                                      CaseSensitivity::CaseSensitive) const {
    return StartsWith(ByteStringView(&ch, 1), casesense);
  }
  inline bool StartsWith(
      const char* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return StartsWith(ByteStringView(sub, sub_len), casesense);
  }
  inline bool StartsWith(
      AsciiString sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return StartsWith(ByteStringView(sub.ConstData(), sub.Len()), casesense);
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline bool StartsWith(UNICHAR ch, CaseSensitivity casesense =
                                         CaseSensitivity::CaseSensitive) const {
    return StartsWith(UNICHAR_TO_UTF8_BUFFER(&ch, 1).ToView(), casesense);
  }
  inline bool StartsWith(
      const UNICHAR* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return StartsWith(UNICHAR_TO_UTF8_BUFFER(sub, sub_len).ToView(), casesense);
  }
  inline bool StartsWith(
      UStringView sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return StartsWith(UNICHAR_TO_UTF8_BUFFER(sub).ToView(), casesense);
  }
#endif

  bool EndsWith(ByteStringView sub, CaseSensitivity casesense =
                                        CaseSensitivity::CaseSensitive) const;

  inline bool EndsWith(char ch, CaseSensitivity casesense =
                                    CaseSensitivity::CaseSensitive) const {
    return EndsWith(ByteStringView(&ch, 1), casesense);
  }
  inline bool EndsWith(
      const char* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return EndsWith(ByteStringView(sub, sub_len), casesense);
  }
  inline bool EndsWith(
      AsciiString sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return EndsWith(ByteStringView(sub.ConstData(), sub.Len()), casesense);
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline bool EndsWith(UNICHAR ch, CaseSensitivity casesense =
                                       CaseSensitivity::CaseSensitive) const {
    return EndsWith(UNICHAR_TO_UTF8_BUFFER(&ch, 1).ToView(), casesense);
  }
  inline bool EndsWith(
      const UNICHAR* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return EndsWith(UNICHAR_TO_UTF8_BUFFER(sub, sub_len).ToView(), casesense);
  }
  inline bool EndsWith(
      UStringView sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return EndsWith(UNICHAR_TO_UTF8_BUFFER(sub).ToView(), casesense);
  }
#endif

  bool GlobMatch(
      ByteStringView pattern,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  inline bool GlobMatch(
      const char* pattern, int32 pattern_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return GlobMatch(ByteStringView(pattern, pattern_len), casesense);
  }
  inline bool GlobMatch(
      AsciiString pattern,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return GlobMatch(ByteStringView(pattern.ConstData(), pattern.Len()),
                     casesense);
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline bool GlobMatch(
      const UNICHAR* pattern, int32 pattern_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return GlobMatch(UNICHAR_TO_UTF8_BUFFER(pattern, pattern_len).ToView(),
                     casesense);
  }
  inline bool GlobMatch(
      UStringView pattern,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return GlobMatch(UNICHAR_TO_UTF8_BUFFER(pattern).ToView(), casesense);
  }
#endif

  ByteString& Truncate(int32 pos);
  ByteString& LeftChop(int32 len);
  ByteString& RightChop(int32 len);

  ByteString& MakeLower();
  ByteString& MakeUpper();
  ByteString ToLower() const;
  ByteString ToUpper() const;

  ByteString& TrimLeft();
  ByteString& TrimRight();
  ByteString& Trim();
  ByteString TrimmedLeft() const;
  ByteString TrimmedRight() const;
  ByteString Trimmed() const;
  int32 LeftSpaces() const;
  int32 RightSpaces() const;
  int32 SideSpaces() const;

  ByteStringRef TrimmedLeftRef() const;
  ByteStringRef TrimmedRightRef() const;
  ByteStringRef TrimmedRef() const;

  ByteString& Simplify();
  ByteString Simplified() const;

  ByteString Repeated(int32 times) const;  // restricted to max 2GB
  ByteString operator*(int32 times) const;

  ByteString LeftJustified(int32 width, char filler = ' ',
                           bool truncate = false) const;
  ByteString RightJustified(int32 width, char filler = ' ',
                            bool truncate = false) const;

  ByteString& Reverse();
  ByteString Reversed() const;

  ByteString& PrependUnitialized(int32 len);
  ByteString& AppendUninitialized(int32 len);
  ByteString& InsertUninitialized(int32 pos, int32 len);
  ByteString& PrependZeroed(int32 len);
  ByteString& AppendZeroed(int32 len);
  ByteString& InsertZeroed(int32 pos, int32 len);

  ByteString& Prepend(char ch);
  ByteString& Prepend(int32 count, char ch);
  inline ByteString& Prepend(const char* str, int32 len) {
    return Prepend(ByteStringView(str, len));
  }
  ByteString& Prepend(ByteStringView str);
  inline ByteString& Prepend(AsciiString str) {
    return Prepend(ByteStringView(str.ConstData(), str.Len()));
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline ByteString& Prepend(UNICHAR ch) {
    return Prepend(UNICHAR_TO_UTF8_BUFFER(&ch, 1).ToView());
  }
  ByteString& Prepend(int32 count, UNICHAR ch);
  inline ByteString& Prepend(const UNICHAR* str, int32 len) {
    return Prepend(UNICHAR_TO_UTF8_BUFFER(str, len).ToView());
  }
  inline ByteString& Prepend(UStringView str) {
    return Prepend(UNICHAR_TO_UTF8_BUFFER(str).ToView());
  }
#endif

  ByteString& Append(ByteStringView str);

  inline ByteString& Append(char ch) { return Append(ByteStringView(&ch, 1)); }
  ByteString& Append(int32 count, char ch);
  inline ByteString& Append(const char* str, int32 len) {
    return Append(ByteStringView(str, len));
  }
  inline ByteString& Append(AsciiString str) {
    return Append(ByteStringView(str.ConstData(), str.Len()));
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline ByteString& Append(UNICHAR ch) {
    return Append(UNICHAR_TO_UTF8_BUFFER(&ch, 1).ToView());
  }
  // TODO TCHAR이 uint16이어야하고, wchar_t는 별개로 처리해야함.
  // inline ByteString& Append(wchar_t ch)
  //{ return Append(UNICHAR_TO_UTF8_BUFFER(&ch, 1).ToView()); }
  ByteString& Append(int32 count, UNICHAR ch);
  inline ByteString& Append(const UNICHAR* str, int32 len) {
    return Append(UNICHAR_TO_UTF8_BUFFER(str, len).ToView());
  }
  inline ByteString& Append(UStringView str) {
    return Append(UNICHAR_TO_UTF8_BUFFER(str).ToView());
  }
#endif

  template <typename ElementType>
  inline ByteString& Append(std::initializer_list<ElementType> list) {
    for (auto& element : list) {
      Append(element);
    }
    return *this;
  }

  template <typename ElementType, typename Allocator>
  inline ByteString& Append(const Array<ElementType, Allocator>& list) {
    for (auto& element : list) {
      Append(element);
    }
    return *this;
  }

  // ByteStringView로 const char* / ByteString 을 구분하지 않고 처리하면,
  // 컴파일러가 제대로 해석을 하지 못하는 문제가 있어서, 각각의 타입을 별개로
  //처리해주어야함.

  inline ByteString& operator+=(char ch) { return Append(ch); }
  inline ByteString& operator+=(const char* str) { return Append(str); }
  inline ByteString& operator+=(const ByteString& str) { return Append(str); }
  inline ByteString& operator+=(const ByteStringRef& str) {
    return Append(str);
  }
  inline ByteString& operator+=(ByteStringView str) { return Append(str); }
  inline ByteString& operator+=(AsciiString str) { return Append(str); }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline ByteString& operator+=(UNICHAR unicode_char) {
    return Append(unicode_char);
  }
  // TODO
  // inline ByteString& operator += (wchar_t unicode_char) { return
  // Append(unicode_char); }
  inline ByteString& operator+=(const UNICHAR* str) { return Append(str); }
  inline ByteString& operator+=(const UString& str) { return Append(str); }
  inline ByteString& operator+=(const UStringRef& str) { return Append(str); }
  inline ByteString& operator+=(UStringView str) { return Append(str); }
#endif

  template <typename ElementType>
  inline ByteString& operator+=(std::initializer_list<ElementType> list) {
    return Append(list);
  }
  template <typename ElementType>
  inline ByteString& operator+=(const Array<ElementType>& list) {
    return Append(list);
  }  // Allocator를 지정할 수 있어야할까??

  inline ByteString& operator<<(char ch) { return Append(ch); }
  inline ByteString& operator<<(const char* str) { return Append(str); }
  inline ByteString& operator<<(const ByteString& str) { return Append(str); }
  inline ByteString& operator<<(const ByteStringRef& str) {
    return Append(str);
  }
  inline ByteString& operator<<(ByteStringView str) { return Append(str); }
  inline ByteString& operator<<(AsciiString str) { return Append(str); }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline ByteString& operator<<(UNICHAR unicode_char) {
    return Append(unicode_char);
  }
  inline ByteString& operator<<(const UNICHAR* str) { return Append(str); }
  inline ByteString& operator<<(const UString& str) { return Append(str); }
  inline ByteString& operator<<(const UStringRef& str) { return Append(str); }
  inline ByteString& operator<<(UStringView str) { return Append(str); }
#endif

  template <typename ElementType>
  inline ByteString& operator<<(std::initializer_list<ElementType> list) {
    return Append(list);
  }
  template <typename ElementType>
  inline ByteString& operator<<(const Array<ElementType>& list) {
    return Append(list);
  }  // Allocator를 지정할 수 있어야할까??

  // ByteStringView로 const char* / ByteString 을 구분하지 않고 처리하면,
  // 컴파일러가 제대로 해석을 하지 못하는 문제가 있어서, 각각의 타입을 별개로
  //처리해주어야함.

  inline ByteString operator+(const ByteString& str) const {
    return ByteString(*this) += str;
  }

  inline ByteString operator+(char ch) const { return ByteString(*this) += ch; }
  inline friend ByteString operator+(char ch, const ByteString& str) {
    return ByteString(ch) += str;
  }

  inline ByteString operator+(const char* str) const {
    return ByteString(*this) += str;
  }
  inline friend ByteString operator+(const char* str1, const ByteString& str2) {
    return ByteString(str1) += str2;
  }

  inline ByteString operator+(const ByteStringRef& str) const {
    return ByteString(*this) += str;
  }
  inline friend ByteString operator+(ByteStringRef& str1,
                                     const ByteString& str2) {
    return ByteString(str1) += str2;
  }

  inline ByteString operator+(ByteStringView str) const {
    return ByteString(*this) += str;
  }
  inline friend ByteString operator+(ByteStringView str1,
                                     const ByteString& str2) {
    return ByteString(str1) += str2;
  }

  inline ByteString operator+(AsciiString str) const {
    return ByteString(*this) += str;
  }
  inline friend ByteString operator+(AsciiString str1, const ByteString& str2) {
    return ByteString(str1) += str2;
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline ByteString operator+(UNICHAR ch) const {
    return ByteString(*this) += ch;
  }
  inline friend ByteString operator+(UNICHAR ch, const ByteString& str) {
    return ByteString(ch) += str;
  }

  inline ByteString operator+(const UNICHAR* str) const {
    return ByteString(*this) += str;
  }
  inline friend ByteString operator+(const UNICHAR* str1,
                                     const ByteString& str2) {
    return ByteString(str1) += str2;
  }

  inline ByteString operator+(const UString& str) const {
    return ByteString(*this) += str;
  }
  // inline friend ByteString operator + (const UString& str1, const ByteString&
  // str2) { return ByteString(str1) += str2; }

  inline ByteString operator+(UStringView str) const {
    return ByteString(*this) += str;
  }
  inline friend ByteString operator+(UStringView str1, const ByteString& str2) {
    return ByteString(str1) += str2;
  }
#endif

  template <typename ElementType>
  inline ByteString operator+(std::initializer_list<ElementType> list) {
    return ByteString(*this) += list;
  }
  template <typename ElementType>
  inline ByteString operator+(const Array<ElementType>& list) {
    return ByteString(*this) += list;
  }  // Allocator를 지정할 수 있어야하나?

  inline ByteString& Insert(int32 pos, char ch) {
    return Insert(pos, ByteStringView(&ch, 1));
  }
  ByteString& Insert(int32 pos, int32 count, char ch);
  inline ByteString& Insert(int32 pos, const char* str, int32 len) {
    return Insert(pos, ByteStringView(str, len));
  }
  ByteString& Insert(int32 pos, ByteStringView str);
  inline ByteString& Insert(int32 pos, AsciiString str) {
    return Insert(pos, ByteStringView(str.ConstData(), str.Len()));
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline ByteString& Insert(int32 pos, UNICHAR ch) {
    return Insert(pos, UNICHAR_TO_UTF8_BUFFER(&ch, 1).ToView());
  }
  ByteString& Insert(int32 pos, int32 count, UNICHAR ch);
  inline ByteString& Insert(int32 pos, const UNICHAR* str, int32 len) {
    return Insert(pos, UNICHAR_TO_UTF8_BUFFER(str, len).ToView());
  }
  inline ByteString& Insert(int32 pos, UStringView str) {
    return Insert(pos, UNICHAR_TO_UTF8_BUFFER(str).ToView());
  }
#endif

  inline ByteString& Overwrite(int32 pos, char ch) {
    return Overwrite(pos, ByteStringView(&ch, 1));
  }
  ByteString& Overwrite(int32 pos, int32 count, char ch);
  inline ByteString& Overwrite(int32 pos, const char* str, int32 len) {
    return Overwrite(pos, ByteStringView(str, len));
  }
  ByteString& Overwrite(int32 pos, ByteStringView str);
  inline ByteString& Overwrite(int32 pos, AsciiString str) {
    return Overwrite(pos, ByteStringView(str.ConstData(), str.Len()));
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline ByteString& Overwrite(int32 pos, UNICHAR ch) {
    return Overwrite(pos, UNICHAR_TO_UTF8_BUFFER(&ch, 1).ToView());
  }
  ByteString& Overwrite(int32 pos, int32 count, UNICHAR ch);
  inline ByteString& Overwrite(int32 pos, const UNICHAR* str, int32 len) {
    return Overwrite(pos, UNICHAR_TO_UTF8_BUFFER(str, len).ToView());
  }
  inline ByteString& Overwrite(int32 pos, UStringView str) {
    return Overwrite(pos, UNICHAR_TO_UTF8_BUFFER(str).ToView());
  }
#endif

  ByteString& Remove(int32 pos, int32 len);

  inline ByteString& RemoveFirst(int32 len = 1) {
    fun_check(len <= Len());
    return Remove(0, len);
  }
  inline ByteString RemoveLast(int32 len = 1) {
    fun_check(len <= Len());
    return Remove(Len() - len, len);
  }

  inline ByteString& FindAndRemove(
      char ch, CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryFindAndRemove(ch, casesense);
    return *this;
  }
  inline ByteString& FindAndRemove(
      const char* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryFindAndRemove(sub, sub_len, casesense);
    return *this;
  }
  inline ByteString& FindAndRemove(
      ByteStringView sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryFindAndRemove(sub, casesense);
    return *this;
  }
  inline ByteString& FindAndRemove(
      AsciiString sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryFindAndRemove(sub, casesense);
    return *this;
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline ByteString& FindAndRemove(
      UNICHAR ch, CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryFindAndRemove(ch, casesense);
    return *this;
  }
  inline ByteString& FindAndRemove(
      const UNICHAR* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryFindAndRemove(sub, sub_len, casesense);
    return *this;
  }
  inline ByteString& FindAndRemove(
      UStringView sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryFindAndRemove(sub, casesense);
    return *this;
  }
#endif

  int32 TryFindAndRemove(
      ByteStringView sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive);

  inline int32 TryFindAndRemove(
      char ch, CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryFindAndRemove(ByteStringView(&ch, 1), casesense);
  }
  inline int32 TryFindAndRemove(
      const char* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryFindAndRemove(ByteStringView(sub, sub_len), casesense);
  }
  inline int32 TryFindAndRemove(
      AsciiString sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryFindAndRemove(ByteStringView(sub.ConstData(), sub.Len()),
                            casesense);
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline int32 TryFindAndRemove(
      UNICHAR ch, CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryFindAndRemove(UNICHAR_TO_UTF8_BUFFER(&ch, 1).ToView(), casesense);
  }
  inline int32 TryFindAndRemove(
      const UNICHAR* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryFindAndRemove(UNICHAR_TO_UTF8_BUFFER(sub, sub_len).ToView(),
                            casesense);
  }
  inline int32 TryFindAndRemove(
      UStringView sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryFindAndRemove(UNICHAR_TO_UTF8_BUFFER(sub).ToView(), casesense);
  }
#endif

  ByteString& Replace(int32 before_pos, int32 before_len, ByteStringView after);

  inline ByteString& Replace(int32 before_pos, int32 before_len, char after) {
    return Replace(before_pos, before_len, ByteStringView(&after, 1));
  }
  inline ByteString& Replace(int32 before_pos, int32 before_len,
                             const char* after, int32 after_len) {
    return Replace(before_pos, before_len, ByteStringView(after, after_len));
  }
  inline ByteString& Replace(int32 before_pos, int32 before_len,
                             AsciiString after) {
    return Replace(before_pos, before_len,
                   ByteStringView(after.ConstData(), after.Len()));
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline ByteString& Replace(int32 before_pos, int32 before_len,
                             UNICHAR after) {
    return Replace(before_pos, before_len,
                   UNICHAR_TO_UTF8_BUFFER(&after, 1).ToView());
  }
  inline ByteString& Replace(int32 before_pos, int32 before_len,
                             const UNICHAR* after, int32 after_len) {
    return Replace(before_pos, before_len,
                   UNICHAR_TO_UTF8_BUFFER(after, after_len).ToView());
  }
  inline ByteString& Replace(int32 before_pos, int32 before_len,
                             UStringView after) {
    return Replace(before_pos, before_len,
                   UNICHAR_TO_UTF8_BUFFER(after).ToView());
  }
#endif

  int32 TryReplace(ByteStringView before, ByteStringView after,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive);

  // char -> *
  inline int32 TryReplace(
      char before, char after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(ByteStringView(&before, 1), ByteStringView(&after, 1),
                      casesense);
  }
  inline int32 TryReplace(
      char before, const char* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(ByteStringView(&before, 1),
                      ByteStringView(after, after_len), casesense);
  }
  inline int32 TryReplace(
      char before, ByteStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(ByteStringView(&before, 1), after, casesense);
  }
  inline int32 TryReplace(
      char before, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(ByteStringView(&before, 1),
                      ByteStringView(after.ConstData(), after.Len()),
                      casesense);
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline int32 TryReplace(
      char before, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(ByteStringView(&before, 1),
                      UNICHAR_TO_UTF8_BUFFER(&after, 1).ToView(), casesense);
  }
  inline int32 TryReplace(
      char before, const UNICHAR* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(ByteStringView(&before, 1),
                      UNICHAR_TO_UTF8_BUFFER(after, after_len).ToView(),
                      casesense);
  }
  inline int32 TryReplace(
      char before, UStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(ByteStringView(&before, 1),
                      UNICHAR_TO_UTF8_BUFFER(after).ToView(), casesense);
  }
#endif

  // const char*,len -> *
  inline int32 TryReplace(
      const char* before, int32 before_len, char after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(ByteStringView(before, before_len),
                      ByteStringView(&after, 1), casesense);
  }
  inline int32 TryReplace(
      const char* before, int32 before_len, const char* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(ByteStringView(before, before_len),
                      ByteStringView(after, after_len), casesense);
  }
  inline int32 TryReplace(
      const char* before, int32 before_len, ByteStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(ByteStringView(before, before_len), after, casesense);
  }
  inline int32 TryReplace(
      const char* before, int32 before_len, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(ByteStringView(before, before_len),
                      ByteStringView(after.ConstData(), after.Len()),
                      casesense);
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline int32 TryReplace(
      const char* before, int32 before_len, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(ByteStringView(before, before_len),
                      UNICHAR_TO_UTF8_BUFFER(&after, 1).ToView(), casesense);
  }
  inline int32 TryReplace(
      const char* before, int32 before_len, const UNICHAR* after,
      int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(ByteStringView(before, before_len),
                      UNICHAR_TO_UTF8_BUFFER(after, after_len).ToView(),
                      casesense);
  }
  inline int32 TryReplace(
      const char* before, int32 before_len, UStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(ByteStringView(before, before_len),
                      UNICHAR_TO_UTF8_BUFFER(after).ToView(), casesense);
  }
#endif

  // ByteStringView -> *
  inline int32 TryReplace(
      ByteStringView before, char after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(before, ByteStringView(&after, 1), casesense);
  }
  inline int32 TryReplace(
      ByteStringView before, const char* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(before, ByteStringView(after, after_len), casesense);
  }
  // int32 TryReplace(ByteStringView before, ByteStringView after,
  // CaseSensitivity casesense = CaseSensitivity::CaseSensitive);
  inline int32 TryReplace(
      ByteStringView before, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(before, ByteStringView(after.ConstData(), after.Len()),
                      casesense);
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline int32 TryReplace(
      ByteStringView before, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(before, UNICHAR_TO_UTF8_BUFFER(&after, 1).ToView(),
                      casesense);
  }
  inline int32 TryReplace(
      ByteStringView before, const UNICHAR* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(before, UNICHAR_TO_UTF8_BUFFER(after, after_len).ToView(),
                      casesense);
  }
  inline int32 TryReplace(
      ByteStringView before, UStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(before, UNICHAR_TO_UTF8_BUFFER(after).ToView(),
                      casesense);
  }
#endif

  // AsciiString -> *
  inline int32 TryReplace(
      AsciiString before, char after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(ByteStringView(before.ConstData(), before.Len()),
                      ByteStringView(&after, 1), casesense);
  }
  inline int32 TryReplace(
      AsciiString before, const char* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(ByteStringView(before.ConstData(), before.Len()),
                      ByteStringView(after, after_len), casesense);
  }
  inline int32 TryReplace(
      AsciiString before, ByteStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(ByteStringView(before.ConstData(), before.Len()), after,
                      casesense);
  }
  inline int32 TryReplace(
      AsciiString before, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(ByteStringView(before.ConstData(), before.Len()),
                      UNICHAR_TO_UTF8_BUFFER(&after, 1).ToByteStringView(),
                      casesense);
  }
  inline int32 TryReplace(
      AsciiString before, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(ByteStringView(before.ConstData(), before.Len()),
                      ByteStringView(after.ConstData(), after.Len()),
                      casesense);
  }
  inline int32 TryReplace(
      AsciiString before, const UNICHAR* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(
        ByteStringView(before.ConstData(), before.Len()),
        UNICHAR_TO_UTF8_BUFFER(after, after_len).ToByteStringView(), casesense);
  }
  inline int32 TryReplace(
      AsciiString before, UStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(ByteStringView(before.ConstData(), before.Len()),
                      UNICHAR_TO_UTF8_BUFFER(after).ToByteStringView(),
                      casesense);
  }

#if FUN_USE_IMPLICIT_STRING_CONVERSION
  // UNICHAR -> *
  inline int32 TryReplace(
      UNICHAR before, char after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UNICHAR_TO_UTF8_BUFFER(&before, 1).ToView(),
                      ByteStringView(&after, 1), casesense);
  }
  inline int32 TryReplace(
      UNICHAR before, const char* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UNICHAR_TO_UTF8_BUFFER(&before, 1).ToView(),
                      ByteStringView(after, after_len), casesense);
  }
  inline int32 TryReplace(
      UNICHAR before, ByteStringView after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UNICHAR_TO_UTF8_BUFFER(&before, 1).ToView(), after,
                      casesense);
  }
  inline int32 TryReplace(
      UNICHAR before, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UNICHAR_TO_UTF8_BUFFER(&before, 1).ToView(),
                      UNICHAR_TO_UTF8_BUFFER(&after, 1).ToByteStringView(),
                      casesense);
  }
  inline int32 TryReplace(
      UNICHAR before, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UNICHAR_TO_UTF8_BUFFER(&before, 1).ToView(),
                      ByteStringView(after.ConstData(), after.Len()),
                      casesense);
  }
  inline int32 TryReplace(
      UNICHAR before, const UNICHAR* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(
        UNICHAR_TO_UTF8_BUFFER(&before, 1).ToView(),
        UNICHAR_TO_UTF8_BUFFER(after, after_len).ToByteStringView(), casesense);
  }
  inline int32 TryReplace(
      UNICHAR before, UStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UNICHAR_TO_UTF8_BUFFER(&before, 1).ToView(),
                      UNICHAR_TO_UTF8_BUFFER(after).ToByteStringView(),
                      casesense);
  }

  // const UNICHAR*,len -> *
  inline int32 TryReplace(
      const UNICHAR* before, int32 before_len, char after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UNICHAR_TO_UTF8_BUFFER(before, before_len).ToView(),
                      ByteStringView(&after, 1), casesense);
  }
  inline int32 TryReplace(
      const UNICHAR* before, int32 before_len, const char* after,
      int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UNICHAR_TO_UTF8_BUFFER(before, before_len).ToView(),
                      ByteStringView(after, after_len), casesense);
  }
  inline int32 TryReplace(
      const UNICHAR* before, int32 before_len, ByteStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UNICHAR_TO_UTF8_BUFFER(before, before_len).ToView(),
                      after, casesense);
  }
  inline int32 TryReplace(
      const UNICHAR* before, int32 before_len, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UNICHAR_TO_UTF8_BUFFER(before, before_len).ToView(),
                      UNICHAR_TO_UTF8_BUFFER(&after, 1).ToByteStringView(),
                      casesense);
  }
  inline int32 TryReplace(
      const UNICHAR* before, int32 before_len, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UNICHAR_TO_UTF8_BUFFER(before, before_len).ToView(),
                      ByteStringView(after.ConstData(), after.Len()),
                      casesense);
  }
  inline int32 TryReplace(
      const UNICHAR* before, int32 before_len, const UNICHAR* after,
      int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(
        UNICHAR_TO_UTF8_BUFFER(before, before_len).ToView(),
        UNICHAR_TO_UTF8_BUFFER(after, after_len).ToByteStringView(), casesense);
  }
  inline int32 TryReplace(
      const UNICHAR* before, int32 before_len, UStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UNICHAR_TO_UTF8_BUFFER(before, before_len).ToView(),
                      UNICHAR_TO_UTF8_BUFFER(after).ToByteStringView(),
                      casesense);
  }

  // UStringView -> *
  inline int32 TryReplace(
      UStringView before, char after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UNICHAR_TO_UTF8_BUFFER(before).ToView(),
                      ByteStringView(&after, 1), casesense);
  }
  inline int32 TryReplace(
      UStringView before, const char* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UNICHAR_TO_UTF8_BUFFER(before).ToView(),
                      ByteStringView(after, after_len), casesense);
  }
  inline int32 TryReplace(
      UStringView before, ByteStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UNICHAR_TO_UTF8_BUFFER(before).ToView(), after,
                      casesense);
  }
  inline int32 TryReplace(
      UStringView before, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UNICHAR_TO_UTF8_BUFFER(before).ToView(),
                      UNICHAR_TO_UTF8_BUFFER(&after, 1).ToByteStringView(),
                      casesense);
  }
  inline int32 TryReplace(
      UStringView before, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UNICHAR_TO_UTF8_BUFFER(before).ToView(),
                      ByteStringView(after.ConstData(), after.Len()),
                      casesense);
  }
  inline int32 TryReplace(
      UStringView before, const UNICHAR* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(
        UNICHAR_TO_UTF8_BUFFER(before).ToView(),
        UNICHAR_TO_UTF8_BUFFER(after, after_len).ToByteStringView(), casesense);
  }
  inline int32 TryReplace(
      UStringView before, UStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return TryReplace(UNICHAR_TO_UTF8_BUFFER(before).ToView(),
                      UNICHAR_TO_UTF8_BUFFER(after).ToByteStringView(),
                      casesense);
  }
#endif

  // char -> *
  inline ByteString& Replace(
      char before, char after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
  inline ByteString& Replace(
      char before, const char* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, after_len, casesense);
    return *this;
  }
  inline ByteString& Replace(
      char before, ByteStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
  inline ByteString& Replace(
      char before, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline ByteString& Replace(
      char before, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
  inline ByteString& Replace(
      char before, const UNICHAR* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, after_len, casesense);
    return *this;
  }
  inline ByteString& Replace(
      char before, UStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
#endif

  // const char*,len -> *
  inline ByteString& Replace(
      const char* before, int32 before_len, char after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, before_len, after, casesense);
    return *this;
  }
  inline ByteString& Replace(
      const char* before, int32 before_len, const char* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, before_len, after, after_len, casesense);
    return *this;
  }
  inline ByteString& Replace(
      const char* before, int32 before_len, ByteStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, before_len, after, casesense);
    return *this;
  }
  inline ByteString& Replace(
      const char* before, int32 before_len, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, before_len, after, casesense);
    return *this;
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline ByteString& Replace(
      const char* before, int32 before_len, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, before_len, after, casesense);
    return *this;
  }
  inline ByteString& Replace(
      const char* before, int32 before_len, const UNICHAR* after,
      int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, before_len, after, after_len, casesense);
    return *this;
  }
  inline ByteString& Replace(
      const char* before, int32 before_len, UStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, before_len, after, casesense);
    return *this;
  }
#endif

  // ByteStringView -> *
  inline ByteString& Replace(
      ByteStringView before, char after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
  inline ByteString& Replace(
      ByteStringView before, const char* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, after_len, casesense);
    return *this;
  }
  inline ByteString& Replace(
      ByteStringView before, ByteStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
  inline ByteString& Replace(
      ByteStringView before, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after.ConstData(), after.Len(), casesense);
    return *this;
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline ByteString& Replace(
      ByteStringView before, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
  inline ByteString& Replace(
      ByteStringView before, const UNICHAR* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, after_len, casesense);
    return *this;
  }
  inline ByteString& Replace(
      ByteStringView before, UStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
#endif

  // AsciiString -> *
  inline ByteString& Replace(
      AsciiString before, char after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
  inline ByteString& Replace(
      AsciiString before, const char* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, after_len, casesense);
    return *this;
  }
  inline ByteString& Replace(
      AsciiString before, ByteStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
  inline ByteString& Replace(
      AsciiString before, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
  inline ByteString& Replace(
      AsciiString before, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
  inline ByteString& Replace(
      AsciiString before, const UNICHAR* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, after_len, casesense);
    return *this;
  }
  inline ByteString& Replace(
      AsciiString before, UStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }

#if FUN_USE_IMPLICIT_STRING_CONVERSION
  // UNICHAR -> *
  inline ByteString& Replace(
      UNICHAR before, char after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
  inline ByteString& Replace(
      UNICHAR before, const char* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, after_len, casesense);
    return *this;
  }
  inline ByteString& Replace(
      UNICHAR before, ByteStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
  inline ByteString& Replace(
      UNICHAR before, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
  inline ByteString& Replace(
      UNICHAR before, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
  inline ByteString& Replace(
      UNICHAR before, const UNICHAR* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, after_len, casesense);
    return *this;
  }
  inline ByteString& Replace(
      UNICHAR before, UStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }

  // const UNICHAR*,len -> *
  inline ByteString& Replace(
      const UNICHAR* before, int32 before_len, char after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, before_len, after, casesense);
    return *this;
  }
  inline ByteString& Replace(
      const UNICHAR* before, int32 before_len, const char* after,
      int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, before_len, after, after_len, casesense);
    return *this;
  }
  inline ByteString& Replace(
      const UNICHAR* before, int32 before_len, ByteStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, before_len, after, casesense);
    return *this;
  }
  inline ByteString& Replace(
      const UNICHAR* before, int32 before_len, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, before_len, after, casesense);
    return *this;
  }
  inline ByteString& Replace(
      const UNICHAR* before, int32 before_len, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, before_len, after, casesense);
    return *this;
  }
  inline ByteString& Replace(
      const UNICHAR* before, int32 before_len, const UNICHAR* after,
      int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, before_len, after, after_len, casesense);
    return *this;
  }
  inline ByteString& Replace(
      const UNICHAR* before, int32 before_len, UStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, before_len, after, casesense);
    return *this;
  }

  // UStringView -> *
  inline ByteString& Replace(
      UStringView before, char after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
  inline ByteString& Replace(
      UStringView before, const char* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, after_len, casesense);
    return *this;
  }
  inline ByteString& Replace(
      UStringView before, ByteStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
  inline ByteString& Replace(
      UStringView before, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
  inline ByteString& Replace(
      UStringView before, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
  inline ByteString& Replace(
      UStringView before, const UNICHAR* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
  inline ByteString& Replace(
      UStringView before, UStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    TryReplace(before, after, casesense);
    return *this;
  }
#endif

  // char -> *
  inline ByteString Replaced(
      char before, char after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return ByteString(*this).Replace(before, after, casesense);
  }
  inline ByteString Replaced(
      char before, const char* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return ByteString(*this).Replace(before, after, casesense);
  }
  inline ByteString Replaced(
      char before, ByteStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return ByteString(*this).Replace(before, after, casesense);
  }
  inline ByteString Replaced(
      char before, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return ByteString(*this).Replace(before, after, casesense);
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline ByteString Replaced(
      char before, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return ByteString(*this).Replace(before, after, casesense);
  }
  inline ByteString Replaced(
      char before, const UNICHAR* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return ByteString(*this).Replace(before, after, after_len, casesense);
  }
  inline ByteString Replaced(
      char before, UStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return ByteString(*this).Replace(before, after, casesense);
  }
#endif

  // const char*,len -> *
  inline ByteString Replaced(
      const char* before, int32 before_len, char after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return ByteString(*this).Replace(before, before_len, after, casesense);
  }
  inline ByteString Replaced(
      const char* before, int32 before_len, const char* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return ByteString(*this).Replace(before, before_len, after, casesense);
  }
  inline ByteString Replaced(
      const char* before, int32 before_len, ByteStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return ByteString(*this).Replace(before, before_len, after, casesense);
  }
  inline ByteString Replaced(
      const char* before, int32 before_len, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return ByteString(*this).Replace(before, before_len, after, casesense);
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline ByteString Replaced(
      const char* before, int32 before_len, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return ByteString(*this).Replace(before, before_len, after, casesense);
  }
  inline ByteString Replaced(
      const char* before, int32 before_len, const UNICHAR* after,
      int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return ByteString(*this).Replace(before, before_len, after, after_len,
                                     casesense);
  }
  inline ByteString Replaced(
      const char* before, int32 before_len, UStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return ByteString(*this).Replace(before, before_len, after, casesense);
  }
#endif

  // ByteStringView -> *
  inline ByteString Replaced(
      ByteStringView before, char after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return ByteString(*this).Replace(before, after, casesense);
  }
  inline ByteString Replaced(
      ByteStringView before, const char* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return ByteString(*this).Replace(before, after, casesense);
  }
  inline ByteString Replaced(
      ByteStringView before, ByteStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return ByteString(*this).Replace(before, after, casesense);
  }
  inline ByteString Replaced(
      ByteStringView before, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return ByteString(*this).Replace(before, after, casesense);
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline ByteString Replaced(
      ByteStringView before, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return ByteString(*this).Replace(before, after, casesense);
  }
  inline ByteString Replaced(
      ByteStringView before, const UNICHAR* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return ByteString(*this).Replace(before, after, after_len, casesense);
  }
  inline ByteString Replaced(
      ByteStringView before, UStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return ByteString(*this).Replace(before, after, casesense);
  }
#endif

  // AsciiString -> *
  inline ByteString Replaced(
      AsciiString before, char after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return ByteString(*this).Replace(before, after, casesense);
  }
  inline ByteString Replaced(
      AsciiString before, const char* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return ByteString(*this).Replace(before, after, after_len, casesense);
  }
  inline ByteString Replaced(
      AsciiString before, ByteStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return ByteString(*this).Replace(before, after, casesense);
  }
  inline ByteString Replaced(
      AsciiString before, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return ByteString(*this).Replace(before, after, casesense);
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline ByteString Replaced(
      AsciiString before, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return ByteString(*this).Replace(before, after, casesense);
  }
  inline ByteString Replaced(
      AsciiString before, const UNICHAR* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return ByteString(*this).Replace(before, after, after_len, casesense);
  }
  inline ByteString Replaced(
      AsciiString before, UStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return ByteString(*this).Replace(before, after, casesense);
  }
#endif

#if FUN_USE_IMPLICIT_STRING_CONVERSION
  // UNICHAR -> *
  inline ByteString Replaced(
      UNICHAR before, char after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return ByteString(*this).Replace(before, after, casesense);
  }
  inline ByteString Replaced(
      UNICHAR before, const char* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return ByteString(*this).Replace(before, after, after_len, casesense);
  }
  inline ByteString Replaced(
      UNICHAR before, ByteStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return ByteString(*this).Replace(before, after, casesense);
  }
  inline ByteString Replaced(
      UNICHAR before, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return ByteString(*this).Replace(before, after, casesense);
  }
  inline ByteString Replaced(
      UNICHAR before, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return ByteString(*this).Replace(before, after, casesense);
  }
  inline ByteString Replaced(
      UNICHAR before, const UNICHAR* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return ByteString(*this).Replace(before, after, after_len, casesense);
  }
  inline ByteString Replaced(
      UNICHAR before, UStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return ByteString(*this).Replace(before, after, casesense);
  }

  // const UNICHAR*,len -> *
  inline ByteString Replaced(
      const UNICHAR* before, int32 before_len, char after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return ByteString(*this).Replace(before, before_len, after, casesense);
  }
  inline ByteString Replaced(
      const UNICHAR* before, int32 before_len, const char* after,
      int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return ByteString(*this).Replace(before, before_len, after, after_len,
                                     casesense);
  }
  inline ByteString Replaced(
      const UNICHAR* before, int32 before_len, ByteStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return ByteString(*this).Replace(before, before_len, after, casesense);
  }
  inline ByteString Replaced(
      const UNICHAR* before, int32 before_len, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return ByteString(*this).Replace(before, before_len, after, casesense);
  }
  inline ByteString Replaced(
      const UNICHAR* before, int32 before_len, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return ByteString(*this).Replace(before, before_len, after, casesense);
  }
  inline ByteString Replaced(
      const UNICHAR* before, int32 before_len, const UNICHAR* after,
      int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return ByteString(*this).Replace(before, before_len, after, after_len,
                                     casesense);
  }
  inline ByteString Replaced(
      const UNICHAR* before, int32 before_len, UStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return ByteString(*this).Replace(before, before_len, after, casesense);
  }

  // UStringView -> *
  inline ByteString Replaced(
      UStringView before, char after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return ByteString(*this).Replace(before, after, casesense);
  }
  inline ByteString Replaced(
      UStringView before, const char* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return ByteString(*this).Replace(before, after, casesense);
  }
  inline ByteString Replaced(
      UStringView before, ByteStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return ByteString(*this).Replace(before, after, casesense);
  }
  inline ByteString Replaced(
      UStringView before, AsciiString after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return ByteString(*this).Replace(before, after, casesense);
  }
  inline ByteString Replaced(
      UStringView before, UNICHAR after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return ByteString(*this).Replace(before, after, casesense);
  }
  inline ByteString Replaced(
      UStringView before, const UNICHAR* after, int32 after_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return ByteString(*this).Replace(before, after, casesense);
  }
  inline ByteString Replaced(
      UStringView before, UStringView after,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return ByteString(*this).Replace(before, after, casesense);
  }
#endif

  Array<ByteString> Split(
      char separator, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  Array<ByteString> Split(
      const Array<char>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteString> Split(
      std::initializer_list<char> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteString> Split(
      ByteStringView separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteString> Split(
      AsciiString separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  Array<ByteString> Split(
      const Array<const char*>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteString> Split(
      const Array<ByteString>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteString> Split(
      const Array<ByteStringRef>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteString> Split(
      const Array<ByteStringView>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteString> Split(
      const Array<AsciiString>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  Array<ByteString> Split(
      std::initializer_list<const char*> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteString> Split(
      std::initializer_list<ByteString> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteString> Split(
      std::initializer_list<ByteStringRef> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteString> Split(
      std::initializer_list<ByteStringView> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteString> Split(
      std::initializer_list<AsciiString> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  Array<ByteString> SplitByWhitespaces(
      ByteString extra_separator = ByteString(), int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteString> SplitLines(
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

#if FUN_USE_IMPLICIT_STRING_CONVERSION
  Array<ByteString> Split(
      UNICHAR separator, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  Array<ByteString> Split(
      const Array<UNICHAR>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteString> Split(
      std::initializer_list<UNICHAR> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteString> Split(
      UStringView separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  Array<ByteString> Split(
      const Array<const UNICHAR*>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteString> Split(
      const Array<UString>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteString> Split(
      const Array<UStringRef>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteString> Split(
      const Array<UStringView>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  Array<ByteString> Split(
      std::initializer_list<const UNICHAR*> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteString> Split(
      std::initializer_list<UString> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteString> Split(
      std::initializer_list<UStringRef> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteString> Split(
      std::initializer_list<UStringView> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
#endif

  int32 Split(Array<ByteString>& list, char separator, int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  int32 Split(Array<ByteString>& list, const Array<char>& separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<ByteString>& list, std::initializer_list<char> separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<ByteString>& list, ByteStringView separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<ByteString>& list, AsciiString separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  int32 Split(Array<ByteString>& list, const Array<const char*>& separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<ByteString>& list, const Array<ByteString>& separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<ByteString>& list, const Array<ByteStringRef>& separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<ByteString>& list, const Array<ByteStringView>& separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<ByteString>& list, const Array<AsciiString>& separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  int32 Split(Array<ByteString>& list,
              std::initializer_list<const char*> separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<ByteString>& list,
              std::initializer_list<ByteString> separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<ByteString>& list,
              std::initializer_list<ByteStringRef> separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<ByteString>& list,
              std::initializer_list<ByteStringView> separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<ByteString>& list,
              std::initializer_list<AsciiString> separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  int32 SplitByWhitespaces(
      Array<ByteString>& list, ByteString extra_separator = ByteString(),
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitLines(
      Array<ByteString>& out_lines,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

#if FUN_USE_IMPLICIT_STRING_CONVERSION
  int32 Split(Array<ByteString>& list, UNICHAR separator, int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  int32 Split(Array<ByteString>& list, const Array<UNICHAR>& separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<ByteString>& list,
              std::initializer_list<UNICHAR> separators, int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<ByteString>& list, UStringView separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  int32 Split(Array<ByteString>& list, const Array<const UNICHAR*>& separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<ByteString>& list, const Array<UString>& separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<ByteString>& list, const Array<UStringRef>& separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<ByteString>& list, const Array<UStringView>& separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  int32 Split(Array<ByteString>& list,
              std::initializer_list<const UNICHAR*> separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<ByteString>& list,
              std::initializer_list<UString> separators, int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<ByteString>& list,
              std::initializer_list<UStringRef> separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<ByteString>& list,
              std::initializer_list<UStringView> separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
#endif

  // Split Ref

  Array<ByteStringRef> SplitRef(
      char separator, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  Array<ByteStringRef> SplitRef(
      const Array<char>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteStringRef> SplitRef(
      std::initializer_list<char> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteStringRef> SplitRef(
      ByteStringView separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteStringRef> SplitRef(
      AsciiString separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  Array<ByteStringRef> SplitRef(
      const Array<const char*>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteStringRef> SplitRef(
      const Array<ByteString>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteStringRef> SplitRef(
      const Array<ByteStringRef>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteStringRef> SplitRef(
      const Array<ByteStringView>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteStringRef> SplitRef(
      const Array<AsciiString>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  Array<ByteStringRef> SplitRef(
      std::initializer_list<const char*> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteStringRef> SplitRef(
      std::initializer_list<ByteString> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteStringRef> SplitRef(
      std::initializer_list<ByteStringRef> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteStringRef> SplitRef(
      std::initializer_list<ByteStringView> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteStringRef> SplitRef(
      std::initializer_list<AsciiString> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  Array<ByteStringRef> SplitByWhitespacesRef(
      ByteString extra_separator = ByteString(), int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteStringRef> SplitLinesRef(
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

#if FUN_USE_IMPLICIT_STRING_CONVERSION
  Array<ByteStringRef> SplitRef(
      UNICHAR separator, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  Array<ByteStringRef> SplitRef(
      const Array<UNICHAR>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteStringRef> SplitRef(
      std::initializer_list<UNICHAR> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteStringRef> SplitRef(
      UStringView separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  Array<ByteStringRef> SplitRef(
      const Array<const UNICHAR*>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteStringRef> SplitRef(
      const Array<UString>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteStringRef> SplitRef(
      const Array<UStringRef>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteStringRef> SplitRef(
      const Array<UStringView>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  Array<ByteStringRef> SplitRef(
      std::initializer_list<const UNICHAR*> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteStringRef> SplitRef(
      std::initializer_list<UString> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteStringRef> SplitRef(
      std::initializer_list<UStringRef> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteStringRef> SplitRef(
      std::initializer_list<UStringView> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
#endif

  int32 SplitRef(
      Array<ByteStringRef>& list, char separator, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  int32 SplitRef(
      Array<ByteStringRef>& list, const Array<char>& separators,
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitRef(
      Array<ByteStringRef>& list, std::initializer_list<char> separators,
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitRef(
      Array<ByteStringRef>& list, ByteStringView separators,
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitRef(
      Array<ByteStringRef>& list, AsciiString separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  int32 SplitRef(
      Array<ByteStringRef>& list, const Array<const char*>& separators,
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitRef(
      Array<ByteStringRef>& list, const Array<ByteString>& separators,
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitRef(
      Array<ByteStringRef>& list, const Array<ByteStringRef>& separators,
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitRef(
      Array<ByteStringRef>& list, const Array<ByteStringView>& separators,
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitRef(
      Array<ByteStringRef>& list, const Array<AsciiString>& separators,
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  int32 SplitRef(
      Array<ByteStringRef>& list, std::initializer_list<const char*> separators,
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitRef(
      Array<ByteStringRef>& list, std::initializer_list<ByteString> separators,
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitRef(
      Array<ByteStringRef>& list,
      std::initializer_list<ByteStringRef> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitRef(
      Array<ByteStringRef>& list,
      std::initializer_list<ByteStringView> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitRef(
      Array<ByteStringRef>& list, std::initializer_list<AsciiString> separators,
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  int32 SplitByWhitespacesRef(
      Array<ByteStringRef>& list, ByteString extra_separator = ByteString(),
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitLinesRef(
      Array<ByteStringRef>& out_lines,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

#if FUN_USE_IMPLICIT_STRING_CONVERSION
  int32 SplitRef(
      Array<ByteStringRef>& list, UNICHAR separator, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  int32 SplitRef(
      Array<ByteStringRef>& list, const Array<UNICHAR>& separators,
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitRef(
      Array<ByteStringRef>& list, std::initializer_list<UNICHAR> separators,
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitRef(
      Array<ByteStringRef>& list, UStringView separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  int32 SplitRef(
      Array<ByteStringRef>& list, const Array<const UNICHAR*>& separators,
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitRef(
      Array<ByteStringRef>& list, const Array<UString>& separators,
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitRef(
      Array<ByteStringRef>& list, const Array<UStringRef>& separators,
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitRef(
      Array<ByteStringRef>& list, const Array<UStringView>& separators,
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  int32 SplitRef(
      Array<ByteStringRef>& list,
      std::initializer_list<const UNICHAR*> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitRef(
      Array<ByteStringRef>& list, std::initializer_list<UString> separators,
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitRef(
      Array<ByteStringRef>& list, std::initializer_list<UStringRef> separators,
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitRef(
      Array<ByteStringRef>& list, std::initializer_list<UStringView> separators,
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
#endif

  bool Divide(ByteStringView delim, ByteString* out_left, ByteString* out_right,
              bool trimming = false,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  inline bool Divide(
      char delim_char, ByteString* out_left, ByteString* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Divide(ByteStringView(&delim_char, 1), out_left, out_right, trimming,
                  casesense);
  }
  inline bool Divide(
      const char* delim, int32 delim_len, ByteString* out_left,
      ByteString* out_right, bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Divide(ByteStringView(delim, delim_len), out_left, out_right,
                  trimming, casesense);
  }
  inline bool Divide(
      AsciiString delim, ByteString* out_left, ByteString* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Divide(ByteStringView(delim.ConstData(), delim.Len()), out_left,
                  out_right, trimming, casesense);
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline bool Divide(
      UNICHAR delim_char, ByteString* out_left, ByteString* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Divide(UNICHAR_TO_UTF8_BUFFER(&delim_char, 1).ToView(), out_left,
                  out_right, trimming, casesense);
  }
  inline bool Divide(
      const UNICHAR* delim, int32 delim_len, ByteString* out_left,
      ByteString* out_right, bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Divide(UNICHAR_TO_UTF8_BUFFER(delim, delim_len).ToView(), out_left,
                  out_right, trimming, casesense);
  }
  inline bool Divide(
      UStringView delim, ByteString* out_left, ByteString* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Divide(UNICHAR_TO_UTF8_BUFFER(delim).ToView(), out_left, out_right,
                  trimming, casesense);
  }
#endif

  bool Divide(ByteStringView delim, ByteStringRef* out_left,
              ByteStringRef* out_right, bool trimming = false,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  inline bool Divide(
      char delim_char, ByteStringRef* out_left, ByteStringRef* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Divide(ByteStringView(&delim_char, 1), out_left, out_right, trimming,
                  casesense);
  }
  inline bool Divide(
      const char* delim, int32 delim_len, ByteStringRef* out_left,
      ByteStringRef* out_right, bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Divide(ByteStringView(delim, delim_len), out_left, out_right,
                  trimming, casesense);
  }
  inline bool Divide(
      AsciiString delim, ByteStringRef* out_left, ByteStringRef* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Divide(ByteStringView(delim.ConstData(), delim.Len()), out_left,
                  out_right, trimming, casesense);
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline bool Divide(
      UNICHAR delim_char, ByteStringRef* out_left, ByteStringRef* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Divide(UNICHAR_TO_UTF8_BUFFER(&delim_char, 1).ToView(), out_left,
                  out_right, trimming, casesense);
  }
  inline bool Divide(
      const UNICHAR* delim, int32 delim_len, ByteStringRef* out_left,
      ByteStringRef* out_right, bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Divide(UNICHAR_TO_UTF8_BUFFER(delim, delim_len).ToView(), out_left,
                  out_right, trimming, casesense);
  }
  inline bool Divide(
      UStringView delim, ByteStringRef* out_left, ByteStringRef* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Divide(UNICHAR_TO_UTF8_BUFFER(delim).ToView(), out_left, out_right,
                  trimming, casesense);
  }
#endif

  bool LastDivide(
      ByteStringView delim, ByteString* out_left, ByteString* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  inline bool LastDivide(
      char delim_char, ByteString* out_left, ByteString* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return LastDivide(ByteStringView(&delim_char, 1), out_left, out_right,
                      trimming, casesense);
  }
  bool LastDivide(
      const char* delim, int32 delim_len, ByteString* out_left,
      ByteString* out_right, bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return LastDivide(ByteStringView(delim, delim_len), out_left, out_right,
                      trimming, casesense);
  }
  inline bool LastDivide(
      AsciiString delim, ByteString* out_left, ByteString* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return LastDivide(ByteStringView(delim.ConstData(), delim.Len()), out_left,
                      out_right, trimming, casesense);
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline bool LastDivide(
      UNICHAR delim_char, ByteString* out_left, ByteString* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return LastDivide(UNICHAR_TO_UTF8_BUFFER(&delim_char, 1).ToView(), out_left,
                      out_right, trimming, casesense);
  }
  inline bool LastDivide(
      const UNICHAR* delim, int32 delim_len, ByteString* out_left,
      ByteString* out_right, bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return LastDivide(UNICHAR_TO_UTF8_BUFFER(delim, delim_len).ToView(),
                      out_left, out_right, trimming, casesense);
  }
  inline bool LastDivide(
      UStringView delim, ByteString* out_left, ByteString* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return LastDivide(UNICHAR_TO_UTF8_BUFFER(delim).ToView(), out_left,
                      out_right, trimming, casesense);
  }
#endif

  bool LastDivide(
      ByteStringView delim, ByteStringRef* out_left, ByteStringRef* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  inline bool LastDivide(
      const char* delim, int32 delim_len, ByteStringRef* out_left,
      ByteStringRef* out_right, bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return LastDivide(ByteStringView(delim, delim_len), out_left, out_right,
                      trimming, casesense);
  }
  inline bool LastDivide(
      char delim_char, ByteStringRef* out_left, ByteStringRef* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return LastDivide(ByteStringView(&delim_char, 1), out_left, out_right,
                      trimming, casesense);
  }
  inline bool LastDivide(
      AsciiString delim, ByteStringRef* out_left, ByteStringRef* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return LastDivide(ByteStringView(delim.ConstData(), delim.Len()), out_left,
                      out_right, trimming, casesense);
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline bool LastDivide(
      UNICHAR delim_char, ByteStringRef* out_left, ByteStringRef* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return LastDivide(UNICHAR_TO_UTF8_BUFFER(&delim_char, 1).ToView(), out_left,
                      out_right, trimming, casesense);
  }
  inline bool LastDivide(
      const UNICHAR* delim, int32 delim_len, ByteStringRef* out_left,
      ByteStringRef* out_right, bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return LastDivide(UNICHAR_TO_UTF8_BUFFER(delim, delim_len).ToView(),
                      out_left, out_right, trimming, casesense);
  }
  inline bool LastDivide(
      UStringView delim, ByteStringRef* out_left, ByteStringRef* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return LastDivide(UNICHAR_TO_UTF8_BUFFER(delim).ToView(), out_left,
                      out_right, trimming, casesense);
  }
#endif

  static ByteString Join(const Array<const char*>& list,
                         ByteStringView separator);
  static ByteString Join(const Array<ByteString>& list,
                         ByteStringView separator);
  static ByteString Join(const Array<ByteStringRef>& list,
                         ByteStringView separator);
  static ByteString Join(const Array<ByteStringView>& list,
                         ByteStringView separator);
  static ByteString Join(const Array<AsciiString>& list,
                         ByteStringView separator);
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  static ByteString Join(const Array<const UNICHAR*>& list,
                         ByteStringView separator);
  static ByteString Join(const Array<UString>& list, ByteStringView separator);
  static ByteString Join(const Array<UStringRef>& list,
                         ByteStringView separator);
  static ByteString Join(const Array<UStringView>& list,
                         ByteStringView separator);
#endif

  static ByteString Join(std::initializer_list<const char*> list,
                         ByteStringView separator);
  static ByteString Join(std::initializer_list<ByteString> list,
                         ByteStringView separator);
  static ByteString Join(std::initializer_list<ByteStringRef> list,
                         ByteStringView separator);
  static ByteString Join(std::initializer_list<ByteStringView> list,
                         ByteStringView separator);
  static ByteString Join(std::initializer_list<AsciiString> list,
                         ByteStringView separator);
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  static ByteString Join(std::initializer_list<const UNICHAR*> list,
                         ByteStringView separator);
  static ByteString Join(std::initializer_list<UString> list,
                         ByteStringView separator);
  static ByteString Join(std::initializer_list<UStringRef> list,
                         ByteStringView separator);
  static ByteString Join(std::initializer_list<UStringView> list,
                         ByteStringView separator);
#endif

  static ByteString Join(const Array<const char*>& list, AsciiString separator);
  static ByteString Join(const Array<ByteString>& list, AsciiString separator);
  static ByteString Join(const Array<ByteStringRef>& list,
                         AsciiString separator);
  static ByteString Join(const Array<ByteStringView>& list,
                         AsciiString separator);
  static ByteString Join(const Array<AsciiString>& list, AsciiString separator);
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  static ByteString Join(const Array<const UNICHAR*>& list,
                         AsciiString separator);
  static ByteString Join(const Array<UString>& list, AsciiString separator);
  static ByteString Join(const Array<UStringRef>& list, AsciiString separator);
  static ByteString Join(const Array<UStringView>& list, AsciiString separator);
#endif

  static ByteString Join(std::initializer_list<const char*> list,
                         AsciiString separator);
  static ByteString Join(std::initializer_list<ByteString> list,
                         AsciiString separator);
  static ByteString Join(std::initializer_list<ByteStringRef> list,
                         AsciiString separator);
  static ByteString Join(std::initializer_list<ByteStringView> list,
                         AsciiString separator);
  static ByteString Join(std::initializer_list<AsciiString> list,
                         AsciiString separator);
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  static ByteString Join(std::initializer_list<const UNICHAR*> list,
                         AsciiString separator);
  static ByteString Join(std::initializer_list<UString> list,
                         AsciiString separator);
  static ByteString Join(std::initializer_list<UStringRef> list,
                         AsciiString separator);
  static ByteString Join(std::initializer_list<UStringView> list,
                         AsciiString separator);
#endif

#if FUN_USE_IMPLICIT_STRING_CONVERSION
  static ByteString Join(const Array<const char*>& list, UStringView separator);
  static ByteString Join(const Array<ByteString>& list, UStringView separator);
  static ByteString Join(const Array<ByteStringRef>& list,
                         UStringView separator);
  static ByteString Join(const Array<ByteStringView>& list,
                         UStringView separator);
  static ByteString Join(const Array<AsciiString>& list, UStringView separator);
  static ByteString Join(const Array<const UNICHAR*>& list,
                         UStringView separator);
  static ByteString Join(const Array<UString>& list, UStringView separator);
  static ByteString Join(const Array<UStringRef>& list, UStringView separator);
  static ByteString Join(const Array<UStringView>& list, UStringView separator);

  static ByteString Join(std::initializer_list<const char*> list,
                         UStringView separator);
  static ByteString Join(std::initializer_list<ByteString> list,
                         UStringView separator);
  static ByteString Join(std::initializer_list<ByteStringRef> list,
                         UStringView separator);
  static ByteString Join(std::initializer_list<ByteStringView> list,
                         UStringView separator);
  static ByteString Join(std::initializer_list<AsciiString> list,
                         UStringView separator);
  static ByteString Join(std::initializer_list<const UNICHAR*> list,
                         UStringView separator);
  static ByteString Join(std::initializer_list<UString> list,
                         UStringView separator);
  static ByteString Join(std::initializer_list<UStringRef> list,
                         UStringView separator);
  static ByteString Join(std::initializer_list<UStringView> list,
                         UStringView separator);
#endif

  //좌/우측 하나라도 객체인 경우에는 friend를 구현하지 않음.

  // ByteString and ByteString
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

  // ByteString and ByteStringRef
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

  // ByteString and ByteStringView
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

  // ByteString and AsciiString
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

  // ByteString and UString
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

  // ByteString and UStringRef
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

  // ByteString and UStringView
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

  // ByteString and char
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
  friend bool operator==(char ch, const ByteString& str);
  friend bool operator!=(char ch, const ByteString& str);
  friend bool operator<(char ch, const ByteString& str);
  friend bool operator<=(char ch, const ByteString& str);
  friend bool operator>(char ch, const ByteString& str);
  friend bool operator>=(char ch, const ByteString& str);

  // ByteString and const char*
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
  friend bool operator==(const char* str1, const ByteString& str2);
  friend bool operator!=(const char* str1, const ByteString& str2);
  friend bool operator<(const char* str1, const ByteString& str2);
  friend bool operator<=(const char* str1, const ByteString& str2);
  friend bool operator>(const char* str1, const ByteString& str2);
  friend bool operator>=(const char* str1, const ByteString& str2);

  // ByteString and UNICHAR
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
  friend bool operator==(UNICHAR ch, const ByteString& str);
  friend bool operator!=(UNICHAR ch, const ByteString& str);
  friend bool operator<(UNICHAR ch, const ByteString& str);
  friend bool operator<=(UNICHAR ch, const ByteString& str);
  friend bool operator>(UNICHAR ch, const ByteString& str);
  friend bool operator>=(UNICHAR ch, const ByteString& str);

  // ByteString and const UNICHAR*
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
  friend bool operator==(const UNICHAR* str1, const ByteString& str2);
  friend bool operator!=(const UNICHAR* str1, const ByteString& str2);
  friend bool operator<(const UNICHAR* str1, const ByteString& str2);
  friend bool operator<=(const UNICHAR* str1, const ByteString& str2);
  friend bool operator>(const UNICHAR* str1, const ByteString& str2);
  friend bool operator>=(const UNICHAR* str1, const ByteString& str2);

  int16 ToInt16(bool* ok = nullptr, int32 base = 10) const;
  uint16 ToUInt16(bool* ok = nullptr, int32 base = 10) const;
  int32 ToInt32(bool* ok = nullptr, int32 base = 10) const;
  uint32 ToUInt32(bool* ok = nullptr, int32 base = 10) const;
  int64 ToInt64(bool* ok = nullptr, int32 base = 10) const;
  uint64 ToUInt64(bool* ok = nullptr, int32 base = 10) const;
  float ToFloat(bool* ok = nullptr) const;
  double ToDouble(bool* ok = nullptr) const;

  ByteString& SetNumber(int16 value, int32 base = 10);
  ByteString& SetNumber(uint16 value, int32 base = 10);
  ByteString& SetNumber(int32 value, int32 base = 10);
  ByteString& SetNumber(uint32 value, int32 base = 10);
  ByteString& SetNumber(int64 value, int32 base = 10);
  ByteString& SetNumber(uint64 value, int32 base = 10);
  ByteString& SetNumber(float value, char F = 'g', int32 precision = 6);
  ByteString& SetNumber(double value, char F = 'g', int32 precision = 6);

  inline ByteString& AppendNumber(int16 value, int32 base = 10) {
    return (*this << ByteString().SetNumber(value, base));
  }
  inline ByteString& AppendNumber(uint16 value, int32 base = 10) {
    return (*this << ByteString().SetNumber(value, base));
  }
  inline ByteString& AppendNumber(int32 value, int32 base = 10) {
    return (*this << ByteString().SetNumber(value, base));
  }
  inline ByteString& AppendNumber(uint32 value, int32 base = 10) {
    return (*this << ByteString().SetNumber(value, base));
  }
  inline ByteString& AppendNumber(int64 value, int32 base = 10) {
    return (*this << ByteString().SetNumber(value, base));
  }
  inline ByteString& AppendNumber(uint64 value, int32 base = 10) {
    return (*this << ByteString().SetNumber(value, base));
  }
  inline ByteString& AppendNumber(float value, char F = 'g',
                                  int32 precision = 6) {
    return (*this << ByteString().SetNumber(value, F, precision));
  }
  inline ByteString& AppendNumber(double value, char F = 'g',
                                  int32 precision = 6) {
    return (*this << ByteString().SetNumber(value, F, precision));
  }

  static ByteString FromNumber(int16 value, int32 base = 10);
  static ByteString FromNumber(uint16 value, int32 base = 10);
  static ByteString FromNumber(int32 value, int32 base = 10);
  static ByteString FromNumber(uint32 value, int32 base = 10);
  static ByteString FromNumber(int64 value, int32 base = 10);
  static ByteString FromNumber(uint64 value, int32 base = 10);
  static ByteString FromNumber(float value, char F = 'g', int32 precision = 6);
  static ByteString FromNumber(double value, char F = 'g', int32 precision = 6);

  ByteString& PathAppend(ByteStringView str);

  inline ByteString& PathAppend(char ch) {
    return PathAppend(ByteStringView(&ch, 1));
  }
  inline ByteString& PathAppend(const char* str, int32 len) {
    return PathAppend(ByteStringView(str, len));
  }
  inline ByteString& PathAppend(AsciiString str) {
    return PathAppend(ByteStringView(str.ConstData(), str.Len()));
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline ByteString& PathAppend(UStringView str) {
    return PathAppend(UNICHAR_TO_UTF8_BUFFER(str).ToView());
  }
  inline ByteString& PathAppend(const UNICHAR* str, int32 len) {
    return PathAppend(UStringView(str, len));
  }
#endif

  bool IsPathTerminated() const;
  ByteString ToPathTerminated() const;

  // ByteStringView로 const char* / ByteString 을 구분하지 않고 처리하면,
  // 컴파일러가 제대로 해석을 하지 못하는 문제가 있어서, 각각의 타입을 별개로
  //처리해주어야함.

  inline ByteString& operator/=(char ch) { return PathAppend(ch); }
  inline ByteString& operator/=(const char* str) { return PathAppend(str); }
  inline ByteString& operator/=(const ByteString& str) {
    return PathAppend(str);
  }
  inline ByteString& operator/=(ByteStringView str) { return PathAppend(str); }
  inline ByteString& operator/=(AsciiString str) { return PathAppend(str); }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline ByteString& operator/=(UNICHAR unicode_char) {
    return PathAppend(unicode_char);
  }
  inline ByteString& operator/=(const UNICHAR* str) { return PathAppend(str); }
  inline ByteString& operator/=(const UString& str) { return PathAppend(str); }
  inline ByteString& operator/=(UStringView str) { return PathAppend(str); }
#endif

  // ByteStringView로 const char* / ByteString 을 구분하지 않고 처리하면,
  // 컴파일러가 제대로 해석을 하지 못하는 문제가 있어서, 각각의 타입을 별개로
  //처리해주어야함.

  inline ByteString operator/(const ByteString& str) {
    return ByteString(*this) /= str;
  }

  inline ByteString operator/(char ch) const { return ByteString(*this) /= ch; }
  inline friend ByteString operator/(char ch, const ByteString& str) {
    return ByteString(ch) /= str;
  }

  inline ByteString operator/(const char* str) {
    return ByteString(*this) /= str;
  }
  inline friend ByteString operator/(const char* str1, const ByteString& str2) {
    return ByteString(str1) /= str2;
  }

  inline ByteString operator/(const ByteStringRef& str) {
    return ByteString(*this) /= str;
  }
  inline friend ByteString operator/(const ByteStringRef& str1,
                                     const ByteString& str2) {
    return ByteString(str1) /= str2;
  }

  inline ByteString operator/(ByteStringView str) {
    return ByteString(*this) /= str;
  }
  inline friend ByteString operator/(ByteStringView str1,
                                     const ByteString& str2) {
    return ByteString(str1) /= str2;
  }

  inline ByteString operator/(AsciiString str) {
    return ByteString(*this) /= str;
  }
  inline friend ByteString operator/(AsciiString str1, const ByteString& str2) {
    return ByteString(str1) /= str2;
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline ByteString operator/(UNICHAR ch) const {
    return ByteString(*this) /= ch;
  }
  inline friend ByteString operator/(UNICHAR ch, const ByteString& str) {
    return ByteString(ch) /= str;
  }

  inline ByteString operator/(const UNICHAR* str) {
    return ByteString(*this) /= str;
  }
  inline friend ByteString operator/(const UNICHAR* str1,
                                     const ByteString& str2) {
    return ByteString(str1) /= str2;
  }

  inline ByteString operator/(const UString& str) {
    return ByteString(*this) /= str;
  }
  inline friend ByteString operator/(const UString& str1,
                                     const ByteString& str2) {
    return ByteString(str1) /= str2;
  }

  inline ByteString operator/(const UStringRef& str) {
    return ByteString(*this) /= str;
  }
  inline friend ByteString operator/(const UStringRef& str1,
                                     const ByteString& str2) {
    return ByteString(str1) /= str2;
  }

  inline ByteString operator/(UStringView str) {
    return ByteString(*this) /= str;
  }
  inline friend ByteString operator/(UStringView str1, const ByteString& str2) {
    return ByteString(str1) /= str2;
  }
#endif

  bool IsAscii() const;

  ByteString ToBase64(Base64Options options) const;
  ByteString ToBase64() const;
  static ByteString FromBase64(const ByteString& base64, Base64Options options);
  static ByteString FromBase64(const ByteString& base64);

  ByteString ToHex() const;
  ByteString ToHex(char separator) const;
  static ByteString FromHex(const ByteString& hex_encoded);

  ByteString ToPercentEncoding(const ByteString& exclude = ByteString(),
                               const ByteString& include = ByteString(),
                               char percent = '%') const;
  static ByteString FromPercentEncoding(const ByteString& pct_encoded,
                                        char percent = '%');

  ByteString ToHtmlEscaped() const;

  bool IsNumeric() const;
  bool IsIdentifier() const;

  bool IsQuoted() const;
  static bool IsQuoted(ByteStringView str);
  ByteString Unquoted(bool* out_quotes_removed = nullptr) const;
  ByteString& Unquotes(bool* out_quotes_removed = nullptr);
  ByteStringRef UnquotedRef(bool* out_quotes_removed = nullptr) const;

  ByteString ReplaceQuotesWithEscapedQuotes() const;
  ByteString ReplaceCharWithEscapedChar(
      const Array<char>* chars = nullptr) const;
  ByteString ReplaceEscapedCharWithChar(
      const Array<char>* chars = nullptr) const;

  ByteString ConvertTabsToSpaces(int32 spaces_per_tab) const;

  static ByteString Chr(char ch) { return ByteString(ch); }
  static ByteString ChrN(char ch, int32 count) { return ByteString(count, ch); }

  ByteString& SetRawData(const char* raw, int32 len);
  static ByteString FromRawData(const char* raw, int32 len);

  // VARARG_DECL(static ByteString, static ByteString, return, Printf,
  // VARARG_NONE, const char*, VARARG_NONE, VARARG_NONE);
  // TODO
  // template <typename Args...>
  // static UString Format(Args args...)
  static ByteString Format(const char* fmt, ...) {
    fun_check(!"TODO");
    return ByteString();
  }

  template <typename... Args>
  ByteString& Appendf(const Args&... args) {
    fun_check(!"TODO");
    return *this;
  }

  // STL compatibilities

  typedef char* iterator;
  typedef const char* const_iterator;
  typedef iterator Iterator;
  typedef const_iterator ConstIterator;
  typedef std::reverse_iterator<iterator> reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

  typedef int32 size_type;
  typedef intptr_t difference_type;
  typedef const char& const_reference;
  typedef char& reference;
  typedef char* pointer;
  typedef const char* const_pointer;
  typedef char value_type;

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

  inline void push_back(char ch) { Append(ch); }
  inline void push_back(ByteStringView str) { Append(str); }
  inline void push_back(AsciiString str) { Append(str); }
  inline void push_front(char ch) { Prepend(ch); }
  inline void push_front(ByteStringView str) { Prepend(str); }
  inline void push_front(AsciiString str) { Prepend(str); }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline void push_back(UNICHAR ch) { Append(ch); }
  inline void push_front(UNICHAR ch) { Prepend(ch); }
  inline void push_back(UStringView str) { Append(str); }
  inline void push_front(UStringView str) { Prepend(str); }
#endif
  inline void shrink_to_fit() { Shrink(); }

  static ByteString FromStdString(const std::string& std_str);
  std::string ToStdString() const;

 private:
  // sharable data block. with COW (Copy On Write)
  Data* data_;

  void ReallocateData(int32 new_alloc, Data::AllocationOptions options);
  void ExpandAt(int32 pos);

 public:
  FUN_BASE_API friend uint32 HashOf(const ByteString&);
  FUN_BASE_API friend Archive& operator&(Archive& ar, ByteString&);

  friend class ByteStringRef;
  friend class UString;

 public:
  // Debugging helper
  static void BytesToDebuggableString(ByteString& output,
                                      const ByteString& data,
                                      bool ascii_only = false,
                                      const ByteString& indent = ByteString());
};

/**
 * 메모리 복사가 없음. 단, 메모리는 소유자(ByteString)에 의해서 관리되어야함.
 */
class FUN_BASE_API ByteStringRef {
 public:
  typedef char CharType;

  ByteStringRef();
  ByteStringRef(const ByteString* str, int32 pos, int32 len);
  ByteStringRef(const ByteString* str);
  ByteStringRef(const ByteStringRef& rhs);
  ByteStringRef& operator=(const ByteStringRef& rhs);
  ByteStringRef(ByteStringRef&& rhs);
  ByteStringRef& operator=(ByteStringRef&& rhs);
  ByteStringRef& operator=(const ByteString* str);

  void Swap(ByteStringRef& rhs);

  const ByteString* Str() const;
  int32 Position() const;
  int32 Len() const;
  bool IsEmpty() const;
  void Clear();
  bool IsNull() const;

  const char* operator*() const;
  const char* ConstData() const;

  ByteString ToString() const;
  ByteStringRef AppendTo(ByteString* str) const;

  char At(int32 index) const;
  char operator[](int32 index) const;

  char First() const;
  char Last() const;

  char FirstOr(const char def = char('\0')) const;
  char LastOr(const char def = char('\0')) const;

  int32 IndexOfAny(ByteStringView chars,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;
  int32 IndexOfAny(AsciiString chars,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;
  int32 IndexOfAny(const Array<char>& chars,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;
  inline int32 IndexOfAny(
      std::initializer_list<char> chars,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_index = nullptr,
      int32* matched_len = nullptr) const {
    return IndexOfAny(ByteStringView(chars.begin(), chars.size()), casesense,
                      from, matched_index, matched_len);
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline int32 IndexOfAny(
      UStringView chars,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_index = nullptr,
      int32* matched_len = nullptr) const {
    return IndexOfAny(UNICHAR_TO_UTF8_BUFFER(chars).ToView(), casesense, from,
                      matched_index, matched_len);
  }
#endif

  int32 LastIndexOfAny(
      ByteStringView chars,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = -1, int32* matched_index = nullptr,
      int32* matched_len = nullptr) const;
  int32 LastIndexOfAny(
      AsciiString chars,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = -1, int32* matched_index = nullptr,
      int32* matched_len = nullptr) const;
  int32 LastIndexOfAny(
      const Array<char>& chars,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = -1, int32* matched_index = nullptr,
      int32* matched_len = nullptr) const;
  inline int32 LastIndexOfAny(
      std::initializer_list<char> chars,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = -1, int32* matched_index = nullptr,
      int32* matched_len = nullptr) const {
    return LastIndexOfAny(ByteStringView(chars.begin(), chars.size()),
                          casesense, from, matched_index, matched_len);
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline int32 LastIndexOfAny(
      UStringView chars,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = -1, int32* matched_index = nullptr,
      int32* matched_len = nullptr) const {
    return LastIndexOfAny(UNICHAR_TO_UTF8_BUFFER(chars).ToView(), casesense,
                          from, matched_index, matched_len);
  }
#endif

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
  int32 IndexOfAny(const Array<AsciiString>& strings,
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
  int32 IndexOfAny(std::initializer_list<AsciiString> strings,
                   CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                   int32 from = 0, int32* matched_index = nullptr,
                   int32* matched_len = nullptr) const;
#if FUN_USE_IMPLICIT_STRING_CONVERSION
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
#endif

  int32 IndexOf(ByteStringView sub,
                CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                int32 from = 0, int32* matched_len = nullptr) const;

  inline int32 IndexOf(
      char ch, CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_len = nullptr) const {
    return IndexOf(ByteStringView(&ch, 1), casesense, from, matched_len);
  }
  inline int32 IndexOf(
      const char* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_len = nullptr) const {
    return IndexOf(ByteStringView(sub, sub_len), casesense, from, matched_len);
  }
  inline int32 IndexOf(
      AsciiString sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_len = nullptr) const {
    return IndexOf(ByteStringView(sub.ConstData(), sub.Len()), casesense, from,
                   matched_len);
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline int32 IndexOf(
      UNICHAR ch, CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_len = nullptr) const {
    return IndexOf(UNICHAR_TO_UTF8_BUFFER(&ch, 1).ToView(), casesense, from,
                   matched_len);
  }
  inline int32 IndexOf(
      const UNICHAR* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_len = nullptr) const {
    return IndexOf(UNICHAR_TO_UTF8_BUFFER(sub, sub_len).ToView(), casesense,
                   from, matched_len);
  }
  inline int32 IndexOf(
      UStringView sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_len = nullptr) const {
    return IndexOf(UNICHAR_TO_UTF8_BUFFER(sub).ToView(), casesense, from,
                   matched_len);
  }
#endif

  template <typename Predicate>
  int32 IndexOfIf(const Predicate& pred, int32 from = 0) const;

  int32 LastIndexOf(ByteStringView sub,
                    CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
                    int32 from = -1, int32* matched_len = nullptr) const;

  inline int32 LastIndexOf(
      char ch, CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = -1, int32* matched_len = nullptr) const {
    return LastIndexOf(ByteStringView(&ch, 1), casesense, from, matched_len);
  }
  inline int32 LastIndexOf(
      const char* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = -1, int32* matched_len = nullptr) const {
    return LastIndexOf(ByteStringView(sub, sub_len), casesense, from,
                       matched_len);
  }
  inline int32 LastIndexOf(
      AsciiString sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = -1, int32* matched_len = nullptr) const {
    return LastIndexOf(ByteStringView(sub.ConstData(), sub.Len()), casesense,
                       from, matched_len);
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline int32 LastIndexOf(
      UNICHAR ch, CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = -1, int32* matched_len = nullptr) const {
    return LastIndexOf(UNICHAR_TO_UTF8_BUFFER(&ch, 1).ToView(), casesense, from,
                       matched_len);
  }
  inline int32 LastIndexOf(
      const UNICHAR* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = -1, int32* matched_len = nullptr) const {
    return LastIndexOf(UNICHAR_TO_UTF8_BUFFER(sub, sub_len).ToView(), casesense,
                       from, matched_len);
  }
  inline int32 LastIndexOf(
      UStringView sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = -1, int32* matched_len = nullptr) const {
    return LastIndexOf(UNICHAR_TO_UTF8_BUFFER(sub).ToView(), casesense, from,
                       matched_len);
  }
#endif

  template <typename Predicate>
  int32 LastIndexOfIf(const Predicate& pred, int32 from = -1) const;

  inline bool Contains(
      char ch, CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_len = nullptr) const {
    return IndexOf(ch, casesense, from, matched_len) != INVALID_INDEX;
  }
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
  inline bool Contains(
      AsciiString sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive,
      int32 from = 0, int32* matched_len = nullptr) const {
    return IndexOf(sub, casesense, from, matched_len) != INVALID_INDEX;
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
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
#endif

  int32 Count(ByteStringView sub,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  inline int32 Count(char ch, CaseSensitivity casesense =
                                  CaseSensitivity::CaseSensitive) const {
    return Count(ByteStringView(&ch, 1), casesense);
  }
  inline int32 Count(
      const char* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Count(ByteStringView(sub, sub_len), casesense);
  }
  inline int32 Count(
      AsciiString sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Count(ByteStringView(sub.ConstData(), sub.Len()), casesense);
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline int32 Count(UNICHAR ch, CaseSensitivity casesense =
                                     CaseSensitivity::CaseSensitive) const {
    return Count(UNICHAR_TO_UTF8_BUFFER(&ch, 1).ToView(), casesense);
  }
  inline int32 Count(
      const UNICHAR* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Count(UNICHAR_TO_UTF8_BUFFER(sub, sub_len).ToView(), casesense);
  }
  inline int32 Count(
      UStringView sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Count(UNICHAR_TO_UTF8_BUFFER(sub).ToView(), casesense);
  }
#endif

  // Substrings
  ByteStringRef Left(int32 len) const;
  ByteStringRef Mid(int32 offset, int32 len = int32_MAX) const;
  ByteStringRef Right(int32 len) const;
  ByteStringRef LeftChopped(int32 len) const;
  ByteStringRef RightChopped(int32 len) const;

  bool StartsWith(ByteStringView sub, CaseSensitivity casesense =
                                          CaseSensitivity::CaseSensitive) const;

  inline bool StartsWith(char ch, CaseSensitivity casesense =
                                      CaseSensitivity::CaseSensitive) const {
    return StartsWith(ByteStringView(&ch, 1), casesense);
  }
  inline bool StartsWith(
      const char* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return StartsWith(ByteStringView(sub, sub_len), casesense);
  }
  inline bool StartsWith(
      AsciiString sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return StartsWith(ByteStringView(sub.ConstData(), sub.Len()), casesense);
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline bool StartsWith(UNICHAR ch, CaseSensitivity casesense =
                                         CaseSensitivity::CaseSensitive) const {
    return StartsWith(UNICHAR_TO_UTF8_BUFFER(&ch, 1).ToView(), casesense);
  }
  inline bool StartsWith(
      const UNICHAR* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return StartsWith(UNICHAR_TO_UTF8_BUFFER(sub, sub_len).ToView(), casesense);
  }
  inline bool StartsWith(
      UStringView sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return StartsWith(UNICHAR_TO_UTF8_BUFFER(sub).ToView(), casesense);
  }
#endif

  bool EndsWith(ByteStringView sub, CaseSensitivity casesense =
                                        CaseSensitivity::CaseSensitive) const;

  inline bool EndsWith(char ch, CaseSensitivity casesense =
                                    CaseSensitivity::CaseSensitive) const {
    return EndsWith(ByteStringView(&ch, 1), casesense);
  }
  inline bool EndsWith(
      const char* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return EndsWith(ByteStringView(sub, sub_len), casesense);
  }
  inline bool EndsWith(
      AsciiString sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return EndsWith(ByteStringView(sub.ConstData(), sub.Len()), casesense);
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline bool EndsWith(UNICHAR ch, CaseSensitivity casesense =
                                       CaseSensitivity::CaseSensitive) const {
    return EndsWith(UNICHAR_TO_UTF8_BUFFER(&ch, 1).ToView(), casesense);
  }
  inline bool EndsWith(
      const UNICHAR* sub, int32 sub_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return EndsWith(UNICHAR_TO_UTF8_BUFFER(sub, sub_len).ToView(), casesense);
  }
  inline bool EndsWith(
      UStringView sub,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return EndsWith(UNICHAR_TO_UTF8_BUFFER(sub).ToView(), casesense);
  }
#endif

  bool GlobMatch(
      ByteStringView pattern,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  inline bool GlobMatch(
      const char* pattern, int32 pattern_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return GlobMatch(ByteStringView(pattern, pattern_len), casesense);
  }
  inline bool GlobMatch(
      AsciiString pattern,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return GlobMatch(ByteStringView(pattern.ConstData(), pattern.Len()),
                     casesense);
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  bool GlobMatch(
      const UNICHAR* pattern, int32 pattern_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return GlobMatch(UNICHAR_TO_UTF8_BUFFER(pattern, pattern_len).ToView(),
                     casesense);
  }
  inline bool GlobMatch(
      UStringView pattern,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return GlobMatch(UNICHAR_TO_UTF8_BUFFER(pattern).ToView(), casesense);
  }
#endif

  ByteStringRef& Truncate(int32 pos);
  ByteStringRef& LeftChop(int32 len);
  ByteStringRef& RightChop(int32 len);

  ByteStringRef& TrimLeft();
  ByteStringRef& TrimRight();
  ByteStringRef& Trim();
  ByteStringRef TrimmedLeft() const;
  ByteStringRef TrimmedRight() const;
  ByteStringRef Trimmed() const;
  int32 LeftSpaces() const;
  int32 RightSpaces() const;
  int32 SideSpaces() const;

  Array<ByteStringRef> Split(
      char separator, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  Array<ByteStringRef> Split(
      const Array<char>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteStringRef> Split(
      std::initializer_list<char> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteStringRef> Split(
      ByteStringView separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteStringRef> Split(
      AsciiString separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  Array<ByteStringRef> Split(
      const Array<const char*>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteStringRef> Split(
      const Array<ByteString>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteStringRef> Split(
      const Array<ByteStringRef>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteStringRef> Split(
      const Array<ByteStringView>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteStringRef> Split(
      const Array<AsciiString>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  Array<ByteStringRef> Split(
      std::initializer_list<const char*> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteStringRef> Split(
      std::initializer_list<ByteString> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteStringRef> Split(
      std::initializer_list<ByteStringRef> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteStringRef> Split(
      std::initializer_list<ByteStringView> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteStringRef> Split(
      std::initializer_list<AsciiString> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  Array<ByteStringRef> SplitByWhitespaces(
      ByteString extra_separator = ByteString(), int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteStringRef> SplitLines(
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

#if FUN_USE_IMPLICIT_STRING_CONVERSION
  Array<ByteStringRef> Split(
      UNICHAR separator, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  Array<ByteStringRef> Split(
      const Array<UNICHAR>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteStringRef> Split(
      std::initializer_list<UNICHAR> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteStringRef> Split(
      UStringView separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  Array<ByteStringRef> Split(
      const Array<const UNICHAR*>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteStringRef> Split(
      const Array<UString>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteStringRef> Split(
      const Array<UStringRef>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteStringRef> Split(
      const Array<UStringView>& separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  Array<ByteStringRef> Split(
      std::initializer_list<const UNICHAR*> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteStringRef> Split(
      std::initializer_list<UString> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteStringRef> Split(
      std::initializer_list<UStringRef> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  Array<ByteStringRef> Split(
      std::initializer_list<UStringView> separators, int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
#endif

  int32 Split(Array<ByteStringRef>& list, char separator, int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  int32 Split(Array<ByteStringRef>& list, const Array<char>& separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<ByteStringRef>& list,
              std::initializer_list<char> separators, int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<ByteStringRef>& list, ByteStringView separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<ByteStringRef>& list, AsciiString separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  int32 Split(Array<ByteStringRef>& list, const Array<const char*>& separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<ByteStringRef>& list, const Array<ByteString>& separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<ByteStringRef>& list,
              const Array<ByteStringRef>& separators, int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<ByteStringRef>& list,
              const Array<ByteStringView>& separators, int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<ByteStringRef>& list, const Array<AsciiString>& separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  int32 Split(Array<ByteStringRef>& list,
              std::initializer_list<const char*> separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<ByteStringRef>& list,
              std::initializer_list<ByteString> separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<ByteStringRef>& list,
              std::initializer_list<ByteStringRef> separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<ByteStringRef>& list,
              std::initializer_list<ByteStringView> separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<ByteStringRef>& list,
              std::initializer_list<AsciiString> separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  int32 SplitByWhitespaces(
      Array<ByteStringRef>& list, ByteString extra_separator = ByteString(),
      int32 max_splits = 0,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 SplitLines(
      Array<ByteStringRef>& out_lines,
      StringSplitOptions split_options = StringSplitOption::None,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

#if FUN_USE_IMPLICIT_STRING_CONVERSION
  int32 Split(Array<ByteStringRef>& list, UNICHAR separator,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  int32 Split(Array<ByteStringRef>& list, const Array<UNICHAR>& separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<ByteStringRef>& list,
              std::initializer_list<UNICHAR> separators, int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<ByteStringRef>& list, UStringView separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  int32 Split(Array<ByteStringRef>& list,
              const Array<const UNICHAR*>& separators, int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<ByteStringRef>& list, const Array<UString>& separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<ByteStringRef>& list, const Array<UStringRef>& separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<ByteStringRef>& list, const Array<UStringView>& separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  int32 Split(Array<ByteStringRef>& list,
              std::initializer_list<const UNICHAR*> separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<ByteStringRef>& list,
              std::initializer_list<UString> separators, int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<ByteStringRef>& list,
              std::initializer_list<UStringRef> separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  int32 Split(Array<ByteStringRef>& list,
              std::initializer_list<UStringView> separators,
              int32 max_splits = 0,
              StringSplitOptions split_options = StringSplitOption::None,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
#endif

  bool Divide(ByteStringView delim, ByteStringRef* out_left,
              ByteStringRef* out_right, bool trimming = false,
              CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  inline bool Divide(
      char delim_char, ByteStringRef* out_left, ByteStringRef* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Divide(ByteStringView(&delim_char, 1), out_left, out_right, trimming,
                  casesense);
  }
  inline bool Divide(
      const char* delim, int32 delim_len, ByteStringRef* out_left,
      ByteStringRef* out_right, bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Divide(ByteStringView(delim, delim_len), out_left, out_right,
                  trimming, casesense);
  }
  inline bool Divide(
      AsciiString delim, ByteStringRef* out_left, ByteStringRef* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Divide(ByteStringView(delim.ConstData(), delim.Len()), out_left,
                  out_right, trimming, casesense);
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline bool Divide(
      UNICHAR delim_char, ByteStringRef* out_left, ByteStringRef* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Divide(UNICHAR_TO_UTF8_BUFFER(&delim_char, 1).ToView(), out_left,
                  out_right, trimming, casesense);
  }
  inline bool Divide(
      const UNICHAR* delim, int32 delim_len, ByteStringRef* out_left,
      ByteStringRef* out_right, bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Divide(UNICHAR_TO_UTF8_BUFFER(delim, delim_len).ToView(), out_left,
                  out_right, trimming, casesense);
  }
  inline bool Divide(
      UStringView delim, ByteStringRef* out_left, ByteStringRef* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return Divide(UNICHAR_TO_UTF8_BUFFER(delim).ToView(), out_left, out_right,
                  trimming, casesense);
  }
#endif

  bool LastDivide(
      ByteStringView delim, ByteStringRef* out_left, ByteStringRef* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;

  inline bool LastDivide(
      char delim_char, ByteStringRef* out_left, ByteStringRef* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return LastDivide(ByteStringView(&delim_char, 1), out_left, out_right,
                      trimming, casesense);
  }
  inline bool LastDivide(
      const char* delim, int32 delim_len, ByteStringRef* out_left,
      ByteStringRef* out_right, bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return LastDivide(ByteStringView(delim, delim_len), out_left, out_right,
                      trimming, casesense);
  }
  inline bool LastDivide(
      AsciiString delim, ByteStringRef* out_left, ByteStringRef* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return LastDivide(ByteStringView(delim.ConstData(), delim.Len()), out_left,
                      out_right, trimming, casesense);
  }
#if FUN_USE_IMPLICIT_STRING_CONVERSION
  inline bool LastDivide(
      UNICHAR delim_char, ByteStringRef* out_left, ByteStringRef* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return LastDivide(UNICHAR_TO_UTF8_BUFFER(&delim_char, 1).ToView(), out_left,
                      out_right, trimming, casesense);
  }
  inline bool LastDivide(
      const UNICHAR* delim, int32 delim_len, ByteStringRef* out_left,
      ByteStringRef* out_right, bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return LastDivide(UNICHAR_TO_UTF8_BUFFER(delim, delim_len).ToView(),
                      out_left, out_right, trimming, casesense);
  }
  inline bool LastDivide(
      UStringView delim, ByteStringRef* out_left, ByteStringRef* out_right,
      bool trimming = false,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const {
    return LastDivide(
        UNICHAR_TO_UTF8_BUFFER(delim.ConstData(), delim.Len()).ToView(),
        out_left, out_right, trimming, casesense);
  }
#endif

  // ByteStringRef and ByteStringRef
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

  // ByteStringRef and ByteString
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

  // ByteStringRef and ByteStringView
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

  // ByteStringRef and AsciiString
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

  // ByteStringRef and UString
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

  // ByteStringRef and UStringRef
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

  // ByteStringRef and UStringView
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

  // ByteStringRef and char
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
  friend bool operator==(char ch, const ByteStringRef& str);
  friend bool operator!=(char ch, const ByteStringRef& str);
  friend bool operator<(char ch, const ByteStringRef& str);
  friend bool operator<=(char ch, const ByteStringRef& str);
  friend bool operator>(char ch, const ByteStringRef& str);
  friend bool operator>=(char ch, const ByteStringRef& str);

  // ByteStringRef and const char*
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
  friend bool operator==(const char* str1, const ByteStringRef& str2);
  friend bool operator!=(const char* str1, const ByteStringRef& str2);
  friend bool operator<(const char* str1, const ByteStringRef& str2);
  friend bool operator<=(const char* str1, const ByteStringRef& str2);
  friend bool operator>(const char* str1, const ByteStringRef& str2);
  friend bool operator>=(const char* str1, const ByteStringRef& str2);

  // ByteStringRef and UNICHAR
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
  friend bool operator==(UNICHAR ch, const ByteStringRef& str);
  friend bool operator!=(UNICHAR ch, const ByteStringRef& str);
  friend bool operator<(UNICHAR ch, const ByteStringRef& str);
  friend bool operator<=(UNICHAR ch, const ByteStringRef& str);
  friend bool operator>(UNICHAR ch, const ByteStringRef& str);
  friend bool operator>=(UNICHAR ch, const ByteStringRef& str);

  // ByteStringRef and const UNICHAR*
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
  friend bool operator==(const UNICHAR* str1, const ByteStringRef& str2);
  friend bool operator!=(const UNICHAR* str1, const ByteStringRef& str2);
  friend bool operator<(const UNICHAR* str1, const ByteStringRef& str2);
  friend bool operator<=(const UNICHAR* str1, const ByteStringRef& str2);
  friend bool operator>(const UNICHAR* str1, const ByteStringRef& str2);
  friend bool operator>=(const UNICHAR* str1, const ByteStringRef& str2);

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
  ByteStringRef Unquoted(bool* out_quotes_removed = nullptr) const;
  ByteStringRef& Unquotes(bool* out_quotes_removed = nullptr);

  bool IsAscii() const;

  // STL compatibilities

  typedef int32 size_type;
  typedef intptr_t difference_type;
  typedef const char& const_reference;
  typedef char& reference;
  typedef char* pointer;
  typedef const char* const_pointer;
  typedef char value_type;

  typedef const char* const_iterator;
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

  inline std::string ToStdString() const {
    return std::string(ConstData(), Len());
  }

 private:
  FUN_BASE_API friend uint32 HashOf(const ByteStringRef&);
  // TODO Saving만 지원할까??
  // FUN_BASE_API friend Archive& operator & (Archive& ar, ByteStringRef&);

 private:
  friend class ByteString;

  const ByteString* string_;
  int32 position_;
  int32 length_;

  void CheckInvariants();
};

int32 CullArray(Array<ByteString>* array, bool trimming = false);
int32 CullArray(Array<ByteStringRef>* array, bool trimming = false);

// TODO friend로만 해주면 Set<T>에서 문법(오버로딩이 안되었다며..) 오류가 난다.
FUN_BASE_API uint32 HashOf(const ByteString&);
FUN_BASE_API uint32 HashOf(const ByteStringRef&);

}  // namespace fun
