//Reference source
//https://github.com/mozilla/positron/blob/master/xpcom/glue/nsTextFormatter.cpp

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h> // malloc / free

//#include "prdtoa.h"
#include "v8/double-conversion.h"


//workaround
struct UnsafeMemory {
  static void* Malloc(size_t size) {
    return ::malloc(size);
  }

  static void Free(void* p) {
    ::free(p);
  }
};

#include <assert.h>

#define fun_check(x)      assert(x)
#define fun_unexpected()  assert(0)

static int32_t strlen16(const char16_t* s)
{
  const char16_t* p = s;
  while (*p) ++p;
  return p - s;
}

///std:c++17 옵션이 필요한 관계로 일단은 disable
//#define FUN_FALLTHROUGH  [[fallthrough]]
#define FUN_FALLTHROUGH


#ifdef _MSC_VER
#define VA_ASSIGN(destination, source)  va_copy(destination, source)
#elif defined(HAVE_VA_COPY)
#define VA_ASSIGN(destination, source)  VA_COPY(destination, source)
#elif defined(HAVE_VA_LIST_AS_ARRAY)
#define VA_ASSIGN(destination, source)  (destination[0] = source[0])
#else
#define VA_ASSIGN(destination, source)  ((destination) = (source))
#endif


typedef struct SprintfStateStr SprintfState;

struct SprintfStateStr {
  int32_t (*stuff)(SprintfState* state, const char16_t* str, uint32_t len);

  char16_t* base;
  char16_t* cur;
  uint32_t max_len;

  void* stuff_closure;
};

struct NumArgState {
  int32_t type;
  va_list ap;

  enum Type {
    INT16,
    UINT16,
    INTN,
    UINTN,
    INT32,
    UINT32,
    INT64,
    UINT64,
    STRING,
    DOUBLE,
    INTSTR,
    UNISTRING,
    UNKNOWN
  };
};

#define NAS_DEFAULT_NUM  20

#define _LEFT    0x01
#define _SIGNED  0x02
#define _SPACED  0x04
#define _ZEROS   0x08
#define _NEG     0x10
#define _HASH    0x20

// Fill into the buffer using the data in src
static int32_t fill2(SprintfState* state, const char16_t* src, int32_t src_len, int32_t width, int32_t flags)
{
  char16_t space = ' ';
  int32_t rv;

  width -= src_len;

  // Right adjusting
  if (width > 0 && !(flags & _LEFT)) {
    if (flags & _ZEROS) {
      space = '0';
    }

    while (--width >= 0) {
      rv = (*state->stuff)(state, &space, 1);
      if (rv < 0) {
        return rv;
      }
    }
  }

  // Copy out the source data
  rv = (*state->stuff)(state, src, src_len);
  if (rv < 0) {
    return rv;
  }

  // Left adjusting
  if (width > 0 && (flags & _LEFT)) {
    while (--width >= 0) {
      rv = (*state->stuff)(state, &space, 1);
      if (rv < 0) {
        return rv;
      }
    }
  }

  return 0;
}

// Fill a number. The order is: optional-sign zero-filling conversion-digits
static int32_t fill_n(SprintfState* state,
                    const char16_t* src,
                    int32_t src_len,
                    int32_t width,
                    int32_t prec,
                    int32_t type,
                    int32_t flags)
{
  int32_t zero_width = 0;
  int32_t prec_width = 0;
  int32_t sign_width = 0;
  int32_t l_spaces = 0;
  int32_t r_spaces = 0;
  int32_t cvt_width;
  int32_t rv;

  char16_t sign;
  char16_t space = ' ';
  char16_t zero = '0';

  if (!(type & 1)) {
    if (flags & _NEG) {
      sign = '-';
      sign_width = 1;
    }
    else if (flags & _SIGNED) {
      sign = '+';
      sign_width = 1;
    }
    else if (flags & _SPACED) {
      sign = ' ';
      sign_width = 1;
    }
  }

  cvt_width = sign_width + src_len;

  if (prec > 0) {
    if (prec > src_len) {
      // Need zero filling.
      prec_width = prec - src_len;
      cvt_width += prec_width;
    }
  }

  if ((flags& _ZEROS) && prec < 0) {
    if (width > cvt_width) {
      // Zero filling.
      zero_width = width - cvt_width;
      cvt_width += zero_width;
    }
  }

  if (flags & _LEFT) {
    if (width > cvt_width) {
      // Space filling on the right (i.e. left adjusting)
      r_spaces = width - cvt_width;
    }
  }
  else {
    if (width > cvt_width) {
      // Space filling on the left (i.e. right adjusting)
      l_spaces = width - cvt_width;
    }
  }

  // Left spaces
  while (--l_spaces >= 0) {
    rv = (*state->stuff)(state, &space, 1);
    if (rv < 0) {
      return rv;
    }
  }

  // Sign
  if (sign_width > 0) {
    rv = (*state->stuff)(state, &sign, 1);
    if (rv < 0) {
      return rv;
    }
  }

  // Precision
  while (--prec_width >= 0) {
    rv = (*state->stuff)(state, &space, 1);
    if (rv < 0) {
      return rv;
    }
  }

  // Zeroes
  while (--zero_width >= 0) {
    rv = (*state->stuff)(state, &zero, 1);
    if (rv < 0) {
      return rv;
    }
  }

  // Main
  rv = (*state->stuff)(state, src, src_len);
  if (rv < 0) {
    return rv;
  }

  // Right spaces
  while (--r_spaces >= 0) {
    rv = (*state->stuff)(state, &space, 1);
    if (rv < 0) {
      return rv;
    }
  }

  return 0;
}

