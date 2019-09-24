#if !defined(__FUN_BASE_BASE_H__)
#error Do not include this file directly. It should be included only in base.h file.
#endif

//
// Platform Identification
//

#define FUN_PLATFORM_FREE_BSD       0x0001
#define FUN_PLATFORM_AIX            0x0002
#define FUN_PLATFORM_HPUX           0x0003
#define FUN_PLATFORM_TRU64          0x0004
#define FUN_PLATFORM_LINUX          0x0005
#define FUN_PLATFORM_MAC_OS_X       0x0006
#define FUN_PLATFORM_NET_BSD        0x0007
#define FUN_PLATFORM_OPEN_BSD       0x0008
#define FUN_PLATFORM_IRIX           0x0009
#define FUN_PLATFORM_SOLARIS        0x000a
#define FUN_PLATFORM_QNX            0x000b
#define FUN_PLATFORM_VXWORKS        0x000c
#define FUN_PLATFORM_CYGWIN         0x000d
#define FUN_PLATFORM_NACL           0x000e
#define FUN_PLATFORM_ANDROID        0x000f
#define FUN_PLATFORM_EMSCRIPTEN     0x0010
#define FUN_PLATFORM_UNKNOWN_UNIX   0x00ff
#define FUN_PLATFORM_WINDOWS_NT     0x1001
#define FUN_PLATFORM_WINDOWS_CE     0x1011
#define FUN_PLATFORM_VMS            0x2001

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
  #define FUN_PLATFORM_UNIX_FAMILY  1
  #define FUN_PLATFORM_BSD_FAMILY  1
  #define FUN_PLATFORM  FUN_PLATFORM_FREE_BSD
#elif defined(_AIX) || defined(__TOS_AIX__)
  #define FUN_PLATFORM_UNIX_FAMILY  1
  #define FUN_PLATFORM  FUN_PLATFORM_AIX
#elif defined(hpux) || defined(_hpux) || defined(__hpux)
  #define FUN_PLATFORM_UNIX_FAMILY  1
  #define FUN_PLATFORM  FUN_PLATFORM_HPUX
#elif defined(__digital__) || defined(__osf__)
  #define FUN_PLATFORM_UNIX_FAMILY  1
  #define FUN_PLATFORM  FUN_PLATFORM_TRU64
#elif defined(__NACL__)
  #define FUN_PLATFORM_UNIX_FAMILY  1
  #define FUN_PLATFORM  FUN_PLATFORM_NACL
#elif defined(__EMSCRIPTEN__) || defined(EMSCRIPTEN)
  #define FUN_PLATFORM_UNIX_FAMILY  1
  #define FUN_PLATFORM FUN_PLATFORM_EMSCRIPTEN
#elif defined(linux) || defined(__linux) || defined(__linux__) || defined(__TOS_LINUX__)
  #define FUN_PLATFORM_UNIX_FAMILY  1
  #if defined(__ANDROID__)
    #define FUN_PLATFORM  FUN_PLATFORM_ANDROID
  #else
    #define FUN_PLATFORM  FUN_PLATFORM_LINUX
  #endif
#elif defined(__APPLE__) || defined(__TOS_MACOS__)
  #define FUN_PLATFORM_UNIX_FAMILY  1
  #define FUN_PLATFORM_BSD_FAMILY  1
  #define FUN_PLATFORM  FUN_PLATFORM_MAC_OS_X
#elif defined(__NetBSD__)
  #define FUN_PLATFORM_UNIX_FAMILY  1
  #define FUN_PLATFORM_BSD_FAMILY  1
  #define FUN_PLATFORM  FUN_PLATFORM_NET_BSD
#elif defined(__OpenBSD__)
  #define FUN_PLATFORM_UNIX_FAMILY  1
  #define FUN_PLATFORM_BSD_FAMILY  1
  #define FUN_PLATFORM FUN_PLATFORM_OPEN_BSD
#elif defined(sgi) || defined(__sgi)
  #define FUN_PLATFORM_UNIX_FAMILY  1
  #define FUN_PLATFORM  FUN_PLATFORM_IRIX
#elif defined(sun) || defined(__sun)
  #define FUN_PLATFORM_UNIX_FAMILY  1
  #define FUN_PLATFORM  FUN_PLATFORM_SOLARIS
#elif defined(__QNX__)
  #define FUN_PLATFORM_UNIX_FAMILY  1
  #define FUN_PLATFORM  FUN_PLATFORM_QNX
#elif defined(__CYGWIN__)
  #define FUN_PLATFORM_UNIX_FAMILY  1
  #define FUN_PLATFORM  FUN_PLATFORM_CYGWIN
#elif defined(FUN_VXWORKS)
  #define FUN_PLATFORM_UNIX_FAMILY  1
  #define FUN_PLATFORM  FUN_PLATFORM_VXWORKS
#elif defined(unix) || defined(__unix) || defined(__unix__)
  #define FUN_PLATFORM_UNIX_FAMILY  1
  #define FUN_PLATFORM  FUN_PLATFORM_UNKNOWN_UNIX
