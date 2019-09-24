#pragma once

#include "fun/base/base.h"

//TODO FUN_ENCODING_EXPORTS 정의에 대해서 재정립이 필요해보임.

// The following block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the FUN_ENCODING_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// FUN_ENCODING_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#if defined(FUN_COMPILER_MSVC) && defined(FUN_DLL)
  #if defined(FUN_ENCODING_EXPORTS)
    #define FUN_ENCODING_API __declspec(dllexport)
  #else
    #define FUN_ENCODING_API __declspec(dllimport)
  #endif
#endif

#if !defined(FUN_ENCODING_API)
  #if !defined(FUN_NO_GCC_API_ATTRIBUTE) && defined (__GNUC__) && (__GNUC__ >= 4)
    #define FUN_ENCODING_API __attribute__ ((visibility ("default")))
  #else
    #define FUN_ENCODING_API
  #endif
#endif

// Automatically link encodings library.
#if defined(FUN_COMPILER_MSVC)
  #if !defined(FUN_NO_AUTOMATIC_LIBS) && !defined(FUN_ENCODING_EXPORTS)
    #pragma comment(lib, "fun-base-encodings" FUN_LIB_SUFFIX)
  #endif
#endif

namespace fun {

/**
 * Registers the character encodings from the encodings library
 * with the TextEncoding class.
 */
void FUN_ENCODING_API RegisterExtraEncodings();

} // namespace fun
