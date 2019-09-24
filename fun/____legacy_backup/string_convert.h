#pragma once

#include "Containers/ContainerAllocationPolicies.h"
#include "HAL/PlatformTypes.h"
#include "HAL/PlatformString.h"

//#define DEFAULT_STRING_CONVERSION_SIZE  128u
#define UNICODE_BOGUS_CHAR_CODEPOINT  '?'

namespace fun {

template <typename from, typename To>
class TStringConvert
{
 public:
  typedef from FromType;
  typedef To ToType;

  inline static void Convert(To* dst, int32 dst_len, const from* source, int32 source_len)
  {
    To* result = CPlatformString::Convert(dst, dst_len, source, source_len, (To)UNICODE_BOGUS_CHAR_CODEPOINT);
    fun_check(result != nullptr);
  }

  inline static int32 ConvertedLength(const from* source, int32 source_len)
  {
    return CPlatformString::ConvertedLength<To>(source, source_len);
  }
};


/**
This is a basic ANSICHAR* wrapper which swallows all output written through it.
*/
struct NullPointerIterator
{
  NullPointerIterator()
    : ptr(nullptr)
  {}

  const NullPointerIterator& operator* () const { return *this; }
  const NullPointerIterator& operator++() { ++ptr; return *this; }
  const NullPointerIterator& operator++(int) { ++ptr; return *this; }

  ANSICHAR operator = (ANSICHAR Val) const
  {
    return Val;
  }

  friend int32 operator - (NullPointerIterator lhs, NullPointerIterator rhs)
  {
    return lhs.ptr - rhs.ptr;
  }

  ANSICHAR* ptr;
};


/**
This should be replaced with Platform stuff when CPlatformString starts to know about UTF-8.
*/
class CTCHARToUTF8_Convert
{
public:
  typedef UNICHAR FromType;
  typedef ANSICHAR ToType;

  // I wrote this function for originally for PhysicsFS. --ryan.
  // !!! FIXME: Maybe this shouldn't be inline...
  template <typename OutputIterator>
  static void utf8fromcodepoint(uint32 cp, OutputIterator* _dst, int32 *_len)
  {
    OutputIterator dst = *_dst;
    int32 len = *_len;

    if (len == 0) {
      return;
    }

    if (cp > 0x10FFFF) { // No unicode codepoints above 10FFFFh, (for now!)
      cp = UNICODE_BOGUS_CHAR_CODEPOINT;
    }
    else if ((cp == 0xFFFE) || (cp == 0xFFFF)) { // illegal values.
      cp = UNICODE_BOGUS_CHAR_CODEPOINT;
    }
    else {
      // There are seven "UTF-16 surrogates" that are illegal in UTF-8.
      switch (cp) {
      case 0xD800:
      case 0xDB7F:
      case 0xDB80:
      case 0xDBFF:
      case 0xDC00:
      case 0xDF80:
      case 0xDFFF:
        cp = UNICODE_BOGUS_CHAR_CODEPOINT;
        break;
      }
    }

    // Do the encoding...
    if (cp < 0x80) {
      *(dst++) = (char)cp;
      len--;
    }

    else if (cp < 0x800) {
      if (len < 2) {
        len = 0;
      }
      else {
        *(dst++) = (char) ((cp >> 6) | 128 | 64);
        *(dst++) = (char) (cp & 0x3F) | 128;
        len -= 2;
      }
    }
    else if (cp < 0x10000) {
      if (len < 3) {
        len = 0;
      }
      else {
        *(dst++) = (char) ((cp >> 12) | 128 | 64 | 32);
        *(dst++) = (char) ((cp >> 6) & 0x3F) | 128;
        *(dst++) = (char) (cp & 0x3F) | 128;
        len -= 3;
      }
    }
    else {
      if (len < 4) {
        len = 0;
      }
      else {
        *(dst++) = (char) ((cp >> 18) | 128 | 64 | 32 | 16);
        *(dst++) = (char) ((cp >> 12) & 0x3F) | 128;
        *(dst++) = (char) ((cp >> 6) & 0x3F) | 128;
        *(dst++) = (char) (cp & 0x3F) | 128;
        len -= 4;
      }
    }

    *_dst = dst;
    *_len = len;
  }