#elif defined(_WIN32_WCE)
  #define FUN_PLATFORM_WINDOWS_FAMILY  1
  #define FUN_PLATFORM FUN_PLATFORM_WINDOWS_CE
#elif defined(_WIN32) || defined(_WIN64)
  #define FUN_PLATFORM_WINDOWS_FAMILY  1
  #define FUN_PLATFORM  FUN_PLATFORM_WINDOWS_NT
#elif defined(__VMS)
  #define FUN_PLATFORM_VMS_FAMILY  1
  #define FUN_PLATFORM  FUN_PLATFORM_VMS
#endif

#if !defined(FUN_PLATFORM)
  #error "Unknown Platform."
#endif


//
// Hardware Architecture and Byte Order
//

#define FUN_ARCH_ALPHA   0x01
#define FUN_ARCH_IA32    0x02
#define FUN_ARCH_IA64    0x03
#define FUN_ARCH_MIPS    0x04
#define FUN_ARCH_HPPA    0x05
#define FUN_ARCH_PPC     0x06
#define FUN_ARCH_POWER   0x07
#define FUN_ARCH_SPARC   0x08
#define FUN_ARCH_AMD64   0x09
#define FUN_ARCH_ARM     0x0a
#define FUN_ARCH_M68K    0x0b
#define FUN_ARCH_S390    0x0c
#define FUN_ARCH_SH      0x0d
#define FUN_ARCH_NIOS2   0x0e
#define FUN_ARCH_AARCH64 0x0f
#define FUN_ARCH_ARM64   0x0f
#define FUN_ARCH_RISCV64 0x10

#if defined(__ALPHA) || defined(__alpha) || defined(__alpha__) || defined(_M_ALPHA)
  #define FUN_ARCH  FUN_ARCH_ALPHA
  #define FUN_ARCH_LITTLE_ENDIAN  1
#elif defined(i386) || defined(__i386) || defined(__i386__) || defined(_M_IX86) || defined(EMSCRIPTEN) || defined(__EMSCRIPTEN__)
  #define FUN_ARCH  FUN_ARCH_IA32
  #define FUN_ARCH_LITTLE_ENDIAN  1
#elif defined(_IA64) || defined(__IA64__) || defined(__ia64__) || defined(__ia64) || defined(_M_IA64)
  #define FUN_ARCH  FUN_ARCH_IA64
  #if defined(hpux) || defined(_hpux)
    #define FUN_ARCH_BIG_ENDIAN  1
  #else
    #define FUN_ARCH_LITTLE_ENDIAN  1
  #endif
#elif defined(__x86_64__) || defined(_M_X64)
  #define FUN_ARCH  FUN_ARCH_AMD64
  #define FUN_ARCH_LITTLE_ENDIAN  1
#elif defined(__mips__) || defined(__mips) || defined(__MIPS__) || defined(_M_MRX000)
  #define FUN_ARCH  FUN_ARCH_MIPS
  #if defined(FUN_PLATFORM_WINDOWS_FAMILY)
    // Is this OK? Supports windows only little endian??
    #define FUN_ARCH_LITTLE_ENDIAN  1
  #elif defined(__MIPSEB__) || defined(_MIPSEB) || defined(__MIPSEB)
    #define FUN_ARCH_BIG_ENDIAN  1
  #elif defined(__MIPSEL__) || defined(_MIPSEL) || defined(__MIPSEL)
    #define FUN_ARCH_LITTLE_ENDIAN  1
  #else
    #error "MIPS but neither MIPSEL nor MIPSEB?"
  #endif
#elif defined(__hppa) || defined(__hppa__)
  #define FUN_ARCH  FUN_ARCH_HPPA
  #define FUN_ARCH_BIG_ENDIAN  1
#elif defined(__PPC) || defined(__POWERPC__) || defined(__powerpc) || defined(__PPC__) || \
      defined(__powerpc__) || defined(__ppc__) || defined(__ppc) || defined(_ARCH_PPC) || defined(_M_PPC)
  #define FUN_ARCH FUN_ARCH_PPC
  #if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
    #define FUN_ARCH_LITTLE_ENDIAN  1
  #else
    #define FUN_ARCH_BIG_ENDIAN  1
  #endif
#elif defined(_POWER) || defined(_ARCH_PWR) || defined(_ARCH_PWR2) || defined(_ARCH_PWR3) || \
      defined(_ARCH_PWR4) || defined(__THW_RS6000)
  #define FUN_ARCH  FUN_ARCH_POWER
  #define FUN_ARCH_BIG_ENDIAN  1
#elif defined(__sparc__) || defined(__sparc) || defined(sparc)
  #define FUN_ARCH  FUN_ARCH_SPARC
  #define FUN_ARCH_BIG_ENDIAN  1
#elif defined(__arm__) || defined(__arm) || defined(ARM) || defined(_ARM_) || defined(__ARM__) || defined(_M_ARM)
  #define FUN_ARCH  FUN_ARCH_ARM
  #if defined(__ARMEB__)
    #define FUN_ARCH_BIG_ENDIAN  1
  #else
    #define FUN_ARCH_LITTLE_ENDIAN  1
  #endif