// Convert a long its printable form.
static int32_t cvt_l( SprintfState* state,
                      long num,
                      int32_t width,
                      int32_t prec,
                      int32_t radix,
                      int32_t type,
                      int32_t flags,
                      const char16_t* hex_str)
{
  const int32_t BUFFER_SIZE = 100;
  char16_t cvt_buf[BUFFER_SIZE];
  char16_t* cvt;
  int32_t digits;

  if (prec == 0 && num == 0) {
    return 0;
  }

  cvt = &cvt_buf[0] + BUFFER_SIZE;
  digits = 0;
  while (num) {
    int digit = (static_cast<unsigned long>(num) % radix) & 0xF;
    *--cvt = hex_str[digit];
    digits++;
    num = static_cast<long>((((unsigned long)num) / radix));
  }

  if (digits == 0) {
    *--cvt = '0';
    digits++;
  }

  return fill_n(state, cvt, digits, width, prec, type, flags);
}

static int32_t cvt_ll(SprintfState* state,
                      int64_t num,
                      int32_t width,
                      int32_t prec,
                      int32_t radix,
                      int32_t type,
                      int32_t flags,
                      const char16_t* hex_str)
{
  const int32_t BUFFER_SIZE = 100;
  char16_t cvt_buf[BUFFER_SIZE];
  char16_t* cvt;
  int32_t digits;
  int64_t rad;

  if (prec == 0 && num == 0) {
    return 0;
  }

  rad = radix;
  cvt = &cvt_buf[0] + BUFFER_SIZE;
  digits = 0;
  while (num) {
    *--cvt = hex_str[static_cast<int32_t>(num % rad) & 0xF];
    digits++;
    num /= rad;
  }

  if (digits == 0) {
    *--cvt = '0';
    digits++;
  }

  return fill_n(state, cvt, digits, width, prec, type, flags);
}

