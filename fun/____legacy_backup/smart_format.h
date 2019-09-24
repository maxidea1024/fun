//TODO 코드를 패기하고 새로운 모드를 수정적용하자.

#pragma once

#pragma warning(disable : 4628)

#include <cassert>
#include <clocale>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <utility> // for std::pair

#include <iterator>

namespace fun {

//TODO?
#define SF_THROW(X)
#define SF_CONCAT(A, B)  A ## B

namespace sf {

using LongLong = int64;
using ULongLong = uint64;

namespace internal {

  template <typename T>
  inline T ConstCheck(T value) { return value; }

} // namespace internal

} // namespace sf


namespace sf {

template <typename CharType>
class BasicWriter;

using WriteA = BasicWriter<ANSICHAR>;
using WriteW = BasicWriter<WIDECHAR>;

template <typename CharType>
class CArgFormatter;

struct FormatSpec;

template <typename Impl, typename CharType, typename SpecT = FormatSpec>
class BasicPrintfArgFormatter;

template <typename CharType, typename ArgFormatter = ArgFormatter<CharType>>
class BasicFormatter;

template <typename CharType>
class BasicStringRef
{
 public:
  BasicStringRef(const CharType* s, size_t len)
    : data_(s)
    , len_(len)
  {
  }

  BasicStringRef(const CharType* s)
    : data_(s)
    , len_(std::char_traits<CharType>::length(s))
  {
  }

  BasicStringRef(const CharType* begin, const CharType* end)
    : data_(begin)
    , len_(end - begin)
  {
  }

  template <typename Allocator>
  BasicStringRef(const std::basic_string<CharType, std::char_traits<CharType>, Allocator>& s)
    : data_(s.c_str())
    , len_(s.size())
  {
  }

  //TODO 꼭 필요할까??
  std::basic_string<CharType> ToString() const
  {
    return std::basic_string<CharType>(data_, len_);
  }

  const CharType* ConstData() const
  {
    return data_;
  }

  size_t Len() const
  {
    return len_;
  }

  int32 Compare(BasicStringRef other) const
  {
    const size_t len = len_ < other.len_ ? len_ : other.len_;
    int32 result = std::char_traits<CharType>::compare(data_, other.data_, len);
    if (result == 0)
    {
      result = len_ == other.len_ ? 0 : (len_ < other.len_ ? -1 : 1);
    }
    return result;
  }

  friend bool operator == (BasicStringRef lhs, BasicStringRef rhs) { return lhs.Compare(rhs) == 0; }
  friend bool operator != (BasicStringRef lhs, BasicStringRef rhs) { return lhs.Compare(rhs) != 0; }
  friend bool operator <  (BasicStringRef lhs, BasicStringRef rhs) { return lhs.Compare(rhs) <  0; }
  friend bool operator <= (BasicStringRef lhs, BasicStringRef rhs) { return lhs.Compare(rhs) <= 0; }
  friend bool operator >  (BasicStringRef lhs, BasicStringRef rhs) { return lhs.Compare(rhs) >  0; }
  friend bool operator >= (BasicStringRef lhs, BasicStringRef rhs) { return lhs.Compare(rhs) >= 0; }

 private:
  const CharType* data_;
  size_t len_;
};

typedef BasicStringRef<char   > CStringRefA;
typedef BasicStringRef<wchar_t> CStringRefW;


template <typename CharType>
class BasicCStringRef
{
 public:
  BasicCStringRef(const CharType* str)
    : data_(str)
  {
  }

  template <typename Allocator>
  BasicCStringRef(const std::basic_string<CharType, std::char_traits<CharType>, Allocator>& str)
    : data_(str.c_str())
  {
  }

  const CharType* c_str() const
  {
    return data_;
  }

 private:
  const CharType* data_;
};

typedef BasicCStringRef<char   > CStringRefA;
typedef BasicCStringRef<wchar_t> CStringRefW;


namespace internal {

//TODO 기본 템플릿 라이브러리 코드쪽으로 빼주는게 좋을듯...
// MakeUnsigned<T>::Type gives an unsigned type corresponding to integer type T.
template <typename T>
struct MakeUnsigned { typedef T Type; };

#define SF_SPECIALIZE_MAKE_UNSIGNED(T, U) \
  template <> struct MakeUnsigned<T> { typedef U Type; }

SF_SPECIALIZE_MAKE_UNSIGNED(char,         unsigned char);
SF_SPECIALIZE_MAKE_UNSIGNED(signed char,  unsigned char);
SF_SPECIALIZE_MAKE_UNSIGNED(short,        unsigned short);
SF_SPECIALIZE_MAKE_UNSIGNED(int,          unsigned);
SF_SPECIALIZE_MAKE_UNSIGNED(long,         unsigned long);
SF_SPECIALIZE_MAKE_UNSIGNED(long long,    unsigned long long);

// Casts nonnegative integer to unsigned.
template <typename Int>
inline typename MakeUnsigned<Int>::Type ToUnsigned(Int value)
{
  fun_check_msg(value >= 0, "negative value");
  return static_cast<typename MakeUnsigned<Int>::Type>(value);
}


// The number of characters to store in the MemoryBuffer object itself
// to avoid dynamic memory allocation.
enum { INLINE_BUFFER_SIZE = 500 };


#if SF_SECURE_SCL
// Use checked iterator to avoid warnings on MSVC.
template <typename T>
inline stdext::checked_array_iterator<T*> MakePtr(T* ptr, size_t len)
{
  return stdext::checked_array_iterator<T*>(ptr, len);
}
#else
template <typename T>
inline T* MakePtr(T *ptr, size_t) { return ptr; }
#endif

} // namespace internal


template <typename T>
class Buffer
{
  Buffer(const Buffer&) = delete;
  Buffer& operator = (const Buffer&) = delete;

 public:
  virtual ~Buffer()
  {
  }

  size_t Len() const
  {
    return len_;
  }

  size_t Capacity() const
  {
    return capacity_;
  }

  void Resize(size_t new_len)
  {
    if (new_len > capacity_)
    {
      Grow(new_len);
    }

    len_ = new_len;
  }

  void Reserve(size_t capacity)
  {
    if (capacity > capacity_)
    {
      Grow(capacity);
    }
  }

  void Clear() noexcept
  {
    len_ = 0;
  }

  void Add(const T& value)
  {
    if (len_ == capacity_)
    {
      Grow(len_ + 1);
    }

    ptr_[len_++] = value;
  }

  template <typename U>
  void Append(const U* Begin, const U* end);

  T& operator[](size_t index)
  {
    return ptr_[index];
  }

  const T& operator[](size_t index) const
  {
    return ptr_[index];
  }

 protected:
  T* ptr_;
  size_t len_;
  size_t capacity_;

  Buffer(T* ptr = nullptr, size_t capacity = 0)
    : ptr_(ptr)
    , len_(0)
    , capacity_(capacity)
  {
  }

  virtual void Grow(size_t len) = 0;
};

template <typename T>
template <typename U>
void Buffer<T>::Append(const U* begin, const U* end)
{
  fun_check_msg(end >= begin, "negative value");

  const size_t new_len = len_ + (end - begin);
  if (new_len > capacity_)
  {
    Grow(new_len);
  }

  std::uninitialized_copy(begin, end, internal::MakePtr(ptr_, capacity_) + len_);
  len_ = new_len;
}


namespace internal {

// A memory buffer for trivially copyable/constructible types with the first
// N elements stored in the object itself.
template <typename T, size_t N, typename Allocator = std::allocator<T> >
class MemoryBuffer : private Allocator, public Buffer<T>
{
 public:
  explicit MemoryBuffer(const Allocator& alloc = Allocator())
    : Allocator(alloc)
    , Buffer<T>(data_, N)
  {
  }

  ~MemoryBuffer()
  {
    Deallocate();
  }

  MemoryBuffer(MemoryBuffer&& other)
  {
    Move(other);
  }

  MemoryBuffer& operator = (MemoryBuffer&& other)
  {
    fun_check(&other != this);
    Deallocate();
    Move(other);
    return *this;
  }

  // Returns a copy of the allocator associated with this buffer.
  Allocator GetAllocator() const
  {
    return *this;
  }

 protected:
  void Grow(size_t len) override;

 private:
  T data_[N];

  void Deallocate()
  {
    if (this->ptr_ != data_)
    {
      Allocator::deallocate(this->ptr_, this->capacity_);
    }
  }

  // Move data from other to this buffer.
  void Move(MemoryBuffer& other)
  {
    Allocator& this_alloc = *this;
    Allocator& other_alloc = other;

    this_alloc = MoveTemp(other_alloc);
    this->len_ = other.len_;
    this->capacity_ = other.capacity_;
    if (other.ptr_ == other.data_)
    {
      this->ptr_ = data_;
      std::uninitialized_copy(other.data_, other.data_ + this->len_, MakePtr(data_, this->capacity_));
    }
    else
    {
      this->ptr_ = other.ptr_;
      // Set pointer to the inline array so that delete is not called when deallocating.
      other.ptr_ = other.data_;
    }
  }
};

template <typename T, size_t N, typename Allocator>
void MemoryBuffer<T, N, Allocator>::Grow(size_t len)
{
  size_t new_capacity = this->capacity_ + this->capacity_ / 2;
  if (len > new_capacity)
  {
    new_capacity = len;
  }

  T* new_ptr = this->allocate(new_capacity, nullptr);
  // The following code doesn't throw, so the raw pointer above doesn't leak.
  std::uninitialized_copy(this->ptr_, this->ptr_ + this->len_, MakePtr(new_ptr, new_capacity));

  const size_t old_capacity = this->capacity_;
  T* old_ptr = this->ptr_;
  this->capacity_ = new_capacity;
  this->ptr_ = new_ptr;

  // deallocate may throw (at least in principle), but it doesn't matter since
  // the buffer already uses the new storage and will deallocate it in case
  // of exception.
  if (old_ptr != data_)
  {
    Allocator::deallocate(old_ptr, old_capacity);
  }
}


// A fixed-size buffer.
template <typename CharType>
class FixedBuffer : public Buffer<CharType>
{
 public:
  FixedBuffer(CharType* array, size_t len)
    : Buffer<CharType>(array, len)
  {
  }

 protected:
  FUN_BASE_API void Grow(size_t len) override;
};


template <typename CharType>
class BasicCharTraits
{
 public:
  typedef stdext::checked_array_iterator<CharType*> CharPtr;

  static CharType Cast(int32 value)
  {
    return static_cast<CharType>(value);
  }
};


template <typename CharType>
class CharTraits;

template <>
class CharTraits<char> : public BasicCharTraits<char>
{
 private:
  static char Convert(wchar_t);

 public:
  static char Convert(char value)
  {
    return value;
  }

  // Formats a floating-point number.
  template <typename T>
  FUN_BASE_API static int32 FormatFloat(char* buffer,
                                        size_t len,
                                        const char* format,
                                        uint32 width,
                                        int32 precision,
                                        T value);
};

template <>
class CharTraits<wchar_t> : public BasicCharTraits<wchar_t>
{
 public:
  static wchar_t Convert(char value)
  {
    return value;
  }

  static wchar_t Convert(wchar_t value)
  {
    return value;
  }

  template <typename T>
  FUN_BASE_API static int32 FormatFloat(wchar_t* buffer,
                                        size_t len,
                                        const wchar_t* format,
                                        uint32 width,
                                        int32 precision,
                                        T value);
};


// Checks if a number is negative - used to avoid warnings.
template <bool IsSigned>
struct SignChecker
{
  template <typename T>
  static bool IsNegative(T value) { return value < T(0); }
};

template <>
struct SignChecker<false>
{
  template <typename T>
  static bool IsNegative(T) { return false; }
};

// Returns true if value is negative, false otherwise.
// Same as (value < 0) but doesn't produce warnings if T is an unsigned type.
template <typename T>
inline bool IsNegative(T value)
{
  return SignChecker<std::numeric_limits<T>::is_signed>::IsNegative(value);
}


// Selects uint32_t if FitsIn32Bits is true, uint64 otherwise.
template <bool FitsIn32Bits>
struct TypeSelector { typedef uint32_t Type; };

template <>
struct TypeSelector<false> { typedef uint64 Type; };

template <typename T>
struct IntTraits
{
  // Smallest of uint32_t and uint64 that is large enough to represent all values of T.
  typedef typename TypeSelector<std::numeric_limits<T>::digits <= 32>::Type MainType;
};


FUN_BASE_API void ReportUnknownType(char code, const char* type);


// Static data is placed in this class template to allow header-only configuration.
template <typename T = void>
struct FUN_BASE_API BasicData
{
  static const uint32_t POWERS_OF_10_32[];
  static const uint64 POWERS_OF_10_64[];
  static const char DIGITS[];
};

typedef BasicData<> Data;


//TODO 별도의 파일로 빼주는게 좋을듯...??

#ifdef SF_BUILTIN_CLZLL
// Returns the number of decimal digits in n. leading zeros are not counted
// except for n == 0 in which case CountDigits returns 1.
inline uint32 CountDigits(uint64 n)
{
  // Based on http://graphics.stanford.edu/~seander/bithacks.html#IntegerLog10
  // and the benchmark https://github.com/localvoid/cxx-benchmark-count-digits.
  const int32 t = (64 - SF_BUILTIN_CLZLL(n | 1)) * 1233 >> 12;
  return ToUnsigned(t) - (n < Data::POWERS_OF_10_64[t]) + 1;
}
#else
// Fallback version of CountDigits used when __builtin_clz is not available.
inline uint32 CountDigits(uint64 n)
{
  uint32 count = 1;
  for (;;)
  {
    // Integer division is slow so do it for a group of four digits instead
    // of for every digit. The idea comes from the talk by Alexandrescu
    // "Three Optimization Tips for c++". See speed-test for a comparison.
    if (n < 10) return count;
    if (n < 100) return count + 1;
    if (n < 1000) return count + 2;
    if (n < 10000) return count + 3;
    n /= 10000u;
    count += 4;
  }
}
#endif

#ifdef SF_BUILTIN_CLZ
// Optional version of CountDigits for better performance on 32-bit platforms.
inline uint32 CountDigits(uint32_t n)
{
  const int32 t = (32 - SF_BUILTIN_CLZ(n | 1)) * 1233 >> 12;
  return ToUnsigned(t) - (n < Data::POWERS_OF_10_32[t]) + 1;
}
#endif


// A functor that doesn't add a thousands separator.
struct NoThousandsSep
{
  template <typename CharType>
  void operator()(CharType*) {}
};


// A functor that adds a thousands separator.
class AddThousandsSep
{
 public:
  explicit AddThousandsSep(::fun::sf::CStringRefA sep)
    : sep_(sep)
    , digit_index_(0)
  {
  }

