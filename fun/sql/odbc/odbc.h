#prgma once

#include "fun/base.h"

#ifdef FUN_PLATFORM_WINDOWS_FAMILY
#include <windows.h>
#endif

// The following block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the FUN_ODBC_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// FUN_ODBC_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#if defined(FUN_COMPILER_MSVC) && defined(FUN_DLL)
  #if defined(FUN_ODBC_EXPORTS)
    #define FUN_ODBC_API __declspec(dllexport)
  #else
    #define FUN_ODBC_API __declspec(dllimport)
  #endif
#endif

#if !defined(FUN_ODBC_API)
  #if !defined(FUN_NO_GCC_API_ATTRIBUTE) && defined (__GNUC__) && (__GNUC__ >= 4)
    #define FUN_ODBC_API __attribute__((visibility("default")))
  #else
    #define FUN_ODBC_API
  #endif
#endif

#include "fun/sql/odbc/unicode.h"

// Automatically link Data library.
#if defined(FUN_COMPILER_MSVC)
  #if !defined(FUN_NO_AUTOMATIC_LIBS) && !defined(FUN_ODBC_EXPORTS)
    #pragma comment(lib, "fun-sql-odbc" FUN_LIB_SUFFIX)
  #endif
#endif
