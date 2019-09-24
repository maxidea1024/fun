#pragma once

#include "fun/base/base.h"
#include "fun/base/manifest.h"
#include <typeinfo>

#if defined(_WIN32)
  #define FUN_LIBRARY_API   __declspec(dllexport)
#elif defined(__GNUC__) && (__GNUC__ >= 4)
  #define FUN_LIBRARY_API   __attribute__((visibility("default")))
#else
  #define FUN_LIBRARY_API
#endif

//
// the entry points for every class library
//
extern "C" {
  bool FUN_LIBRARY_API fun_BuildManifest(fun::ManifestBase* manifest);
  void FUN_LIBRARY_API fun_InitializeLibrary();
  void FUN_LIBRARY_API fun_UninitializeLibrary();
}

#define FUN_DECLARE_NAMED_MANIFEST(name) \
  extern "C" { \
    bool FUN_LIBRARY_API FUN_JOIN(fun_BuildManifest, name)(fun::ManifestBase* manifest); \
  }


//
// Macros to automatically implement fun_BuildManifest
//
// usage:
//
// FUN_BEGIN_MANIFEST(MyBaseClass)
//     FUN_EXPORT_CLASS(MyFirstClass)
//     FUN_EXPORT_CLASS(MySecondClass)
//     ...
// FUN_END_MANIFEST
//
#define FUN_BEGIN_MANIFEST_IMPL(func_name, base) \
  bool func_name(fun::ManifestBase* manifest) { \
    typedef base _Base; \
    typedef fun::Manifest<_Base> _Manifest; \
    std::string required_type(typeid(_Manifest).name()); \
    std::string actual_type(manifest->GetClassName()); \
    if (required_type == actual_type) { \
      fun::Manifest<_Base>* manifest = static_cast<_Manifest*>(manifest);

#define FUN_BEGIN_MANIFEST(base) \
  FUN_BEGIN_MANIFEST_IMPL(fun_BuildManifest, base)

#define FUN_BEGIN_NAMED_MANIFEST(name, base) \
  FUN_DECLARE_NAMED_MANIFEST(name) \
  FUN_BEGIN_MANIFEST_IMPL(FUN_JOIN(fun_BuildManifest, name), base)

#define FUN_END_MANIFEST \
      return true; \
    } else { \
      return false; } \
    } \
  }

#define FUN_EXPORT_CLASS(cls) \
  manifest->insert(new fun::MetaObject<cls, _Base>(#cls));

#define FUN_EXPORT_INTERFACE(cls, itf) \
  manifest->insert(new fun::MetaObject<cls, _Base>(itf));

#define FUN_EXPORT_SINGLETON(cls) \
  manifest->insert(new fun::MetaSingleton<cls, _Base>(#cls));