  template <typename CharType>
  void operator () (CharType*& buffer)
  {
    if (++digit_index_ % 3 == 0)
    {
      buffer -= sep_.Len();

      std::uninitialized_copy(sep_.ConstData(),
                              sep_.ConstData() + sep_.Len(),
                              internal::MakePtr(buffer, sep_.Len()));
    }
  }

 private:
  CStringRefA sep_;

  // index of a decimal digit with the least significant digit having index 0.
  uint32 digit_index_;
};


// Formats a decimal unsigned integer value writing into buffer.
// AddThousandsSep is a functor that is called after writing each char to
// add a thousands separator if necessary.
template <typename UIntT, typename CharType, typename AddThousandsSep>
inline void FormatDecimal(CharType* buffer,
                          UIntT value,
                          uint32 digit_count,
                          AddThousandsSep thousands_sep)
{
  buffer += digit_count;

  while (value >= 100)
  {
    // Integer division is slow so do it for a group of two digits instead
    // of for every digit. The idea comes from the talk by Alexandrescu
    // "Three Optimization Tips for c++". See speed-test for a comparison.
    const uint32 index = static_cast<uint32>((value % 100) * 2);
    value /= 100;
    *--buffer = Data::DIGITS[index + 1];
    thousands_sep(buffer);
    *--buffer = Data::DIGITS[index];
    thousands_sep(buffer);
  }

  if (value < 10)
  {
    *--buffer = static_cast<char>('0' + value);
    return;
  }

  const uint32 index = static_cast<uint32>(value * 2);
  *--buffer = Data::DIGITS[index + 1];
  thousands_sep(buffer);
  *--buffer = Data::DIGITS[index];
}

template <typename UIntT, typename CharType>
inline void FormatDecimal(CharType* buffer, UIntT value, uint32 digit_count)
{
  FormatDecimal(buffer, value, digit_count, NoThousandsSep());
}



/*

template <unsigned BASE_BITS, typename Char, typename UInt>
inline Char* FormatUInt(Char* buffer, UInt value, unsigned digit_count, bool upper = false)
{
  buffer += digit_count;

  Char* end = buffer;
  do
  {
    const char* digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    unsigned digit = (value & ((1 << BASE_BITS) - 1));
    *--buffer = BASE_BITS < 4 ? static_cast<char>('0' + digit) : digits[digit];
  } while ((value >>= BASE_BITS) != 0);
  return end;
}

template <unsigned BASE_BITS, typename It, typename UInt>
inline It FormatUInt(It out, UInt value, unsigned digit_count, bool upper = false)
{
  // Buffer should be large enough to hold all digits (digits / BASE_BITS + 1)
  // and null.

  char buffer[std::numeric_limits<UInt>::digits / BASE_BITS + 2];
  FormatUInt<BASE_BITS>(buffer, value, digit_count, upper);
  return std::copy_n(buffer, digit_count, out);
}


*/





class Value
{
 public:
  template <typename CharType>
  struct CStringValue
  {
    const CharType* value;
    size_t len;
  };

  //warning format_str_ptr은 증가형임.
  typedef void (*FormatFunc)(void* formatter, const void* arg, void* format_str_ptr);

  struct Customvalue
  {
    const void* value;
    FormatFunc format;
  };

  union
  {
    int32 int32_value;
    uint32 uint32_value;

    int64 int64_value;
    uint64 uint64_value;

    double double_value;
    long double long_double_value;

    const void* pointer;

    CStringValue<char> String;
    CStringValue<signed char> SString;
    CStringValue<unsigned char> UString;
    CStringValue<wchar_t> WString;

    Customvalue custom;
  };

  enum Type
  {
    NONE,
    NAMED_ARG,
    INT,
    UINT,
    LONG_LONG,
    ULONG_LONG,
    BOOL,
    CHAR,
    LAST_INTEGER_TYPE = CHAR,
    DOUBLE,
    LONG_DOUBLE,
    LAST_NUMERIC_TYPE = LONG_DOUBLE,
    CSTRING,
    STRING,
    WSTRING,
    POINTER,
    CUSTOM,
  };
};


struct Arg : public Value
{
  Type type;
};

template <typename CharType>
struct NamedArg;

template <typename CharType, typename T>
struct NamedArgWithType;


template <typename T = void>
struct Null {};

template <typename T,typename CharType>
struct WCharHelper
{
  typedef Null<T> Supported;
  typedef T Unsupported;
};

template <typename T>
struct WCharHelper<T,wchar_t>
{
  typedef T Supported;
  typedef Null<T> Unsupported;
};

typedef char Yes[1];
typedef char No[2];

template <typename T>
T& Get();

// These are non-members to workaround an overload resolution bug in bcc32.
Yes& Convert(::fun::sf::ULongLong);
No& Convert(...);

template <typename T, bool ENABLE_CONVERSION>
struct ConvertToIntImpl
{
  enum { Value = ENABLE_CONVERSION };
};

template <typename T, bool ENABLE_CONVERSION>
struct ConvertToIntImpl2
{
  enum { Value = false };
};

template <typename T>
struct ConvertToIntImpl2<T, true>
{
    // Don't convert numeric types.
  enum { Value = ConvertToIntImpl<T, !std::numeric_limits<T>::is_specialized>::Value };
};

template <typename T>
struct ConvertToInt
{
  enum { EnableConversion = sizeof(internal::Convert(Get<T>())) == sizeof(Yes) };
  enum { Value = ConvertToIntImpl2<T, EnableConversion>::Value };
};

#define SF_DISABLE_CONVERSION_TO_INT(Type) \
  template <> struct ConvertToInt<Type> { enum { Value = 0 }; }

// Silence warnings about convering float to int.
SF_DISABLE_CONVERSION_TO_INT(float);
SF_DISABLE_CONVERSION_TO_INT(double);
SF_DISABLE_CONVERSION_TO_INT(long double);

//@TODO 제거...
template <bool B, typename T = void>
struct EnableIf {};
template <typename T>
struct EnableIf<true, T> { typedef T Type; };

// For bcc32 which doesn't understand ! in template arguments.
template <bool>
struct Not { enum { Value = 0 }; };

template <>
struct Not<false> { enum { Value = 1 }; };

template <typename T>
struct FalseType { enum { Value = 0 }; };

template <typename T, T>
struct LConvCheck
{
  LConvCheck(int32) {}
};

// Returns the thousands separator for the current locale.
// We check if ``lconv`` contains ``AddThousandsSep`` because on Android
// ``lconv`` is stubbed as an empty struct.
template <typename LConv>
inline CStringRefA AddThousandsSep(LConv* LC, LConvCheck<char* LConv::*, &LConv::AddThousandsSep> = 0)
{
  return LC->AddThousandsSep;
}

//기본적으로 3자리마다 ','를 넣어줌.
//만약, 로케일에 따라서 처리가 된 경우에는 ',' 대신에 각 로케일에 맞는 문자가 반영될것임.
inline CStringRefA AddThousandsSep(...) { return ","; }

//TODO operator << 를 지원한다고 해서 되는건지??
template <typename FormatterT, typename CharType, typename T>
void FormatArg(FormatterT&, const CharType*, const T&)
{
  static_assert(FalseType<T>::Value, "Cannot format argument. To enable the use of ostream operator <<  include Fmt/ostream.h. Otherwise provide an overload of FormatArg.");
}



// Makes an Arg object from any type.
template <typename FormatterT>
class MakeValue : public Arg
{
 public:
  typedef typename FormatterT::CharType CharType;

 private:
  // The following two methods are private to disallow formatting of
  // arbitrary pointers. If you want to output a pointer cast it to
  // "void *" or "const void *". In particular, this forbids formatting
  // of "[const] volatile char* " which is printed as bool by iostreams.
  // Do not implement!
  template <typename T> MakeValue(const T* value);
  template <typename T> MakeValue(T* value);

  // The following methods are private to disallow formatting of wide
  // characters and strings into narrow strings as in
  //   Format("{}", L"test");
  // To fix this, use a wide format string: Format(L"{}", L"test").
#if !defined(_MSC_VER) || defined(_NATIVE_WCHAR_T_DEFINED)
  MakeValue(typename WCharHelper<wchar_t, CharType>::Unsupported);
#endif
  MakeValue(typename WCharHelper<wchar_t*, CharType>::Unsupported);
  MakeValue(typename WCharHelper<const wchar_t*, CharType>::Unsupported);
  MakeValue(typename WCharHelper<const std::wstring&, CharType>::Unsupported);
  MakeValue(typename WCharHelper<CStringRefW, CharType>::Unsupported);

  void SetString(CStringRefA str)
  {
    String.value = str.ConstData();
    String.len = str.Len();
  }

  void SetString(CStringRefW str)
  {
    WString.value = str.ConstData();
    WString.len = str.Len();
  }

  // Formats an argument of a custom type, such as a user-defined class.
  template <typename T>
  static void FormatCustomArg(void* formatter, const void* arg, void* format_str_ptr)
  {
    FormatArg(*static_cast<FormatterT*>(formatter), *static_cast<const CharType**>(format_str_ptr), *static_cast<const T*>(arg));
  }

 public:
  MakeValue() {}

#define SF_MAKE_VALUE_(_Type, Field, TYPE, rhs) \
  MakeValue(_Type value) { Field = rhs; } \
  static uint64 Type(_Type) { return Arg::TYPE; }

#define SF_MAKE_VALUE(_Type, Field, TYPE) \
  SF_MAKE_VALUE_(_Type, Field, TYPE, value);

  SF_MAKE_VALUE(bool, int32_value, BOOL);
  SF_MAKE_VALUE(short, int32_value, INT);
  SF_MAKE_VALUE(unsigned short, uint32_value, UINT);
  SF_MAKE_VALUE(int, int32_value, INT);
  SF_MAKE_VALUE(unsigned, uint32_value, UINT);

  MakeValue(long value)
  {
    // to minimize the number of types we need to deal with, long is
    // translated either to int or to long long depending on its size.
    if (ConstCheck(sizeof(long) == sizeof(int)))
    {
      int32_value = static_cast<int>(value);
    }
    else
    {
      int64_value = value;
    }
  }

  static uint64 Type(long)
  {
    return sizeof(long) == sizeof(int) ? Arg::INT : Arg::LONG_LONG;
  }

  MakeValue(unsigned long value)
  {
    if (ConstCheck(sizeof(unsigned long) == sizeof(unsigned)))
    {
      uint32_value = static_cast<unsigned>(value);
    }
    else
    {
      uint64_value = value;
    }
  }

  static uint64 Type(unsigned long)
  {
    return sizeof(unsigned long) == sizeof(unsigned) ? Arg::UINT : Arg::ULONG_LONG;
  }

  SF_MAKE_VALUE(LongLong,       int64_value,        LONG_LONG)
  SF_MAKE_VALUE(ULongLong,      uint64_value,       ULONG_LONG)
  SF_MAKE_VALUE(float,          double_value,       DOUBLE)
  SF_MAKE_VALUE(double,         double_value,       DOUBLE)
  SF_MAKE_VALUE(long double,    long_double_value,  LONG_DOUBLE)
  SF_MAKE_VALUE(signed char,    int32_value,        INT)
  SF_MAKE_VALUE(unsigned char,  uint32_value,       UINT)
  SF_MAKE_VALUE(char,           int32_value,        CHAR)

#if !defined(_MSC_VER) || defined(_NATIVE_WCHAR_T_DEFINED)
  MakeValue(typename WCharHelper<wchar_t, CharType>::Supported value)
  {
    int32_value = value;
  }

  static uint64 Type(wchar_t) { return Arg::CHAR; }
#endif

#define SF_MAKE_STR_VALUE(_Type, TYPE) \
  MakeValue(_Type value) { SetString(value); } \
  static uint64 Type(_Type) { return Arg::TYPE; }

  SF_MAKE_VALUE(char*, String.value, CSTRING);
  SF_MAKE_VALUE(const char*, String.value, CSTRING);
  SF_MAKE_VALUE(signed char*, SString.value, CSTRING);
  SF_MAKE_VALUE(const signed char*, SString.value, CSTRING);
  SF_MAKE_VALUE(unsigned char*, UString.value, CSTRING);
  SF_MAKE_VALUE(const unsigned char*, UString.value, CSTRING);
  SF_MAKE_STR_VALUE(const std::string&, STRING);
  SF_MAKE_STR_VALUE(CStringRefA, STRING);
  SF_MAKE_VALUE_(CStringRefA, String.value, CSTRING, value.c_str());

#define SF_MAKE_WSTR_VALUE(_Type, TYPE) \
  MakeValue(typename WCharHelper<_Type, CharType>::Supported value) { SetString(value); } \
  static uint64 Type(_Type) { return Arg::TYPE; }

  // wide string.
  SF_MAKE_WSTR_VALUE(wchar_t*, WSTRING);
  SF_MAKE_WSTR_VALUE(const wchar_t*, WSTRING);
  SF_MAKE_WSTR_VALUE(const std::wstring&, WSTRING);
  SF_MAKE_WSTR_VALUE(CStringRefW, WSTRING);

  SF_MAKE_VALUE(void*, pointer, POINTER);
  SF_MAKE_VALUE(const void*, pointer, POINTER);

  template <typename T>
  MakeValue(const T& value, typename EnableIf<Not<ConvertToInt<T>::Value>::Value, int32>::Type = 0)
  {
    custom.value = &value;
    custom.Format = &FormatCustomArg<T>;
  }

  template <typename T>
  static typename EnableIf<Not<ConvertToInt<T>::Value>::Value, uint64>::Type
    Type(const T&)
  {
    return Arg::CUSTOM;
  }

  // Additional template param `CharType` is needed here because MakeType always uses char.
  template <typename CharType>
  MakeValue(const NamedArg<CharType>& value)
  {
    pointer = &value;
  }

  template <typename CharType, typename T>
  MakeValue(const NamedArgWithType<CharType, T>& value)
  {
    pointer = &value;
  }

  template <typename CharType>
  static uint64 Type(const NamedArg<CharType>&)
  {
    return Arg::NAMED_ARG;
  }

  template <typename CharType, typename T>
  static uint64 Type(const NamedArgWithType<CharType, T>&)
  {
    return Arg::NAMED_ARG;
  }
};


template <typename FormatterTy>
struct MakeArg : public Arg
{
  MakeArg()
  {
    type = Arg::NONE;
  }

  template <typename T>
  MakeArg(const T& value)
    : Arg(MakeValue<FormatterTy>(value))
  {
    type = static_cast<Arg::Type>(MakeValue<FormatterTy>::Type(value));
  }
};


template <typename CharType>
struct NamedArg : Arg
{
  BasicStringRef<CharType> name;

