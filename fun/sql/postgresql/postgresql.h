#pragma once

#include "fun/base.h"

// The following block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the FUN_ODBC_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// FUN_ODBC_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#if defined(FUN_COMPILER_MSVC) && defined(FUN_DLL)
  #if defined(FUN_POSTGRESQL_EXPORTS)
    #define FUN_POSTGRESQL_API __declspec(dllexport)
  #else
    #define FUN_POSTGRESQL_API __declspec(dllimport)
  #endif
#endif

#if !defined(FUN_POSTGRESQL_API)
  #if !defined(FUN_NO_GCC_API_ATTRIBUTE) && defined (__GNUC__) && (__GNUC__ >= 4)
    #define FUN_POSTGRESQL_API __attribute__((visibility("default")))
  #else
    #define FUN_POSTGRESQL_API
  #endif
#endif

// Automatically link Data library.
#if defined(FUN_COMPILER_MSVC)
  #if !defined(FUN_NO_AUTOMATIC_LIBS) && !defined(FUN_POSTGRESQL_EXPORTS)
    #pragma comment(lib, "fun-sql-postgresql" FUN_LIB_SUFFIX)
  #endif
#endif