static int32_t cvt_f( SprintfState* state,
                      double dbl,
                      int32_t width,
                      int32_t prec,
                      const char16_t type,
                      int32_t flags)
{
  int32_t mode = 2;
  int32_t decpt;
  int32_t sign;
  char buf[256];
  char* bufp = buf;
  int32_t bufsz = 256;
  char num[256];
  char* nump;
  char* endnum;
  int32_t numdigits = 0;
  char exp = 'e';

  if (prec == -1) {
    prec = 6;
  }
  else if (prec > 50) {
    // limit precision to avoid PR_dtoa bug 108335
    // and to prevent buffers overflows.
    prec = 50;
  }

  switch (type) {
    case 'f':
      numdigits = prec;
      mode = 3;
      break;
    case 'E':
      exp = 'E';
      FUN_FALLTHROUGH
    case 'e':
      numdigits = prec + 1;
      mode = 2;
      break;
    case 'G':
      exp = 'E';
      FUN_FALLTHROUGH
    case 'g':
      if (prec == 0) {
        prec = 1;
      }
      numdigits = prec;
      mode = 2;
      break;
    default:
      // invalid type passed to cvt_f
      fun_unexpected();
  }

  //double_conversion::DoubleToStringConverter::DoubleToAscii(
  //  dbl,
  //  mode,
  //  numdigits,
  //  num,
  //  bufsiz,
  //  &sign,
  //  &buflen,
  //  *decpt);

  //NaN, Infinity가 오는 경우가 있을수 있음.

  //TODO
  //if (PR_dtoa(dbl, mode, numdigits, &decpt, &sign, &endnum, num, bufsiz) == PR_FAILURE) {
  //  buf[0] = '\0';
  //  return -1;
  //}
  //TODO 아래코드는 임시임...
  mode = 0;
  numdigits = 0;
  decpt = 0;
  sign = '+';
  endnum = nullptr;


  numdigits = endnum - num;
  nump = num;

  if (sign) {
    *bufp++ = '-';
  }
  else if (flags & _SIGNED) {
    *bufp++ = '+';
  }

  if (decpt == 9999) {
    while ((*bufp++ = *nump++)) ;
  }
  else {
    switch (type) {
      case 'E':
      case 'e':
        *bufp++ = *nump++;
        if (prec > 0) {
          *bufp++ = '.';
          while (*nump) {
            *bufp++ = *nump++;
            prec--;
          }
          while (prec-- > 0) {
            *bufp++ = '0';
          }
        }
        *bufp++ = exp;

        snprintf(bufp, bufsz - (bufp - buf), "%+03d", decpt - 1);
        break;

      case 'f':
        if (decpt < 1) {
          *bufp++ = '0';
          if (prec > 0) {
            *bufp++ = '.';
            while (decpt++ && prec-- < 0) {
              *bufp++ = '0';
            }
            while (*nump && prec-- > 0) {
              *bufp++ = *nump++;
            }
            while (prec-- > 0) {
              *bufp++ = '0';
            }
          }
        }
        else {
          while (*nump && decpt-- > 0) {
            *bufp++ = *nump++;
          }
          while (decpt-- > 0) {
            *bufp++ = '0';
          }
          if (prec > 0) {
            *bufp++ = '.';
            while (*nump && prec-- > 0) {
              *bufp++ = *nump++;
            }
            while (prec-- > 0) {
              *bufp++ = '0';
            }
          }
        }
        *bufp = '\0';
        break;

      case 'G':
      case 'g':
        if (decpt < -3 || (decpt - 1) >= prec) {
          *bufp++ = *nump++;
          numdigits--;
          if (numdigits > 0) {
            *bufp++ = '.';
            while (*nump) {
              *bufp++ = *nump++;
            }
          }
          *bufp++ = exp;
          snprintf(bufp, bufsz - (bufp - buf), "%+03d", decpt - 1);
        }
        else {
          if (decpt < 1) {
            *bufp++ = '0';
            if (prec > 0) {
              *bufp++ = '.';
              while (decpt++) {
                *bufp++ = '0';
              }
              while (*nump) {
                *bufp++ = *nump++;
              }
            }
          }
          else {
            while (*nump && decpt-- > 0) {
              *bufp++ = *nump++;
              numdigits--;
            }
            while (decpt-- > 0) {
              *bufp++ = '0';
            }
            if (numdigits > 0) {
              *bufp++ = '.';
              while (*nump) {
                *bufp++ = *nump++;
              }
            }
          }
          *bufp = '\0';
        }
        break;
    }
  }

  char16_t rbuf[256];
  char16_t* rbufp = rbuf;
  bufp = buf;

  // cast to char16_t
  while ((*rbufp++ = *bufp++)) ;
  *rbufp = '\0';

  return fill2(state, rbuf, rbufp - rbuf, width, flags);
}

static int32_t cvt_S( SprintfState* state,
                      const char16_t* str,
                      int32_t width,
                      int32_t prec,
                      int32_t flags)
{
  int32_t str_len;

  if (prec == 0) {
    return 0;
  }

  str_len = str ? strlen16(str) : 6; //6=strlen16("(null)")
  if (prec > 0) {
    if (prec < str_len) {
      str_len = prec;
    }
  }

  //return fill2(state, str ? str : UTEXT("(null)"), str_len, width, flags);
  return fill2(state, str ? str : u"(null)", str_len, width, flags);
}