  template <typename T>
  NamedArg(BasicStringRef<CharType> arg_name, const T& value)
    : Arg(MakeArg< BasicFormatter<CharType> >(value))
    , name(arg_name)
  {
  }
};


template <typename CharType, typename T>
struct NamedArgWithType : NamedArg<CharType>
{
  NamedArgWithType(BasicStringRef<CharType> arg_name, const T& value)
    : NamedArg<CharType>(arg_name, value)
  {
  }
};


template <typename CharType>
class ArgMap;

} // namespace internal


/// An argument list.
class ArgList
{
 public:
  // Maximum number of arguments with packed types.
  enum { MAX_PACKED_ARGS = 16 };

  ArgList()
    : types_(0)
  {
  }

  ArgList(ULongLong types, const internal::Value* values)
    : types_(types)
    , values_(values)
  {
  }

  ArgList(ULongLong types, const internal::Arg* args)
    : types_(types)
    , args_(args)
  {
  }

  uint64 Types() const
  {
    return types_;
  }

  /// Returns the argument at specified index.
  internal::Arg operator[](uint32 index) const
  {
    using internal::Arg;

    Arg arg;
    const bool use_values = Type(MAX_PACKED_ARGS - 1) == Arg::NONE;
    if (index < MAX_PACKED_ARGS)
    {
      const Arg::Type arg_type = Type(index);
      internal::Value& val = arg;
      if (arg_type != Arg::NONE)
      {
        val = use_values ? values_[index] : args_[index];
      }
      arg.type = arg_type;
      return arg;
    }

    if (use_values)
    {
      // The index is greater than the number of arguments that can be stored
      // in values, so return a "none" argument.
      arg.type = Arg::NONE;
      return arg;
    }

    for (uint32 i = MAX_PACKED_ARGS; i <= index; ++i)
    {
      if (args_[i].type == Arg::NONE)
      {
        return args_[i];
      }
    }
    return args_[index];
  }

  static internal::Arg::Type Type(uint64 types, uint32 index)
  {
    const uint32 shift = index << 2; // x4
    const uint64 mask = 0xF;
    return static_cast<internal::Arg::Type>((types & (mask << shift)) >> shift);
  }

 private:
  // to reduce compiled code size per formatting function call, types of first
  // MAX_PACKED_ARGS arguments are passed in the types_ field.
  uint64 types_;

  union
  {
    // If the number of arguments is less than MAX_PACKED_ARGS, the argument
    // values are stored in values_, otherwise they are stored in args_.
    // This is done to reduce compiled code size as storing larger objects
    // may require more code (at least on x86-64) even if the same amount of
    // data is actually copied to stack. It saves ~10% on the bloat test.
    const internal::Value* values_;
    const internal::Arg* args_;
  };

  internal::Arg::Type Type(uint32 index) const
  {
    return Type(types_, index);
  }

  template <typename CharType>
  friend class internal::ArgMap;
};


#define SF_DISPATCH(Call)  static_cast<Impl*>(this)->Call

template <typename Impl, typename ResultType>
class ArgVisitor
{
  typedef internal::Arg Arg;

 public:
  void ReportUnhandledArg()
  {
  }

  ResultType VisitUnhandledArg()
  {
    SF_DISPATCH(ReportUnhandledArg());
    return ResultType();
  }

  ResultType VisitInt(int32 value)
  {
    return SF_DISPATCH(VisitAnyInt(value));
  }

  ResultType VisitLongLong(LongLong value)
  {
    return SF_DISPATCH(VisitAnyInt(value));
  }

  ResultType VisitUInt(uint32 value)
  {
    return SF_DISPATCH(VisitAnyInt(value));
  }

  ResultType VisitULongLong(ULongLong value)
  {
    return SF_DISPATCH(VisitAnyInt(value));
  }

  ResultType VisitBool(bool value)
  {
    return SF_DISPATCH(VisitAnyInt(value));
  }

  ResultType VisitChar(int32 value)
  {
    return SF_DISPATCH(VisitAnyInt(value));
  }

  template <typename T>
  ResultType VisitAnyInt(T)
  {
    return SF_DISPATCH(VisitUnhandledArg());
  }

  ResultType VisitDouble(double value)
  {
    return SF_DISPATCH(VisitAnyDouble(value));
  }

  ResultType VisitLongDouble(long double value)
  {
    return SF_DISPATCH(VisitAnyDouble(value));
  }

  template <typename T>
  ResultType VisitAnyDouble(T)
  {
    return SF_DISPATCH(VisitUnhandledArg());
  }

  ResultType VisitCString(const char*)
  {
    return SF_DISPATCH(VisitUnhandledArg());
  }

  ResultType VisitString(Arg::CStringValue<char>)
  {
    return SF_DISPATCH(VisitUnhandledArg());
  }

  ResultType VisitStringW(Arg::CStringValue<wchar_t>)
  {
    return SF_DISPATCH(VisitUnhandledArg());
  }

  ResultType VisitPointer(const void*)
  {
    return SF_DISPATCH(VisitUnhandledArg());
  }

  ResultType VisitCustom(Arg::Customvalue)
  {
    return SF_DISPATCH(VisitUnhandledArg());
  }

  ResultType Visit(const Arg& arg)
  {
    switch (arg.type) {
    case Arg::INT:
      return SF_DISPATCH(VisitInt(arg.int32_value));
    case Arg::UINT:
      return SF_DISPATCH(VisitUInt(arg.uint32_value));
    case Arg::LONG_LONG:
      return SF_DISPATCH(VisitLongLong(arg.int64_value));
    case Arg::ULONG_LONG:
      return SF_DISPATCH(VisitULongLong(arg.uint64_value));
    case Arg::BOOL:
      return SF_DISPATCH(VisitBool(arg.int32_value != 0));
    case Arg::CHAR:
      return SF_DISPATCH(VisitChar(arg.int32_value));
    case Arg::DOUBLE:
      return SF_DISPATCH(VisitDouble(arg.double_value));
    case Arg::LONG_DOUBLE:
      return SF_DISPATCH(VisitLongDouble(arg.long_double_value));
    case Arg::CSTRING:
      return SF_DISPATCH(VisitCString(arg.String.value));
    case Arg::STRING:
      return SF_DISPATCH(VisitString(arg.String));
    case Arg::WSTRING:
      return SF_DISPATCH(VisitStringW(arg.WString));
    case Arg::POINTER:
      return SF_DISPATCH(VisitPointer(arg.pointer));
    case Arg::CUSTOM:
      return SF_DISPATCH(VisitCustom(arg.custom));
    case Arg::NONE:
    case Arg::NAMED_ARG:
      fun_check_msg(false, "invalid argument type");
      break;
    }
    return ResultType();
  }
};


enum Alignment
{
  ALIGN_DEFAULT,
  ALIGN_LEFT,
  ALIGN_RIGHT,
  ALIGN_CENTER,
  ALIGN_NUMERIC
};


enum
{
  SIGN_FLAG = 1,
  PLUS_FLAG = 2,
  MINUS_FLAG = 4,
  HASH_FLAG = 8,
  CHAR_FLAG = 0x10  // Argument has char type - used in error reporting.
};


// An empty format specifier.
struct EmptySpec {};

// A type specifier.
template <char TYPE>
struct TypeSpec : EmptySpec
{
  Alignment Align() const { return ALIGN_DEFAULT; }
  uint32 Width() const { return 0; }
  int32 Precision() const { return -1; }
  bool Flag(uint32) const { return false; }
  char Type() const { return TYPE; }
  char TypePrefix() const { return TYPE; }
  char Fill() const { return ' '; }
};

// A width specifier.
class WidthSpec
{
 public:
  WidthSpec(uint32 width, wchar_t fill)
    : width_(width)
    , fill_(fill)
  {
  }

  uint32 Width() const { return width_; }
  wchar_t Fill() const { return fill_; }

 private:
  uint32 width_;
  // fill is always wchar_t and cast to char if necessary to avoid having
  // two specialization of WidthSpec and its subclasses.
  wchar_t fill_;
};

// An alignment specifier.
class AlignSpec : WidthSpec
{
 public:
  AlignSpec(uint32 width, wchar_t fill, Alignment align = ALIGN_DEFAULT)
    : WidthSpec(width, fill)
    , align_(align)
  {
  }

  Alignment Align() const { return align_; }
  int32 Precision() const { return -1; }

 private:
  Alignment align_;
};

// An alignment and type specifier.
template <char TYPE>
struct AlignTypeSpec : AlignSpec
{
  AlignTypeSpec(uint32 width, wchar_t fill)
    : AlignSpec(width, fill)
  {
  }

  bool Flag(uint32) const { return false; }
  char Type() const { return TYPE; }
  char TypePrefix() const { return TYPE; }
};

// A full format specifier.
class FormatSpec : AlignSpec
{
 public:
  FormatSpec(uint32 width = 0, char type = 0, wchar_t fill = ' ')
    : AlignSpec(width, fill)
    , flags_(0)
    , precision_(-1)
    , type_(Type)
  {
  }

  bool Flag(uint32 mask) const { return (flags_ & mask) != 0; }
  int32 Precision() const { return precision_; }
  char Type() const { return type_; }
  char TypePrefix() const { return type_; }

 private:
  uint32 flags_;
  int32 precision_;
  char type_;
};

// An integer format specifier.
template <typename T, typename SpecT = TypeSpec<0>, typename CharType = char>
class IntFormatSpec : public SpecT
{
 public:
  IntFormatSpec(T value, const SpecT& spec = SpecT())
    : SpecT(spec)
    , value_(value)
  {
  }

  T value() const { return value_; }

 private:
  T value_;
};

// A string format specifier.
template <typename CharType>
class StrFormatSpec : public AlignSpec
{
 public:
  template <typename fill_char>
  StrFormatSpec(const CharType* str, uint32 width, fill_char fill)
    : AlignSpec(width, fill)
    , str_(str)
  {
    internal::CharTraits<CharType>::Convert(fill_char());
  }

  const CharType* Str() const { return str_; }

 private:
  const CharType* str_;
};


IntFormatSpec<int32, TypeSpec<'b'> > Bin(int32 value);
IntFormatSpec<int32, TypeSpec<'o'> > Oct(int32 value);
IntFormatSpec<int32, TypeSpec<'x'> > Hex(int32 value);
IntFormatSpec<int32, TypeSpec<'X'> > Hexu(int32 value);

template <char TYPE_CODE, typename CharType>
IntFormatSpec<int32, AlignTypeSpec<TYPE_CODE>, CharType> Pad(int32 value, uint32 width, CharType fill = ' ');

#define SF_DEFINE_INT_FORMATTERS(TYPE) \
  inline IntFormatSpec<TYPE, TypeSpec<'b'> > Bin(TYPE value) \
  { \
    return IntFormatSpec<TYPE, TypeSpec<'b'> >(value, TypeSpec<'b'>()); \
  } \
  \
  inline IntFormatSpec<TYPE, TypeSpec<'o'> > Oct(TYPE value) \
  { \
    return IntFormatSpec<TYPE, TypeSpec<'o'> >(value, TypeSpec<'o'>()); \
  } \
  \
  inline IntFormatSpec<TYPE, TypeSpec<'x'> > Hex(TYPE value) \
  { \
    return IntFormatSpec<TYPE, TypeSpec<'x'> >(value, TypeSpec<'x'>()); \
  } \
  \
  inline IntFormatSpec<TYPE, TypeSpec<'X'> > Hexu(TYPE value) \
  { \
    return IntFormatSpec<TYPE, TypeSpec<'X'> >(value, TypeSpec<'X'>()); \
  } \
  \
  template <char TYPE_CODE> \
  inline IntFormatSpec<TYPE, AlignTypeSpec<TYPE_CODE> > Pad(IntFormatSpec<TYPE, TypeSpec<TYPE_CODE> > f, uint32 width) \
  { \
    return IntFormatSpec<TYPE, AlignTypeSpec<TYPE_CODE> >(f.Value(), AlignTypeSpec<TYPE_CODE>(width, ' ')); \
  } \
  \
  /* For compatibility with older compilers we provide two overloads for Pad, */ \
  /* one that takes a fill character and one that doesn't. In the future this */ \
  /* can be replaced with one overload making the template argument CharType */ \
  /* default to char (c++11). */ \
  template <char TYPE_CODE, typename CharType> \
  inline IntFormatSpec<TYPE, AlignTypeSpec<TYPE_CODE>, CharType> Pad(IntFormatSpec<TYPE, TypeSpec<TYPE_CODE>, CharType> f, uint32 width, CharType fill) \
  { \
    return IntFormatSpec<TYPE, AlignTypeSpec<TYPE_CODE>, CharType>(f.Value(), AlignTypeSpec<TYPE_CODE>(width, fill)); \
  } \
  \
  inline IntFormatSpec<TYPE, AlignTypeSpec<0> > Pad(TYPE value, uint32 width) \
  { \
    return IntFormatSpec<TYPE, AlignTypeSpec<0> >(value, AlignTypeSpec<0>(width, ' ')); \
  } \
  \
  template <typename CharType> \
  inline IntFormatSpec<TYPE, AlignTypeSpec<0>, CharType> Pad(TYPE value, uint32 width, CharType fill) \
  { \
    return IntFormatSpec<TYPE, AlignTypeSpec<0>, CharType>(value, AlignTypeSpec<0>(width, fill)); \
  }

SF_DEFINE_INT_FORMATTERS(int)
SF_DEFINE_INT_FORMATTERS(long)
SF_DEFINE_INT_FORMATTERS(unsigned)
SF_DEFINE_INT_FORMATTERS(unsigned long)
SF_DEFINE_INT_FORMATTERS(LongLong)
SF_DEFINE_INT_FORMATTERS(ULongLong)

template <typename CharType>
inline StrFormatSpec<CharType> Pad(const CharType* str, unsigned width, CharType fill = ' ')
{
  return StrFormatSpec<CharType>(str, width, fill);
}

inline StrFormatSpec<wchar_t> Pad(const wchar_t* str, unsigned width, char fill = ' ')
{
  return StrFormatSpec<wchar_t>(str, width, fill);
}



namespace internal {

template <typename CharType>
class ArgMap
{
 public:
  FUN_BASE_API void Init(const ArgList& args);

  const internal::Arg* Find(const BasicStringRef<CharType>& name) const
  {
    // The list is unsorted, so just return the first matching name.
    for (const auto& pair : map_)
    {
      if (pair.first == name)
      {
        return &pair.second;
      }
    }

    return nullptr;
  }

 private:
  typedef std::vector<std::pair<BasicStringRef<CharType>, Arg>> MapType;
  typedef typename MapType::Value_type Pair;

  MapType map_;
};


template <typename Impl, typename CharType, typename SpecT = FormatSpec>
class ArgFormatterBase : public ArgVisitor<Impl, void>
{
  ArgFormatterBase(const ArgFormatterBase&) = delete;
  ArgFormatterBase& operator = (const ArgFormatterBase&) = delete;