  /**
  Converts the string to the desired format.

  \param dst - The destination buffer of the converted string.
  \param dst_len - The length of the destination buffer.
  \param source - The source string to convert.
  \param source_len - The length of the source string.
  */
  static inline void Convert(ANSICHAR* dst, int32 dst_len, const UNICHAR* source, int32 source_len)
  {
    fun_check_dbg(source_len >= 0);

    // Now do the conversion
    // You have to do this even if !UNICODE, since high-ASCII chars
    // become multibyte. If you aren't using UNICODE and aren't using
    // a Latin1 charset, you are just screwed, since we don't handle
    // codepages, etc.
    while (source_len--) {
      utf8fromcodepoint((uint32)*source++, &dst, &dst_len);
    }
  }

  /**
  Determines the length of the converted string.

  \return The length of the string in UTF-8 code units.
  */
  static int32 ConvertedLength(const UNICHAR* source, int32 source_len)
  {
    fun_check_dbg(source_len >= 0);

    NullPointerIterator dest_start;
    NullPointerIterator dst;
    int32 dst_len = source_len * 4;
    while (source_len--) {
      utf8fromcodepoint((uint32)*source++, &dst, &dst_len);
    }
    return dst - dest_start;
  }
};


/**
This should be replaced with Platform stuff when CPlatformString starts to know about UTF-8.
Also, it's dangerous as it may read past the provided memory buffer if passed a malformed UTF-8 string.
*/
class CUTF8ToTCHAR_Convert
{
public:
  typedef ANSICHAR FromType;
  typedef UNICHAR ToType;

