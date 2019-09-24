#if !defined(__FUN_BASE_BASE_H__)
#error Do not include this file directly. It should be included only in base.h file.
#endif

//
// PA-RISC based HP-UX platforms have some issues...
//

#if defined(hpux) || defined(_hpux)
  #if defined(__hppa) || defined(__hppa__)
    #define FUN_NO_SYS_SELECT_H  1
    #if defined(__HP_aCC)
      #define FUN_NO_TEMPLATE_ICOMPARE  1
    #endif
  #endif
#endif


//
// Thread-safety of local static initialization
//

#if __cplusplus >= 201103L || __GNUC__ >= 4 || defined(__clang__)
  #ifndef FUN_LOCAL_STATIC_INIT_IS_THREADSAFE
    #define FUN_LOCAL_STATIC_INIT_IS_THREADSAFE  1
  #endif
#endif


//
// No syslog.h on QNX/BB10
//
#if defined(__QNXNTO__)
  #define FUN_NO_SYSLOGCHANNEL  1
#endif


//
// C++14 support
//

// Enable C++14 support for AppleClang 503.x (Clang 3.4)
#if defined(__clang__) && defined(__apple_build_version__) && (__apple_build_version__ >= 5030038) && !defined(FUN_ENABLE_CPP14) && !defined(FUN_DISABLE_CPP14)
  #define FUN_ENABLE_CPP14  1
#endif

// Enable C++14 support for Clang 3.4
#if defined(__clang__) && !defined(__apple_build_version__) && (__clang_major__ > 3 || (__clang_major__ == 3 && __clang_minor__ >= 4)) && !defined(FUN_ENABLE_CPP14) && !defined(FUN_DISABLE_CPP14)
  #define FUN_ENABLE_CPP14  1
#endif

// Enable C++14 support for GCC 4.9.2
#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && (__GNUC_MINOR__ > 9 || (__GNUC_MINOR__ == 9 && __GNUC_PATCHLEVEL__ >= 2)))) && !defined(FUN_ENABLE_CPP14) && !defined(FUN_DISABLE_CPP14)
  #define FUN_ENABLE_CPP14  1
#endif


#define FUN_ALWAYS_INLINE   inline __attribute__((always_inline))   // Force code to be inline
#define FUN_NO_INLINE       __attribute__((noinline))               // Force code to NOT be inline
#define FUN_NO_RETURN       __attribute__((noreturn))               // Indicate that the function never returns


#define FUN_DLL_EXPORT      __attribute__((visibility("default")))
#define FUN_DLL_IMPORT      __attribute__((visibility("default")))


// Only enable vectorintrinsics on x86(-64) for now
#if defined(_M_IX86) || defined(__i386__) || defined(_M_X64) || defined(__x86_64__) || defined (__amd64__)
#define FUN_PLATFORM_ENABLE_VECTORINTRINSICS  1
#else
#define FUN_PLATFORM_ENABLE_VECTORINTRINSICS  1
#endif


// RISC 계열 CPU에서는 aligned access를 해야함.
#if defined(__arm__)
#define FUN_REQUIRES_ALIGNED_ACCESS   1
#else
#define FUN_REQUIRES_ALIGNED_ACCESS   0
#endif