 public:
  typedef SpecT SpecType;

  ArgFormatterBase(BasicWriter<CharType>& writer, SpecT& spec)
    : writer_(writer)
    , spec_(spec)
  {
  }

  template <typename T>
  void VisitAnyInt(T value)
  {
    writer_.WriteInt(value, spec_);
  }

  template <typename T>
  void VisitAnyDouble(T value)
  {
    writer_.WriteDouble(value, spec_);
  }

  void VisitBool(bool value)
  {
    if (spec_.type_)
    {
      VisitAnyInt(value);
      return;
    }

    Write(value);
  }

  void VisitChar(int32 value)
  {
    if (spec_.type_ && spec_.type_ != 'c')
    {
      spec_.flags_ |= CHAR_FLAG;
      writer_.WriteInt(value, spec_);
      return;
    }

    if (spec_.align_ == ALIGN_NUMERIC || spec_.flags_ != 0)
    {
      SF_THROW(FormatError("invalid Format specifier for char"));
    }

    typedef typename BasicWriter<CharType>::CharPtr CharPtr;
    const CharType fill = internal::CharTraits<CharType>::Cast(spec_.Fill());
    CharPtr out = CharPtr();
    const uint32 char_len = 1;
    if (spec_.width_ > char_len)
    {
      out = writer_.GrowBuffer(spec_.width_);

      if (spec_.align_ == ALIGN_RIGHT) // right alignment
      {
        std::uninitialized_fill_n(out, spec_.width_ - char_len, fill);
        out += spec_.width_ - char_len;
      }
      else if (spec_.align_ == ALIGN_CENTER) // center alignment
      {
        out = writer_.FillPadding(out, spec_.width_, internal::ConstCheck(char_len), fill);
      }
      else // left alignment(default)
      {
        std::uninitialized_fill_n(out + char_len, spec_.width_ - char_len, fill);
      }
    }
    else
    {
      out = writer_.GrowBuffer(char_len);
    }
    *out = internal::CharTraits<CharType>::Cast(value);
  }

  void VisitCString(const char* value)
  {
    if (spec_.type_ == 'p')
    {
      return WritePointer(value);
    }
    Write(value);
  }

  // Qualification with "internal" here and below is a workaround for nvcc.
  void VisitString(internal::Arg::CStringValue<char> value)
  {
    //TODO
    //Writer_가 wchar_t일 경우에는 wchar_t를 utf8로 변환해서 출력함.
    writer_.WriteString(value, spec_);
  }

  using ArgVisitor<Impl, void>::VisitStringW;

  void VisitStringW(internal::Arg::CStringValue<CharType> value)
  {
    //TODO
    //Writer_가 utf8일 경우에는 wchar_t를 utf8로 변환해서 출력함.
    writer_.WriteString(value, spec_);
  }

  void VisitPointer(const void* value)
  {
    if (spec_.type_ && spec_.type_ != 'p')
    {
      ReportUnknownType(spec_.type_, "pointer");
    }

    WritePointer(value);
  }

 protected:
  BasicWriter<CharType>& Writer()
  {
    return writer_;
  }

  SpecT& Spec()
  {
    return spec_;
  }

  void Write(bool value)
  {
    const char* str_value = value ? "true" : "false";
    Arg::CStringValue<char> str = { str_value, std::strlen(str_value) };
    writer_.WriteString(str, spec_);
  }

  void Write(const char* value)
  {
    Arg::CStringValue<char> str = { value, value ? std::strlen(value) : 0 };
    writer_.WriteString(str, spec_);
  }

 private:
  // workaround MSVC two-phase lookup issue
  typedef internal::Arg Arg;

  BasicWriter<CharType>& writer_;
  SpecT& spec_;

  void WritePointer(const void* ptr)
  {
    spec_.flags_ = HASH_FLAG;
    spec_.type_ = 'x';
    writer_.WriteInt(reinterpret_cast<uintptr_t>(ptr), spec_);
  }
};


class FormatterBase
{
 protected:
  const ArgList& Args() const
  {
    return args_;
  }

  explicit FormatterBase(const ArgList& args)
    : args_(args)
    , next_arg_index_(0)
  {
  }

  // Returns the next argument.
  Arg NextArg(const char*& error)
  {
    if (next_arg_index_ >= 0)
    {
      return DoGetArg(internal::ToUnsigned(next_arg_index_++), error);
    }
    error = "cannot switch from manual to automatic argument indexing";
    return Arg();
  }

  // Checks if manual indexing is used and returns the argument with specified index.
  Arg GetArg(uint32 arg_index, const char*& error)
  {
    return CheckNoAutoIndex(error) ? DoGetArg(arg_index, error) : Arg();
  }

  bool CheckNoAutoIndex(const char*& error)
  {
    if (next_arg_index_ > 0)
    {
      error = "cannot switch from automatic to manual argument indexing";
      return false;
    }
    next_arg_index_ = -1;
    return true;
  }

  template <typename CharType>
  void Write(BasicWriter<CharType>& writer, const CharType* start, const CharType* end)
  {
    if (start != end)
    {
      Writer << BasicStringRef<CharType>(start, internal::ToUnsigned(end - start));
    }
  }

 private:
  ArgList args_;
  int32 next_arg_index_;

  // Returns the argument with specified index.
  FUN_BASE_API Arg DoGetArg(uint32 arg_index, const char*& error);
};

} // namespace internal


template <typename Impl, typename CharType, typename SpecT = FormatSpec>
class BasicArgFormatter : public internal::ArgFormatterBase<Impl, CharType, SpecT>
{
 public:
  BasicArgFormatter(BasicFormatter<CharType, Impl>& formatter, SpecT& spec, const CharType* format_str)
    : internal::ArgFormatterBase<Impl, CharType, SpecT>(formatter.Writer(), spec)
    , formatter_(formatter)
    , format_(format_str)
  {
  }

  /// Formats an argument of a custom (user-defined) type.
  void VisitCustom(internal::Arg::Customvalue custom)
  {
    custom.Format(&formatter_, custom.value, &format_);
  }

 private:
  BasicFormatter<CharType, Impl>& formatter_;
  const CharType* format_;
};

/// The default argument formatter.
template <typename CharType>
class ArgFormatter : public BasicArgFormatter<ArgFormatter<CharType>, CharType, FormatSpec>
{
 public:
  /// Constructs an argument formatter object.
  ArgFormatter(BasicFormatter<CharType>& formatter, FormatSpec& spec, const CharType* fmt)
    : BasicArgFormatter<ArgFormatter<CharType>, CharType, FormatSpec>(formatter, spec, fmt)
  {
  }
};

/// This template formats data and writes the output to a writer.
template <typename CharType, typename ArgFormatter>
class BasicFormatter : private internal::FormatterBase
{
 public:
  /// The character Type for the output.
  typedef CharType CharType;

 public:
  BasicFormatter(const ArgList& args, BasicWriter<CharType>& writer)
    : internal::FormatterBase(args)
    , writer_(writer)
  {
  }

  /// Returns a reference to the writer associated with this formatter.
  BasicWriter<CharType>& Writer()
  {
    return writer_;
  }

  /// Formats stored arguments and writes the output to the Writer.
  void Format(BasicCStringRef<CharType> format_str);

  /// Formats a single argument and advances format_str, a format string pointer.
  const CharType* Format(const CharType*& format_str, const internal::Arg& arg);

 private:
  BasicWriter<CharType>& writer_;
  internal::ArgMap<CharType> map_;

  BasicFormatter(const BasicFormatter&) = delete;
  BasicFormatter& operator = (const BasicFormatter&) = delete;

  using internal::FormatterBase::GetArg;

  // Checks if manual indexing is used and returns the argument with specified name.
  internal::Arg GetArg(BasicStringRef<CharType> arg_name, const char*& error);

  // Parses argument index and returns corresponding argument.
  internal::Arg ParseArgIndex(const CharType*& s);

  // Parses argument name and returns corresponding argument.
  internal::Arg ParseArgName(const CharType*& s);
};


// Generates a comma-separated list with results of applying F to numbers 0..N-1.
#define SF_GEN(N, F)  SF_GEN##N(F)
#define SF_GEN1(F)    F(0)
#define SF_GEN2(F)    SF_GEN1(F),  F(1)
#define SF_GEN3(F)    SF_GEN2(F),  F(2)
#define SF_GEN4(F)    SF_GEN3(F),  F(3)
#define SF_GEN5(F)    SF_GEN4(F),  F(4)
#define SF_GEN6(F)    SF_GEN5(F),  F(5)
#define SF_GEN7(F)    SF_GEN6(F),  F(6)
#define SF_GEN8(F)    SF_GEN7(F),  F(7)
#define SF_GEN9(F)    SF_GEN8(F),  F(8)
#define SF_GEN10(F)   SF_GEN9(F),  F(9)
#define SF_GEN11(F)   SF_GEN10(F), F(10)
#define SF_GEN12(F)   SF_GEN11(F), F(11)
#define SF_GEN13(F)   SF_GEN12(F), F(12)
#define SF_GEN14(F)   SF_GEN13(F), F(13)
#define SF_GEN15(F)   SF_GEN14(F), F(14)


namespace internal {

inline uint64 MakeType()
{
  return 0;
}

template <typename T>
inline uint64 MakeType(const T& arg)
{
  return MakeValue<BasicFormatter<char>>::Type(arg);
}

template <size_t N, bool/*IsPacked*/= (N < ArgList::MAX_PACKED_ARGS)>
struct ArgArray;

template <size_t N>
struct ArgArray<N, true/*IsPacked*/>
{
  typedef Value Type[N > 0 ? N : 1];

  template <typename FormatterT, typename T>
  static Value Make(const T& value)
  {
#ifdef __clang__
    Value result = MakeValue<FormatterT>(value);
    // Workaround a bug in Apple LLVM version 4.2 (clang-425.0.28) of clang:
    // https://github.com/fmtlib/Fmt/issues/276
    (void)result.custom.Format;
    return result;
#else
    return MakeValue<FormatterT>(value);
#endif
  }
};

template <size_t N>
struct ArgArray<N, false/*IsPacked*/>
{
  typedef Arg Type[N + 1]; // +1 for the list end Arg::NONE

  template <typename FormatterT, typename T>
  static Arg Make(const T& value) { return MakeArg<FormatterT>(value); }
};

template <typename Arg, typename... args>
inline uint64 MakeType(const Arg& first, const args& ... tail)
{
  return MakeType(first) | (MakeType(tail...) << 4);
}

} // end of namespace internal



//TODO 아래의 매크로는 제거하는쪽으로... tuple을 사용해야하나??

#define SF_MAKE_TEMPLATE_ARG(N)  typename T##N
#define SF_MAKE_ARG_TYPE(N)      T##N
#define SF_MAKE_ARG(N)           const T##N &v##N
#define SF_ASSIGN_char(N)        arr[N] = ::fun::sf::internal::MakeValue<::fun::sf::BasicFormatter<char   >>(v##N)
#define SF_ASSIGN_wchar_t(N)     arr[N] = ::fun::sf::internal::MakeValue<::fun::sf::BasicFormatter<wchar_t>>(v##N)

// Defines a variadic function returning void.
#define SF_VARIADIC_VOID(Func, ArgType) \
  template <typename... ArgsT> \
  void Func(ArgType Arg0, const ArgsT& ... args) \
  { \
    typedef ::fun::sf::internal::ArgArray<sizeof...(ArgsT)> ArgArray; \
    typename ArgArray::Type Array { ArgArray::template Make<::fun::sf::BasicFormatter<CharType> >(args)... }; \
    Func(Arg0, ::fun::sf::ArgList(::fun::sf::internal::MakeType(args...), Array)); \
  }

// Defines a variadic constructor.
#define SF_VARIADIC_CTOR(Ctor, Func, Arg0Type, Arg1Type) \
  template <typename... ArgsT> \
  Ctor(Arg0Type Arg0, Arg1Type Arg1, const ArgsT& ... args) \
  { \
    typedef ::fun::sf::internal::ArgArray<sizeof...(ArgsT)> ArgArray; \
    typename ArgArray::Type Array { ArgArray::template Make<::fun::sf::BasicFormatter<CharType> >(args)... }; \
    Func(Arg0, Arg1, ::fun::sf::ArgList(::fun::sf::internal::MakeType(args...), Array)); \
  }

// Generates a comma-separated list with results of applying f to pairs(argument, index).
#define SF_FOR_EACH1(F, X0)  F(X0, 0)
#define SF_FOR_EACH2(F, X0, X1)  SF_FOR_EACH1(F, X0), F(X1, 1)
#define SF_FOR_EACH3(F, X0, X1, X2)  SF_FOR_EACH2(F, X0 ,X1), F(X2, 2)
#define SF_FOR_EACH4(F, X0, X1, X2, X3)  SF_FOR_EACH3(F, X0, X1, X2), F(X3, 3)
#define SF_FOR_EACH5(F, X0, X1, X2, X3, X4)  SF_FOR_EACH4(F, X0, X1, X2, X3), F(X4, 4)
#define SF_FOR_EACH6(F, X0, X1, X2, X3, X4, X5)  SF_FOR_EACH5(F, X0, X1, X2, X3, X4), F(X5, 5)
#define SF_FOR_EACH7(F, X0, X1, X2, X3, X4, X5, X6)  SF_FOR_EACH6(F, X0, X1, X2, X3, X4, X5), F(X6, 6)
#define SF_FOR_EACH8(F, X0, X1, X2, X3, X4, X5, X6, X7)  SF_FOR_EACH7(F, X0, X1, X2, X3, X4, X5, X6), F(X7, 7)
#define SF_FOR_EACH9(F, X0, X1, X2, X3, X4, X5, X6, X7, X8)  SF_FOR_EACH8(F, X0, X1, X2, X3, X4, X5, X6, X7), F(X8, 8)
#define SF_FOR_EACH10(F, X0, X1, X2, X3, X4, X5, X6, X7, X8, X9)  SF_FOR_EACH9(F, X0, X1, X2, X3, X4, X5, X6, X7, X8), F(X9, 9)


template <typename CharType>
class BasicWriter
{
 public:
  virtual ~BasicWriter()
  {
  }

  size_t Len() const
  {
    return buffer_.Len();
  }

  const CharType* ConstData() const noexcept
  {
    return &buffer_[0];
  }

  // Makes to nul-term.
  //이제는 nul-term이 아닌 경우에도 처리가 가능해졌으므로, 구지 이것을 통해서 할필요는 없어보임.
  const CharType* c_str() const
  {
    const size_t len = buffer_.Len();
    buffer_.Reserve(len + 1);
    buffer_[len] = '\0';
    return &buffer_[0];
  }

  std::basic_string<CharType> str() const
  {
    return std::basic_string<CharType>(&buffer_[0], buffer_.Len());
  }

