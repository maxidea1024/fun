#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * This class provides some static methods that are
 * used by the
 * fun_check_dbg(), fun_check(), fun_check_ptr(),
 * fun_bugcheck() and fun_unexpected() macros.
 *
 * You should not invoke these methods
 * directly. Use the macros instead, as they
 * automatically provide useful context information.
 */
class FUN_BASE_API Debug {
 public:
  /**
   * An assertion failed. Break into the debugger, if
   * possible, then throw an AssertionViolationException.
   */
  static void Assertion(const char* cond, const char* file, int32 line,
                        const char* text = nullptr);

  /**
   * An null pointer was encountefun. Break into the debugger, if
   * possible, then throw an NullPointerException.
   */
  static void NullPointer(const char* ptr, const char* file, int32 line);

  /**
   * An internal error was encountefun. Break into the debugger, if
   * possible, then throw an BugcheckException.
   */
  static void Bugcheck(const char* file, int32 line);

  /**
   * An internal error was encountefun. Break into the debugger, if
   * possible, then throw an BugcheckException.
   */
  static void Bugcheck(const char* msg, const char* file, int32 line);

  /**
   * An exception was caught in a destructor. Break into debugger,
   * if possible and report exception. Must only be called from
   * within a catch () block as it rethrows the exception to
   * determine its class.
   */
  static void Unexpected(const char* file, int32 line);

  /**
   * An internal error was encountefun. Break into the debugger, if possible.
   */
  static void Debugger(const char* file, int32 line);

  /**
   * An internal error was encountefun. Break into the debugger, if possible.
   */
  static void Debugger(const char* msg, const char* file, int32 line);

 protected:
  static String What(const char* msg, const char* file, int32 line,
                     const char* text = nullptr);
};

}  // namespace fun

// TODO 메시지 출력이 필요한 경우에는 format을 지원하는게 좋을듯...

#if defined(__clang_analyzer__)

#include <cstdlib>  // for abort

#define fun_check_dbg(cond)    \
  do {                         \
    if (!(cond)) std::abort(); \
  } while (0);
#define fun_check_msg_dbg(cond, msg, ...) \
  do {                                    \
    if (!(cond)) std::abort();            \
  } while (0);
#define fun_check(cond)        \
  do {                         \
    if (!(cond)) std::abort(); \
  } while (0);
#define fun_check_msg(cond, msg, ...) \
  do {                                \
    if (!(cond)) std::abort();        \
  } while (0);
#define fun_check_ptr(ptr)    \
  do {                        \
    if (!(ptr)) std::abort(); \
  } while (0);
#define fun_bugcheck() \
  do {                 \
    std::abort();      \
  } while (0);
#define fun_bugcheck_msg(msg, ...) \
  do {                             \
    std::abort();                  \
  } while (0);

#else  // defined(__clang_analyzer__)

#if defined(_DEBUG)
#define fun_check_dbg(cond)                           \
  if (!(cond))                                        \
    fun::Debug::Assertion(#cond, __FILE__, __LINE__); \
  else                                                \
    (void)0;
   // TODO 가변 포맷 제대로 지원하자! (현재는 컴파일 오류만 안나게 해놓은
   // 상태임)
#define fun_check_msg_dbg(cond, msg, ...)                  \
  if (!(cond))                                             \
    fun::Debug::Assertion(#cond, __FILE__, __LINE__, msg); \
  else                                                     \
    (void)0;
#else
#define fun_check_dbg(cond, ...)
#define fun_check_msg_dbg(cond, msg, ...)
#endif

#define fun_check(cond)                               \
  if (!(cond))                                        \
    fun::Debug::Assertion(#cond, __FILE__, __LINE__); \
  else                                                \
    (void)0;

// TODO 가변 포맷 제대로 지원하자! (현재는 컴파일 오류만 안나게 해놓은 상태임)
#define fun_check_msg(cond, msg, ...)                      \
  if (!(cond))                                             \
    fun::Debug::Assertion(#cond, __FILE__, __LINE__, msg); \
  else                                                     \
    (void)0;

#define fun_check_ptr(ptr)                             \
  if (!(ptr))                                          \
    fun::Debug::NullPointer(#ptr, __FILE__, __LINE__); \
  else                                                 \
    (void)0;

#define fun_bugcheck() fun::Debug::Bugcheck(__FILE__, __LINE__);

// TODO 가변 포맷 제대로 지원하자! (현재는 컴파일 오류만 안나게 해놓은 상태임)
#define fun_bugcheck_msg(msg, ...) \
  fun::Debug::Bugcheck(msg, __FILE__, __LINE__);

#endif  // // defined(__clang_analyzer__)

#define fun_unexpected() fun::Debug::Unexpected(__FILE__, __LINE__);

#define fun_debugger() fun::Debug::Debugger(__FILE__, __LINE__);

// TODO 가변 포맷 제대로 지원하자! (현재는 컴파일 오류만 안나게 해놓은 상태임)
#define fun_debugger_msg(msg, ...) \
  fun::Debug::Debugger(msg, __FILE__, __LINE__);

#if defined(_DEBUG)
#define fun_stderr_dbg(str)                                           \
  std::cerr << __FILE__ << '(' << std::dec << __LINE__ << "):" << str \
            << std::endl;
#else
#define fun_stderr_dbg(str)
#endif
