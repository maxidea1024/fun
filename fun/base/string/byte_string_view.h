#pragma once

#include "fun/base/ftl/type_traits.h"
#include "fun/base/string/cstring_traits.h"
#include "fun/base/string/utf.h"
#include <string>

namespace fun {

namespace ByteStringView_internal {

// Char
template <typename CharType>
struct IsCompatibleCharTypeHelper
  : IntegralConstant<bool,
    IsSame<CharType, char>::Value ||
    IsSame<CharType, int8>::Value ||
    IsSame<CharType, uint8>::Value
    > {};

template <typename CharType>
struct IsCompatibleCharType
  : IsCompatibleCharTypeHelper<typename RemoveCV<typename RemoveReference<CharType>::Type>::Type> {};


//
// Array
//

template <typename Array>
struct IsCompatibleArrayHelper : FalseType {};

template <typename CharType, size_t N>
struct IsCompatibleArrayHelper<CharType[N]>
  : IsCompatibleCharType<CharType> {};

template <typename Array>
struct IsCompatibleArray
  : IsCompatibleArrayHelper<typename RemoveCV<typename RemoveReference<Array>::Type>::Type> {};


//
// Pointer
//

template <typename Pointer>
struct IsCompatiblePointerHelper : FalseType {};

template <typename CharType>
struct IsCompatiblePointerHelper<CharType*>
  : IsCompatibleCharType<CharType> {};

template <typename Pointer>
struct IsCompatiblePointer
  : IsCompatiblePointerHelper<typename RemoveCV<typename RemoveReference<Pointer>::Type>::Type> {};


//
// STL
//

template <typename T>
struct IsCompatibleStdBasicStringHelper : FalseType {};

template <typename CharType, typename...Args>
struct IsCompatibleStdBasicStringHelper<std::basic_string<CharType, Args...>>
  : IsCompatibleCharType<CharType> {};

template <typename T>
struct IsCompatibleStdBasicString
  : IsCompatibleStdBasicStringHelper<typename RemoveCV<typename RemoveReference<T>::Type>::Type> {};

} // end of namespace ByteStringView_internal


template <typename T>
struct IsSupportByteStringView
  : IntegralConstant<bool,
    ByteStringView_internal::IsCompatibleCharType<T>::Value ||
    ByteStringView_internal::IsCompatibleArray<T>::Value ||
    ByteStringView_internal::IsCompatiblePointer<T>::Value ||
    ByteStringView_internal::IsCompatibleStdBasicString<T>::Value ||
    IsSame<T, ByteString>::Value ||
    IsSame<T, ByteStringRef>::Value
    > {};

class ByteStringView {
 public:
  typedef char storage_type;

  typedef const char value_type;
  typedef std::ptrdiff_t difference_type;
  //TODO ssize_t 가 표준에 있지 않아서... 흠... 문제가 없으려나??
  //TODO ssize_t 가 표준에 있지 않아서... 흠... 문제가 없으려나??
  //TODO ssize_t 가 표준에 있지 않아서... 흠... 문제가 없으려나??
  //typedef ssize_t size_type;
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
  using IfCompatibleChar = typename EnableIf<ByteStringView_internal::IsCompatibleCharType<CharType>::Value, bool>::Type;

  template <typename Array>
  using IfCompatibleArray = typename EnableIf<ByteStringView_internal::IsCompatibleArray<Array>::Value, bool>::Type;

  template <typename Pointer>
  using IfCompatiblePointer = typename EnableIf<ByteStringView_internal::IsCompatiblePointer<Pointer>::Value, bool>::Type;

  template <typename T>
  using IfCompatibleStdString = typename EnableIf<ByteStringView_internal::IsCompatibleStdBasicString<T>::Value, bool>::Type;

  template <typename T>
  using IfCompatibleByteStringFamily = typename EnableIf<IsSame<T,ByteString>::Value || IsSame<T,ByteStringRef>::Value, bool>::Type;

  //std::string("123\0\0") == 3 처럼 동작하도록 함.
  //template <typename CharType, size_t N>
  //static size_t LengthHelperArray(const CharType (&)[N]) noexcept { return N - 1; }
  template <typename CharType, size_t N>
  static size_t LengthHelperArray(const CharType (&arr)[N]) noexcept {
    return CStringTraitsA::Strlen(reinterpret_cast<const storage_type*>(arr));
  }

