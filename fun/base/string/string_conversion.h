#pragma once

#include "fun/base/base.h"
#include "fun/base/string/utf.h"
#include "fun/base/string/byte_string_view.h"
#include "fun/base/string/uni_string_view.h"

namespace fun {

struct UTF8_TO_UNICHAR_BUFFER : public UtfConversionBuffer<char, UNICHAR> {
  using Super = UtfConversionBuffer<char, UNICHAR>;

  FUN_ALWAYS_INLINE UTF8_TO_UNICHAR_BUFFER(const char* str) : Super(str) {}
  FUN_ALWAYS_INLINE UTF8_TO_UNICHAR_BUFFER(const char* str, int32 len) : Super(str, len) {}
  FUN_ALWAYS_INLINE UTF8_TO_UNICHAR_BUFFER(ByteStringView str) : Super(str.ConstData(), str.Len()) {}
  //TODO ArrayView로 처리하는게 좋을듯 싶은데... 찍어내기 좋게...
  FUN_ALWAYS_INLINE UStringView ToUStringView() const { return UStringView(ConstData(), Len()); }

  // Disable default constructor and copy.
  UTF8_TO_UNICHAR_BUFFER() = delete;
  UTF8_TO_UNICHAR_BUFFER(const UTF8_TO_UNICHAR_BUFFER&) = delete;
  UTF8_TO_UNICHAR_BUFFER& operator = (const UTF8_TO_UNICHAR_BUFFER&) = delete;
};


struct UNICHAR_TO_UTF8_BUFFER : public UtfConversionBuffer<UNICHAR, char> {
  using Super = UtfConversionBuffer<UNICHAR, char>;

  FUN_ALWAYS_INLINE UNICHAR_TO_UTF8_BUFFER(const UNICHAR* str) : Super(str) {}
  FUN_ALWAYS_INLINE UNICHAR_TO_UTF8_BUFFER(const UNICHAR* str, int32 len) : Super(str, len) {}
  FUN_ALWAYS_INLINE UNICHAR_TO_UTF8_BUFFER(UStringView str) : Super(str.ConstData(), str.Len()) {}
  FUN_ALWAYS_INLINE ByteStringView ToByteStringView() const { return ByteStringView(ConstData(), Len()); }

