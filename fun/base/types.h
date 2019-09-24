#if !defined(__FUN_BASE_BASE_H__)
#error Do not include this file directly. It should be included only in base.h file.
#endif


#include <cstdint>
#include <cstddef> // std::ptrdiff_t, std::size_t

#define FUN_HAVE_INT64  1

namespace fun {

typedef std::int8_t   int8;
typedef std::uint8_t  uint8;
typedef std::int16_t  int16;
typedef std::uint16_t uint16;
typedef std::int32_t  int32;
typedef std::uint32_t uint32;
typedef std::int64_t  int64;
typedef std::uint64_t uint64;

#if defined(_MSC_VER)
  //
  // Windows/Visual C++
  //

  #if defined(_WIN64)
    #define FUN_PTR_IS_64_BIT  1
    typedef std::int64_t  intptr_t;
    typedef std::uint64_t uintptr_t;
  #else
    typedef std::int32_t  intptr_t;
    typedef std::uint32_t uintptr_t;
  #endif

#elif defined(__GNUC__) || defined(__clang__)
  //
  // Unix/GCC/Clang
  //

  #if defined(_WIN64)
    #define FUN_PTR_IS_64_BIT  1
    typedef std::int64_t  intptr_t;
    typedef std::uint64_t uintptr_t;
  #else
    #if defined(__LP64__)
      typedef std::int64_t  intptr_t;
      typedef std::uint64_t uintptr_t;
      #define FUN_PTR_IS_64_BIT   1
      #define FUN_LONG_IS_64_BIT  1
    #else
      typedef std::int32_t  intptr_t;
      typedef std::uint32_t uintptr_t;
    #endif
  #endif

#elif defined(__DECCXX)
  //
  // Compaq C++
  //

  typedef int64 intptr_t;
  typedef uint64 uintptr_t;
  #define FUN_PTR_IS_64_BIT   1
  #define FUN_LONG_IS_64_BIT  1

#elif defined(__HP_aCC)
  //
  // HP Ansi C++
  //

  #if defined(__LP64__)
    #define FUN_PTR_IS_64_BIT   1
    #define FUN_LONG_IS_64_BIT  1
    typedef int64 intptr_t;
    typedef uint64 uintptr_t;
  #else
    typedef int32 intptr_t;
    typedef uint32 uintptr_t;
  #endif

#elif defined(__SUNPRO_CC)
  //
  // SUN Forte C++
  //

  #if defined(__sparcv9)
    #define FUN_PTR_IS_64_BIT   1
    #define FUN_LONG_IS_64_BIT  1
    typedef int64 intptr_t;
    typedef uint64 uintptr_t;
  #else
    typedef int32 intptr_t;
    typedef uint32 uintptr_t;
  #endif

#elif defined(__IBMCPP__)
  //
  // IBM XL C++
  //

  #if defined(__64BIT__)
    #define FUN_PTR_IS_64_BIT   1
    #define FUN_LONG_IS_64_BIT  1
    typedef int64 intptr_t;
    typedef uint64 uintptr_t;
  #else
    typedef int32 intptr_t;
    typedef uint32 uintptr_t;
  #endif

#elif defined(__sgi)
  //
  // MIPSpro C++
  //

  #if _MIPS_SZLONG == 64
    #define FUN_PTR_IS_64_BIT   1
    #define FUN_LONG_IS_64_BIT  1
    typedef int64 intptr_t;
    typedef uint64 uintptr_t;
  #else
    typedef int32 intptr_t;
    typedef uint32 uintptr_t;
  #endif

#endif

typedef std::ptrdiff_t ptrdiff_t;

typedef std::size_t size_t;
//typedef std::ssize_t ssize_t;


typedef uint8  UTF8CHAR;
typedef uint16 UTF16CHAR;
typedef uint32 UTF32CHAR;

//typedef uint16 UCS2CHAR;
//typedef char16_t UNICHAR;
//typedef uint16 UNICHAR;

#if FUN_PLATFORM_WINDOWS_FAMILY
typedef wchar_t UNICHAR;
#define __UTEXT(s)  L ## s
#else
typedef char16_t UNICHAR;
#define __UTEXT(s)  u ## s
#endif

#define UTEXT(s)  __UTEXT(s)


#if defined(FUN_PTR_IS_64_BIT) && (FUN_PTR_IS_64_BIT == 1)
  #define FUN_64_BIT  1
#endif


// 이파일은 무조건 include되는 파일이므로 되도록 내용을 많이 포함하고 있을경우
// 컴파일 타임에 지대한 영향을 미치게 되므로, 가급적 가볍게 유지하도록 하자.


template <typename ElementType, size_t N>
//char (*__countof_helper(_UNALIGNED ElementType (&array)[N]))[N];
char (*__countof_helper(ElementType (&array)[N]))[N];

#define countof(array)  (sizeof(*__countof_helper(array)) + 0)

//TODO offsetof

} // namespace fun
