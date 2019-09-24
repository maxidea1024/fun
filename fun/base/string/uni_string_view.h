#pragma once

#include "fun/base/base.h"
#include "fun/base/string/cstring_traits.h"

namespace fun {

namespace UniStringView_internal {

template <typename CharType>
struct IsCompatibleCharTypeHelper
    : IntegralConstant<bool, IsSame<CharType, UNICHAR>::Value ||
                                 IsSame<CharType, int16>::Value ||
                                 IsSame<CharType, uint16>::Value ||
                                 IsSame<CharType, char16_t>::Value ||
                                 (IsSame<CharType, wchar_t>::Value &&
                                  sizeof(wchar_t) == sizeof(UNICHAR))> {};

template <typename CharType>
struct IsCompatibleCharType
    : IsCompatibleCharTypeHelper<
          typename RemoveCV<typename RemoveReference<CharType>::Type>::Type> {};

template <typename Array>
struct IsCompatibleArrayHelper : FalseType {};

template <typename CharType, size_t N>
struct IsCompatibleArrayHelper<CharType[N]> : IsCompatibleCharType<CharType> {};

template <typename Array>
struct IsCompatibleArray
    : IsCompatibleArrayHelper<
          typename RemoveCV<typename RemoveReference<Array>::Type>::Type> {};

template <typename Pointer>
struct IsCompatiblePointerHelper : FalseType {};

template <typename CharType>
struct IsCompatiblePointerHelper<CharType*> : IsCompatibleCharType<CharType> {};

template <typename Pointer>
struct IsCompatiblePointer
    : IsCompatiblePointerHelper<
          typename RemoveCV<typename RemoveReference<Pointer>::Type>::Type> {};

// STL
template <typename T>
struct IsCompatibleStdBasicStringHelper : FalseType {};

template <typename CharType, typename... Args>
struct IsCompatibleStdBasicStringHelper<std::basic_string<CharType, Args...>>
    : IsCompatibleCharType<CharType> {};

template <typename T>
struct IsCompatibleStdBasicString
    : IsCompatibleStdBasicStringHelper<
          typename RemoveCV<typename RemoveReference<T>::Type>::Type> {};

}  // namespace UniStringView_internal

template <typename T>
struct IsSupportStringView
    : IntegralConstant<
          bool,
          UniStringView_internal::IsCompatibleCharType<T>::Value ||
              UniStringView_internal::IsCompatibleArray<T>::Value ||
              UniStringView_internal::IsCompatiblePointer<T>::Value ||
              UniStringView_internal::IsCompatibleStdBasicString<T>::Value ||
              IsSame<T, UString>::Value || IsSame<T, UStringRef>::Value> {};

class UStringView {
 public:
#ifdef _MSC_VER
  typedef wchar_t storage_type;
#else
  typedef char16_t storage_type;
#endif

  typedef const UNICHAR value_type;
  typedef std::ptrdiff_t difference_type;
  // TODO ssize_t가 표준에 포함되어 있지 않음... 문제가 되려나???
  // TODO ssize_t가 표준에 포함되어 있지 않음... 문제가 되려나???
  // TODO ssize_t가 표준에 포함되어 있지 않음... 문제가 되려나???
  // TODO ssize_t가 표준에 포함되어 있지 않음... 문제가 되려나???
  // typedef ssize_t size_type;
  typedef size_t size_type;
  typedef value_type& reference;
  typedef value_type& const_reference;
  typedef value_type* pointer;
  typedef value_type* const_pointer;

  typedef pointer iterator;
  typedef const_pointer const_iterator;
  typedef std::reverse_iterator<iterator> reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

 private:
  template <typename CharType>
  using IfCompatibleChar = typename EnableIf<
      UniStringView_internal::IsCompatibleCharType<CharType>::Value,
      bool>::Type;

  template <typename Array>
  using IfCompatibleArray =
      typename EnableIf<UniStringView_internal::IsCompatibleArray<Array>::Value,
                        bool>::Type;

  template <typename Pointer>
  using IfCompatiblePointer = typename EnableIf<
      UniStringView_internal::IsCompatiblePointer<Pointer>::Value, bool>::Type;