  template <typename CharType>
  static size_t LengthHelperPointer(const CharType* str) noexcept {
    return CStringTraitsA::Strlen(reinterpret_cast<const storage_type*>(str));
  }

  template <typename CharType>
  static const storage_type* CastHelper(const CharType* str) noexcept {
    return reinterpret_cast<const storage_type*>(str);
  }

  static const storage_type* CastHelper(const storage_type* str) noexcept {
    return str;
  }

 public:
  ByteStringView() noexcept
    : data_(nullptr), length_(0) {}

  ByteStringView(decltype(nullptr))
    : ByteStringView() {}

  template <typename CharType, IfCompatibleChar<CharType> = true>
  ByteStringView(const CharType* str, size_t len)
    : data_(CastHelper(str)), length_(len) {
    fun_check(len >= 0);
    fun_check(str || len == 0);
  }

  template <typename Array, IfCompatibleArray<Array> = true>
  ByteStringView(const Array& str) noexcept
    : ByteStringView(str, LengthHelperArray(str)) {}

  template <typename Pointer, IfCompatiblePointer<Pointer> = true>
  ByteStringView(const Pointer& str) noexcept
    : ByteStringView(str, LengthHelperPointer(str)) {}

  template <typename Str, IfCompatibleByteStringFamily<Str> = true> //byte_string_inline.h
  ByteStringView(const Str& str) throw();

  //size_t -> int32 truncation 경고가 너무 많이 나와서...
  //FUN_ALWAYS_INLINE size_t Len() const { return length_; }
  FUN_ALWAYS_INLINE int32 Len() const { return (int32)length_; }
  FUN_ALWAYS_INLINE bool IsNull() const noexcept { return data_ == nullptr; }
  FUN_ALWAYS_INLINE bool IsEmpty() const noexcept { return length_ == 0; }
  FUN_ALWAYS_INLINE char First() const { return At(0); }
  FUN_ALWAYS_INLINE char Last() const { return At(Len() - 1); }
  FUN_ALWAYS_INLINE char FirstOr(const char def = char(0)) const { return Len() ? At(0) : def; }
  FUN_ALWAYS_INLINE char LastOr(const char def = char(0)) const { return Len() ? At(Len() - 1) : def; }

  FUN_ALWAYS_INLINE const_pointer ConstData() const noexcept { return reinterpret_cast<const_pointer>(data_); }

  FUN_ALWAYS_INLINE char operator[](size_t index) const { fun_check(index < Len()); return char(data_[index]); }
  FUN_ALWAYS_INLINE char At(size_t index) const { fun_check(index < Len()); return char(data_[index]); }

  //TODO?
  //ByteString ToANSI() const;
  //ByteString ToUtf8() const;
  //ByteString ToLocal8Bit() const;
  //Array<uint32> ToUCS4() const;

  FUN_ALWAYS_INLINE ByteStringView Mid(size_t pos) const { fun_check(pos <= length_); return ByteStringView(data_ + pos, length_ - pos); }
  FUN_ALWAYS_INLINE ByteStringView Mid(size_t pos, size_t len) const { fun_check(pos + len <= length_); return ByteStringView(data_ + pos, len); }
  FUN_ALWAYS_INLINE ByteStringView Left(size_t len) { fun_check(len <= length_); return ByteStringView(data_, len); }
  FUN_ALWAYS_INLINE ByteStringView Right(size_t len) { fun_check(len <= length_); return ByteStringView(data_ + length_ - len, len); }
  //TODO Chopped

  ByteStringView& Truncate(size_t pos) { fun_check(pos <= Len()); length_ = pos; }