  void Write(BasicCStringRef<CharType> format, ArgList args)
  {
    BasicFormatter<CharType>(args, *this).Format(format);
  }
  SF_VARIADIC_VOID(Write, BasicCStringRef<CharType>)

  BasicWriter& operator << (int32 value)
  {
    WriteDecimal(value);
    return *this;
  }

  BasicWriter& operator << (unsigned value)
  {
    return *this << IntFormatSpec<unsigned>(value);
  }

  BasicWriter& operator << (long value)
  {
    WriteDecimal(value);
    return *this;
  }

  BasicWriter& operator << (unsigned long value)
  {
    return *this << IntFormatSpec<unsigned long>(value);
  }

  BasicWriter& operator << (LongLong value)
  {
    WriteDecimal(value);
    return *this;
  }

  BasicWriter& operator << (ULongLong value)
  {
    return *this << IntFormatSpec<ULongLong>(value);
  }

  BasicWriter& operator << (double value)
  {
    WriteDouble(value, FormatSpec());
    return *this;
  }

  BasicWriter& operator << (long double value)
  {
    WriteDouble(value, FormatSpec());
    return *this;
  }

  BasicWriter& operator << (char value)
  {
    buffer_.Add(value);
    return *this;
  }

  BasicWriter& operator << (typename internal::WCharHelper<wchar_t, CharType>::Supported value)
  {
    buffer_.Add(value);
    return *this;
  }

  BasicWriter& operator << (::fun::sf::BasicStringRef<CharType> value)
  {
    const CharType* str = value.ConstData();
    buffer_.Append(str, str + value.Len());
    return *this;
  }

  BasicWriter& operator << (typename internal::WCharHelper<CStringRefA, CharType>::Supported value)
  {
    const char* str = value.ConstData();
    buffer_.Append(str, str + value.Len());
    return *this;
  }

  template <typename T, typename SpecT, typename fill_char>
  BasicWriter& operator << (IntFormatSpec<T, SpecT, fill_char> spec)
  {
    internal::CharTraits<CharType>::Convert(fill_char());
    WriteInt(spec.value(), spec);
    return *this;
  }

  template <typename StrChar>
  BasicWriter& operator << (const StrFormatSpec<StrChar>& spec)
  {
    const StrChar* s = spec.str();
    WriteString(s, std::char_traits<CharType>::length(s), spec);
    return *this;
  }

  void Clear() noexcept
  {
    buffer_.Clear();
  }

  Buffer<CharType>& Buffer() noexcept
  {
    return buffer_;
  }

 protected:
  explicit BasicWriter(Buffer<CharType>& buffer)
    : buffer_(buffer)
  {
  }

 private:
  // Output buffer.
  Buffer<CharType>& buffer_;

  BasicWriter(const BasicWriter&) = delete;
  BasicWriter& operator = (const BasicWriter&);

  //typedef typename internal::CharTraits<CharType>::CharPtr CharPtr;
  typedef CharType* CharPtr;

#if SF_SECURE_SCL
  // Returns pointer value.
  static CharType* Get(CharPtr ptr) { return ptr.base(); }
#else
  static CharType* Get(CharType* ptr) { return ptr; }
#endif

  static CharPtr FillPadding( CharPtr buffer,
                              uint32 total_len,
                              size_t content_len,
                              wchar_t fill);

  CharPtr GrowBuffer(size_t extra)
  {
    const size_t old_len = buffer_.Len();
    buffer_.Resize(old_len + extra);
    return internal::MakePtr(&buffer_[old_len], extra);
  }

  template <typename UIntT>
  CharType* WriteUnsignedDecimal(UIntT value, uint32 prefix_len = 0)
  {
    const uint32 digit_count = internal::CountDigits(value);
    CharType* ptr = Get(GrowBuffer(prefix_len + digit_count));
    internal::FormatDecimal(ptr + prefix_len, value, digit_count);
    return ptr;
  }

  // Write a decimal integer.
  template <typename IntT>
  void WriteDecimal(IntT value)
  {
    typedef typename internal::IntTraits<IntT>::MainType MainType;
    MainType abs_value = static_cast<MainType>(value);
    if (internal::IsNegative(value))
    {
      abs_value = 0 - abs_value;
      *WriteUnsignedDecimal(abs_value, 1) = '-';
    }
    else
    {
      WriteUnsignedDecimal(abs_value, 0);
    }
    /*
    bool is_negative = internal::IsNegative(value);
    if (is_negative)
    {
      abs_value = 0 - abs_value;
    }
    unsigned digit_count = internal::CountDigits(abs_value);
    auto&& it = Reserve((is_negative ? 1 : 0) + digit_count);
    if (is_negative)
    {
      *it++ = '-';
    }
    it = internal::FormatDecimal(it, abs_value, digit_count);
    */
  }

  CharPtr PrepareIntBuffer( uint32 digit_count,
                            const EmptySpec&,
                            const char* prefix,
                            uint32 prefix_len)
  {
    const uint32 len = prefix_len + digit_count;
    CharPtr p = GrowBuffer(len);
    std::uninitialized_copy(prefix, prefix + prefix_len, p);
    return p + len - 1;
  }

  template <typename SpecT>
  CharPtr PrepareIntBuffer( uint32 digit_count,
                            const SpecT& spec,
                            const char* prefix,
                            uint32 prefix_len);

  template <typename T, typename SpecT>
  void WriteInt(T value, SpecT spec);

  template <typename T, typename SpecT>
  void WriteDouble(T value, const SpecT& spec);

  template <typename StrChar>
  CharPtr WriteString(const StrChar* s, size_t len, const AlignSpec& spec);

  template <typename StrChar, typename SpecT>
  void WriteString(const internal::Arg::CStringValue<StrChar>& str, const SpecT& spec);

  void operator << (typename internal::WCharHelper<wchar_t, CharType>::Unsupported);
  void operator << (typename internal::WCharHelper<const wchar_t*, CharType>::Unsupported);

  void AppendFloatLength(CharType*& format_ptr, long double)
  {
    *format_ptr++ = 'L';
  }

  template <typename T>
  void AppendFloatLength(CharType*&, T) {}

  template <typename Impl, typename Char, typename Spec>
  friend class internal::ArgFormatterBase;

  template <typename Impl, typename Char, typename Spec>
  friend class BasicPrintfArgFormatter;
};


template <typename CharType>
template <typename StrChar>
typename BasicWriter<CharType>::CharPtr
  BasicWriter<CharType>::WriteString( const StrChar* s,
                                      size_t len,
                                      const AlignSpec& spec)
{
  CharPtr out = CharPtr();

  if (spec.Width() > len)
  {
    out = GrowBuffer(spec.Width());

    const CharType fill = internal::CharTraits<CharType>::Cast(spec.Fill());

    if (spec.Align() == ALIGN_RIGHT) // right alignment
    {
      std::uninitialized_fill_n(out, spec.Width() - len, fill);
      out += spec.Width() - len;
    }
    else if (spec.Align() == ALIGN_CENTER) // center alignment
    {
      out = FillPadding(out, spec.Width(), len, fill);
    }
    else // left alignment
    {
      std::uninitialized_fill_n(out + len, spec.Width() - len, fill);
    }
  }
  else
  {
    out = GrowBufferlensize);
  }

  // s ~ s + len - 1 -> out
  std::uninitialized_copy(s, s + len, out);

  return out;
}

template <typename CharType>
template <typename StrChar, typename SpecT>
void BasicWriter<CharType>::WriteString(
        const internal::Arg::CStringValue<StrChar>& s, const SpecT& spec)
{
  // Check if StrChar is convertible to CharType.
  internal::CharTraits<CharType>::Convert(StrChar());

  // string일 경우, specifier를 지정하지 않거나, 's' 여야만함.
  if (spec.type_ && spec.type_ != 's')
  {
    internal::ReportUnknownType(spec.type_, "string");
  }

  const StrChar* str_value = s.value;
  size_t str_len = s.len;
  if (str_len == 0) //현재 길이를 체크하고 있는데, 주소를 체크해야하는거 아닌지?
  {
    if (!str_value)
    {
      SF_THROW(FormatError("string pointer is null"));
    }
  }

  const size_t precision = static_cast<size_t>(spec.precision_);
  if (spec.precision_ >= 0 && precision < str_len)
  {
    str_len = precision;
  }

  WriteString(str_value, str_len, spec);
}

template <typename CharType>
typename BasicWriter<CharType>::CharPtr
  BasicWriter<CharType>::FillPadding( CharPtr buffer,
                                      uint32 total_len,
                                      size_t content_len,
                                      wchar_t fill)
{
  const size_t padding = total_len - content_len;
  const size_t left_padding = padding / 2;
  const CharType fill_char = internal::CharTraits<CharType>::Cast(fill);
  std::uninitialized_fill_n(buffer, left_padding, fill_char);
  buffer += left_padding;
  CharPtr content = buffer;
  std::uninitialized_fill_n(buffer + content_len, padding - left_padding, fill_char);
  return content;
}

template <typename CharType>
template <typename SpecT>
typename BasicWriter<CharType>::CharPtr
  BasicWriter<CharType>::PrepareIntBuffer(uint32 digit_count,
                                          const SpecT& spec,
                                          const char* prefix,
                                          uint32 prefix_len)
{
  const uint32 width = spec.Width();
  const Alignment align = spec.Align();
  const CharType fill = internal::CharTraits<CharType>::Cast(spec.Fill());
  if (spec.Precision() > static_cast<int32>(digit_count))
  {
    // Octal prefix '0' is counted as a digit, so ignore it if precision is specified.
    if (prefix_len > 0 && prefix[prefix_len - 1] == '0')
    {
      --prefix_len;
    }

    const uint32 number_len = prefix_len + internal::ToUnsigned(spec.Precision());
    AlignSpec sub_spec(number_len, '0', ALIGN_NUMERIC);
    if (number_len >= width)
    {
      return PrepareIntBuffer(digit_count, sub_spec, prefix, prefix_len);
    }

    buffer_.Reserve(width);

    const uint32 fill_len = width - number_len;
    if (align != ALIGN_LEFT)
    {
      CharPtr p = GrowBuffer(fill_len);
      std::uninitialized_fill(p, p + fill_len, fill);
    }

    CharPtr result = PrepareIntBuffer(digit_count, sub_spec, prefix, prefix_len);
    if (align == ALIGN_LEFT)
    {
      CharPtr p = GrowBuffer(fill_len);
      std::uninitialized_fill(p, p + fill_len, fill);
    }

    return result;
  }

  uint32 len = prefix_len + digit_count;
  if (width <= len)
  {
    CharPtr p = GrowBuffer(len);
    std::uninitialized_copy(prefix, prefix + prefix_len, p);
    return p + len - 1;
  }

  CharPtr p = GrowBuffer(width);
  CharPtr end = p + width;
  if (align == ALIGN_LEFT)
  {
    std::uninitialized_copy(prefix, prefix + prefix_len, p);
    p += len;
    std::uninitialized_fill(p, end, fill);
  }
  else if (align == ALIGN_CENTER)
  {
    p = FillPadding(p, width, len, fill);
    std::uninitialized_copy(prefix, prefix + prefix_len, p);
    p += len;
  }
  else
  {
    if (align == ALIGN_NUMERIC)
    {
      if (prefix_len != 0)
      {
        p = std::uninitialized_copy(prefix, prefix + prefix_len, p);
        len -= prefix_len;
      }
    }
    else
    {
      std::uninitialized_copy(prefix, prefix + prefix_len, end - len);
    }
    std::uninitialized_fill(p, end - len, fill);
    p = end;
  }

  return p - 1;
}

template <typename CharType>
template <typename T, typename SpecT>
void BasicWriter<CharType>::WriteInt(T value, SpecT spec)
{
  uint32 prefix_len = 0;
  typedef typename internal::IntTraits<T>::MainType UnsignedType;
  UnsignedType abs_value = static_cast<UnsignedType>(value);
  char prefix[4] = "";

  if (internal::IsNegative(value))
  {
    prefix[prefix_len++] = '-';
    abs_value = 0 - abs_value;
  }
  else if (spec.Flag(SIGN_FLAG))
  {
    prefix[prefix_len++] = spec.Flag(PLUS_FLAG) ? '+' : ' ';
  }

  switch (spec.Type()) {
  case 0: case 'd': {
    const uint32 digit_count = internal::CountDigits(abs_value);
    CharPtr p = PrepareIntBuffer(digit_count, spec, prefix, prefix_len) + 1;
    internal::FormatDecimal(Get(p), abs_value, 0);
    break;
  }

  case 'x': case 'X': {
    UnsignedType n = abs_value;

    if (spec.Flag(HASH_FLAG))
    {
      // prefix: 0x / 0X
      prefix[prefix_len++] = '0';
      prefix[prefix_len++] = spec.TypePrefix();
    }

    uint32 digit_count = 0;
    do { ++digit_count; } while ((n >>= 4) != 0);
    CharType* p = Get(PrepareIntBuffer(digit_count, spec, prefix, prefix_len));
    n = abs_value;
    const char* digits = spec.Type() == 'x' ? "0123456789abcdef" : "0123456789ABCDEF";
    do { *p-- = digits[n & 0xF]; } while ((n >>= 4) != 0);

    //*NEW*
    /*
    UnsignedType n = abs_value;

    if (spec.Flag(HASH_FLAG)) {
      prefix[prefix_len++] = '0';
      prefix[prefix_len++] = static_char<char>(spec.Type()); // 'x' or 'X'
    }

    unsigned digit_count = CountDigits<4>();
    writer.WriteInt(digit_count, GetPrefix(), spec, HexWriter{*this, digit_count});
    */
    break;
  }

  case 'b': case 'B': {
    UnsignedType n = abs_value;

    if (spec.Flag(HASH_FLAG))
    {
      // prefix: 0b / 0B
      prefix[prefix_len++] = '0';
      prefix[prefix_len++] = spec.TypePrefix();
    }

    uint32 digit_count = 0;
    do { ++digit_count; } while ((n >>= 1) != 0);
    CharType* p = Get(PrepareIntBuffer(digit_count, spec, prefix, prefix_len));
    n = abs_value;
    do { *p-- = static_cast<CharType>('0' + (n & 1)); } while ((n >>= 1) != 0);

    //*NEW*
    /*
    UnsignedType n = abs_value;

    if (spec.Flag(HASH_FLAG)) {
      prefix[prefix_len++] = '0';
      prefix[prefix_len++] = static_char<char>(spec.Type()); // 'b' or 'B'
    }

    unsigned digit_count = CountDigits<1>();
    writer.WriteInt(digit_count, GetPrefix(), spec, HexWriter{*this, digit_count});
    */
    break;
  }

  case 'o': {
    UnsignedType n = abs_value;

    if (spec.Flag(HASH_FLAG))
    {
      // prefix: 0
      prefix[prefix_len++] = '0';
    }

    uint32 digit_count = 0;
    do { ++digit_count; } while ((n >>= 3) != 0);
    CharType* p = Get(PrepareIntBuffer(digit_count, spec, prefix, prefix_len));
    n = abs_value;
    do { *p-- = static_cast<CharType>('0' + (n & 7)); } while ((n >>= 3) != 0);
    break;
  }

  case 'n': {
    const uint32 digit_count = internal::CountDigits(abs_value);
    ::fun::sf::CStringRefA sep = "";
#if !(defined(ANDROID) || defined(__ANDROID__))
    sep = internal::AddThousandsSep(std::localeconv());
#endif
    const uint32 len = static_cast<uint32>(digit_count + sep.Len() * ((digit_count - 1) / 3));
    CharPtr p = PrepareIntBuffer(len, spec, prefix, prefix_len) + 1;
    internal::FormatDecimal(Get(p), abs_value, 0, internal::AddThousandsSep(sep));
    break;
  }

  default:
    internal::ReportUnknownType(spec.Type(), spec.Flag(CHAR_FLAG) ? "char" : "integer");
    break;
  }
}

template <typename CharType>
template <typename T, typename SpecT>
void BasicWriter<CharType>::WriteDouble(T value, const SpecT& spec)
{
  // check type.
  char type = spec.Type();
  bool is_upper = false;
  switch (type) {
  case 0:
    Type = 'g';
    break;

  case 'e': case 'f': case 'g': case 'a':
    break;

  case 'F':
#ifdef _MSC_VER
    // MSVC's printf doesn't support 'F'.
    Type = 'f';
#endif
    // fall through.

  case 'E': case 'G': case 'A':
    is_upper = true;
    break;

  default:
    internal::ReportUnknownType(Type, "double");
    break;
  }

  char sign = 0;

  //NaN일 경우, 음수일 경우에 항상 false를 돌리므로, 별도의 함수로 체크함.
  //그런데, 이런 경우에 실질적으로 문제가 되는지?

  // Use isnegative instead of value < 0 because the latter is always
  // false for NaN.
  //TODO
  //if (internal::FPUtil::isnegative(static_cast<double>(value)))
  if (value < 0)
  {
    sign = '-';
    value = -value;
  }
  else if (spec.Flag(SIGN_FLAG))
  {
    sign = spec.Flag(PLUS_FLAG) ? '+' : ' ';
  }

  //TODO
  //if (internal::FPUtil::isnotanumber(value))
  if (std::isnan(value))
  {
    // Format NaN ourselves because sprintf's output is not consistent across platforms.
    size_t nan_len = 4;
    const char* nan = is_upper ? " NAN" : " nan";
    if (!sign)
    {
      --nan_len;
      ++nan;
    }

    CharPtr out = WriteString(nan, nan_len, spec);

    if (sign)
    {
      *out = sign;
    }

    return;
  }

  //TODO
  //if (internal::FPUtil::isinfinity(value))
  if (!std::isfinite(value))
  {
    // Format infinity ourselves because sprintf's output is not consistent across platforms.
    size_t inf_len = 4;
    const char* inf = is_upper ? " INF" : " inf";
    if (!sign)
    {
      --inf_len;
      ++inf;
    }

    CharPtr out = WriteString(inf, inf_len, spec);

    if (sign)
    {
      *out = sign;
    }

    return;
  }

  size_t offset = buffer_.Len();
  uint32 width = spec.Width();
  if (sign)
  {
    buffer_.Reserve(buffer_.Len() + (width > 1u ? width : 1u));
    if (width > 0)
    {
      --width;
    }
    ++offset;
  }

  // Build format string.
  enum { MAX_FORMAT_LEN = 10}; // longest format: %#-*.*Lg
  CharType format[MAX_FORMAT_LEN];
  CharType* format_ptr = format;
  *format_ptr++ = '%';
  uint32 width_for_sprintf = width;
  if (spec.Flag(HASH_FLAG))
  {
    *format_ptr++ = '#';
  }

  if (spec.Align() == ALIGN_CENTER)
  {
    width_for_sprintf = 0;
  }
  else
  {
    if (spec.Align() == ALIGN_LEFT)
    {
      *format_ptr++ = '-';
    }

    if (width != 0)
    {
      *format_ptr++ = '*';
    }
  }

  if (spec.Precision() >= 0)
  {
    *format_ptr++ = '.';
    *format_ptr++ = '*';
  }

  AppendFloatLength(format_ptr, value);
  *format_ptr++ = Type;
  *format_ptr = '\0';

  // Format using snprintf.
  const CharType fill = internal::CharTraits<CharType>::Cast(spec.Fill());
  uint32 n = 0;
  CharType* start = nullptr;
  for (;;)
  {
    size_t buffer_len = buffer_.Capacity() - offset;
#ifdef _MSC_VER
    // MSVC's vsnprintf_s doesn't work with zero size, so reserve
    // space for at least one extra character to Make the size non-zero.
    // Note that the buffer's capacity will increase by more than 1.
    if (buffer_len == 0)
    {
      buffer_.Reserve(offset + 1);
      buffer_len = buffer_.Capacity() - offset;
    }
#endif
    start = &buffer_[offset];
    const int32 result =
            internal::CharTraits<CharType>::FormatFloat(start,
                                                        buffer_len,
                                                        format,
                                                        width_for_sprintf,
                                                        spec.Precision(),
                                                        value);
    if (result >= 0)
    {
      n = internal::ToUnsigned(result);
      if (offset + n < buffer_.Capacity())
      {
        break;  // The buffer is large enough - continue with formatting.
      }
      buffer_.Reserve(offset + n + 1);
    }
    else
    {
      // If result is negative we ask to increase the capacity by at least 1,
      // but as std::vector, the buffer grows exponentially.
      buffer_.Reserve(buffer_.Capacity() + 1);
    }
  }

  if (sign)
  {
    if ((spec.Align() != ALIGN_RIGHT && spec.Align() != ALIGN_DEFAULT) || *start != ' ')
    {
      *(start - 1) = sign;
      sign = 0;
    }
    else
    {
      *(start - 1) = fill;
    }
    ++n;
  }

  if (spec.Align() == ALIGN_CENTER && spec.Width() > n)
  {
    width = spec.Width();
    CharPtr p = GrowBuffer(width);
    UnsafeMemory::Memmove(Get(p) + (width - n) / 2, Get(p), n * sizeof(CharType));
    FillPadding(p, spec.Width(), n, fill);
    return;
  }

  if (spec.Fill() != ' ' || sign)
  {
    while (*start == ' ')
    {
      *start++ = fill;
    }

    if (sign)
    {
      *(start - 1) = sign;
    }
  }

  GrowBuffer(n);
}


template <typename CharType, typename Allocator = std::allocator<CharType> >
class BasicMemoryWriter : public BasicWriter<CharType>
{
 public:
  explicit BasicMemoryWriter(const Allocator& alloc = Allocator())
    : BasicWriter<CharType>(buffer_)
    , buffer_(alloc)
  {
  }