  // Disable default constructor and copy.
  UNICHAR_TO_UTF8_BUFFER() = delete;
  UNICHAR_TO_UTF8_BUFFER(const UNICHAR_TO_UTF8_BUFFER&) = delete;
  UNICHAR_TO_UTF8_BUFFER& operator = (const UNICHAR_TO_UTF8_BUFFER&) = delete;
};


/*

UTF8:
  UTF8_TO_UTF16
  UTF8_TO_UTF32
  UTF8_TO_WCHAR

UTF16:
  UTF16_TO_UTF8
  UTF16_TO_UTF32
  UTF16_TO_WCHAR

UTF32:
  UTF32_TO_UTF8
  UTF32_TO_UTF16
  UTF32_TO_WCHAR

WCHAR:
  WCHAR_TO_UTF8
  WCHAR_TO_UTF16
  WCHAR_TO_UTF32
*/

//TODO wchar_t 크기에 따라서 단순 캐스팅으로 대체될수 있어야함.
//일관성을 위해서 단순 랩퍼 함수가 필요할수도...
//변환이 아닌 단순 캐스팅...
//TODO 길이를 지정할 수 있는 버젼도 하나 만들어야함.
//아무리 생각해봐도 StringView, ByteStringView 보다는 ArrayView가 좋을듯 싶은데...
//TODO 매크로가 아닌 템플릿으로 모두 처리하는 방법에 대해서 연구해보자.
//SFINAE를 활용해서 처리하면 되지 않을까??

//TODO
// wchar_t가 16비트 일 경우에는 단순히 타입만 캐스팅 하고
// wchar_t가 16비트가 아닌 경우에는 값 자체를 해야함.


//-----------------------------------------------------------------------------
// As String Pointer
//-----------------------------------------------------------------------------

#define UTF8_TO_UTF16(str)            UtfConversionBuffer<char, UTF16CHAR>(str).ConstData()
#define UTF8_TO_UTF32(str)            UtfConversionBuffer<char, UTF32CHAR>(str).ConstData()
#define UTF8_TO_WCHAR(str)            UtfConversionBuffer<char, wchar_t>(str).ConstData()

#define UTF16_TO_UTF8(str)            UtfConversionBuffer<UTF16CHAR, char>(str).ConstData()
#define UTF16_TO_UTF32(str)           UtfConversionBuffer<UTF16CHAR, UTF32CHAR>(str).ConstData()
#define UTF16_TO_WCHAR(str)           UtfConversionBuffer<UTF16CHAR, wchar_t>(str).ConstData()

#define UTF32_TO_UTF8(str)            UtfConversionBuffer<UTF32CHAR, char>(str).ConstData()
#define UTF32_TO_UTF16(str)           UtfConversionBuffer<UTF32CHAR, UTF16CHAR>(str).ConstData()
#define UTF32_TO_WCHAR(str)           UtfConversionBuffer<UTF32CHAR, wchar_t>(str).ConstData()

#define WCHAR_TO_UTF8(str)            UtfConversionBuffer<wchar_t, char>(str).ConstData()
#define WCHAR_TO_UTF16(str)           UtfConversionBuffer<wchar_t, UTF16CHAR>(str).ConstData()
#define WCHAR_TO_UTF32(str)           UtfConversionBuffer<wchar_t, UTF32CHAR>(str).ConstData()

#define UTF8_TO_UTF16_N(str, len)     UtfConversionBuffer<char, UTF16CHAR>(str, len).ConstData()
#define UTF8_TO_UTF32_N(str, len)     UtfConversionBuffer<char, UTF32Char>(str, len).ConstData()
#define UTF8_TO_WCHAR_N(str, len)     UtfConversionBuffer<char, wchar_t>(str, len).ConstData()

#define UTF16_TO_UTF8_N(str, len)     UtfConversionBuffer<UTF16CHAR, char>(str, len).ConstData()
#define UTF16_TO_UTF32_N(str, len)    UtfConversionBuffer<UTF16CHAR, UTF32CHAR>(str, len).ConstData()
#define UTF16_TO_WCHAR_N(str, len)    UtfConversionBuffer<UTF16CHAR, wchar_t>(str, len).ConstData()

#define UTF32_TO_UTF8_N(str, len)     UtfConversionBuffer<UTF32CHAR, char>(str, len).ConstData()
#define UTF32_TO_UTF16_N(str, len)    UtfConversionBuffer<UTF32CHAR, UTF16CHAR>(str, len).ConstData()
#define UTF32_TO_WCHAR_N(str, len)    UtfConversionBuffer<UTF32CHAR, wchar_t>(str, len).ConstData()

#define WCHAR_TO_UTF8_N(str, len)     UtfConversionBuffer<wchar_t, char>(str, len).ConstData()
#define WCHAR_TO_UTF16_N(str, len)    UtfConversionBuffer<wchar_t, UTF16CHAR>(str, len).ConstData()
#define WCHAR_TO_UTF32_N(str, len)    UtfConversionBuffer<wchar_t, UTF32CHAR>(str, len).ConstData()


#define UNICHAR_TO_UTF8(str)          UtfConversionBuffer<UNICHAR, char>(str).ConstData()
#define UNICHAR_TO_UTF8_N(str, len)   UtfConversionBuffer<UNICHAR, char>(str, len).ConstData()
#define UTF8_TO_UNICHAR(str)          UtfConversionBuffer<char, UNICHAR>(str).ConstData()
#define UTF8_TO_UNICHAR_N(str, len)   UtfConversionBuffer<char, UNICHAR>(str, len).ConstData()


//-----------------------------------------------------------------------------
// As ArrayView
//-----------------------------------------------------------------------------

#define UTF8_TO_UTF16_AS_VIEW(str)            UtfConversionBuffer<char, UTF16CHAR>(str).ToArrayView()
#define UTF8_TO_UTF32_AS_VIEW(str)            UtfConversionBuffer<char, UTF32CHAR>(str).ToArrayView()
#define UTF8_TO_WCHAR_AS_VIEW(str)            UtfConversionBuffer<char, wchar_t>(str).ToArrayView()

#define UTF16_TO_UTF8_AS_VIEW(str)            UtfConversionBuffer<UTF16CHAR, char>(str).ToArrayView()
#define UTF16_TO_UTF32_AS_VIEW(str)           UtfConversionBuffer<UTF16CHAR, UTF32CHAR>(str).ToArrayView()
#define UTF16_TO_WCHAR_AS_VIEW(str)           UtfConversionBuffer<UTF16CHAR, wchar_t>(str).ToArrayView()

#define UTF32_TO_UTF8_AS_VIEW(str)            UtfConversionBuffer<UTF32CHAR, char>(str).ToArrayView()
#define UTF32_TO_UTF16_AS_VIEW(str)           UtfConversionBuffer<UTF32CHAR, UTF16CHAR>(str).ToArrayView()
#define UTF32_TO_WCHAR_AS_VIEW(str)           UtfConversionBuffer<UTF32CHAR, wchar_t>(str).ToArrayView()

#define WCHAR_TO_UTF8_AS_VIEW(str)            UtfConversionBuffer<wchar_t, char>(str).ToArrayView()
#define WCHAR_TO_UTF16_AS_VIEW(str)           UtfConversionBuffer<wchar_t, UTF16CHAR>(str).ToArrayView()
#define WCHAR_TO_UTF32_AS_VIEW(str)           UtfConversionBuffer<wchar_t, UTF32CHAR>(str).ToArrayView()

#define UTF8_TO_UTF16_N_AS_VIEW(str, len)     UtfConversionBuffer<char, UTF16CHAR>(str, len).ToArrayView()
#define UTF8_TO_UTF32_N_AS_VIEW(str, len)     UtfConversionBuffer<char, UTF32Char>(str, len).ToArrayView()
#define UTF8_TO_WCHAR_N_AS_VIEW(str, len)     UtfConversionBuffer<char, wchar_t>(str, len).ToArrayView()

#define UTF16_TO_UTF8_N_AS_VIEW(str, len)     UtfConversionBuffer<UTF16CHAR, char>(str, len).ToArrayView()
#define UTF16_TO_UTF32_N_AS_VIEW(str, len)    UtfConversionBuffer<UTF16CHAR, UTF32CHAR>(str, len).ToArrayView()
#define UTF16_TO_WCHAR_N_AS_VIEW(str, len)    UtfConversionBuffer<UTF16CHAR, wchar_t>(str, len).ToArrayView()

#define UTF32_TO_UTF8_N_AS_VIEW(str, len)     UtfConversionBuffer<UTF32CHAR, char>(str, len).ToArrayView()
#define UTF32_TO_UTF16_N_AS_VIEW(str, len)    UtfConversionBuffer<UTF32CHAR, UTF16CHAR>(str, len).ToArrayView()
#define UTF32_TO_WCHAR_N_AS_VIEW(str, len)    UtfConversionBuffer<UTF32CHAR, wchar_t>(str, len).ToArrayView()

#define WCHAR_TO_UTF8_N_AS_VIEW(str, len)     UtfConversionBuffer<wchar_t, char>(str, len).ToArrayView()
#define WCHAR_TO_UTF16_N_AS_VIEW(str, len)    UtfConversionBuffer<wchar_t, UTF16CHAR>(str, len).ToArrayView()
#define WCHAR_TO_UTF32_N_AS_VIEW(str, len)    UtfConversionBuffer<wchar_t, UTF32CHAR>(str, len).ToArrayView()


#define UNICHAR_TO_UTF8_AS_VIEW(str)          UtfConversionBuffer<UNICHAR, char>(str).ToArrayView()
#define UNICHAR_TO_UTF8_N_AS_VIEW(str, len)   UtfConversionBuffer<UNICHAR, char>(str, len).ToArrayView()
#define UTF8_TO_UNICHAR_AS_VIEW(str)          UtfConversionBuffer<char, UNICHAR>(str).ToArrayView()
#define UTF8_TO_UNICHAR_N_AS_VIEW(str, len)   UtfConversionBuffer<char, UNICHAR>(str, len).ToArrayView()


//TODO 단순 캐스팅...

/*
두 인코딩 간의 크기가 같은 경우 단순히 캐스팅?
크기가 같다고 하여도 인코딩은 다를 수 있지 않나??
예를들어 로컬 인코딩인데 ANSI로 바꾼다던지...
*/

#if 0

template <typename T>
class StringPointer {
 public:
  FUN_ALWAYS_INLINE explicit StringPointer(const T* ptr) : ptr_(ptr) {}

