#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * The Error class provides utility functions
 * for error reporting.
 */
class FUN_BASE_API Error {
 public:
#ifdef FUN_PLATFORM_WINDOWS_FAMILY
  static uint32 Code();
  static String Message(uint32 error_code);
#else
  static int32 Code();
  static String Message(int32 error_code);

 private:
  static const char* strerror_result(int, const char* s) { return s; }
  static const char* strerror_result(const char* s, const char*) { return s; }
#endif
};

}  // namespace fun
