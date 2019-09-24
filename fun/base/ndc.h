#pragma once

#include "fun/base/base.h"

#include "fun/base/string/string.h"
#include "fun/base/container/array.h"

#include <vector>
#include <ostream>
#include <typeinfo>
#include <utility>

#if FUN_COMPILER_GCC && (FUN_PLATFORM == FUN_PLATFORM_LINUX) && (FUN_PLATFORM != FUN_PLATFORM_CYGWIN)
  #define FUN_HAS_BACKTRACE
#endif

#ifdef FUN_HAS_BACKTRACE
  #ifdef FUN_COMPILER_GCC
    #include <cxxabi.h>
    #include <execinfo.h>
    #include <dlfcn.h>
  #endif
#endif

namespace fun {

class ScopedNdc;

class FUN_BASE_API Ndc {
 public:
  Ndc();
  Ndc(const Ndc& rhs);
  ~Ndc();

  Ndc& operator = (const Ndc& rhs);

  void Push(const String& info);
  void Push(const String& info, int32 line, const char* file);
  void Pop();
  int32 GetDepth() const;
  String ToString() const;
  void Dump(std::ostream& ostr) const;
  void Dump(std::ostream& ostr, const String& delimeter, bool name_only = false) const;
  void Clear();

  static Ndc& Current();

  template <typename T>
  static String TypeName(bool full = true) {
    String name(typeid(T).name());

#ifdef FUN_HAS_BACKTRACE
#ifdef FUN_COMPILER_GCC
    int status = 0;
#if (FUN_PLATFORM == FUN_PLATFORM_CYGWIN)
    char* name_ptr = __cxxabiv1::__cxa_demangle(typeid(T).name(), 0, 0, &status);
#else
    char* name_ptr = abi::__cxa_demangle(typeid(T).name(), 0, 0, &status);
#endif
    if (status == 0) {
      name = name_ptr;
    }
    free(name_ptr);

    if (!full) { // strip scope, if any
      std::size_t pos = name.rfind("::");
      if (pos != std::string::npos) {
        name = name.substr(pos + 2);
      }
    }
  #endif // TODO: demangle other compilers
#endif
    return name;
  }

  static String Backtrace(int32 skip_end = 1,
                          int32 skip_begin = 0,
                          int32 stack_size = 128,
                          int32 buf_size = 1024);

  static bool HasBacktrace();

 private:
  struct Context {
    String info;
    const char* file;
    int32 line;
    String trace;
  };
  Array<Context> stack_;
};


class FUN_BASE_API ScopedNdc {
 public:
  ScopedNdc(const String& info);
  ScopedNdc(const String& info, int32 line, const char* file);
  ~ScopedNdc();
};


//
// inlines
//

FUN_ALWAYS_INLINE bool Ndc::HasBacktrace() {
#ifdef FUN_HAS_BACKTRACE
  return true;
#else
  return false;
#endif
}

FUN_ALWAYS_INLINE ScopedNdc::ScopedNdc(const String& info) {
  Ndc::Current().Push(info);
}

FUN_ALWAYS_INLINE ScopedNdc::ScopedNdc(const String& info, int32 line, const char* file) {
  Ndc::Current().Push(info, line, file);
}

FUN_ALWAYS_INLINE ScopedNdc::~ScopedNdc() {
  try {
    Ndc::Current().Pop();
  } catch (...) {
    fun_unexpected();
  }
}


//
// helper macros
//

#define fun_ndc_func \
  fun::ScopedNdc ndc_scope_(__func__), __LINE__, __FILE__);

#define fun_ndc(func) \
  fun::ScopedNdc ndc_scope_(#func, __LINE__, __FILE__);

#define fun_ndc_str(str) \
  fun::ScopedNdc ndc_scope_(str, __LINE__, __FILE__);

#define fun_ndc_bt(from, to) \
  fun::ScopedNdc ndc_scope_(Ndc::Backtrace(from, to), __LINE__, __FILE__);


#if defined(_DEBUG)
  #define fun_ndc_func_dbg \
      fun::ScopedNdc ndc_scope_(__func__, __LINE__, __FILE__);

  #define fun_ndc_dbg(func) \
      fun::ScopedNdc ndc_scope_(#func, __LINE__, __FILE__);

  #define fun_ndc_dbg_str(str) \
      fun::ScopedNdc ndc_scope_(str, __LINE__, __FILE__);

  #define fun_ndc_bt_dbg(from, to) \
    fun::ScopedNdc ndc_scope_(Ndc::Backtrace(from, to), __LINE__, __FILE__);
#else
  #define fun_ndc_func_dbg
  #define fun_ndc_dbg(func)
  #define fun_ndc_dbg_str(str)
  #define fun_ndc_bt_dbg(from, to)
#endif

} // namespace fun