#elif defined(__arm64__) || defined(__arm64)
  #define FUN_ARCH  FUN_ARCH_ARM64
  #if defined(__ARMEB__)
    #define FUN_ARCH_BIG_ENDIAN  1
  #elif defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    #define FUN_ARCH_BIG_ENDIAN  1
  #else
    #define FUN_ARCH_LITTLE_ENDIAN  1
  #endif
#elif defined(__m68k__)
  #define FUN_ARCH  FUN_ARCH_M68K
  #define FUN_ARCH_BIG_ENDIAN  1
#elif defined(__s390__)
  #define FUN_ARCH  FUN_ARCH_S390
  #define FUN_ARCH_BIG_ENDIAN  1
#elif defined(__sh__) || defined(__sh) || defined(SHx) || defined(_SHX_)
  #define FUN_ARCH FUN_ARCH_SH
  #if defined(__LITTLE_ENDIAN__) || (FUN_PLATFORM == FUN_PLATFORM_WINDOWS_CE)
    #define FUN_ARCH_LITTLE_ENDIAN  1
  #else
    #define FUN_ARCH_BIG_ENDIAN  1
  #endif
#elif defined (nios2) || defined(__nios2) || defined(__nios2__)
  #define FUN_ARCH  FUN_ARCH_NIOS2
  #if defined(__nios2_little_endian) || defined(nios2_little_endian) || defined(__nios2_little_endian__)
    #define FUN_ARCH_LITTLE_ENDIAN  1
  #else
    #define FUN_ARCH_BIG_ENDIAN  1
  #endif
#elif defined(__AARCH64EL__)
  #define FUN_ARCH  FUN_ARCH_AARCH64
  #define FUN_ARCH_LITTLE_ENDIAN  1
#elif defined(__AARCH64EB__)
  #define FUN_ARCH  FUN_ARCH_AARCH64
  #define FUN_ARCH_BIG_ENDIAN  1
#elif defined(__riscv) && (__riscv_xlen == 64)
  #define FUN_ARCH  FUN_ARCH_RISCV64
  #define FUN_ARCH_LITTLE_ENDIAN  1
#endif

#if defined(__clang__)
  #define FUN_COMPILER_CLANG  1
  #if defined(__apple_build_version__)
    #define FUN_COMPILER_APPLECLANG  1
  #endif
#elif defined(_MSC_VER)
  #define FUN_COMPILER_MSVC  1
#elif defined (__GNUC__)
  #define FUN_COMPILER_GCC  1
#elif defined (__MINGW32__) || defined (__MINGW64__)
  #define FUN_COMPILER_MINGW  1
#elif defined (__INTEL_COMPILER) || defined(__ICC) || defined(__ECC) || defined(__ICL)
  #define FUN_COMPILER_INTEL  1
#elif defined (__SUNPRO_CC)
  #define FUN_COMPILER_SUN  1
#elif defined (__MWERKS__) || defined(__CWCC__)
  #define FUN_COMPILER_CODEWARRIOR  1
#elif defined (__sgi) || defined(sgi)
  #define FUN_COMPILER_SGI  1
#elif defined (__HP_aCC)
  #define FUN_COMPILER_HP_ACC  1
#elif defined (__BORLANDC__) || defined(__CODEGEARC__)
  #define FUN_COMPILER_CBUILDER  1
#elif defined (__DMC__)
  #define FUN_COMPILER_DMARS  1
#elif defined (__DECCXX)
  #define FUN_COMPILER_COMPAC  1
#elif (defined (__xlc__) || defined (__xlC__)) && defined(__IBMCPP__)
  #define FUN_COMPILER_IBM_XLC  1 // IBM XL C++
#elif defined (__IBMCPP__) && defined(__COMPILER_VER__)
  #define FUN_COMPILER_IBM_XLC_ZOS  1 // IBM z/OS C++
#endif

#ifdef __GNUC__
#define FUN_UNUSED  __attribute__((unused))
#else
#define FUN_UNUSED
#endif // __GNUC__

#if !defined(FUN_ARCH)
  #error "Unknown Hardware Architecture."
#endif


#if FUN_PLATFORM_WINDOWS_FAMILY
  #define FUN_DEFAULT_NEWLINE_CHARS  "\r\n"
#else
  #define FUN_DEFAULT_NEWLINE_CHARS  "\n"
#endif


#ifdef __GNUC__
#define FUN_LIKELY(x)       __builtin_expect(!!(x), 1)
#define FUN_UNLIKELY(x)     __builtin_expect(!!(x), 0)
#else
#define FUN_LIKELY(x)       (x)
#define FUN_UNLIKELY(x)     (x)
#endif


//TODO c++17 이상의 feature이므로, c++17이 활성화 되었는지 여부를 체크한 후 처리하면 될듯...
//일단은 c++14로 진행중이니 주석처리함.
//#define FUN_FALLTHROUGH  [[fallthrough]]
#define FUN_FALLTHROUGH


#define FUN_PLATFORM_CACHE_LINE_SIZE    128


//TODO 일단은 임시로 나둠. 차후에 정확한 의미를 부여하자.
#define FUN_ALWAYS_INLINE_DEBUGGABLE
