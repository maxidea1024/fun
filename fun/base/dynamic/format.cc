#include "fun/base/format.h"
#include "fun/base/exception.h"
#include "fun/base/ascii.h"

#include <sstream>
#if !defined(FUN_NO_LOCALE)
#include <locale>
#endif
#include <cstddef>

namespace fun {

namespace {

void ParseFlags(std::ostream& str, String::const_iterator& itFmt, const String::const_iterator& endFmt) {
  bool is_flag = true;
  while (is_flag && itFmt != endFmt) {
    switch (*itFmt) {
      case '-': str.setf(std::ios::left); ++itFmt; break;
      case '+': str.setf(std::ios::showpos); ++itFmt; break;
      case '0': str.fill('0'); str.setf(std::ios::internal); ++itFmt; break;
      case '#': str.setf(std::ios::showpoint | std::ios::showbase); ++itFmt; break;
      default:  is_flag = false; break;
    }
  }
}

void ParseWidth(std::ostream& str, String::const_iterator& itFmt, const String::const_iterator& endFmt, std::vector<Any>::const_iterator& itVal) {
  int width = 0;
  if (itFmt != endFmt && *itFmt == '*') {
    ++itFmt;
    width = AnyCast<int>(*itVal++);
  } else {
    while (itFmt != endFmt && Ascii::IsDigit(*itFmt)) {
      width = 10*width + *itFmt - '0';
      ++itFmt;
    }
  }

  if (width > 0) {
    str.width(width);
  }
}

void ParsePrecision(std::ostream& str, String::const_iterator& itFmt, const String::const_iterator& endFmt, std::vector<Any>::const_iterator& itVal) {
  if (itFmt != endFmt && *itFmt == '.') {
    ++itFmt;
    int prec = 0;
    if (itFmt != endFmt && *itFmt == '*') {
      ++itFmt;
      prec = AnyCast<int>(*itVal++);
    } else {
      while (itFmt != endFmt && Ascii::IsDigit(*itFmt)) {
        prec = 10*prec + *itFmt - '0';
        ++itFmt;
      }
    }
    if (prec >= 0) {
      str.precision(prec);
    }
  }
}

char ParseModifier(String::const_iterator& it, const String::const_iterator& end) {
  char mod = 0;
  if (it != end) {
    switch (*it) {
      case 'l':
      case 'h':
      case 'L':
      case '?': mod = *it++; break;
    }
  }
  return mod;
}

size_t ParseIndex(String::const_iterator& it, const String::const_iterator& end) {
  int index = 0;
  while (it != end && Ascii::IsDigit(*it)) {
    index = 10*index + *it - '0';
    ++it;
  }

  if (it != end && *it == ']') {
    ++it;
  }

  return index;
}

void PrepareFormat(std::ostream& str, char type) {
  switch (type) {
    case 'd':
    case 'i': str << std::dec; break;
    case 'o': str << std::oct; break;
    case 'x': str << std::hex; break;
    case 'X': str << std::hex << std::uppercase; break;
    case 'e': str << std::scientific; break;
    case 'E': str << std::scientific << std::uppercase; break;
    case 'f': str << std::fixed; break;
  }
}

void WriteAnyInt(std::ostream& str, const Any& any) {
  if (any.Type() == typeid(char)) {
    str << static_cast<int>(AnyCast<char>(any));
  } else if (any.Type() == typeid(signed char)) {
    str << static_cast<int>(AnyCast<signed char>(any));
  } else if (any.Type() == typeid(unsigned char)) {
    str << static_cast<unsigned>(AnyCast<unsigned char>(any));
  } else if (any.Type() == typeid(short)) {
    str << AnyCast<short>(any);
  } else if (any.Type() == typeid(unsigned short)) {
    str << AnyCast<unsigned short>(any);
  } else if (any.Type() == typeid(int)) {
    str << AnyCast<int>(any);
  } else if (any.Type() == typeid(unsigned int)) {
    str << AnyCast<unsigned int>(any);
  } else if (any.Type() == typeid(long)) {
    str << AnyCast<long>(any);
  } else if (any.Type() == typeid(unsigned long)) {
    str << AnyCast<unsigned long>(any);
  } else if (any.Type() == typeid(int64)) {
    str << AnyCast<int64>(any);
  } else if (any.Type() == typeid(uint64)) {
    str << AnyCast<uint64>(any);
  } else if (any.Type() == typeid(bool)) {
    str << AnyCast<bool>(any);
  }
}

void FormatOne(String& result, String::const_iterator& itFmt, const String::const_iterator& endFmt, std::vector<Any>::const_iterator& itVal) {
  std::ostringstream str;
#if !defined(FUN_NO_LOCALE)
  str.imbue(std::locale::classic());
#endif
  try {
    ParseFlags(str, itFmt, endFmt);
    ParseWidth(str, itFmt, endFmt, itVal);
    ParsePrecision(str, itFmt, endFmt, itVal);
    char mod = ParseModifier(itFmt, endFmt);
    if (itFmt != endFmt) {
      char type = *itFmt++;
      PrepareFormat(str, type);
      switch (type) {
        case 'b':
          str << AnyCast<bool>(*itVal++);
          break;
        case 'c':
          str << AnyCast<char>(*itVal++);
          break;
        case 'd':
        case 'i':
          switch (mod) {
          case 'l': str << AnyCast<long>(*itVal++); break;
          case 'L': str << AnyCast<int64>(*itVal++); break;
          case 'h': str << AnyCast<short>(*itVal++); break;
          case '?': WriteAnyInt(str, *itVal++); break;
          default:  str << AnyCast<int>(*itVal++); break;
          }
          break;
        case 'o':
        case 'u':
        case 'x':
        case 'X':
          switch (mod) {
          case 'l': str << AnyCast<unsigned long>(*itVal++); break;
          case 'L': str << AnyCast<uint64>(*itVal++); break;
          case 'h': str << AnyCast<unsigned short>(*itVal++); break;
          case '?': WriteAnyInt(str, *itVal++); break;
          default:  str << AnyCast<unsigned>(*itVal++); break;
          }
          break;
        case 'e':
        case 'E':
        case 'f':
          switch (mod) {
          case 'l': str << AnyCast<long double>(*itVal++); break;
          case 'L': str << AnyCast<long double>(*itVal++); break;
          case 'h': str << AnyCast<float>(*itVal++); break;
          default:  str << AnyCast<double>(*itVal++); break;
          }
          break;
        case 's':
          str << RefAnyCast<String>(*itVal++);
          break;
        case 'z':
          str << AnyCast<size_t>(*itVal++);
          break;
        case 'I':
        case 'D':
        default:
          str << type;
      }
    }
  } catch (fun::BadCastException&) {
    str << "[ERRFMT]";
  }
  result.Append(str.str());
}

} // namespace


String Format(const String& fmt, const Any& value) {
  String result;
  Format(result, fmt, value);
  return result;
}

void Format(String& result, const char *fmt, const std::vector<Any>& values) {
  Format(result, String(fmt), values);
}

void Format(String& result, const String& fmt, const std::vector<Any>& values) {
  String::const_iterator itFmt  = fmt.begin();
  String::const_iterator endFmt = fmt.end();
  std::vector<Any>::const_iterator itVal  = values.begin();
  std::vector<Any>::const_iterator endVal = values.end();
  while (itFmt != endFmt) {
    switch (*itFmt) {
      case '%':
        ++itFmt;
        if (itFmt != endFmt && (itVal != endVal || *itFmt == '[')) {
          if (*itFmt == '[') {
            ++itFmt;
            size_t index = ParseIndex(itFmt, endFmt);
            if (index < values.size()) {
              std::vector<Any>::const_iterator it = values.begin() + index;
              FormatOne(result, itFmt, endFmt, it);
            } else {
              throw InvalidArgumentException("format argument index out of range", fmt);
            }
          } else {
            FormatOne(result, itFmt, endFmt, itVal);
          }
        } else if (itFmt != endFmt) {
          result += *itFmt++;
        }
        break;
      default:
        result += *itFmt;
        ++itFmt;
        break;
    }
  }
}

} // namespace fun
