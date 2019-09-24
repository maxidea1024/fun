#include "fun/base/string/ucstring.h"
#include "fun/base/memory.h"
#include "fun/base/string/char_traits.h"

namespace fun {

size_t ucslen(const UNICHAR* str) {
  const UNICHAR* s = str;
  while (*s++)
    ;
  return s - str;
}

size_t ucsnlen(const UNICHAR* str, size_t n) {
  const UNICHAR* s = str;
  while (*s && n) {
    ++s;
    --n;
  }
  return s - str;
}

UNICHAR* ucsnset(UNICHAR* dst, UNICHAR ch, size_t n) {
  UNICHAR* d = dst;
  while (n--) *d++ = ch;
  return dst;
}

UNICHAR* ucscpy(UNICHAR* __restrict dst, const UNICHAR* __restrict src) {
  UNICHAR* d = dst;
  while ((*d++ = *src++) != '\0')
    ;
  return dst;
}

UNICHAR* ucsncpy(UNICHAR* __restrict dst, const UNICHAR* __restrict src,
                 size_t n) {
  if (n != 0) {
    UNICHAR* d = dst;
    const UNICHAR* s = src;

    do {
      if ((*d++ = *s++) == '\0') {
        // NUL pad the remaining n-1 bytes
        while (--n) {
          *d++ = '\0';
        }
        break;
      }
    } while (--n);
  }

  return dst;
}

UNICHAR* ucscat(UNICHAR* __restrict dst, const UNICHAR* __restrict src) {
  UNICHAR* d = dst;
  while (*d++)
    ;
  while ((*d++ = *src++))
    ;
  return dst;
}

UNICHAR* ucsncat(UNICHAR* __restrict dst, const UNICHAR* __restrict src,
                 size_t n) {
  UNICHAR* d;
  UNICHAR* q;
  const UNICHAR* r;

  d = dst;
  while (*d++)
    ;
  q = d;
  r = src;
  while (n && *r) {
    *q++ = *r++;
    --n;
  }
  *q = '\0';
  return dst;
}

UNICHAR* ucsupr(UNICHAR* dst, size_t n) {
  UNICHAR* d = dst;
  while (*d && n--) {
    *d = CharTraitsU::ToUpper(*d);
    ++d;
  }
  return dst;
}

UNICHAR* ucslwr(UNICHAR* dst, size_t n) {
  UNICHAR* d = dst;
  while (*d && n--) {
    *d = CharTraitsU::ToLower(*d);
    ++d;
  }
  return dst;
}

int32 ucscmp(const UNICHAR* str1, const UNICHAR* str2) {
  while (*str1 == *str2++) {
    if (*str1++ == '\0') {
      return 0;
    }
  }

  --str2;
  return (int32)*str1 - (int32)*str2;
}

int32 ucsncmp(const UNICHAR* str1, const UNICHAR* str2, size_t n) {
  if (n == 0) {
    return 0;
  }

  do {
    if (*str1 != *str2++) {
      --str2;
      return (int32)*str1 - (int32)*str2;
    }

    if (*str1++ == 0) {
      break;
    }
  } while (--n != 0);

  return 0;
}

int32 ucsicmp(const UNICHAR* str1, const UNICHAR* str2) {
  while (CharTraitsU::ToLower(*str1) == CharTraitsU::ToLower(*str2++)) {
    if (*str1++ == '\0') {
      return 0;
    }
  }

  --str2;
  return (int32)CharTraitsU::ToLower(*str1) -
         (int32)CharTraitsU::ToLower(*str2);
}

int32 ucsnicmp(const UNICHAR* str1, const UNICHAR* str2, size_t n) {
  UNICHAR c1, c2;

  if (n == 0) {
    return 0;
  }

  for (; *str1; ++str1, ++str2) {
    c1 = CharTraitsU::ToLower(*str1);
    c2 = CharTraitsU::ToLower(*str2);

    if (c1 != c2) {
      return (int32)c1 - (int32)c2;
    }

    if (--n == 0) {
      return 0;
    }
  }

  return -(int32)*str2;
}

UNICHAR* ucschr(const UNICHAR* str, UNICHAR ch) {
  while (*str != ch && *str != '\0') ++str;
  if (*str == ch) {
    return (UNICHAR*)str;
  }
  return nullptr;
}

UNICHAR* ucsrchr(const UNICHAR* str, UNICHAR ch) {
  const UNICHAR* last;

  last = nullptr;
  for (;;) {
    if (*str == ch) {
      last = str;
    }

    if (*str == '\0') {
      break;
    }

    ++str;
  }

  return (UNICHAR*)last;
}

UNICHAR* ucsichr(const UNICHAR* str, UNICHAR ch) {
  ch = CharTraitsU::ToLower(ch);
  while (CharTraitsU::ToLower(*str) != ch && *str != '\0') {
    ++str;
  }
  if (CharTraitsU::ToLower(*str) == ch) {
    return (UNICHAR*)str;
  }
  return nullptr;
}

// TODO?
// UNICHAR* ucsrichr(const UNICHAR* str, UNICHAR ch)

UNICHAR* ucsstr(const UNICHAR* __restrict str, const UNICHAR* __restrict sub) {
  UNICHAR c, sc;
  size_t len;

  if ((c = *sub++) != '\0') {
    len = ucslen(sub);
    do {
      do {
        if ((sc = *str++) == '\0') {
          return nullptr;
        }
      } while (sc != c);
    } while (ucsncmp(str, sub, len) != 0);

    --str;
  }

  return (UNICHAR*)str;
}

UNICHAR* ucsrstr(const UNICHAR* __restrict str, const UNICHAR* __restrict sub) {
  size_t str_len = ucslen(str);
  size_t sub_len = ucslen(sub);

  if (sub_len > str_len) {
    return nullptr;
  }

  const UNICHAR* s = str + str_len - sub_len;
  do {
    if (ucsncmp(s, sub, sub_len) == 0) {
      return (UNICHAR*)s;
    }
    --s;
  } while (s >= str);

  return nullptr;
}

UNICHAR* ucsistr(const UNICHAR* __restrict str, const UNICHAR* __restrict sub) {
  UNICHAR c, sc;
  size_t len;

  if ((c = CharTraitsU::ToLower(*sub++)) != '\0') {
    len = ucslen(sub);
    do {
      do {
        if ((sc = CharTraitsU::ToLower(*str++)) == '\0') {
          return nullptr;
        }
      } while (sc != c);
    } while (ucsnicmp(str, sub, len) != 0);
    --str;
  }

  return (UNICHAR*)str;
}

UNICHAR* ucsristr(const UNICHAR* __restrict str,
                  const UNICHAR* __restrict sub) {
  const size_t str_len = ucslen(str);
  const size_t sub_len = ucslen(sub);

  if (sub_len > str_len) {
    return nullptr;
  }

  const UNICHAR* s = str + str_len - sub_len;
  do {
    if (ucsnicmp(s, sub, sub_len) == 0) {
      return (UNICHAR*)s;
    }
    --s;
  } while (s >= str);

  return nullptr;
}

UNICHAR* ucstok(UNICHAR* __restrict str, const UNICHAR* __restrict delim,
                UNICHAR** __restrict last) {
  const UNICHAR* spanp;
  UNICHAR* tok;
  UNICHAR c;
  UNICHAR sc;

  if (str == nullptr && (str = *last) == nullptr) {
    return nullptr;
  }

  // Skip (span) leading delimiters (str += ucsspn(str, delim), sort of).
Cont:
  c = *str++;
  for (spanp = delim; (sc = *spanp++) != '\0';) {
    if (c == sc) {
      goto Cont;
    }
  }

  if (c == '\0') {  // no non-delimiter characters
    *last = nullptr;
    return nullptr;
  }
  tok = str - 1;

  // Scan token (scan for delimiters: str += ucscspn(str, delim), sort of).
  // Note that delim must have one NUL; we stop if we see that, too.
  for (;;) {
    c = *str++;
    spanp = delim;
    do {
      if ((sc = *spanp++) == c) {
        if (c == '\0') {
          str = nullptr;
        } else {
          str[-1] = '\0';
        }
        *last = str;
        return tok;
      }
    } while (sc != '\0');
  }
  // NOTREACHED
}

UNICHAR* ucsitok(UNICHAR* __restrict str, const UNICHAR* __restrict delim,
                 UNICHAR** __restrict last) {
  const UNICHAR* spanp;
  UNICHAR* tok;
  UNICHAR c;
  UNICHAR sc;

  if (str == nullptr && (str = *last) == nullptr) {
    return nullptr;
  }

  // Skip (span) leading delimiters (str += ucsspn(str, delim), sort of).
Cont:
  c = CharTraitsU::ToLower(*str++);
  for (spanp = delim; (sc = CharTraitsU::ToLower(*spanp++)) != '\0';) {
    if (c == sc) {
      goto Cont;
    }
  }

  if (c == '\0') {  // no non-delimiter characters
    *last = nullptr;
    return nullptr;
  }
  tok = str - 1;

  // Scan token (scan for delimiters: str += ucscspn(str, delim), sort of).
  // Note that delim must have one NUL; we stop if we see that, too.
  for (;;) {
    c = CharTraitsU::ToLower(*str++);
    spanp = delim;
    do {
      if ((sc = CharTraitsU::ToLower(*spanp++)) == c) {
        if (c == '\0') {
          str = nullptr;
        } else {
          str[-1] = '\0';
        }
        *last = str;
        return tok;
      }
    } while (sc != '\0');
  }
  // NOTREACHED
}

size_t ucsspn(const UNICHAR* str, const UNICHAR* set) {
  const UNICHAR* s;
  const UNICHAR* q;

  s = str;
  while (*s) {
    q = set;

    while (*q) {
      if (*s == *q) {
        break;
      }

      ++q;
    }

    if (!*q) {
      goto Done;
    }

    ++s;
  }

Done:
  return (s - str);
}

UNICHAR* ucspbrk(const UNICHAR* str, const UNICHAR* set) {
  const UNICHAR* s;
  const UNICHAR* q;

  s = str;
  while (*s) {
    q = set;
    while (*q) {
      if (*s == *q) {
        return (UNICHAR*)s;
      }
      ++q;
    }
    ++s;
  }

  return nullptr;
}

size_t ucscspn(const UNICHAR* str, const UNICHAR* set) {
  const UNICHAR* s;
  const UNICHAR* q;

  s = str;
  while (*s) {
    q = set;
    while (*q) {
      if (*s == *q) {
        goto Done;
      }
      ++q;
    }
    ++s;
  }

Done:
  return (s - str);
}

UNICHAR* ucsdup(const UNICHAR* str) {
  const size_t len = ucslen(str) + 1;
  UNICHAR* copy = (UNICHAR*)UnsafeMemory::Malloc(len * sizeof(UNICHAR));
  if (copy == nullptr) {
    return nullptr;
  }
  umemcpy(copy, str, len);
  return copy;
}

UNICHAR* umemset(UNICHAR* dst, UNICHAR ch, size_t n) {
  UNICHAR* d = dst;
  while (n--) {
    *d++ = ch;
  }
  return dst;
}

UNICHAR* umemmove(UNICHAR* dst, const UNICHAR* src, size_t n) {
  return (UNICHAR*)UnsafeMemory::Memmove(dst, src, n * sizeof(UNICHAR));
}

UNICHAR* umemcpy(UNICHAR* __restrict dst, const UNICHAR* __restrict src,
                 size_t n) {
  return (UNICHAR*)UnsafeMemory::Memcpy(dst, src, n * sizeof(UNICHAR));
}

int32 umemcmp(const UNICHAR* str1, const UNICHAR* str2, size_t n) {
  while (n--) {
    if (*str1 != *str2) {
      return *str1 > *str2 ? 1 : -1;
    }
    ++str1;
    ++str2;
  }
  return 0;
}

UNICHAR* umemchr(const UNICHAR* str, UNICHAR ch, size_t n) {
  while (n--) {
    if (*str++ == ch) {
      return (UNICHAR*)(str - 1);
    }
  }
  return nullptr;
}

}  // namespace fun
