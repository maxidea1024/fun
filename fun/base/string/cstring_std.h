#pragma once

#include "fun/base/string/cstring_generic.h"

#include <string.h>

namespace fun {

class FUN_BASE_API StdCString : public GenericCString {
 public:
  //
  // UNICODE
  //

  // TODO 그냥 제너릭쪽으로 빼줄까??

  FUN_ALWAYS_INLINE static UNICHAR* Strcpy(UNICHAR* dst, size_t dest_count,
                                           const UNICHAR* src) {
    // TODO
  }

  FUN_ALWAYS_INLINE static UNICHAR* Strncpy(UNICHAR* dst, const UNICHAR* src,
                                            int32 max_len) {
    // TODO
  }

  FUN_ALWAYS_INLINE static UNICHAR* Strcat(UNICHAR* dst, size_t dest_count,
                                           const UNICHAR* src) {
    // TODO
  }

  FUN_ALWAYS_INLINE static int32 Strcmp(const UNICHAR* str1,
                                        const UNICHAR* str2) {
    // TODO
  }

  FUN_ALWAYS_INLINE static int32 Strncmp(const UNICHAR* str1,
                                         const UNICHAR* str2, size_t len) {
    // TODO
  }

  FUN_ALWAYS_INLINE static int32 Stricmp(const UNICHAR* str1,
                                         const UNICHAR* str2) {
    // TODO
  }

  FUN_ALWAYS_INLINE static int32 Strnicmp(const UNICHAR* str1,
                                          const UNICHAR* str2, size_t len) {
    // TODO
  }

  FUN_ALWAYS_INLINE static int32 Strlen(const UNICHAR* str) {
    // TODO
  }

  FUN_ALWAYS_INLINE static const UNICHAR* Strstr(const UNICHAR* str,
                                                 const UNICHAR* find) {
    // TODO
  }

  FUN_ALWAYS_INLINE static const UNICHAR* Strchr(const UNICHAR* str,
                                                 UNICHAR ch) {
    // TODO
  }

  FUN_ALWAYS_INLINE static const UNICHAR* Strrchr(const UNICHAR* str,
                                                  UNICHAR ch) {
    // TODO
  }

  FUN_ALWAYS_INLINE static int32 Atoi(const UNICHAR* str) {
    // TODO
  }

  FUN_ALWAYS_INLINE static int64 Atoi64(const UNICHAR* str) {
    // TODO
  }

  FUN_ALWAYS_INLINE static float Atof(const UNICHAR* str) {
    // TODO
  }

  FUN_ALWAYS_INLINE static double Atod(const UNICHAR* str) {
    // TODO
  }

  FUN_ALWAYS_INLINE static int32 Strtoi(const UNICHAR* start, UNICHAR** end,
                                        int32 base) {
    // TODO
  }

  FUN_ALWAYS_INLINE static int64 Strtoi64(const UNICHAR* start, UNICHAR** end,
                                          int32 base) {
    // TODO
  }

  FUN_ALWAYS_INLINE static uint64 Strtoui64(const UNICHAR* start, UNICHAR** end,
                                            int32 base) {
    // TODO
  }

  FUN_ALWAYS_INLINE static UNICHAR* Strtok(UNICHAR* str, const UNICHAR* delim,
                                           UNICHAR** context) {
    // TODO
  }

  //
  // char
  //

  FUN_ALWAYS_INLINE static char* Strcpy(char* dst, size_t dest_count,
                                        const char* src) {
    return strcpy(dst, src);
  }

  FUN_ALWAYS_INLINE static char* Strncpy(char* dst, const char* src,
                                         int32 max_len) {
    strncpy(dst, src, max_len - 1);
    dst[max_len - 1] = 0;
    return dst;
  }

  FUN_ALWAYS_INLINE static char* Strcat(char* dst, size_t dest_count,
                                        const char* src) {
    return strcat(dst, src);
  }

  FUN_ALWAYS_INLINE static int32 Strcmp(const char* str1, const char* str2) {
    return strcmp(str1, str2);
  }

  FUN_ALWAYS_INLINE static int32 Strncmp(const char* str1, const char* str2,
                                         size_t len) {
    return strncmp(str1, str2, len);
  }

  FUN_ALWAYS_INLINE static int32 Strlen(const char* str) { return strlen(str); }

  FUN_ALWAYS_INLINE static const char* Strstr(const char* str,
                                              const char* find) {
    return strstr(str, find);
  }

  FUN_ALWAYS_INLINE static const char* Strchr(const char* str, char ch) {
    return strchr(str, ch);
  }

  FUN_ALWAYS_INLINE static const char* Strrchr(const char* str, char ch) {
    return strrchr(str, ch);
  }

  FUN_ALWAYS_INLINE static int32 Atoi(const char* str) { return atoi(str); }

  FUN_ALWAYS_INLINE static int64 Atoi64(const char* str) {
#if FUN_PLATFORM_HTML5_WIN32
    return _atoi64(str);
#else
    return strtoll(str, nullptr, 10);
#endif
  }

  FUN_ALWAYS_INLINE static float Atof(const char* str) {
    return (float)atof(str);
  }

  FUN_ALWAYS_INLINE static double Atod(const char* str) { return atof(str); }

  FUN_ALWAYS_INLINE static int32 Strtoi(const char* start, char** end,
                                        int32 base) {
    return strtol(start, end, base);
  }

  FUN_ALWAYS_INLINE static int64 Strtoi64(const char* start, char** end,
                                          int32 base) {
#if FUN_PLATFORM_HTML5_WIN32
    return _atoi64(start);
#else
    return strtoll(start, end, base);
#endif
  }

  FUN_ALWAYS_INLINE static uint64 Strtoui64(const char* start, char** end,
                                            int32 base) {
#if FUN_PLATFORM_HTML5_WIN32
    return _atoi64(start);
#else
    return strtoull(start, end, base);
#endif
  }

  FUN_ALWAYS_INLINE static char* Strtok(char* str, const char* delim,
                                        char** context) {
    return strtok(str, delim);
  }
};

typedef StdCString CString;

}  // namespace fun