static int32_t cvt_s( SprintfState* state,
                      const char* str,
                      int32_t width,
                      int32_t prec,
                      int32_t flags)
{
  char16_t* utf16 = (char16_t*)UnsafeMemory::Malloc(sizeof(char16_t) * (strlen(str) + 1));
  char16_t* d = utf16;
  while (*str) {
    *d++ = (char16_t)*str++;
  }
  *d = '\0';

  int32_t rv = cvt_S(state, utf16, width, prec, flags);
  UnsafeMemory::Free(utf16);
  return rv;

  //TODO
  //UTF8_TO_UTF16 utf16(str);
  //return cvt_S(state, utf16.ConstData(), width, prec, flags);
}

static struct NumArgState*
BuildArgArray(const char16_t* fmt,
              va_list args,
              int32_t* out_rv,
              struct NumArgState* nas_array)
{
  int32_t number = 0, cn = 0, i;
  const char16_t* p;
  char16_t c;
  struct NumArgState* nas;

  p = fmt;
  *out_rv = 0;
  i = 0;
  while ((c = *p++) != 0) {
    if (c != '%') {
      continue;
    }

    // skip %% case
    if ((c = *p++) == '%') {
      continue;
    }

    while (c != 0) {
      if (c > '9' || c < '0') {
        // numered argument case
        if (c == '$') {
          if (i > 0) {
            *out_rv = -1;
            return nullptr;
          }
          number++;
          break;
        }
        else {
          // non numered argument case
          if (number > 0) {
            *out_rv = -1;
            return nullptr;
          }
          i = 1;
          break;
        }
      }
      c = *p++;
    }
  }

  if (number == 0) {
    return nullptr;
  }

  if (number > NAS_DEFAULT_NUM) {
    nas = (struct NumArgState*)UnsafeMemory::Malloc(number * sizeof(struct NumArgState));
    if (!nas) {
      *out_rv = -1;
      return nullptr;
    }
  }
  else {
    nas = nas_array;
  }

  for (i = 0; i < number; ++i) {
    nas[i].type = NumArgState::UNKNOWN;
  }

  // Second pass
  p = fmt;
  while ((c = *p++) != 0) {
    if (c != '%') {
      continue;
    }

    c = *p++;
    if (c == '%') {
      continue;
    }

    cn = 0;
    // should improve error check later.
    while (c && c != '$') {
      cn = cn * 10 + (c - '0');
      c = *p++;
    }

    if (!c || cn < 1 || cn > number) {
      *out_rv = -1;
      break;
    }

    cn--;
    if (nas[cn].type != NumArgState::UNKNOWN) {
      continue;
    }

    c = *p++;

    // Width
    if (c == '*') {
      // not supported feature, for the argument is not numbered
      *out_rv = -1;
      break;
    }
    else {
      while (c >= '0' && c <= '9') {
        c = *p++;
      }
    }

    // Precision
    if (c == '.') {
      c = *p++;
      if (c == '*') {
        // not supported feature, for the argument is not numbered
        *out_rv = -1;
        break;
      }
      else {
        while (c >= '0' && c <= '9') {
          c = *p++;
        }
      }
    }

    // Size
    nas[cn].type = NumArgState::INTN;
    if (c == 'h') {
      nas[cn].type = NumArgState::INT16;
      c = *p++;
    }
    else if (c == 'L') {
      // XXX not quiet sure here
      nas[cn].type = NumArgState::INT64;
      c = *p++;
    }
    else if (c == 'l') {
      nas[cn].type = NumArgState::INT32;
      c = *p++;
      if (c == 'l') {
        nas[cn].type = NumArgState::INT64;
        c = *p++;
      }
    }

    // Format
    switch (c) {
      case 'd':
      case 'c':
      case 'i':
      case 'o':
      case 'u':
      case 'x':
      case 'X':
        break;

      case 'e':
      case 'f':
      case 'g':
        nas[cn].type = NumArgState::DOUBLE;
        break;

      case 'p':
        // XXX should use cpp
        if (sizeof(void*) == sizeof(int32_t)) {
          nas[cn].type = NumArgState::UINT32;
        }
        else if (sizeof(void*) == sizeof(int64_t)) {
          nas[cn].type = NumArgState::UINT64;
        }
        else if (sizeof(void*) == sizeof(int)) {
          nas[cn].type = NumArgState::UINTN;
        }
        else {
          nas[cn].type = NumArgState::UNKNOWN;
        }
        break;

      case 'C':
        // XXX not supported I suppose
        fun_unexpected();
        nas[cn].type = NumArgState::UNKNOWN;
        break;

      case 'S':
        nas[cn].type = NumArgState::UNISTRING;
        break;

      case 's':
        nas[cn].type = NumArgState::STRING;
        break;

      case 'n':
        nas[cn].type = NumArgState::INTSTR;
        break;

      default:
        fun_unexpected();
        nas[cn].type = NumArgState::UNKNOWN;
        break;
    }

    // Get a legal para.
    if (nas[cn].type == NumArgState::UNKNOWN) {
      *out_rv = -1;
      break;
    }
  }