  static uint32 utf8codepoint(const ANSICHAR** str_)
  {
    const char* str = *str_;
    uint32 retval = 0;
    uint32 octet = (uint32) ((uint8) *str);
    uint32 octet2, octet3, octet4;

    if (octet < 128) // one octet char: 0 to 127
    {
      (*str_)++; // skip to next possible start of codepoint.
      return(octet);
    }
    else if (octet < 192) // bad (starts with 10xxxxxx).
    {
      // Apparently each of these is supposed to be flagged as a bogus
      //  char, instead of just resyncing to the next valid codepoint.
      (*str_)++; // skip to next possible start of codepoint.
      return UNICODE_BOGUS_CHAR_CODEPOINT;
    }
    else if (octet < 224) // two octets
    {
      octet -= (128+64);
      octet2 = (uint32) ((uint8) *(++str));
      if ((octet2 & (128 + 64)) != 128) // Format isn't 10xxxxxx?
      {
        (*str_)++; // Sequence was not valid UTF-8. Skip the first byte and continue.
        return UNICODE_BOGUS_CHAR_CODEPOINT;
      }

      retval = ((octet << 6) | (octet2 - 128));
      if ((retval >= 0x80) && (retval <= 0x7FF))
      {
        *str_ += 2; // skip to next possible start of codepoint.
        return retval;
      }
    }
    else if (octet < 240) // three octets
    {
      octet -= (128+64+32);
      octet2 = (uint32) ((uint8) *(++str));
      if ((octet2 & (128+64)) != 128) // Format isn't 10xxxxxx?
      {
        (*str_)++; // Sequence was not valid UTF-8. Skip the first byte and continue.
        return UNICODE_BOGUS_CHAR_CODEPOINT;
      }

      octet3 = (uint32) ((uint8) *(++str));
      if ((octet3 & (128+64)) != 128) // Format isn't 10xxxxxx?
      {
        (*str_)++; // Sequence was not valid UTF-8. Skip the first byte and continue.
        return UNICODE_BOGUS_CHAR_CODEPOINT;
      }

      retval = (((octet << 12)) | ((octet2-128) << 6) | ((octet3-128)));

      // There are seven "UTF-16 surrogates" that are illegal in UTF-8.
      switch (retval) {
      case 0xD800:
      case 0xDB7F:
      case 0xDB80:
      case 0xDBFF:
      case 0xDC00:
      case 0xDF80:
      case 0xDFFF:
        (*str_)++; // Sequence was not valid UTF-8. Skip the first byte and continue.
        return UNICODE_BOGUS_CHAR_CODEPOINT;
      }

      // 0xFFFE and 0xFFFF are illegal, too, so we check them at the edge.
      if ((retval >= 0x800) && (retval <= 0xFFFD))
      {
        *str_ += 3; // skip to next possible start of codepoint.
        return retval;
      }
    }
    else if (octet < 248) // four octets
    {
      octet -= (128+64+32+16);
      octet2 = (uint32) ((uint8) *(++str));
      if ((octet2 & (128+64)) != 128) // Format isn't 10xxxxxx?
      {
        (*str_)++; // Sequence was not valid UTF-8. Skip the first byte and continue.
        return UNICODE_BOGUS_CHAR_CODEPOINT;
      }

      octet3 = (uint32) ((uint8) *(++str));
      if ((octet3 & (128+64)) != 128) // Format isn't 10xxxxxx?
      {
        (*str_)++; // Sequence was not valid UTF-8. Skip the first byte and continue.
        return UNICODE_BOGUS_CHAR_CODEPOINT;
      }

      octet4 = (uint32) ((uint8) *(++str));
      if ((octet4 & (128+64)) != 128) // Format isn't 10xxxxxx?
      {
        (*str_)++; // Sequence was not valid UTF-8. Skip the first byte and continue.
        return UNICODE_BOGUS_CHAR_CODEPOINT;
      }

      retval = (((octet << 18)) | ((octet2 - 128) << 12) |
            ((octet3 - 128) << 6) | ((octet4 - 128)));
      if ((retval >= 0x10000) && (retval <= 0x10FFFF))
      {
        *str_ += 4; // skip to next possible start of codepoint.
        return retval;
      }
    }

    // Five and six octet sequences became illegal in rfc3629.
    // We throw the codepoint away, but parse them to make sure we move
    // ahead the right number of bytes and don't overflow the buffer.
    else if (octet < 252) // five octets
    {
      octet = (uint32) ((uint8) *(++str));
      if ((octet & (128+64)) != 128) // Format isn't 10xxxxxx?
      {
        (*str_)++; // Sequence was not valid UTF-8. Skip the first byte and continue.
        return UNICODE_BOGUS_CHAR_CODEPOINT;
      }

      octet = (uint32) ((uint8) *(++str));
      if ((octet & (128+64)) != 128) // Format isn't 10xxxxxx?
      {
        (*str_)++; // Sequence was not valid UTF-8. Skip the first byte and continue.
        return UNICODE_BOGUS_CHAR_CODEPOINT;
      }

      octet = (uint32) ((uint8) *(++str));
      if ((octet & (128+64)) != 128) // Format isn't 10xxxxxx?
      {
        (*str_)++; // Sequence was not valid UTF-8. Skip the first byte and continue.
        return UNICODE_BOGUS_CHAR_CODEPOINT;
      }

      octet = (uint32) ((uint8) *(++str));
      if ((octet & (128+64)) != 128) // Format isn't 10xxxxxx?
      {
        (*str_)++; // Sequence was not valid UTF-8. Skip the first byte and continue.
        return UNICODE_BOGUS_CHAR_CODEPOINT;
      }

      *str_ += 5; // skip to next possible start of codepoint.
      return UNICODE_BOGUS_CHAR_CODEPOINT;
    }

    else // six octets
    {
      octet = (uint32) ((uint8) *(++str));
      if ((octet & (128+64)) != 128) // Format isn't 10xxxxxx?
      {
        (*str_)++; // Sequence was not valid UTF-8. Skip the first byte and continue.
        return UNICODE_BOGUS_CHAR_CODEPOINT;
      }

      octet = (uint32) ((uint8) *(++str));
      if ((octet & (128+64)) != 128) // Format isn't 10xxxxxx?
      {
        (*str_)++; // Sequence was not valid UTF-8. Skip the first byte and continue.
        return UNICODE_BOGUS_CHAR_CODEPOINT;
      }

      octet = (uint32) ((uint8) *(++str));
      if ((octet & (128+64)) != 128) // Format isn't 10xxxxxx?
      {
        (*str_)++; // Sequence was not valid UTF-8. Skip the first byte and continue.
        return UNICODE_BOGUS_CHAR_CODEPOINT;
      }

      octet = (uint32) ((uint8) *(++str));
      if ((octet & (128+64)) != 128) // Format isn't 10xxxxxx?
      {
        (*str_)++; // Sequence was not valid UTF-8. Skip the first byte and continue.
        return UNICODE_BOGUS_CHAR_CODEPOINT;
      }

      octet = (uint32) ((uint8) *(++str));
      if ((octet & (128+64)) != 128) // Format isn't 10xxxxxx?
      {
        (*str_)++; // Sequence was not valid UTF-8. Skip the first byte and continue.
        return UNICODE_BOGUS_CHAR_CODEPOINT;
      }

      *str_ += 6; // skip to next possible start of codepoint.
      return UNICODE_BOGUS_CHAR_CODEPOINT;
    }

    (*str_)++; // Sequence was not valid UTF-8. Skip the first byte and continue.
    return UNICODE_BOGUS_CHAR_CODEPOINT; // catch everything else.
  }