  FUN_ALWAYS_INLINE bool StartsWith(ByteStringView sub, CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const
  { return StringCmp::StartsWith(ConstData(), Len(), sub.ConstData(), sub.Len(), casesense); }
  FUN_ALWAYS_INLINE bool StartsWith(AsciiString sub, CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  FUN_ALWAYS_INLINE bool StartsWith(char ch, CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const
  { return StringCmp::StartsWith(ConstData(), Len(), &ch, 1, casesense); }

  FUN_ALWAYS_INLINE bool EndsWith(ByteStringView sub, CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const
  { return StringCmp::EndsWith(ConstData(), Len(), sub.ConstData(), sub.Len(), casesense); }
  FUN_ALWAYS_INLINE bool EndsWith(AsciiString sub, CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  FUN_ALWAYS_INLINE bool EndsWith(char ch, CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const
  { return StringCmp::EndsWith(ConstData(), Len(), &ch, 1, casesense); }

  // STL compatibility API:

  FUN_ALWAYS_INLINE const_iterator begin() const noexcept { return ConstData(); }
  FUN_ALWAYS_INLINE const_iterator end() const noexcept { return ConstData() + size(); }
  FUN_ALWAYS_INLINE const_iterator cbegin() const noexcept { return begin(); }
  FUN_ALWAYS_INLINE const_iterator cend() const noexcept { return end(); }
  FUN_ALWAYS_INLINE const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
  FUN_ALWAYS_INLINE const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
  FUN_ALWAYS_INLINE const_reverse_iterator crbegin() const noexcept { return rbegin(); }
  FUN_ALWAYS_INLINE const_reverse_iterator crend() const noexcept { return rend(); }

  FUN_ALWAYS_INLINE bool empty() const noexcept { return size() == 0; }
  FUN_ALWAYS_INLINE char front() const { fun_check(!empty()); return char(data_[0]); }
  FUN_ALWAYS_INLINE char back() const { fun_check(!empty()); return char(data_[length_ - 1]); }
  FUN_ALWAYS_INLINE size_t size() const { return length_; }

  //ByteStringView and ByteStringView
  int32 Compare(const ByteStringView& str, CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool Equals(const ByteStringView& str, CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool operator == (const ByteStringView& str) const;
  bool operator != (const ByteStringView& str) const;
  bool operator <  (const ByteStringView& str) const;
  bool operator <= (const ByteStringView& str) const;
  bool operator >  (const ByteStringView& str) const;
  bool operator >= (const ByteStringView& str) const;

  //ByteStringView and ByteString
  int32 Compare(const ByteString& str, CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool Equals(const ByteString& str, CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool operator == (const ByteString& str) const;
  bool operator != (const ByteString& str) const;
  bool operator <  (const ByteString& str) const;
  bool operator <= (const ByteString& str) const;
  bool operator >  (const ByteString& str) const;
  bool operator >= (const ByteString& str) const;

  //ByteStringView and ByteStringRef
  int32 Compare(const ByteStringRef& str, CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool Equals(const ByteStringRef& str, CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool operator == (const ByteStringRef& str) const;
  bool operator != (const ByteStringRef& str) const;
  bool operator <  (const ByteStringRef& str) const;
  bool operator <= (const ByteStringRef& str) const;
  bool operator >  (const ByteStringRef& str) const;
  bool operator >= (const ByteStringRef& str) const;

  //ByteStringView and char
  int32 Compare(char ch, CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool Equals(char ch, CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool operator == (char ch) const;
  bool operator != (char ch) const;
  bool operator <  (char ch) const;
  bool operator <= (char ch) const;
  bool operator >  (char ch) const;
  bool operator >= (char ch) const;
  friend bool operator == (char ch, const ByteStringView& str);
  friend bool operator != (char ch, const ByteStringView& str);
  friend bool operator <  (char ch, const ByteStringView& str);
  friend bool operator <= (char ch, const ByteStringView& str);
  friend bool operator >  (char ch, const ByteStringView& str);
  friend bool operator >= (char ch, const ByteStringView& str);

  //ByteStringView and const char*
  int32 Compare(const char* str, CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool Equals(const char* str, CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool operator == (const char* str) const;
  bool operator != (const char* str) const;
  bool operator <  (const char* str) const;
  bool operator <= (const char* str) const;
  bool operator >  (const char* str) const;
  bool operator >= (const char* str) const;
  friend bool operator == (const char* str1, const ByteStringView& str2);
  friend bool operator != (const char* str1, const ByteStringView& str2);
  friend bool operator <  (const char* str1, const ByteStringView& str2);
  friend bool operator <= (const char* str1, const ByteStringView& str2);
  friend bool operator >  (const char* str1, const ByteStringView& str2);
  friend bool operator >= (const char* str1, const ByteStringView& str2);

  //ByteStringView and AsciiString
  int32 Compare(const AsciiString& str, CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool Equals(const AsciiString& str, CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool operator == (const AsciiString& str) const;
  bool operator != (const AsciiString& str) const;
  bool operator <  (const AsciiString& str) const;
  bool operator <= (const AsciiString& str) const;
  bool operator >  (const AsciiString& str) const;
  bool operator >= (const AsciiString& str) const;

  //ByteStringView and UString
  int32 Compare(const UString& str, CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool Equals(const UString& str, CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool operator == (const UString& str) const;
  bool operator != (const UString& str) const;
  bool operator <  (const UString& str) const;
  bool operator <= (const UString& str) const;
  bool operator >  (const UString& str) const;
  bool operator >= (const UString& str) const;

  //ByteStringView and UStringRef
  int32 Compare(const UStringRef& str, CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool Equals(const UStringRef& str, CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool operator == (const UStringRef& str) const;
  bool operator != (const UStringRef& str) const;
  bool operator <  (const UStringRef& str) const;
  bool operator <= (const UStringRef& str) const;
  bool operator >  (const UStringRef& str) const;
  bool operator >= (const UStringRef& str) const;

  //ByteStringView and UStringView
  int32 Compare(const UStringView& str, CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool Equals(const UStringView& str, CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool operator == (const UStringView& str) const;
  bool operator != (const UStringView& str) const;
  bool operator <  (const UStringView& str) const;
  bool operator <= (const UStringView& str) const;
  bool operator >  (const UStringView& str) const;
  bool operator >= (const UStringView& str) const;

  //ByteStringView and UNICHAR
  int32 Compare(UNICHAR ch, CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool Equals(UNICHAR ch, CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool operator == (UNICHAR ch) const;
  bool operator != (UNICHAR ch) const;
  bool operator <  (UNICHAR ch) const;
  bool operator <= (UNICHAR ch) const;
  bool operator >  (UNICHAR ch) const;
  bool operator >= (UNICHAR ch) const;
  friend bool operator == (UNICHAR ch, const ByteStringView& str);
  friend bool operator != (UNICHAR ch, const ByteStringView& str);
  friend bool operator <  (UNICHAR ch, const ByteStringView& str);
  friend bool operator <= (UNICHAR ch, const ByteStringView& str);
  friend bool operator >  (UNICHAR ch, const ByteStringView& str);
  friend bool operator >= (UNICHAR ch, const ByteStringView& str);

  //ByteStringView and const UNICHAR*
  int32 Compare(const UNICHAR* str, CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool Equals(const UNICHAR* str, CaseSensitivity casesense = CaseSensitivity::CaseSensitive) const;
  bool operator == (const UNICHAR* str) const;
  bool operator != (const UNICHAR* str) const;
  bool operator <  (const UNICHAR* str) const;
  bool operator <= (const UNICHAR* str) const;
  bool operator >  (const UNICHAR* str) const;
  bool operator >= (const UNICHAR* str) const;
  friend bool operator == (const UNICHAR* str1, const ByteStringView& str2);
  friend bool operator != (const UNICHAR* str1, const ByteStringView& str2);
  friend bool operator <  (const UNICHAR* str1, const ByteStringView& str2);
  friend bool operator <= (const UNICHAR* str1, const ByteStringView& str2);
  friend bool operator >  (const UNICHAR* str1, const ByteStringView& str2);
  friend bool operator >= (const UNICHAR* str1, const ByteStringView& str2);

 private:
  const storage_type* data_;
  size_t length_;
};


template <typename ByteStringFamily, typename EnableIf<
    IsSame<ByteStringFamily, ByteString>::Value ||
    IsSame<ByteStringFamily, ByteStringRef>::Value, bool>::Type = true
  >
FUN_ALWAYS_INLINE ByteStringView ToByteStringView(const ByteStringFamily& str) noexcept {
  return ByteStringView(str.ConstData(), str.Len());
}

} // namespace fun