  BasicMemoryWriter(BasicMemoryWriter&& other)
    : BasicWriter<CharType>(buffer_)
    , buffer_(MoveTemp(other.buffer_))
  {
  }

  BasicMemoryWriter &operator = (BasicMemoryWriter&& other)
  {
    buffer_ = MoveTemp(other.buffer_);
    return *this;
  }

 private:
  internal::MemoryBuffer<CharType, internal::INLINE_BUFFER_SIZE, Allocator> buffer_;
};

typedef BasicMemoryWriter<char   > MemoryWriterA;
typedef BasicMemoryWriter<wchar_t> MemoryWriterW;


template <typename CharType>
class BasicArrayWriter : public BasicWriter<CharType>
{
 public:
  BasicArrayWriter(CharType* array, size_t len)
    : BasicWriter<CharType>(buffer_)
    , buffer_(array, len)
  {
  }

  template <size_t N>
  explicit BasicArrayWriter(CharType (&array)[N])
    : BasicWriter<CharType>(buffer_)
    , buffer_(array, N)
  {
  }

 private:
  internal::FixedBuffer<CharType> buffer_;
};

typedef BasicArrayWriter<char   > ArrayWriterA;
typedef BasicArrayWriter<wchar_t> ArrayWriterW;


//TODO?
// Reports a system error without throwing an exception.
// Can be used to report errors from destructors.
//FUN_BASE_API void ReportSystemError(int ErrorCode, CStringRefA Message) noexcept;

enum Color { BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE };

FUN_BASE_API void PrintColored(Color c, CStringRefA Format, ArgList args);

inline std::string Format(CStringRefA format_str, ArgList args)
{
  MemoryWriterA writer;
  writer.Write(format_str, args);
  return writer.str();
}

inline std::wstring Format(CStringRefW format_str, ArgList args)
{
  MemoryWriterW writer;
  writer.Write(format_str, args);
  return writer.str();
}

FUN_BASE_API void Print(FILE* fp, CStringRefA format_str, ArgList args);
FUN_BASE_API void Print(CStringRefA format_str, ArgList args);


//TODO 이놈은 별도로 빼주는게 좋을듯...
//TODO 외부 버퍼를 지정할 수 있도록 하는것도 좋을듯...

/// Fast integer formatter.
class FormatInt
{
 public:
  explicit FormatInt(int value)
  {
    FormatSigned(value);
  }

  explicit FormatInt(long value)
  {
    FormatSigned(value);
  }

  explicit FormatInt(LongLong value)
  {
    FormatSigned(value);
  }

  explicit FormatInt(unsigned value)
    : str_(FormatDecimal(value))
  {
  }

  explicit FormatInt(unsigned long value)
    : str_(FormatDecimal(value))
  {
  }

  explicit FormatInt(ULongLong value)
    : str_(FormatDecimal(value))
  {
  }

  size_t Len() const
  {
    return internal::ToUnsigned(buffer_ - str_ + BUFFER_LEN - 1);
  }

  const char* ConstData() const
  {
    return str_;
  }

  const char* c_str() const
  {
    buffer_[BUFFER_LEN - 1] = '\0';
    return str_;
  }

  //ToStdString
  std::string Str() const
  {
    return std::string(str_, Len());
  }

 private:
  // Buffer should be large enough to hold all digits (digits10 + 1),
  // a Sign and a null character.
  enum { BUFFER_LEN = std::numeric_limits<ULongLong>::digits10 + 3 };
  mutable char buffer_[BUFFER_LEN];
  char* str_;

  // Formats value in reverse and returns the number of digits.
  char* FormatDecimal(ULongLong value)
  {
    char* buffer_end = buffer_ + BUFFER_LEN - 1;
    while (value >= 100)
    {
      // Integer division is slow so do it for a group of two digits instead
      // of for every digit. The idea comes from the talk by Alexandrescu
      // "Three Optimization Tips for c++". See speed-test for a comparison.
      const uint32 index = static_cast<uint32>((value % 100) * 2);
      value /= 100;
      *--buffer_end = internal::Data::DIGITS[index + 1];
      *--buffer_end = internal::Data::DIGITS[index];
    }

    if (value < 10)
    {
      *--buffer_end = static_cast<char>('0' + value);
      return buffer_end;
    }

    const uint32 index = static_cast<uint32>(value * 2);
    *--buffer_end = internal::Data::DIGITS[index + 1];
    *--buffer_end = internal::Data::DIGITS[index];
    return buffer_end;
  }

  void FormatSigned(LongLong value)
  {
    ULongLong abs_value = static_cast<ULongLong>(value);
    const bool is_negative = value < 0;

    if (is_negative)
    {
      abs_value = 0 - abs_value;
    }

    str_ = FormatDecimal(abs_value);

    if (is_negative)
    {
      *--str_ = '-';
    }
  }
};


// Formats a decimal integer value writing into buffer and returns
// a pointer to the end of the formatted string. This function doesn't
// Write a terminating null character.
template <typename T>
inline void FormatDecimal(char*& buffer, T value)
{
  typedef typename internal::IntTraits<T>::MainType MainType;
  MainType abs_value = static_cast<MainType>(value);

  if (internal::IsNegative(value))
  {
    *buffer++ = '-';
    abs_value = 0 - abs_value;
  }

  if (abs_value < 100)
  {
    if (abs_value < 10)
    {
      *buffer++ = static_cast<char>('0' + abs_value);
      return;
    }

    const uint32 index = static_cast<uint32>(abs_value * 2);
    *buffer++ = internal::Data::DIGITS[index];
    *buffer++ = internal::Data::DIGITS[index + 1];
    return;
  }

  const uint32 digit_count = internal::CountDigits(abs_value);
  internal::FormatDecimal(buffer, abs_value, digit_count);
  buffer += digit_count;
}



//
// Arg
//

template <typename T>
inline internal::NamedArgWithType<char, T> Arg(CStringRefA name, const T& arg)
{
  return internal::NamedArgWithType<char, T>(name, arg);
}

template <typename T>
inline internal::NamedArgWithType<wchar_t, T> Arg(CStringRefW name, const T& arg)
{
  return internal::NamedArgWithType<wchar_t, T>(name, arg);
}

// The following two functions are deleted intentionally to disable
// nested named arguments as in ``Format("{}", arg("a", arg("b", 42)))``.
template <typename CharType>
void Arg(CStringRefA, const internal::NamedArg<CharType>&) = delete;

template <typename CharType>
void Arg(CStringRefW, const internal::NamedArg<CharType>&) = delete;

} // end of namespace sf



//TODO 아래의 매크로들은 제거하는 쪽으로 연구를 해봐야함.
//TODO 아래의 매크로들은 제거하는 쪽으로 연구를 해봐야함.
//TODO 아래의 매크로들은 제거하는 쪽으로 연구를 해봐야함.
//TODO 아래의 매크로들은 제거하는 쪽으로 연구를 해봐야함.
//TODO 아래의 매크로들은 제거하는 쪽으로 연구를 해봐야함.
//TODO 아래의 매크로들은 제거하는 쪽으로 연구를 해봐야함.

#define SF_EXPAND(args)  args

// Returns the number of arguments.
// Based on https://groups.google.com/forum/#!topic/comp.std.c/d-6Mj5Lko_s.
#define SF_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N, ...)  N
#define SF_NARG_(...)  SF_EXPAND(SF_ARG_N(__VA_ARGS__))
#define SF_RSEQ_N()    10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
#define SF_NARG(...)   SF_NARG_(__VA_ARGS__, SF_RSEQ_N())

#define SF_FOR_EACH_(N, F, ...)  SF_EXPAND(SF_CONCAT(SF_FOR_EACH, N)(F, __VA_ARGS__))
#define SF_FOR_EACH(F, ...)      SF_EXPAND(SF_FOR_EACH_(SF_NARG(__VA_ARGS__), F, __VA_ARGS__))