  /**
  Converts the string to the desired format.

  \param dst - The destination buffer of the converted string.
  \param dst_len - The length of the destination buffer.
  \param source - The source string to convert.
  \param source_len - The length of the source string.
  */
  static inline void Convert(UNICHAR* dst, int32 dst_len, const ANSICHAR* source, int32 source_len)
  {
    // Now do the conversion
    // You have to do this even if !UNICODE, since high-ASCII chars
    // become multibyte. If you aren't using UNICODE and aren't using
    // a Latin1 charset, you are just screwed, since we don't handle
    // codepages, etc.
    const ANSICHAR* SourceEnd = source + source_len;
    while (source < SourceEnd)
    {
      uint32 cp = utf8codepoint(&source);

      // Please note that we're truncating this to a UCS-2 Windows UNICHAR.
      //  A UCS-4 Unix wchar_t can hold this, and we're ignoring UTF-16 for now.
      if (cp > 0xFFFF)
      {
        cp = UNICODE_BOGUS_CHAR_CODEPOINT;
      }

      *dst++ = cp;
    }
  }

  /**
  Determines the length of the converted string.

  \return The length of the string in UTF-8 code units.
  */
  static int32 ConvertedLength(const ANSICHAR* source, int32 source_len)
  {
    int32 dst_len = 0;
    const ANSICHAR* SourceEnd = source + source_len;
    while (source < SourceEnd)
    {
      utf8codepoint(&source);
      ++dst_len;
    }
    return dst_len;
  }
};


enum class ENullTerminatedString : uint8
{
  No = 0,
  Yes = 1
};


/**
Class takes one type of string and converts it to another. The class includes a
chunk of presized memory of the destination type. If the presized array is
too small, it mallocs the memory needed and frees on the class going out of
scope.
*/
template <typename Converter, int32 DefaultConversionSize = DEFAULT_STRING_CONVERSION_SIZE>
class TStringConversion
  : private Converter
  , private InlineAllocator<DefaultConversionSize>::template ForElementType<typename Converter::ToType>
{
  typedef typename InlineAllocator<DefaultConversionSize>::template
    ForElementType<typename Converter::ToType> AllocatorType;

  typedef typename Converter::FromType FromType;
  typedef typename Converter::ToType ToType;

  /**
  Converts the data by using the Convert() method on the base class
  */
  void Init(const FromType* source, int32 source_len, ENullTerminatedString NullTerminated)
  {
    StringLength = Converter::ConvertedLength(source, source_len);

    const int32 buffer_size = StringLength + (int32)NullTerminated;

    AllocatorType::ResizeAllocation(0, buffer_size, sizeof(ToType));

    ptr = (ToType*)AllocatorType::GetAllocation();
    Converter::Convert(ptr, buffer_size, source, source_len + (int32)NullTerminated);
  }

public:
  explicit TStringConversion(const FromType* source)
  {
    if (source != nullptr)
    {
      Init(source, CharTraits<FromType>::Strlen(source), ENullTerminatedString::Yes);
    }
    else
    {
      ptr = nullptr;
      StringLength = 0;
    }
  }

  TStringConversion(const FromType* source, int32 source_len)
  {
    if (source != nullptr)
    {
      Init(source, source_len, ENullTerminatedString::No);
    }
    else
    {
      ptr = nullptr;
      StringLength = 0;
    }
  }

  /**
  Move constructor
  */
  TStringConversion(TStringConversion&& other)
    : Converter(MoveTemp((Converter&&)other))
  {
    AllocatorType::MoveToEmpty(other);
  }

  /**
  Accessor for the converted string.

  \return A const pointer to the null-terminated converted string.
  */
  inline const ToType* Get() const
  {
    return ptr;
  }

  /**
  length of the converted string.

  \return The number of characters in the converted string, excluding any null terminator.
  */
  inline int32 length() const
  {
    return StringLength;
  }

private:
  // Non-copyable
  TStringConversion(const TStringConversion&);
  TStringConversion& operator = (const TStringConversion&);

  ToType* ptr;
  int32 StringLength;
};


/**
NOTE: The objects these macros declare have very short lifetimes. They are
meant to be used as parameters to functions. You cannot assign a variable
to the contents of the converted string as the object will go out of
scope and the string released.

NOTE: The parameter you pass in MUST be a proper string, as the parameter
is typecast to a pointer. If you pass in a char, not char* it will compile
and then crash at runtime.

Usage:

     SomeApi(TCHAR_TO_ANSI(SomeUnicodeString));

     const char* SomePointer = TCHAR_TO_ANSI(SomeUnicodeString); <--- Bad!!!
*/

// These should be replaced with StringCasts when CPlatformString starts to know about UTF-8.
typedef TStringConversion<CTCHARToUTF8_Convert> CTCHARToUTF8;
typedef TStringConversion<CUTF8ToTCHAR_Convert> CUTF8ToTCHAR;

// Usage of these should be replaced with StringCasts.
#define TCHAR_TO_ANSI(str)  (ANSICHAR*)StringCast<ANSICHAR>(static_cast<const UNICHAR*>(str)).Get()
#define ANSI_TO_TCHAR(str)  (UNICHAR*)StringCast<UNICHAR>(static_cast<const ANSICHAR*>(str)).Get()
#define TCHAR_TO_UTF8(str)  (ANSICHAR*)CTCHARToUTF8((const UNICHAR*)str).Get()
#define UTF8_TO_TCHAR(str)  (UNICHAR*)CUTF8ToTCHAR((const ANSICHAR*)str).Get()

// This seemingly-pointless class is intended to be API-compatible with TStringConversion
// and is returned by StringCast when no string conversion is necessary.
template <typename T>
class TStringPointer
{
public:
  inline explicit TStringPointer(const T* InPtr)
    : ptr(InPtr)
  {}

