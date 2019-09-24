#pragma once

#include "fun/base/base.h"
#include "fun/base/string/utf.h"

namespace fun {

class FUN_BASE_API AsciiString {
 public:
  inline AsciiString() : data_(nullptr), length_(0) {}
  explicit inline AsciiString(const char* str)
      : data_(str), length_(CStringTraitsA::Strlen(str)) {}
  explicit inline AsciiString(const char* str, int32 len)
      : data_(str), length_(len) {}
  explicit inline AsciiString(const ByteString& str);

  // TODO Latin1String이하는걸 하나더 만드는게 좋으려나???
  explicit inline AsciiString(const uint8* str)
      : data_((const char*)str),
        length_(CStringTraitsA::Strlen((const char*)str)) {}
  explicit inline AsciiString(const uint8* str, int32 len)
      : data_((const char*)str), length_(len) {}

  inline const char* ConstData() const { return data_; }
  inline const char* ANSI() const { return data_; }

  inline char At(int32 index) const {
    fun_check(index >= 0);
    fun_check(index < Len());
    return data_[index];
  }
  inline char operator[](int32 index) const { return At(index); }

  inline char First() const { return At(0); }
  inline char Last() const { return At(Len() - 1); }

  inline char FirstOr(const char def = char(0)) const {
    return Len() ? First() : def;
  }
  inline char LastOr(const char def = char(0)) const {
    return Len() ? Last() : def;
  }

  inline AsciiString Left(int32 len) const {
    fun_check(len >= 0);
    fun_check(len <= Len());
    return AsciiString(data_, len);
  }

  inline AsciiString Right(int32 len) const {
    fun_check(len >= 0);
    fun_check(len <= Len());
    return AsciiString(data_ + length_ - len, len);
  }

  inline AsciiString Mid(int32 offset, int32 len) const {
    fun_check(offset >= 0);
    fun_check(offset < length_);
    fun_check((offset + len) <= length_);
    return AsciiString(data_ + offset, len);
  }

  inline void Clear() {
    data_ = nullptr;
    length_ = 0;
  }
  inline int32 Len() const { return length_; }
  inline bool IsNull() const { return data_ == nullptr; }
  inline bool IsEmpty() const { return length_ == 0; }

  inline AsciiString& LeftChop(int32 len) {
    fun_check(len <= length_);
    length_ -= len;
    return *this;
  }

  inline AsciiString& RightChop(int32 len) {
    fun_check(len <= length_);
    data_ += len;
    length_ -= len;
    return *this;
  }

  inline AsciiString& Truncate(int32 pos) {
    fun_check(pos >= 0);
    fun_check(pos <= Len());
    length_ = pos;
    return *this;
  }

  bool StartsWith(UNICHAR ch, CaseSensitivity casesense =
                                  CaseSensitivity::CaseSensitive) const;
  bool StartsWith(UStringView sub, CaseSensitivity casesense =
                                       CaseSensitivity::CaseSensitive) const;

  bool EndsWith(UNICHAR ch, CaseSensitivity casesense =
                                CaseSensitivity::CaseSensitive) const;
  bool EndsWith(UStringView sub, CaseSensitivity casesense =
                                     CaseSensitivity::CaseSensitive) const;

  // STL compatibilities

  using value_type = const char;
  using reference = value_type&;
  using const_reference = reference;
  using iterator = value_type*;
  using const_iterator = iterator;
  using difference_type = int;  // violates container concept requirements
  using size_type = int;        // violates container concept requirements

  inline const_iterator begin() const { return ConstData(); }
  inline const_iterator cbegin() const { return ConstData(); }
  inline const_iterator end() const { return ConstData() + Len(); }
  inline const_iterator cend() const { return ConstData() + Len(); }

  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = reverse_iterator;

  inline const_reverse_iterator rbegin() const {
    return const_reverse_iterator(end());
  }
  inline const_reverse_iterator crbegin() const {
    return const_reverse_iterator(end());
  }
  inline const_reverse_iterator rend() const {
    return const_reverse_iterator(begin());
  }
  inline const_reverse_iterator crend() const {
    return const_reverse_iterator(begin());
  }

  // AsciiString and ByteString
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

  // AsciiString and ByteStringRef
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

  // AsciiString and ByteStringView
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

  // AsciiString and UString
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

  // AsciiString and UStringRef
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

  // AsciiString and UStringView
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

  // AsciiString and char
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
  friend bool operator==(char ch, AsciiString str);
  friend bool operator!=(char ch, AsciiString str);
  friend bool operator<(char ch, AsciiString str);
  friend bool operator<=(char ch, AsciiString str);
  friend bool operator>(char ch, AsciiString str);
  friend bool operator>=(char ch, AsciiString str);

  // AsciiString and const char*
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
  friend bool operator==(const char* str1, AsciiString str2);
  friend bool operator!=(const char* str1, AsciiString str2);
  friend bool operator<(const char* str1, AsciiString str2);
  friend bool operator<=(const char* str1, AsciiString str2);
  friend bool operator>(const char* str1, AsciiString str2);
  friend bool operator>=(const char* str1, AsciiString str2);

  // AsciiString and UNICHAR
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
  friend bool operator==(UNICHAR ch, AsciiString str);
  friend bool operator!=(UNICHAR ch, AsciiString str);
  friend bool operator<(UNICHAR ch, AsciiString str);
  friend bool operator<=(UNICHAR ch, AsciiString str);
  friend bool operator>(UNICHAR ch, AsciiString str);
  friend bool operator>=(UNICHAR ch, AsciiString str);

  // AsciiString and const UNICHAR*
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
  friend bool operator==(const UNICHAR* str1, AsciiString str2);
  friend bool operator!=(const UNICHAR* str1, AsciiString str2);
  friend bool operator<(const UNICHAR* str1, AsciiString str2);
  friend bool operator<=(const UNICHAR* str1, AsciiString str2);
  friend bool operator>(const UNICHAR* str1, AsciiString str2);
  friend bool operator>=(const UNICHAR* str1, AsciiString str2);

  FUN_BASE_API uint32 HashOf(const AsciiString& str);

 private:
  const char* data_;
  int32 length_;
};

}  // namespace fun