  FUN_ALWAYS_INLINE const T* Get() const {
    return ptr_;
  }

  FUN_ALWAYS_INLINE int32 Len() const {
    return ptr_ ? StringTraits<T>::Strlen(ptr_) : 0;
  }

 private:
  const T* ptr_;
};


// 인코딩이 호환이 되는 경우에는 변환 없이 캐스팅만 함.
// 인코딩이 어떻게 호환이 된다고 판단하지??
template <typename From, typename To>
FUN_ALWAYS_INLINE
typename EnableIf<
    AreEncodingsCompatible<From, To>::Value,
    StringPointer<To>
  >::Type
StringCast(const From* src)
{
  return StringPointer<To>((const To*)str);
}

template <typename From, typename To>
FUN_ALWAYS_INLINE
typename EnableIf<
    !AreEncodingsCompatible<From, To>::Value,
    StringConversion<From, To>
  >::Type
StringCast(const From* src)
{
  return StringConversion<From, To>(str);
}


template <typename From, typename To>
FUN_ALWAYS_INLINE
typename EnableIf<
    AreEncodingsCompatible<From, To>::Value,
    StringPointer<To>
  >::Type
StringCast(const From* src, int32 src_len)
{
  FUN_UNUSED(src_len);
  return StringPointer<To>((const To*)str);
}

template <typename From, typename To>
FUN_ALWAYS_INLINE
typename EnableIf<
    !AreEncodingsCompatible<From, To>::Value,
    StringConversion<From, To>
  >::Type
StringCast(const From* src, int32 src_len)
{
  return StringConversion<From, To>(str, src_len);
}


template <typename From, typename To>
FUN_ALWAYS_INLINE To CharCast(From ch)
{
  To result;
  PlatformString::Convert(&result, 1, &ch, 1, (To)'?');
  return result;
}


/*

template <typename _FromType, typename _ToType>
class StringPassthru : private InlineAllocator<500>::template ForElementType<_FromType>
{
 public:
  using FromType = _FromType;
  using ToType = _ToType;
  using Allocator = InlineAllocator<500>::template ForElementType<FromType>;

 public:
  FUN_ALWAYS_INLINE StringPassthru(ToType* dst, int32 dst_len, int32 src_len)
    : dst_(dst), dst_len_(dst_len), src_len_(src_len) {
    Allocator::ResizeAllocation(0, src_len, sizeof(FromType));
  }

  FUN_ALWAYS_INLINE StringPassthru(StringPassthru&& other) {
    Allocator::MoveToEmpty(other);
  }

  FUN_ALWAYS_INLINE void Apply() const {
    const FromType* src = (const FromType*)Allocator::GetAllocation();
    //TODO
    //fun_check(CString::ConvertedLength<ToType>(src, src_len) <= dst_len);
    //TODO convert...
  }

  FUN_ALWAYS_INLINE FromType* Get() {
    return (FromType*)Allocator::GetAllocation();
  }

  // Disable copy
  StringPassthru(const StringPassthru&) = delete;
  StringPassthru& operator = (const StringPassthru&) = delete;

 private:
  ToType* dst_;
  int32 dst_len_;
  int32 src_len_;
};


template <typename T>
class PassthruPointer {
 public:
  FUN_ALWAYS_INLINE explicit PassthruPointer(T* ptr)
    : ptr_(ptr) {}

  FUN_ALWAYS_INLINE T* Get() const {
    return ptr_;
  }

  FUN_ALWAYS_INLINE void Apply() const {
  }

 private:
  T* ptr_;
};


template <typename From, typename To>
FUN_ALWAYS_INLINE
typename EnableIf<
  AreEncodingsCompatible<From,To>::Value,
  PassthruPointer<From>>::Type
  StringMemoryPassthru(T* buffer, int32 buffer_len, int32 source_len)
{
  fun_check(source_len <= buffer_len);
  return PassthruPointer<From>((From*)buffer);
}

template <typename From, typename To>
FUN_ALWAYS_INLINE
typename EnableIf<
  !AreEncodingsCompatible<From,To>::Value,
  StringPassthru<From>>::Type
  StringMemoryPassthru(T* buffer, int32 buffer_len, int32 source_len)
{
  fun_check(source_len <= buffer_len);
  return StringPassthru<From, To>((From*)buffer);
}

*/

#endif

} // namespace fun