  // Third pass
  if (*out_rv < 0) {
    if (nas != nas_array) {
      UnsafeMemory::Free(nas);
    }
    return nullptr;
  }

  cn = 0;
  while (cn < number) {
    if (nas[cn].type == NumArgState::UNKNOWN) {
      cn++;
      continue;
    }

    VA_ASSIGN(nas[cn].ap, args);

    switch (nas[cn].type) {
      case NumArgState::INT16:
      case NumArgState::UINT16:
      case NumArgState::INTN:
      case NumArgState::UINTN:
        (void)va_arg(args, int);
        break;
      case NumArgState::INT32:
        (void)va_arg(args, int32_t);
        break;
      case NumArgState::UINT32:
        (void)va_arg(args, uint32_t);
        break;
      case NumArgState::INT64:
        (void)va_arg(args, int64_t);
        break;
      case NumArgState::UINT64:
        (void)va_arg(args, uint64_t);
        break;
      case NumArgState::STRING:
        (void)va_arg(args, char*);
        break;
      case NumArgState::INTSTR:
        (void)va_arg(args, int*);
        break;
      case NumArgState::DOUBLE:
        (void)va_arg(args, double);
        break;
      case NumArgState::UNISTRING:
        (void)va_arg(args, char16_t*);
        break;

      default:
        if (nas != nas_array) {
          UnsafeMemory::Free(nas);
        }
        *out_rv = -1;
        va_end(args);
        return nullptr;
    }
    cn++;
  }
  va_end(args);
  return nas;
}

