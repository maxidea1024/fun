#pragma once

#include "fun/base/string/cstring_generic.h"

#include <string.h>
#include <tchar.h>

// TODO 자동으로 정의할수 있을텐데...................
#define FUN_USE_SECURE_CRT 1

namespace fun {

class FUN_BASE_API WindowsCString : public GenericCString {
 public:
  //
  // UNICODE
  //

  FUN_ALWAYS_INLINE static UNICHAR* Strcpy(UNICHAR* dst, size_t dst_count,
                                           const UNICHAR* src) {
#if FUN_USE_SECURE_CRT
    _tcscpy_s(dst, dst_count, src);
    return dst;
#else
    return _tcscpy(dst, src);
#endif
  }

  FUN_ALWAYS_INLINE static UNICHAR* Strncpy(UNICHAR* dst, const UNICHAR* src,
                                            int32 max_len) {
#if FUN_USE_SECURE_CRT
    _tcsncpy_s(dst, max_len, src, max_len - 1);
#else
    _tcsncpy(dst, src, max_len - 1);
    dst[max_len - 1] = 0;
#endif
    return dst;
  }

  FUN_ALWAYS_INLINE static UNICHAR* Strcat(UNICHAR* dst, size_t dst_count,
                                           const UNICHAR* src) {
#if FUN_USE_SECURE_CRT
    _tcscat_s(dst, dst_count, src);
    return dst;
#else
    return _tcscat(dst, src);
#endif
  }

  FUN_ALWAYS_INLINE static int32 Strcmp(const UNICHAR* str1,
                                        const UNICHAR* str2) {
    return _tcscmp(str1, str2);
  }

  FUN_ALWAYS_INLINE static int32 Strncmp(const UNICHAR* str1,
                                         const UNICHAR* str2, size_t len) {
    return _tcsncmp(str1, str2, len);
  }

  FUN_ALWAYS_INLINE static int32 Stricmp(const UNICHAR* str1,
                                         const UNICHAR* str2) {
    return _tcsicmp(str1, str2);
  }

  FUN_ALWAYS_INLINE static int32 Strnicmp(const UNICHAR* str1,
                                          const UNICHAR* str2, size_t len) {
    return _tcsnicmp(str1, str2, len);
  }

  FUN_ALWAYS_INLINE static int32 Strlen(const UNICHAR* str) {
    return (int32)_tcslen(str);
  }

  FUN_ALWAYS_INLINE static const UNICHAR* Strstr(const UNICHAR* str,
                                                 const UNICHAR* find) {
    return _tcsstr(str, find);
  }

  FUN_ALWAYS_INLINE static const UNICHAR* Strchr(const UNICHAR* str,
                                                 UNICHAR ch) {
    return _tcschr(str, ch);
  }

  FUN_ALWAYS_INLINE static const UNICHAR* Strrchr(const UNICHAR* str,
                                                  UNICHAR ch) {
    return _tcsrchr(str, ch);
  }

  FUN_ALWAYS_INLINE static int32 Atoi(const UNICHAR* str) {
    return _tstoi(str);
  }

  FUN_ALWAYS_INLINE static int64 Atoi64(const UNICHAR* str) {
    return _tstoi64(str);
  }

  FUN_ALWAYS_INLINE static float Atof(const UNICHAR* str) {
    return (float)_tstof(str);
  }

  FUN_ALWAYS_INLINE static double Atod(const UNICHAR* str) {
    return _tcstod(str, nullptr);
  }

  FUN_ALWAYS_INLINE static int32 Strtoi(const UNICHAR* start, UNICHAR** end,
                                        int32 base) {
    return _tcstoul(start, end, base);
  }

  FUN_ALWAYS_INLINE static int64 Strtoi64(const UNICHAR* start, UNICHAR** end,
                                          int32 base) {
    return _tcstoi64(start, end, base);
  }

  FUN_ALWAYS_INLINE static uint64 Strtoui64(const UNICHAR* start, UNICHAR** end,
                                            int32 base) {
    return _tcstoui64(start, end, base);
  }

  FUN_ALWAYS_INLINE static UNICHAR* Strtok(UNICHAR* str, const UNICHAR* delim,
                                           UNICHAR** context) {
    return _tcstok_s(str, delim, context);
  }

  //
  // char
  //

  FUN_ALWAYS_INLINE static char* Strcpy(char* dst, size_t dst_count,
                                        const char* src) {
#if FUN_USE_SECURE_CRT
    strcpy_s(dst, dst_count, src);
    return dst;
#else
    return (char*)strcpy(dst, src);
#endif
  }

  FUN_ALWAYS_INLINE static void Strncpy(char* dst, const char* src,
                                        size_t max_len) {
#if FUN_USE_SECURE_CRT
    strncpy_s(dst, max_len, src, max_len - 1);
#else
    strncpy(dst, src, max_len);
    dst[max_len - 1] = 0;
#endif
  }

  FUN_ALWAYS_INLINE static char* Strcat(char* dst, size_t dst_count,
                                        const char* src) {
#if FUN_USE_SECURE_CRT
    strcat_s(dst, dst_count, src);
    return dst;
#else
    return (char*)strcat(dst, src);
#endif
  }

  FUN_ALWAYS_INLINE static char* Strupr(char* dst, size_t dst_count) {
#if FUN_USE_SECURE_CRT
    _strupr_s(dst, dst_count);
    return dst;
#else
    return (char*)strupr(dst);
#endif
  }

  FUN_ALWAYS_INLINE static int32 Strcmp(const char* str1, const char* str2) {
    return strcmp(str1, str2);
  }

  FUN_ALWAYS_INLINE static int32 Strncmp(const char* str1, const char* str2,
                                         size_t len) {
    return strncmp(str1, str2, len);
  }

  FUN_ALWAYS_INLINE static int32 Stricmp(const char* str1, const char* str2) {
    return _stricmp(str1, str2);
  }

  FUN_ALWAYS_INLINE static int32 Strnicmp(const char* str1, const char* str2,
                                          size_t len) {
    return _strnicmp(str1, str2, len);
  }

  FUN_ALWAYS_INLINE static int32 Strlen(const char* str) {
    return (int32)strlen(str);
  }

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
    return _strtoi64(str, nullptr, 10);
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
    return _strtoi64(start, end, base);
  }

  FUN_ALWAYS_INLINE static uint64 Strtoui64(const char* start, char** end,
                                            int32 base) {
    return _strtoui64(start, end, base);
  }

  FUN_ALWAYS_INLINE static char* Strtok(char* str, const char* delim,
                                        char** context) {
    return strtok_s(str, delim, context);
  }
};

typedef WindowsCString CString;

}  // namespace fun