  inline const T* Get() const
  {
    return ptr;
  }

  inline int32 length() const
  {
    return ptr ? CharTraits<T>::Strlen(ptr) : 0;
  }

private:
  const T* ptr;
};

/**
StringCast example usage:

void Func(const string& str)
{
    auto src = StringCast<ANSICHAR>();
    const ANSICHAR* ptr = src.Get(); // ptr is a pointer to an ANSICHAR representing the potentially-converted string data.
}
*/

/**
Creates an object which acts as a source of a given string type.  See example above.

\param str - The null-terminated source string to convert.
*/
template <typename To, typename from>
inline typename EnableIf<CPlatformString::TAreEncodingsCompatible<To, from>::Value, TStringPointer<To>>::Type
  StringCast(const from* str)
{
  return TStringPointer<To>((const To*)str);
}

/**
Creates an object which acts as a source of a given string type.  See example above.

\param str - The null-terminated source string to convert.
*/
template <typename To, typename from>
inline typename EnableIf<!CPlatformString::TAreEncodingsCompatible<To, from>::Value, TStringConversion<TStringConvert<from, To>>>::Type
  StringCast(const from* str)
{
  return TStringConversion<TStringConvert<from, To>>(str);
}

/**
Creates an object which acts as a source of a given string type.  See example above.

\param str - The source string to convert, not necessarily null-terminated.
\param Len - The number of from elements in str.
*/
template <typename To, typename from>
inline typename EnableIf<CPlatformString::TAreEncodingsCompatible<To, from>::Value, TStringPointer<To>>::Type
  StringCast(const from* str, int32 Len)
{
  return TStringPointer<To>((const To*)str);
}

/**
Creates an object which acts as a source of a given string type.  See example above.

\param str - The source string to convert, not necessarily null-terminated.
\param Len - The number of from elements in str.
*/
template <typename To, typename from>
inline typename EnableIf<!CPlatformString::TAreEncodingsCompatible<To, from>::Value, TStringConversion<TStringConvert<from, To>>>::Type
  StringCast(const from* str, int32 Len)
{
  return TStringConversion<TStringConvert<from, To>>(str, Len);
}


/**
Casts one fixed-width char type into another.

\param ch - The ch to convert.

\return The converted ch.
*/
template <typename To, typename from>
inline To CharCast(from ch)
{
  To result;
  CPlatformString::Convert(&result, 1, &ch, 1, (To)UNICODE_BOGUS_CHAR_CODEPOINT);
  return result;
}

/**
This class is returned by StringPassthru and is not intended to be used directly.
*/
template <typename ToType, typename FromType>
class TStringPassthru
  : private InlineAllocator<DEFAULT_STRING_CONVERSION_SIZE>::template ForElementType<FromType>
{
  typedef typename InlineAllocator<DEFAULT_STRING_CONVERSION_SIZE>::template ForElementType<FromType> AllocatorType;

public:
  inline TStringPassthru(ToType* dst, int32 dst_len, int32 src_len)
    : dst(dst)
    , dst_len(dst_len)
    , src_len(src_len)
  {
    AllocatorType::ResizeAllocation(0, src_len, sizeof(FromType));
  }

  inline TStringPassthru(TStringPassthru&& other)
  {
    AllocatorType::MoveToEmpty(other);
  }

  inline void Apply() const
  {
    const FromType* source = (const FromType*)AllocatorType::GetAllocation();
    fun_check(CPlatformString::ConvertedLength<ToType>(source, src_len) <= dst_len);
    CPlatformString::Convert(dst, dst_len, source, src_len);
  }

  inline FromType* Get()
  {
    return (FromType*)AllocatorType::GetAllocation();
  }

private:
  // Non-copyable
  TStringPassthru(const TStringPassthru&);
  TStringPassthru& operator = (const TStringPassthru&);

  ToType* dst;
  int32 dst_len;
  int32 src_len;
};


/**
This seemingly-pointless class is intended to be API-compatible with TStringPassthru
and is returned by StringPassthru when no string conversion is necessary.
*/
template <typename T>
class TPassthruPointer
{
public:
  inline explicit TPassthruPointer(T* ptr)
    : ptr(ptr)
  {}

