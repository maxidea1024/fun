#pragma once

#ifndef __FUN_BASE_BASE_H__
#define __FUN_BASE_BASE_H__


#include "fun/base/build_config.h"
#include "fun/base/base_defs.h"


//
// Include platform-specific definitions
//

#include "fun/base/platform.h"

//TODO android, ios, emscripten, tizen등 별도로 처리하자.
#if defined(_WIN32)
  #include "fun/base/platform_win32.h"
#elif FUN_PLATFORM != FUN_PLATFORM_VXWORKS
  #include "fun/base/platform_vx.h"
#elif FUN_PLATFORM_UNIX_FAMILY
  #include "fun/base/platform_posix.h"
#endif



//
// Ensure that FUN_DLL is default unless FUN_STATIC is defined
//

#if defined(_WIN32) && defined(_DLL)
  #if !defined(FUN_DLL) && !defined(FUN_STATIC)
    #define FUN_DLL  1
  #endif
#endif


//
// The following block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the FUN_BASE_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// FUN_BASE_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
//

#if (defined(_WIN32) || defined(_WIN32_WCE)) && defined(FUN_DLL)
  #if defined(FUN_BASE_EXPORTS)
    #define FUN_BASE_API  __declspec(dllexport)
  #else
    #define FUN_BASE_API  __declspec(dllimport)
  #endif
#endif


#if !defined(FUN_BASE_API)
  #if !defined(FUN_NO_GCC_API_ATTRIBUTE) && defined(__GNUC__) && (__GNUC__ >= 4)
    #define FUN_BASE_API  __attribute__ ((visibility ("default")))
  #else
    #define FUN_BASE_API
  #endif
#endif


#ifndef FUN_BASE_API
#define FUN_BASE_API
#endif


//
// Automatically link Foundation library.
//

#if defined(_MSC_VER)
  #if defined(FUN_DLL)
    #if defined(_DEBUG)
      #define FUN_LIB_SUFFIX  "d.lib"
    #else
      #define FUN_LIB_SUFFIX  ".lib"
    #endif
  #elif defined(_DLL)
    #if defined(_DEBUG)
      #define FUN_LIB_SUFFIX  "mdd.lib"
    #else
      #define FUN_LIB_SUFFIX  "md.lib"
    #endif
  #else
    
    #if defined(_DEBUG)
      #define FUN_LIB_SUFFIX  "mtd.lib"
    #else
      #define FUN_LIB_SUFFIX  "mt.lib"
    #endif
  #endif

  #if !defined(FUN_NO_AUTOMATIC_LIBS) && !defined(FUN_BASE_EXPORTS)
    #pragma comment(lib, "fun-base" FUN_LIB_SUFFIX)
  #endif
#endif




//
// FUN_JOIN
//
// The following piece of macro magic joins the two
// arguments together, even when one of the arguments is
// itself a macro (see 16.3.1 in C++ standard).  The key
// is that macro expansion of macro arguments does not
// occur in FUN_DO_JOIN2 but does in FUN_DO_JOIN.
//

#define FUN_JOIN(X, Y)      FUN_DO_JOIN(X, Y)
#define FUN_DO_JOIN(X, Y)   FUN_DO_JOIN2(X, Y)
#define FUN_DO_JOIN2(X, Y)  X ## Y


//
// FUN_DISALLOW_COPY_AND_ASSIGNMENT
//

#define FUN_DISALLOW_COPY_AND_ASSIGNMENT(Class) \
  public: \
    Class(const Class&) = delete; \
    Class& operator = (const Class&) = delete;


//
// Pull in basic definitions
//

#include "fun/base/types.h"
#include "fun/base/base_forward_decls.h"
#include "fun/base/debug.h"
#include "fun/base/alignment.h"
#include "fun/base/constraint_tags.h"

#undef __FUN_BASE_BASE_H__

#endif // #ifndef __FUN_BASE_BASE_H__