  template <typename T>
  using IfCompatibleStdString = typename EnableIf<
      UniStringView_internal::IsCompatibleStdBasicString<T>::Value, bool>::Type;

  template <typename T>
  using IfCompatibleUStringFamily = typename EnableIf<
      IsSame<T, UString>::Value || IsSame<T, UStringRef>::Value, bool>::Type;

  // std::string("123\0\0") == 3 처럼 동작하도록 함.
  // template <typename CharType, size_t N>
  // static size_t LengthHelperArray(const CharType (&)[N]) noexcept { return N
  // - 1; }
  template <typename CharType, size_t N>
  static size_t LengthHelperArray(const CharType (&array)[N]) noexcept {
    return CStringTraitsU::Strlen(reinterpret_cast<const storage_type*>(array));
  }

  template <typename CharType>
  static size_t LengthHelperPointer(const CharType* str) noexcept {
    return CStringTraitsU::Strlen(reinterpret_cast<const storage_type*>(str));
  }

  template <typename CharType>
  static const storage_type* CastHelper(const CharType* str) noexcept {
    return reinterpret_cast<const storage_type*>(str);
  }

  static const storage_type* CastHelper(const storage_type* str) noexcept {
    return str;
  }

 public:
  UStringView() noexcept : data_(nullptr), length_(0) {}

  UStringView(decltype(nullptr)) : UStringView() {}

  template <typename CharType, IfCompatibleChar<CharType> = true>
  UStringView(const CharType* str, size_t len)
      : data_(CastHelper(str)), length_(len) {
    fun_check(len >= 0);
    fun_check(str || len == 0);
  }

  template <typename Array, IfCompatibleArray<Array> = true>
  UStringView(const Array& str) noexcept
      : UStringView(str, LengthHelperArray(str)) {}

  template <typename Pointer, IfCompatiblePointer<Pointer> = true>
  UStringView(const Pointer& str) noexcept
      : UStringView(str, LengthHelperPointer(str)) {}

  template <typename Str,
            IfCompatibleUStringFamily<Str> = true>  // string_inline.h
  UStringView(const Str& str) throw();

  inline UString ToString() const;  // defined in string_inline.h

  // size_t -> int32 truncation 경고가 너무 많이 나와서...
  // inline size_t Len() const { return length_; }
  inline int32 Len() const { return (int32)length_; }
  inline bool IsNull() const noexcept { return data_ == nullptr; }
  inline bool IsEmpty() const noexcept { return length_ == 0; }
  inline UNICHAR First() const { return At(0); }
  inline UNICHAR Last() const { return At(Len() - 1); }
  inline UNICHAR FirstOr(const UNICHAR def = UNICHAR(0)) const {
    return Len() ? At(0) : def;
  }
  inline UNICHAR LastOr(const UNICHAR def = UNICHAR(0)) const {
    return Len() ? At(Len() - 1) : def;
  }

  inline const_pointer ConstData() const noexcept {
    return reinterpret_cast<const_pointer>(data_);
  }
  inline const storage_type* UTF16() const noexcept { return data_; }

  inline UNICHAR operator[](size_t index) const {
    fun_check(index < Len());
    return UNICHAR(data_[index]);
  }
  inline UNICHAR At(size_t index) const {
    fun_check(index < Len());
    return UNICHAR(data_[index]);
  }

  ByteString ToANSI() const;
  ByteString ToUtf8() const;
  ByteString ToLocal8Bit() const;
  Array<uint32> ToUCS4() const;

  inline UStringView Mid(size_t pos) const {
    fun_check(pos <= length_);
    return UStringView(data_ + pos, length_ - pos);
  }
  inline UStringView Mid(size_t pos, size_t len) const {
    fun_check(pos + len <= length_);
    return UStringView(data_ + pos, len);
  }
  inline UStringView Left(size_t len) {
    fun_check(len <= length_);
    return UStringView(data_, len);
  }
  inline UStringView Right(size_t len) {
    fun_check(len <= length_);
    return UStringView(data_ + length_ - len, len);
  }
  // TODO Chopped