#define SF_ADD_ARG_NAME(Type, index)  Type arg##index
#define SF_GET_ARG_NAME(Type, index)  arg##index

#define SF_VARIADIC_(CharType, ReturnType, Func, Call, ...) \
  template <typename... ArgsT> \
  ReturnType Func(SF_FOR_EACH(SF_ADD_ARG_NAME, __VA_ARGS__), const ArgsT&  ... args) \
  { \
    typedef ::fun::sf::internal::ArgArray<sizeof...(ArgsT)> ArgArray; \
    typename ArgArray::Type Array { ArgArray::template Make<::fun::sf::BasicFormatter<CharType> >(args)... }; \
    Call(SF_FOR_EACH(SF_GET_ARG_NAME, __VA_ARGS__), ::fun::sf::ArgList(::fun::sf::internal::MakeType(args...), Array)); \
  }

#define SF_VARIADIC(ReturnType, Func, ...)  SF_VARIADIC_(char, ReturnType, Func, return Func, __VA_ARGS__)
#define SF_VARIADIC_W(ReturnType, Func, ...)  SF_VARIADIC_(wchar_t, ReturnType, Func, return Func, __VA_ARGS__)

#define SF_CAPTURE_ARG_(Id, index)  ::fun::sf::arg(#Id, Id)
#define SF_CAPTURE_ARG_W_(Id, index)  ::fun::sf::arg(L###Id, Id)

#define SF_CAPTURE(...)  SF_FOR_EACH(SF_CAPTURE_ARG_, __VA_ARGS__)
#define SF_CAPTURE_W(...)  SF_FOR_EACH(SF_CAPTURE_ARG_W_, __VA_ARGS__)


namespace sf {

SF_VARIADIC(std::string, Format, CStringRefA);
SF_VARIADIC_W(std::wstring, Format, CStringRefW);
SF_VARIADIC(void, Print, CStringRefA);
SF_VARIADIC(void, Print, FILE*, CStringRefA);
SF_VARIADIC(void, PrintColored, Color, CStringRefA);

namespace internal {

  template <typename CharType>
  inline bool IsNameStart(CharType c)
  {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
  }

  template <typename CharType>
  inline uint32 ParseNonNegativeInt(const CharType*& s)
  {
    fun_check('0' <= *s && *s <= '9');
    uint32 value = 0;
    do {
      const uint32 new_value = value * 10 + (*s++ - '0');
      // check if value wrapped around.
      if (new_value < value)
      {
        value = (std::numeric_limits<uint32>::max)();
        break;
      }
      value = new_value;
    } while ('0' <= *s && *s <= '9');

    // convert to unsigned to prevent a warning.
    const uint32 max = std::numeric_limits<int32>::max();
    if (value > max)
    {
      SF_THROW(FormatError("number is too big"));
    }
    return value;
  }

  inline void RequireNumericArgument(const Arg& arg, char spec)
  {
    if (arg.type > Arg::Type::LAST_NUMERIC_TYPE)
    {
      //TODO?
      //std::string Message = ::fun::sf::Format("format specifier '{}' requires numeric argument", spec);
      //SF_THROW(::fun::sf::FormatError(Message));
    }
  }

  template <typename CharType>
  inline void CheckSign(const CharType*& s, const Arg& arg)
  {
    const char sign = static_cast<char>(*s);
    if (arg.type == Arg::UINT || arg.type == Arg::ULONG_LONG)
    {
      SF_THROW(FormatError(::fun::sf::Format("format specifier '{}' requires signed argument", sign)));
    }
    ++s; // consume sign flag.
  }

} // end of namespace internal


template <typename CharType, typename ArgFormatter>
inline internal::Arg BasicFormatter<CharType,ArgFormatter>::GetArg(
                  BasicStringRef<CharType> arg_name, const char*& error)
{
  if (CheckNoAutoIndex(error))
  {
    map_.Init(args());
    if (const internal::Arg* arg = map_.Find(arg_name))
    {
      return *arg;
    }
    error = "argument not found";
  }
  return internal::Arg();
}

template <typename CharType, typename ArgFormatter>
inline internal::Arg BasicFormatter<CharType,ArgFormatter>::ParseArgIndex(const CharType*& s)
{
  const char* error = nullptr;
  internal::Arg arg = (*s < '0' || *s > '9') ? NextArg(error) : GetArg(internal::ParseNonNegativeInt(s), error);
  if (error)
  {
    SF_THROW(FormatError(*s != '}' && *s != ':' ? "invalid format string" : error));
  }
  return arg;
}

template <typename CharType, typename ArgFormatter>
inline internal::Arg BasicFormatter<CharType,ArgFormatter>::ParseArgName(const CharType*& s)
{
  fun_check(internal::IsNameStart(*s));
  const CharType* start = s;
  CharType c;
  do { c = *++s; } while (internal::IsNameStart(c) || (c >= '0' && c <= '9'));

  const char* error = nullptr;
  internal::Arg arg = GetArg(BasicStringRef<CharType>(start, s - start), error);
  if (error)
  {
    SF_THROW(FormatError(error));
  }
  return arg;
}

template <typename CharType, typename ArgFormatter>
const CharType* BasicFormatter<CharType,ArgFormatter>::Format(const CharType*& format_str, const internal::Arg& arg)
{
  using internal::Arg;

  const CharType* s = format_str;
  typename ArgFormatter::SpecType spec;

  if (*s == ':')
  {
    if (arg.type == Arg::CUSTOM)
    {
      arg.custom.Format(this, arg.custom.value, &s);
      return s;
    }
    ++s; // skip ':'

    // parse fill and alignment.
    CharType c = *s;
    if (Char != 0)
    {
      const CharType* p = s + 1;
      spec.align_ = ALIGN_DEFAULT;
      do {
        switch (*p) {
        case '<': spec.align_ = ALIGN_LEFT; break;
        case '>': spec.align_ = ALIGN_RIGHT; break;
        case '=': spec.align_ = ALIGN_NUMERIC; break;
        case '^': spec.align_ = ALIGN_CENTER; break;
        }

        if (spec.align_ != ALIGN_DEFAULT)
        {
          if (p != s)
          {
            if (c == '}') break;
            if (c == '{')
            {
              SF_THROW(FormatError("invalid fill character '{'"));
            }
            s += 2;
            spec.fill_ = c;
          }
          else
          {
            ++s;
          }

          if (spec.align_ == ALIGN_NUMERIC)
          {
            RequireNumericArgument(arg, '=');
          }
          break;
        }
      } while (--p >= s);
    }

    // parse sign.
    switch (*s) {
    case '+': CheckSign(s, arg); spec.flags_ |= SIGN_FLAG | PLUS_FLAG; break;
    case '-': CheckSign(s, arg); spec.flags_ |= MINUS_FLAG; break;
    case ' ': CheckSign(s, arg); spec.flags_ |= SIGN_FLAG; break;
    }

    if (*s == '#')
    {
      RequireNumericArgument(arg, '#');
      spec.flags_ |= HASH_FLAG;
      ++s;
    }

    // parse zero flag.
    if (*s == '0')
    {
      RequireNumericArgument(arg, '0');
      spec.align_ = ALIGN_NUMERIC;
      spec.fill_ = '0';
      ++s;
    }

    // parse width.
    if (*s >= '0' && *s <= '9')
    {
      spec.width_ = internal::ParseNonNegativeInt(s);
    }
    else if (*s == '{')
    {
      ++s; // skip '{'

      const Arg width_arg = internal::IsNameStart(*s) ? ParseArgName(s) : ParseArgIndex(s);
      if (*s++ != '}')
      {
        SF_THROW(FormatError("invalid format string"));
      }

      uint64 width = 0;
      switch (width_arg.type) {
      case Arg::INT:
        if (width_arg.int32_value < 0)
        {
          SF_THROW(FormatError("negative width"));
        }
        width = width_arg.int32_value;
        break;

      case Arg::UINT:
        width = width_arg.uint32_value;
        break;

      case Arg::LONG_LONG:
        if (width_arg.int64_value < 0)
        {
          SF_THROW(FormatError("negative width"));
        }
        width = width_arg.int64_value;
        break;

      case Arg::ULONG_LONG:
        width = width_arg.uint64_value;
        break;

      default:
        SF_THROW(FormatError("width is not integer"));
        break;
      }

      if (width > NumericLimits<int32>::Max())
      {
        SF_THROW(FormatError("number is too big"));
      }
      spec.width_ = width;
    }

    // parse precision.
    if (*s == '.')
    {
      ++s; // skip '.'

      spec.precision_ = 0;

      if (*s >= '0' && *s <= '9')
      {
        spec.precision_ = internal::ParseNonNegativeInt(s);
      }
      else if (*s == '{')
      {
        ++s; // skip '{'

        // complicated.
        const Arg precision_arg = internal::IsNameStart(*s) ? ParseArgName(s) : ParseArgIndex(s);
        if (*s++ != '}')
        {
          SF_THROW(FormatError("invalid format string"));
        }

        uint64 precision = 0;
        switch (precision_arg.type) {
        case Arg::INT:
          if (precision_arg.int32_value < 0)
          {
            SF_THROW(FormatError("negative precision"));
          }
          precision = precision_arg.int32_value;
          break;

        case Arg::UINT:
          precision = precision_arg.uint32_value;
          break;

        case Arg::LONG_LONG:
          if (precision_arg.int64_value < 0)
          {
            SF_THROW(FormatError("negative precision"));
          }
          precision = precision_arg.int64_value;
          break;

        case Arg::ULONG_LONG:
          precision = precision_arg.uint64_value;
          break;

        default:
          SF_THROW(FormatError("precision is not integer"));
          break;
        }

        if (precision > NumericLimits<int32>::Max())
        {
          SF_THROW(FormatError("number is too big"));
        }
        spec.precision_ = Precision;
      }
      else
      {
        SF_THROW(FormatError("missing precision specifier"));
      }

      if (arg.type <= Arg::Type::LAST_INTEGER_TYPE || arg.type == Arg::POINTER)
      {
        SF_THROW(FormatError(::fun::sf::Format("precision not allowed in {} format specifier", arg.type == Arg::POINTER ? "pointer" : "integer")));
      }
    }

    // parse type.
    if (*s != '}' && *s)
    {
      spec.type_ = static_cast<char>(*s++);
    }
  }

  if (*s++ != '}')
  {
    SF_THROW(FormatError("missing '}' in format string"));
  }

  // format argument.
  ArgFormatter(*this, spec, s - 1).Visit(arg);
  return s;
}

template <typename CharType, typename ArgumentFormatter>
void BasicFormatter<CharType, ArgumentFormatter>::Format(BasicCStringRef<CharType> format_str)
{
  const CharType* s = format_str.c_str();
  const CharType* start = s;
  while (*s)
  {
    const CharType c = *s++;
    if (c != '{' && c != '}')
    {
      continue;
    }

    if (*s == c)
    {
      Write(writer_, start, s);
      start = ++s;
      continue;
    }

    if (c == '}')
    {
      SF_THROW(FormatError("unmatched '}' in format string"));
    }

    Write(writer_, start, s - 1);

    auto arg = internal::IsNameStart(*s) ? ParseArgName(s) : ParseArgIndex(s);
    start = s = Format(s, arg);
  }

  // Rest?
  Write(writer_, start, s);
}


template <typename CharType, typename It>
struct ArgJoin
{
  It first;
  It last;
  BasicCStringRef<CharType> sep;

  ArgJoin(It first, It last, const BasicCStringRef<CharType>& sep)
    : first(first)
    , last(last)
    , sep(sep)
  {
  }
};

template <typename It>
ArgJoin<char, It> Join(It first, It last, const BasicCStringRef<char>& sep)
{
  return ArgJoin<char, It>(first, last, sep);
}

template <typename It>
ArgJoin<wchar_t, It> Join(It first, It last, const BasicCStringRef<wchar_t>& sep)
{
  return ArgJoin<wchar_t, It>(first, last, sep);
}

template <typename ArgFormatter, typename CharType, typename IterType>
void FormatArg(::fun::sf::BasicFormatter<CharType, ArgFormatter>& formatter,
                const CharType*& format_str,
                const ArgJoin<CharType, IterType>& e)
{
  const CharType* end = format_str;
  if (*end == ':')
  {
    ++end;
  }

  while (*end && *end != '}')
  {
    ++end;
  }

  if (*end != '}')
  {
    SF_THROW(FormatError("missing '}' in format string"));
  }

  IterType iter = e.first;
  if (Iter != e.last)
  {
    const CharType* save = format_str;
    formatter.Format(format_str, internal::MakeArg<BasicFormatter<CharType, ArgFormatter>>(*iter++));
    while (iter != e.last)
    {
      formatter.Writer().Write(e.sep);
      format_str = save;
      formatter.Format(format_str, internal::MakeArg<BasicFormatter<CharType, ArgFormatter>>(*iter++));
    }
  }
  format_str = end + 1;
}


//
// Printf
//

namespace internal {

template <bool IsSigned>
struct IntChecker
{
  template <typename T>
  static bool FitsInInt(T val)
  {
    return val <= (T)std::numeric_limits<int32>::max();
  }

  static bool FitsInInt(bool)
  {
    return true;
  }
};

template <>
struct IntChecker<true>
{
  template <typename T>
  static bool FitsInInt(T val)
  {
    return val >= (T)std::numeric_limits<int32>::min() && val <= (T)std::numeric_limits<int32>::max();
  }

  static bool FitsInInt(int32)
  {
    return true;
  }
};

class PrecisionHandler : public ArgVisitor<PrecisionHandler, int32>
{
 public:
  void ReportUnhandledArg()
  {
    SF_THROW(FormatError("precision is not integer"));
  }

  template <typename T>
  int32 VisitAnyInt(T val)
  {
    if (!IntChecker<std::numeric_limits<T>::is_signed>::FitsInInt(val))
    {
      SF_THROW(FormatError("number is too big"));
    }
    return static_cast<int32>(val);
  }
};

class IsZeroInt : public ArgVisitor<IsZeroInt, bool>
{
 public:

  template <typename T>
  bool VisitAnyInt(T val) { return val == 0; }
};

class DefaultType : public ArgVisitor<DefaultType, char>
{
 public:
  char VisitChar(int) { return 'c'; }
  char VisitBool(bool) { return 's'; }
  char VisitPointer(const void*) { return 'p'; }
  template <typename T> char VisitAnyInt(T) { return 'd'; }
  template <typename T> char VisitAnyDouble(T) { return 'g'; }
  char VisitUnhandledArg() { return 's'; }
};


template <typename T = void>
class ArgConverter : public ArgVisitor<ArgConverter<T>, void>
{
 public:
  internal::Arg& arg;
  wchar_t type;

