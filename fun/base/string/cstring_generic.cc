#include "fun/base/string/cstring_generic.h"

namespace fun {

// Sizes as defined by the ISO C99 standard - fallback
#ifndef LLONG_MAX
#define LLONG_MAX 0x7FFFFFFFFFFFFFFFLL
#endif

#ifndef LLONG_MIN
#define LLONG_MIN (-LLONG_MAX - 1)
#endif

#ifndef ULLONG_MAX
#define ULLONG_MAX 0xFFFFFFFFFFFFFFFFULL
#endif

#define FAILURE_IF_END() \
  if (cur >= end) {      \
    goto failure;        \
  }

namespace {

template <typename Char>
int64 Strtoi64Impl(const Char* str, const Char* end, Char** end_ptr,
                   int32 base) {
  const Char* cur;
  uint64 acc;
  Char ch;
  uint64 cut_off;
  int32 neg, any, cut_lim;

  acc = any = 0;

  cur = str;
  do {
    ch = *cur++;
  } while (cur != end && CharTraits<Char>::IsWhitespace(ch));

  FAILURE_IF_END();

  if (ch == '-') {
    neg = 1;
    ch = *cur++;
    FAILURE_IF_END();
  } else {
    neg = 0;
    if (ch == '+') {
      ch = *cur++;
      FAILURE_IF_END();
    }
  }

  if ((base == 0 || base == 16) && ch == '0' && (*cur == 'x' || *cur == 'X') &&
      ((cur[1] >= '0' && cur[1] <= '9') || (cur[1] >= 'a' && cur[1] <= 'f') ||
       (cur[1] >= 'A' && cur[1] <= 'F'))) {
    ch = cur[1];
    cur += 2;
    base = 16;
  }

  FAILURE_IF_END();

  if (base == 0) {
    base = ch == '0' ? 8 : 10;
  }

  if (base < 2 || base > 36) {
    goto failure;
  }

  cut_off = neg ? (uint64) - (LLONG_MIN + LLONG_MAX) + LLONG_MAX : LLONG_MAX;
  cut_lim = cut_off % base;
  cut_off /= base;

  for (; cur < end + 1; ch = *cur++) {
    if (ch >= '0' && ch <= '9') {
      ch -= '0';
    } else if (ch >= 'A' && ch <= 'Z') {
      ch -= 'A';
    } else if (ch >= 'a' && ch <= 'z') {
      ch -= 'a';
    } else {
      break;
    }

    if (ch >= base) {
      break;
    }

    if (any < 0 || acc > cut_off || (acc == cut_off && ch > cut_lim)) {
      any = -1;
    } else {
      any = 1;
      acc *= base;
      acc += ch;
    }
  }

  if (any < 0) {
    acc = neg ? LLONG_MIN : LLONG_MAX;
    errno = ERANGE;
  } else if (!any) {
  failure:
    errno = EINVAL;
  } else if (neg) {
    acc = (uint64) - (int64)acc;
  }

  if (end_ptr) {
    *end_ptr = const_cast<Char*>(any ? cur - 1 : str);
  }

  return acc;
}

template <typename Char>
uint64 Strtoui64Impl(const Char* str, const Char* end, Char** end_ptr,
                     int32 base) {
  const Char* cur;
  uint64 acc;
  Char ch;
  uint64 cut_off;
  int32 neg, any, cut_lim;

  acc = any = 0;

  cur = str;
  do {
    ch = *cur++;
  } while (CharTraits<Char>::IsWhitespace(ch));

  FAILURE_IF_END();

  if (ch == '-') {
    neg = 1;
    ch = *cur++;
    FAILURE_IF_END();
  } else {
    neg = 0;
    if (ch == '+') {
      ch = *cur++;
      FAILURE_IF_END();
    }
  }

  if ((base == 0 || base == 16) && ch == '0' && (*cur == 'x' || *cur == 'X') &&
      ((cur[1] >= '0' && cur[1] <= '9') || (cur[1] >= 'a' && cur[1] <= 'f') ||
       (cur[1] >= 'A' && cur[1] <= 'F'))) {
    ch = cur[1];
    cur += 2;
    base = 16;
  }

  FAILURE_IF_END();

  if (base == 0) {
    base = ch == '0' ? 8 : 10;
  }

  acc = any = 0;
  if (base < 2 || base > 36) {
    goto failure;
  }

  cut_off = ULLONG_MAX / base;
  cut_lim = ULLONG_MAX % base;
  for (; cur < end + 1; ch = *cur++) {
    if (ch >= '0' && ch <= '9') {
      ch -= '0';
    } else if (ch >= 'A' && ch <= 'Z') {
      ch -= 'A';
    } else if (ch >= 'a' && ch <= 'z') {
      ch -= 'a';
    } else {
      break;
    }

    if (ch >= base) {
      break;
    }

    if (any < 0 || acc > cut_off || (acc == cut_off && ch > cut_lim)) {
      any = -1;
    } else {
      any = 1;
      acc *= base;
      acc += ch;
    }
  }

  if (any < 0) {
    acc = ULLONG_MAX;
    errno = ERANGE;
  } else if (!any) {
  failure:
    errno = EINVAL;
  } else if (neg) {
    acc = (uint64) - (int64)acc;
  }

  if (end_ptr) {
    *end_ptr = const_cast<Char*>(any ? cur - 1 : str);
  }

  return acc;
}

/**

int32 Strtoi32(const Char* str, const Char* end, Char** end_ptr, int32 base,
bool* ok = nullptr) { if (ok) { *ok = true;
  }

  errno = 0;
  Char* end_ptr2 = nullptr;
  const int64 result = Strtoi64Impl<Char>(str, end, &end_ptr2, base);
  if (end_ptr) {
    *end_ptr = end_ptr2;
  }

  if ((result == 0 || result < int32_MIN || result > int32_MAX) && (errno ||
end_ptr2 == str)) { if (ok) { *ok = false;
    }
    return 0;
  }

  return (int32)result;
}

uint32 Strtoui32(const Char* str, const Char* end, Char** end_ptr, int32 base,
bool* ok = nullptr) { if (ok) { *ok = true;
  }

  errno = 0;
  Char* end_ptr2 = nullptr;
  const uint64 result = Strtoui64Impl<Char>(str, end, &end_ptr2, base);
  if (end_ptr) {
    *end_ptr = end_ptr2;
  }

  if ((result == 0 || result > uint32_MAX) && (errno || end_ptr2 == str)) {
    if (ok) {
      *ok = false;
    }
    return 0;
  }

  return (uint32)result;
}


int64 Strtoi64(const Char* str, const Char* end, Char** end_ptr, int32 base,
bool* ok = nullptr) { if (ok) { *ok = true;
  }

  errno = 0;
  Char* end_ptr2 = nullptr;
  const int64 result = Strtoi64Impl(str, end, &end_ptr2, base);
  if (end_ptr) {
    *end_ptr = end_ptr2;
  }

  if ((result == 0 || result == int64_MIN || result == int64_MAX) && (errno ||
end_ptr2 == str)) { if (ok) { *ok = false;
    }
    return 0;
  }

  return result;
}


int64 Strtoui64(const Char* str, const Char* end, Char** end_ptr, int32 base,
bool* ok = nullptr) { if (ok) { *ok = true;
  }

  errno = 0;
  Char* end_ptr2 = nullptr;
  const uint64 result = Strtoui64Impl(str, end, &end_ptr2, base);
  if (end_ptr) {
    *end_ptr = end_ptr2;
  }

  if ((result == 0 || result == uint64_MAX) && (errno || end_ptr2 == str)) {
    if (ok) {
      *ok = false;
    }
    return 0;
  }

  return result;
}


double GenericCString::Strtod(const char* str, const char* end, char** end_ptr,
bool* ok) { int32 processed = 0; bool non_null_ok = false; double result =
AsciiToDouble(str, end - str, non_null_ok, processed); if (end_ptr) { *end_ptr =
const_cast<char*>(str) + processed;
  }
  if (ok) {
    *ok = non_null_ok;
  }
  return result;
}

double GenericCString::Strtod(const UNICHAR* str, const UNICHAR* end, UNICHAR**
end_ptr, bool* ok) { auto ascii = UNICHAR_TO_ASCII_VIEW(str, end - str);

  int32 processed = 0;
  bool non_null_ok = false;
  double result = AsciiToDouble(ascii.ConstData(), ascii.Len(), non_null_ok,
processed); if (end_ptr) { *end_ptr = const_cast<UNICHAR*>(str) + processed;
  }
  if (ok) {
    *ok = non_null_ok;
  }
  return result;
}

*/

}  // namespace

}  // namespace fun