  inline T* Get() const
  {
    return ptr;
  }

  inline void Apply() const
  {
  }

private:
  T* ptr;
};


/**
Allows the efficient conversion of strings by means of a temporary memory buffer only when necessary.  Intended to be used
when you have an API which populates a buffer with some string representation which is ultimately going to be stored in another
representation, but where you don't want to do a conversion or create a temporary buffer for that string if it's not necessary.

Intended use:

// Populates the buffer str with StrLen characters.
void SomeAPI(APICharType* str, int32 StrLen);

void Func(DestChar* buffer, int32 buffer_size)
{
    // Create a passthru.  This takes the buffer (and its size) which will ultimately hold the string, as well as the length of the
    // string that's being converted, which must be known in advance.
    // An explicit template argument is also passed to indicate the ch type of the source string.
    // buffer must be correctly typed for the destination string type.
    auto Passthru = StringMemoryPassthru<APICharType>(buffer, buffer_size, src_len);

    // Passthru.Get() returns an APICharType buffer pointer which is guaranteed to be src_len characters in size.
    // It's possible, and in fact intended, for Get() to return the same pointer as buffer if DestChar and APICharType are
    // compatible string types.  If this is the case, SomeAPI will write directly into buffer.  If the string types are not
    // compatible, Get() will return a pointer to some temporary memory which allocated by and owned by the passthru.
    SomeAPI(Passthru.Get(), src_len);

    // If the string types were not compatible, then the passthru used temporary storage, and we need to write that back to buffer.
    // We do that with the Apply call.  If the string types were compatible, then the data was already written to buffer directly
    // and so Apply is a no-op.
    Passthru.Apply();

    // Now buffer holds the data output by SomeAPI, already converted if necessary.
}
*/
template <typename from, typename To>
inline typename EnableIf<CPlatformString::TAreEncodingsCompatible<To, from>::Value, TPassthruPointer<from>>::Type
  StringMemoryPassthru(To* buffer, int32 buffer_size, int32 src_len)
{
  fun_check(src_len <= buffer_size);
  return TPassthruPointer<from>((from*)buffer);
}

template <typename from, typename To>
inline typename EnableIf<!CPlatformString::TAreEncodingsCompatible<To, from>::Value, TStringPassthru<To, from>>::Type
  StringMemoryPassthru(To* buffer, int32 buffer_size, int32 src_len)
{
  return TStringPassthru<To, from>(buffer, buffer_size, src_len);
}

template <typename ToType, typename FromType>
inline Array<ToType> StringToArray(const FromType* src, int32 src_len)
{
  const int32 dst_len = CPlatformString::ConvertedLength<UNICHAR>(src, src_len);
  Array<ToType> result;
  result.AddUninitialized(dst_len);
  CPlatformString::Convert(result.ConstData(), dst_len, src, src_len);
  return result;
}

template <typename ToType, typename FromType>
inline Array<ToType> StringToArray(const FromType* str)
{
  return ToArray(str, CharTraits<FromType>::Strlen(str) + 1);
}

} // namespace fun