static int32_t do_sprintf(SprintfState* state, const char16_t* fmt, va_list args)
{
  char16_t c;
  int32_t flags, width, prec, radix, type;

  union {
    char16_t ch;
    int32_t i;
    long l;
    int64_t ll;
    double d;
    const char* s;
    const char16_t* S;
    int32_t* ip;
  } u;

  char16_t space = ' ';

  //const char16_t* hex = UTEXT("0123456789abcdef");
  //const char16_t* HEX = UTEXT("0123456789ABCDEF");
  const char16_t* hex = u"0123456789abcdef";
  const char16_t* HEX = u"0123456789ABCDEF";

  const char16_t* hexp;
  int32_t rv, i;
  struct NumArgState* nas = nullptr;
  struct NumArgState nas_array[NAS_DEFAULT_NUM];

  nas = BuildArgArray(fmt, args, &rv, nas_array);
  if (rv < 0) {
    //fun_unexpected();
    return rv;
  }

  while ((c = *fmt++) != 0) {
    if (c != '%') {
      rv = (*state->stuff)(state, fmt - 1, 1);
      if (rv < 0) {
        va_end(args);
        if (nas != nas_array) {
          UnsafeMemory::Free(nas);
          return rv;
        }
      }
      continue;
    }

    flags = 0;
    c = *fmt++;
    if (c == '%') {
      rv = (*state->stuff)(state, fmt - 1, 1);
      if (rv < 0) {
        va_end(args);
        if (nas != nas_array) {
          UnsafeMemory::Free(nas);
          return rv;
        }
      }
      continue;
    }

    if (nas) {
      i = 0;
      while (c && c != '$') {
        i = (i * 10) + (c - '0');
        c = *fmt++;
      }

      if (nas[i-1].type == NumArgState::UNKNOWN) {
        if (nas != nas_array) {
          UnsafeMemory::Free(nas);
        }
        va_end(args);
        return -1;
      }

      VA_ASSIGN(args, nas[i-1].ap);
      c = *fmt++;
    }

    //TODO '#'

    while (c == '-' || c == '+' || c == ' ' || c == '0' || c == '#') {
      if (c == '-') {
        flags |= _LEFT;
      }
      else if (c == '+') {
        flags |= _SIGNED;
      }
      else if (c == ' ') {
        flags |= _SPACED;
      }
      else if (c == '0') {
        flags |= _ZEROS;
      }
      else if (c == '#') {
        flags |= _HASH;
      }
      c = *fmt++;
    }

    if (flags & _SIGNED) {
      flags &= ~_SPACED;
    }

    if (flags & _LEFT) {
      flags &= ~_ZEROS;
    }

    // Width
    if (c == '*') {
      c = *fmt++;
      width = va_arg(args, int32_t);
    }
    else {
      width = 0;
      while (c >= '0' && c <= '9') {
        width = (width * 10) + (c - '0');
        c = *fmt++;
      }
    }

    // Precision
    prec = -1;
    if (c == '.') {
      c = *fmt++;
      if (c == '*') {
        c = *fmt++;
        prec = va_arg(args, int32_t);
      }
      else {
        prec = 0;
        while (c >= '0' && c <= '9') {
          prec = (prec * 10) + (c - '0');
          c = *fmt++;
        }
      }
    }

    // Size
    type = NumArgState::INTN;
    if (c == 'h') {
      type = NumArgState::INT16;
      c = *fmt++;
    }
    else if (c == 'L') {
      // XXX not quiet sure here
      type = NumArgState::INT64;
      c = *fmt++;
    }
    else if (c == 'l') {
      type = NumArgState::INT32;
      c = *fmt++;
      if (c == 'l') {
        type = NumArgState::INT64;
        c = *fmt++;
      }
    }

    // Format
    hexp = hex;
    switch (c) {
      // decimal / integer
      case 'd':
      case 'i':
        radix = 10;
        goto fetch_and_convert;

      // octal
      case 'o':
        radix = 8;
        type |= 1;
        goto fetch_and_convert;

      // unsigned decimal
      case 'u':
        radix = 10;
        type |= 1;
        goto fetch_and_convert;

      // unsigned hex (lower cased)
      case 'x':
        radix = 16;
        type |= 1;
        goto fetch_and_convert;

      // unsigned HEX (upper cased)
      case 'X':
        hexp = HEX;
        radix = 16;
        type |= 1;
        goto fetch_and_convert;

       fetch_and_convert:
        switch (type) {
          case NumArgState::INT16:
            u.l = va_arg(args, int);
            if (u.l < 0) {
              u.l = -u.l;
              flags |= _NEG;
            }
            goto do_long;
          case NumArgState::UINT16:
            u.l = va_arg(args, int) & 0xffff;
            goto do_long;
          case NumArgState::INTN:
            u.l = va_arg(args, int);
            if (u.l < 0) {
              u.l = -u.l;
              flags |= _NEG;
            }
            goto do_long;
          case NumArgState::UINTN:
            u.l = (long)va_arg(args, unsigned int);
            goto do_long;
          case NumArgState::INT32:
            u.l = va_arg(args, int32_t);
            if (u.l < 0) {
              u.l = -u.l;
              flags |= _NEG;
            }
            goto do_long;
          case NumArgState::UINT32:
            u.l = (long)va_arg(args, uint32_t);
          do_long:
            rv = cvt_l(state, u.l, width, prec, radix, type, flags, hexp);
            if (rv < 0) {
              va_end(args);
              if (nas != nas_array) {
                UnsafeMemory::Free(nas);
              }
              return rv;
            }
            break;

          case NumArgState::INT64:
            u.ll = va_arg(args, int64_t);
            if (u.ll < 0) {
              u.ll = -u.ll;
              flags |= _NEG;
            }
            goto do_longlong;
          case NumArgState::UINT64:
            u.ll = va_arg(args, uint64_t);
          do_longlong:
            rv = cvt_ll(state, u.ll, width, prec, radix, type, flags, hexp);
            if (rv < 0) {
              va_end(args);
              if (nas != nas_array) {
                UnsafeMemory::Free(nas);
              }
              return rv;
            }
            break;
        }
        break;

      case 'e':
      case 'E':
      case 'f':
      case 'g':
      case 'G':
        u.d = va_arg(args, double);
        rv = cvt_f(state, u.d, width, prec, c, flags);
        if (rv < 0) {
          va_end(args);
          if (nas != nas_array) {
            UnsafeMemory::Free(nas);
          }
          return rv;
        }
        break;

      case 'c':
        u.ch = va_arg(args, int);
        if (!(flags & _LEFT)) {
          //TODO 루프를 제거하는게 좋을듯??
          while (width-- > 1) {
            rv = (*state->stuff)(state, &space, 1);
            if (rv < 0) {
              va_end(args);
              if (nas != nas_array) {
                UnsafeMemory::Free(nas);
              }
              return rv;
            }
          }
        }

        rv = (*state->stuff)(state, &u.ch, 1);
        if (rv < 0) {
          va_end(args);
          if (nas != nas_array) {
            UnsafeMemory::Free(nas);
          }
          return rv;
        }

        if (flags & _LEFT) {
          //TODO 루프를 제거하는게 좋을듯??
          while (width-- > 1) {
            rv = (*state->stuff)(state, &space, 1);
            if (rv < 0) {
              va_end(args);
              if (nas != nas_array) {
                UnsafeMemory::Free(nas);
              }
              return rv;
            }
          }
        }
        break;

      case 'p':
        if (sizeof(void*) == sizeof(int32_t)) {
          type = NumArgState::UINT32;
        }
        else if (sizeof(void*) == sizeof(int64_t)) {
          type = NumArgState::UINT64;
        }
        else if (sizeof(void*) == sizeof(int)) {
          type = NumArgState::UINTN;
        }
        else {
          fun_unexpected();
          break;
        }
        radix = 16;
        goto fetch_and_convert;

      case 'C':
        // XXX not supported I suppose
        fun_unexpected();
        break;

      case 'S':
        u.S = va_arg(args, const char16_t*);
        rv = cvt_S(state, u.S, width, prec, flags);
        if (rv < 0) {
          va_end(args);
          if (nas != nas_array) {
            UnsafeMemory::Free(nas);
          }
          return rv;
        }
        break;

      case 's':
        u.s = va_arg(args, const char*);
        rv = cvt_s(state, u.s, width, prec, flags);
        if (rv < 0) {
          va_end(args);
          if (nas != nas_array) {
            UnsafeMemory::Free(nas);
          }
          return rv;
        }
        break;

      case 'n':
        u.ip = va_arg(args, int*);
        if (u.ip) {
          *u.ip = state->cur - state->base;
        }
        break;

      default:
        // Not a % token after all... skip it

        char16_t perct = '%';
        rv = (*state->stuff)(state, &perct, 1);
        if (rv < 0) {
          va_end(args);
          if (nas != nas_array) {
            UnsafeMemory::Free(nas);
          }
          return rv;
        }

        rv = (*state->stuff)(state, fmt - 1, 1);
        if (rv < 0) {
          va_end(args);
          if (nas != nas_array) {
            UnsafeMemory::Free(nas);
          }
          return rv;
        }
        break;
    }
  }

  // Stuff trailing NUL
  char16_t nul = '\0';
  rv = (*state->stuff)(state, &nul, 1);

  va_end(args);
  if (nas != nas_array) {
    UnsafeMemory::Free(nas);
  }

  return rv;
}