  UStringView& Truncate(size_t pos) {
    fun_check(pos <= Len());
    length_ = pos;
  }

  bool StartsWith(UNICHAR ch, CaseSensitivity casesense =
                                  CaseSensitivity::CaseSensitive) const;
  bool StartsWith(UStringView sub, CaseSensitivity casesense =
                                       CaseSensitivity::CaseSensitive) const;
  bool StartsWith(AsciiString sub, CaseSensitivity casesense =
                                       CaseSensitivity::CaseSensitive) const;

  bool EndsWith(UNICHAR ch, CaseSensitivity casesense =
                                CaseSensitivity::CaseSensitive) const;
  bool EndsWith(UStringView sub, CaseSensitivity casesense =
                                     CaseSensitivity::CaseSensitive) const;
  bool EndsWith(AsciiString sub, CaseSensitivity casesense =
                                     CaseSensitivity::CaseSensitive) const;

  // STL compatibility API:

  inline const_iterator begin() const noexcept { return ConstData(); }
  inline const_iterator end() const noexcept { return ConstData() + size(); }
  inline const_iterator cbegin() const noexcept { return begin(); }
  inline const_iterator cend() const noexcept { return end(); }
  inline const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator(end());
  }
  inline const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator(begin());
  }
  inline const_reverse_iterator crbegin() const noexcept { return rbegin(); }
  inline const_reverse_iterator crend() const noexcept { return rend(); }

  inline bool empty() const noexcept { return size() == 0; }
  inline UNICHAR front() const {
    fun_check(!empty());
    return UNICHAR(data_[0]);
  }
  inline UNICHAR back() const {
    fun_check(!empty());
    return UNICHAR(data_[length_ - 1]);
  }
  inline size_t size() const { return length_; }

  // UStringView and UStringView
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

  // UStringView and ByteString
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

  // UStringView and ByteStringRef
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

  // UStringView and ByteStringView
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

  // UStringView and AsciiString
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

  // UStringView and UString
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

  // UStringView and UStringRef
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

  // UStringView and char
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
  friend bool operator==(UNICHAR ch, const UStringView& str);
  friend bool operator!=(UNICHAR ch, const UStringView& str);
  friend bool operator<(UNICHAR ch, const UStringView& str);
  friend bool operator<=(UNICHAR ch, const UStringView& str);
  friend bool operator>(UNICHAR ch, const UStringView& str);
  friend bool operator>=(UNICHAR ch, const UStringView& str);

  // UStringView and const char*
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
  friend bool operator==(const char* str1, const UStringView& str2);
  friend bool operator!=(const char* str1, const UStringView& str2);
  friend bool operator<(const char* str1, const UStringView& str2);
  friend bool operator<=(const char* str1, const UStringView& str2);
  friend bool operator>(const char* str1, const UStringView& str2);
  friend bool operator>=(const char* str1, const UStringView& str2);

  // UStringView and UNICHAR
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
  friend bool operator==(char ch, const UStringView& str);
  friend bool operator!=(char ch, const UStringView& str);
  friend bool operator<(char ch, const UStringView& str);
  friend bool operator<=(char ch, const UStringView& str);
  friend bool operator>(char ch, const UStringView& str);
  friend bool operator>=(char ch, const UStringView& str);

  // UStringView and const UNICHAR*
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
  friend bool operator==(const UNICHAR* str1, const UStringView& str2);
  friend bool operator!=(const UNICHAR* str1, const UStringView& str2);
  friend bool operator<(const UNICHAR* str1, const UStringView& str2);
  friend bool operator<=(const UNICHAR* str1, const UStringView& str2);
  friend bool operator>(const UNICHAR* str1, const UStringView& str2);
  friend bool operator>=(const UNICHAR* str1, const UStringView& str2);

 private:
  const storage_type* data_;
  size_t length_;
};

template <typename StringFamily,
          typename EnableIf<IsSame<StringFamily, UString>::Value ||
                                IsSame<StringFamily, UStringRef>::Value,
                            bool>::Type = true>
inline UStringView ToUStringView(const StringFamily& str) noexcept {
  return UStringView(str.ConstData(), str.Len());
}

}  // namespace fun