  ArgConverter(const ArgConverter&) = delete;
  ArgConverter& operator = (const ArgConverter&) = delete;

 public:
  ArgConverter(internal::Arg& arg, wchar_t type)
    : arg(arg)
    , type(type)
  {
  }

  void VisitBool(bool val)
  {
    if (type != 's')
    {
      VisitAnyInt(val);
    }
  }

  void VisitChar(char val)
  {
    if (type != 's')
    {
      VisitAnyInt(val);
    }
  }

  template <typename U>
  void VisitAnyInt(U val)
  {
    bool is_signed = type == 'd' || type == 'i';
    if (type == 's')
    {
      is_signed = std::numeric_limits<U>::is_signed;
    }

    using internal::Arg;
    typedef typename TChooseClass<IsSame<T,void>::Value, U, T>::result TargetType;
    if (sizeof(TargetType) <= sizeof(int32))
    {
      if (is_signed)
      {
        arg.type = Arg::INT;
        arg.int32_value = static_cast<int32>(static_cast<TargetType>(val));
      }
      else
      {
        arg.type = Arg::UINT;
        typedef typename internal::MakeUnsigned<TargetType>::Type Unsigned;
        arg.uint32_value = static_cast<uint32>(static_cast<Unsigned>(val));
      }
    }
    else
    {
      if (is_signed)
      {
        arg.type = Arg::LONG_LONG;
        arg.int64_value = static_cast<LongLong>(val);
      }
      else
      {
        arg.type = Arg::ULONG_LONG;
        arg.uint64_value = static_cast<typename internal::MakeUnsigned<U>::Type>(val);
      }
    }
  }
};

class CharConverter : public ArgVisitor<CharConverter,void>
{
 private:
  internal::Arg& arg;

  CharConverter(const CharConverter&) = delete;
  CharConverter& operator = (const CharConverter&) = delete;

 public:
  explicit CharConverter(internal::Arg& arg)
    : arg(arg)
  {
  }

  template <typename T>
  void VisitAnyInt(T val)
  {
    arg.type = internal::Arg::CHAR;
    arg.int32_value = static_cast<char>(val);
  }
};

class WidthHandler : public ArgVisitor<WidthHandler,uint32>
{
 private:
  FormatSpec& spec;

  WidthHandler(const WidthHandler&) = delete;
  WidthHandler& operator = (const WidthHandler&) = delete;

 public:
  explicit WidthHandler(FormatSpec& spec)
    : spec(spec)
  {
  }

  void ReportUnhandledArg()
  {
    SF_THROW(FormatError("width is not integer"));
  }

  template <typename T>
  uint32 VisitAnyInt(T val)
  {
    typedef typename internal::IntTraits<T>::MainType UnsignedType;
    UnsignedType width = static_cast<UnsignedType>(val);
    if (internal::IsNegative(val))
    {
      spec.align_ = ALIGN_LEFT;
      width = 0 - width;
    }

    const uint32 max = std::numeric_limits<int32>::max();
    if (width > max)
    {
      SF_THROW(FormatError("number is too big"));
    }
    return static_cast<uint32>(width);
  }
};

} // namespace internal


template <typename Impl, typename CharT, typename SpecT>
class BasicPrintfArgFormatter : public internal::ArgFormatterBase<Impl, CharT, SpecT>
{
 public:
  typedef internal::ArgFormatterBase<Impl, CharT, SpecT> BaseType;

  BasicPrintfArgFormatter(BasicWriter<CharT>& writer, SpecT& spec)
    : BaseType(writer, spec)
  {
  }

  void VisitBool(bool val)
  {
    SpecT& s = this->Spec();
    if (s.type_ != 's')
    {
      return this->VisitAnyInt(val);
    }
    s.type_ = 0;
    this->Write(val);
  }

  void VisitChar(int32 val)
  {
    const SpecT& s = this->Spec();
    BasicWriter<CharT>& writer = this->Writer();

    if (s.type_ && s.type_ != 'c')
    {
      writer.WriteInt(val, s);
    }

    typedef typename BasicWriter<CharT>::CharPtr CharPtr;
    CharPtr out = CharPtr();
    if (s.width_ > 1)
    {
      const CharT fill = ' ';
      out = writer.GrowBuffer(s.width_);
      if (s.align_ != ALIGN_LEFT)
      {
        std::fill_n(out, s.width_ - 1, fill);
        out += s.width_ - 1;
      }
      else
      {
        std::fill_n(out + 1, s.width_ - 1, fill);
      }
    }
    else
    {
      out = writer.GrowBuffer(1);
    }
    *out = static_cast<CharT>(val);
  }

  void VisitCString(const char* val)
  {
    if (val)
    {
      BaseType::VisitCString(val);
    }
    else if (this->Spec().type_ == 'p')
    {
      WriteNullPointer();
    }
    else
    {
      this->Write("(null)");
    }
  }

  void VisitPointer(const void* val)
  {
    if (val)
    {
      BaseType::VisitPointer(val);
    }
    else
    {
      this->Spec().type_ = 0;
      WriteNullPointer();
    }
  }

  void VisitCustom(internal::Arg::Customvalue val)
  {
    BasicFormatter<CharT> formatter(ArgList(), this->Writer());
    const CharT format_str[] = { '}', 0 };
    const CharT* format = format_str;
    val.Format(&formatter, val.value, &format);
  }

 private:
  void WriteNullPointer()
  {
    this->Spec().type_ = 0;
    this->Write("(nil)");
  }
};


template <typename CharT>
class PrintfArgFormatter : public BasicPrintfArgFormatter<PrintfArgFormatter<CharT>, CharT, FormatSpec>
{
 public:
  PrintfArgFormatter(BasicWriter<CharT>& writer, FormatSpec& spec)
    : BasicPrintfArgFormatter<PrintfArgFormatter<CharT>, CharT, FormatSpec>(writer, spec)
  {
  }
};


template <typename CharT, typename ArgFormatter = PrintfArgFormatter<CharT>>
class PrintfFormatter : private internal::FormatterBase
{
 public:
  explicit PrintfFormatter(const ArgList& arg_list, BasicWriter<CharT>& writer)
    : FormatterBase(arg_list)
    , writer_(writer)
  {
  }

  void Format(const BasicCStringRef<CharT>& format_str);

 private:
  BasicWriter<CharT>& writer_;

  void ParseFlags(FormatSpec& spec, const CharT*& s);

  internal::Arg GetArg(const CharT* s, uint32 arg_index = std::numeric_limits<uint32>::max());

  uint32 ParseHeader(const CharT*& s, FormatSpec& spec);
};

//TODO 길이를 지정할 수 있게 하는게 좋을듯 한데...
template <typename CharT, typename ArgFormatter>
void PrintfFormatter<CharT,ArgFormatter>::ParseFlags(FormatSpec& spec, const CharT*& s)
{
  for (;;)
  {
    switch (*s++) {
    case '-': spec.align_ = ALIGN_LEFT; break;
    case '+': spec.flags_ |= SIGN_FLAG | PLUS_FLAG; break;
    case '0': spec.fill_ = '0'; break;
    case ' ': spec.flags_ |= SIGN_FLAG; break;
    case '#': spec.flags_ |= HASH_FLAG; break;
    default: --s; return; // 관심 플래그가 아니므로, 원래대로 돌려주어야함.
    }
  }
}

template <typename CharT, typename ArgFormatter>
internal::Arg PrintfFormatter<CharT,ArgFormatter>::GetArg(const CharT* s, uint32 arg_index)
{
  (void)s;
  const char* error = nullptr;
  auto arg = arg_index == std::numeric_limits<uint32>::max() ? NextArg(error) : FormatterBase::GetArg(arg_index - 1, error);
  if (error)
  {
    SF_THROW(FormatError(!*s ? "invalid format string" : error));
  }
  return arg;
}

template <typename CharT, typename ArgFormatter>
uint32 PrintfFormatter<CharT,ArgFormatter>::ParseHeader(const CharT*& s, FormatSpec& spec)
{
  uint32 arg_index = std::numeric_limits<uint32>::max();
  CharT c = *s;
  if (c >= '0' && c <= '9')
  {
    uint32 val = internal::ParseNonNegativeInt(s);
    if (*s == '$') // value is an argument index
    {
      ++s;
      arg_index = val;
    }
    else
    {
      if (c == '0')
      {
        spec.fill_ = '0';
      }

      if (val != 0)
      {
        spec.width_ = val;
        return arg_index;
      }
    }
  }

  ParseFlags(spec, s);

  // parse width.
  if (*s >= '0' && *s <= '9')
  {
    spec.width_ = internal::ParseNonNegativeInt(s);
  }
  else if (*s == '*')
  {
    ++s;
    spec.width_ = internal::WidthHandler(spec).Visit(GetArg(s));
  }
  return arg_index;
}

template <typename CharT, typename ArgFormatter>
void PrintfFormatter<CharT,ArgFormatter>::Format(const BasicCStringRef<CharT>& format_str)
{
  const CharT* start = format_str.c_str();
  const CharT* s = start;
  while (*s) //TODO nul-term이 아니어도 상관 없도록 개선하자.
  {
    const CharT c = *s++;

    // % 가 아니라면, plain text이므로, 포인터만 이동. (길이 측정)
    if (c != '%')
    {
      continue;
    }

    // %% 오면 %으로 대체. (% escape)
    if (*s == c) // %%
    {
      Write(Writer, start, s);
      start = ++s;
      continue;
    }

    // 포맷 지정자가 아닌 plain text 출력.
    Write(Writer, start, s - 1);

    FormatSpec spec;
    spec.align_ = ALIGN_RIGHT;

    // parse argument index, flags and width.
    uint32 arg_index = ParseHeader(s, spec);

    // parse precision.
    if (*s == '.')
    {
      ++s; // skip '.'

      if (*s >= '0' && *s <= '9')
      {
        spec.precision_ = static_cast<int32>(internal::ParseNonNegativeInt(s));
      }
      else if (*s == '*')
      {
        ++s;
        spec.precision_ = internal::PrecisionHandler().Visit(GetArg(s));
      }
      else
      {
        spec.precision_ = 0;
      }
    }

    using internal::Arg;
    Arg arg = GetArg(s, arg_index); // mutable

    if (spec.Flag(HASH_FLAG) && internal::IsZeroInt().Visit(arg))
    {
      spec.flags_ &= ~internal::ToUnsigned<int32>(HASH_FLAG);
    }

    if (spec.fill_ == '0')
    {
      if (arg.type <= Arg::LAST_NUMERIC_TYPE)
      {
        spec.align_ = ALIGN_NUMERIC;
      }
      else
      {
        spec.fill_ = ' ';  // ignore '0' flag for non-numeric types.
      }
    }

    // parse length and convert the argument to the required type.
    using internal::ArgConverter;
    switch (*s++) {
    case 'h':
      if (*s == 'h')
      {
        ArgConverter<signed char>(arg, *++s).Visit(arg);
      }
      else
      {
        ArgConverter<short>(arg, *s).Visit(arg);
      }
      break;
    case 'l':
      if (*s == 'l')
      {
        ArgConverter<LongLong>(arg, *++s).Visit(arg);
      }
      else
      {
        ArgConverter<long>(arg, *s).Visit(arg);
      }
      break;
    case 'j':
      ArgConverter<intmax_t>(arg, *s).Visit(arg);
      break;
    case 'z':
      ArgConverter<size_t>(arg, *s).Visit(arg);
      break;
    case 't':
      ArgConverter<std::ptrdiff_t>(arg, *s).Visit(arg);
      break;
    case 'L':
      // printf produces garbage when 'L' is omitted for long double, no need to do the same.
      break;
    default:
      ArgConverter<void>(arg, *--s).Visit(arg);
      break;
    }

    // parse type.
    if (!*s)
    {
      SF_THROW(FormatError("invalid format string"));
    }
    spec.type_ = static_cast<char>(*s++);

    if (spec.type_ == 's')
    {
      // set the format type to the default if 's' is specified
      spec.type_ = internal::DefaultType().Visit(arg);
    }

    if (arg.type <= Arg::LAST_INTEGER_TYPE)
    {
      // normalize type
      switch (spec.type_) {
      case 'i': case 'u':
        spec.type_ = 'd';
        break;
      case 'c':
        //TODO handle wchar_t
        internal::CharConverter(arg).Visit(arg);
        break;
      }
    }

    start = s;

    // format argument.
    ArgFormatter(Writer, spec).Visit(arg);
  }

  Write(Writer, start, s);
}

inline void printf(WriteA& writer, const CStringRefA& format_str, const ArgList args)
{
  PrintfFormatter<char>(args, writer).Format(format_str);
}
//TODO 매크로를 제거하는쪽으로 변경하도록 하자.
SF_VARIADIC(void, printf, WriteA&, const CStringRefA&);


inline std::string sprintf(const CStringRefA& format_str, const ArgList args)
{
  MemoryWriterA writer;
  printf(writer, format_str, args);
  return writer.str();
}
SF_VARIADIC(std::string, sprintf, const CStringRefA&);

FUN_BASE_API int32 fprintf(std::FILE* fp, const CStringRefA& format_str, const ArgList&);
SF_VARIADIC(int32, fprintf, std::FILE*, const CStringRefA&);

inline int32 printf(const CStringRefA& format_str, const ArgList& args)
{
  return fprintf(stdout, format_str, args);
}
SF_VARIADIC(int32, printf, const CStringRefA&);


} // namespace sf
} // namespace fun




/*

template <typename Context, typename ...Args>
class FormatArgStorage
{
 private:
  static const size_t NUM_ARGS = sizeof... (Args);

  static const bool IS_PACKED = NUM_ARGS < internal::MAX_PACKED_ARGS;

  typedef typename Conditional<IS_PAKCED,
            internal::Value<Context>, BasicFormatArg<Context>>::Type ValueType;

  static const size_t DATA_SIZE =
         NUM_ARGS + (IS_PACKED && NUM_ARGS != 0 ? 0 : 1);
  ValueType data_[DATA_SIZE];

  friend class BasicFormatArgs<Context>;

  static int64 GetTypes()
  {
    return IS_PACKED ?
        static_cast<int64>(internal::GetTypes<Context, Args...>()) :
        -static_cast<int64>(NUM_ARGS);
  }

 public:
  static const int64 TYPES = GetTypes();

  FormatArgStorage(const Args&... args)
    : data_{internal::MakeArg<IS_PACKED, Context>(args)...}
  {
  }
};


template <typename Context, typename... Args>
inline FormatArgStorage<Context, Args...>
  MakeFormatArgs(const Args&... args)
{
  return {args...};
}

template <typename... Args>
inline FormatArgStorage<FormatContext, Args...>
  MakeFormatArgs(const Args&... args)
{
  return {args...};
}


*/