// Stuff routine that discards overflow data.
static int32_t limit_stuff(SprintfState* state, const char16_t* str, uint32_t len)
{
  uint32_t limit = state->max_len - (state->cur - state->base);

  if (len > limit) {
    len = limit;
  }

  while (len) {
    len--;
    *state->cur++ = *str++;
  }

  return 0;
}

int32_t vsnprintf16(char16_t* out, uint32_t out_len, const char16_t* fmt, va_list args)
{
  SprintfState ss;
  int32_t n;

  fun_check((int32_t)out_len > 0);
  if ((int32_t)out_len <= 0) {
    return 0;
  }

  ss.stuff = limit_stuff;
  ss.base = out;
  ss.cur = out;
  ss.max_len = out_len;
  do_sprintf(&ss, fmt, args);

  // If we added chars, and we didn't append a null, do it now.
  if (ss.cur != ss.base && ss.cur[-1] != '\0') {
    *(--ss.cur) = '\0';
  }

  n = ss.cur - ss.base;
  return n ? n - 1 : n;
}

int32_t snprintf16(char16_t* out, uint32_t out_len, const char16_t* fmt, ...)
{
  va_list args;
  int32_t rv;

  fun_check((int32_t)out_len > 0);
  if ((int32_t)out_len <= 0) {
    return 0;
  }

  va_start(args, fmt);
  rv = vsnprintf16(out, out_len, fmt, args);
  va_end(args);
  return rv;
}
