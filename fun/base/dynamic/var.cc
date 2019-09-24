#include "fun/base/dynamic/var.h"
#include "fun/base/dynamic/struct.h"
#include "fun/base/number_Parser.h"

#include <algorithm>
#include <cctype>
#include <vector>
#include <list>
#include <deque>

namespace fun {
namespace dynamic {

Var::Var()
#ifdef FUN_NO_SOO
  : holder_(nullptr)
#endif
{
}

Var::Var(const char* pVal)
#ifdef FUN_NO_SOO
  : holder_(new VarHolderImpl<String>(pVal)) {
}
#else
{
  Construct(String(pVal));
}
#endif

Var::Var(const Var& other)
#ifdef FUN_NO_SOO
  : holder_(other.holder_ ? other.holder_->Clone() : nullptr) {
}
#else
{
  if ((this != &other) && !other.IsEmpty()) {
    Construct(other);
  }
}
#endif

Var::~Var() {
  Destruct();
}

Var& Var::operator = (const Var& rhs) {
#ifdef FUN_NO_SOO
  Var tmp(rhs);
  Swap(tmp);
#else
  if ((this != &rhs) && !rhs.IsEmpty()) {
    Construct(rhs);
  } else if ((this != &rhs) && rhs.IsEmpty()) {
    placeholder_.Clear();
  }
#endif
  return *this;
}

const Var Var::operator + (const Var& other) const {
  if (IsInteger()) {
    if (IsSigned()) {
      return Add<fun::int64>(other);
    } else {
      return Add<fun::uint64>(other);
    }
  } else if (IsNumeric()) {
    return Add<double>(other);
  } else if (IsString()) {
    return Add<String>(other);
  } else {
    throw InvalidArgumentException("Invalid operation for this data type.");
  }
}

Var& Var::operator += (const Var& other) {
  if (IsInteger()) {
    if (IsSigned()) {
      return *this = Add<fun::int64>(other);
    } else {
      return *this = Add<fun::uint64>(other);
    }
  } else if (IsNumeric()) {
    return *this = Add<double>(other);
  } else if (IsString()) {
    return *this = Add<String>(other);
  } else {
    throw InvalidArgumentException("Invalid operation for this data type.");
  }
}

const Var Var::operator - (const Var& other) const {
  if (IsInteger()) {
    if (IsSigned()) {
      return Subtract<fun::int64>(other);
    } else {
      return Subtract<fun::uint64>(other);
    }
  } else if (IsNumeric()) {
    return Subtract<double>(other);
  } else {
    throw InvalidArgumentException("Invalid operation for this data type.");
  }
}

Var& Var::operator -= (const Var& other) {
  if (IsInteger()) {
    if (IsSigned()) {
      return *this = Subtract<fun::int64>(other);
    } else {
      return *this = Subtract<fun::uint64>(other);
    }
  } else if (IsNumeric()) {
    return *this = Subtract<double>(other);
  } else {
    throw InvalidArgumentException("Invalid operation for this data type.");
  }
}

const Var Var::operator * (const Var& other) const {
  if (IsInteger()) {
    if (IsSigned()) {
      return Multiply<fun::int64>(other);
    } else {
      return Multiply<fun::uint64>(other);
    }
  } else if (IsNumeric()) {
    return Multiply<double>(other);
  } else {
    throw InvalidArgumentException("Invalid operation for this data type.");
  }
}

Var& Var::operator *= (const Var& other) {
  if (IsInteger()) {
    if (IsSigned()) {
      return *this = Multiply<fun::int64>(other);
    } else {
      return *this = Multiply<fun::uint64>(other);
    }
  } else if (IsNumeric()) {
    return *this = Multiply<double>(other);
  } else {
    throw InvalidArgumentException("Invalid operation for this data type.");
  }
}

const Var Var::operator / (const Var& other) const {
  if (IsInteger()) {
    if (IsSigned()) {
      return Divide<fun::int64>(other);
    } else {
      return Divide<fun::uint64>(other);
    }
  } else if (IsNumeric()) {
    return Divide<double>(other);
  } else {
    throw InvalidArgumentException("Invalid operation for this data type.");
  }
}

Var& Var::operator /= (const Var& other) {
  if (IsInteger()) {
    if (IsSigned()) {
      return *this = Divide<fun::int64>(other);
    } else {
      return *this = Divide<fun::uint64>(other);
    }
  } else if (IsNumeric()) {
    return *this = Divide<double>(other);
  } else {
    throw InvalidArgumentException("Invalid operation for this data type.");
  }
}

Var& Var::operator ++ () {
  if (!IsInteger()) {
    throw InvalidArgumentException("Invalid operation for this data type.");
  }

  return *this = *this + 1;
}

const Var Var::operator ++ (int) {
  if (!IsInteger()) {
    throw InvalidArgumentException("Invalid operation for this data type.");
  }

  Var tmp(*this);
  *this += 1;
  return tmp;
}

Var& Var::operator -- () {
  if (!IsInteger()) {
    throw InvalidArgumentException("Invalid operation for this data type.");
  }

  return *this = *this - 1;
}

const Var Var::operator -- (int) {
  if (!IsInteger()) {
    throw InvalidArgumentException("Invalid operation for this data type.");
  }

  Var tmp(*this);
  *this -= 1;
  return tmp;
}

bool Var::operator == (const Var& other) const {
  if (IsEmpty() != other.IsEmpty()) {
    return false;
  }

  if (IsEmpty() && other.IsEmpty()) {
    return true;
  }

  return Convert<String>() == other.Convert<String>();
}

bool Var::operator == (const char* other) const {
  if (IsEmpty()) {
    return false;
  }

  return Convert<String>() == other;
}

bool Var::operator != (const Var& other) const {
  if (IsEmpty() && other.IsEmpty()) {
    return false;
  } else if (IsEmpty() || other.IsEmpty()) {
    return true;
  }

  return Convert<String>() != other.Convert<String>();
}

bool Var::operator != (const char* other) const {
  if (IsEmpty()) {
    return true;
  }

  return Convert<String>() != other;
}

bool Var::operator < (const Var& other) const {
  if (IsEmpty() || other.IsEmpty()) {
    return false;
  }

  return Convert<String>() < other.Convert<String>();
}

bool Var::operator <= (const Var& other) const {
  if (IsEmpty() || other.IsEmpty()) {
    return false;
  }

  return Convert<String>() <= other.Convert<String>();
}

bool Var::operator > (const Var& other) const {
  if (IsEmpty() || other.IsEmpty()) {
    return false;
  }

  return Convert<String>() > other.Convert<String>();
}

bool Var::operator >= (const Var& other) const {
  if (IsEmpty() || other.IsEmpty()) {
    return false;
  }

  return Convert<String>() >= other.Convert<String>();
}

bool Var::operator || (const Var& other) const {
  if (IsEmpty() || other.IsEmpty()) {
    return false;
  }

  return Convert<bool>() || other.Convert<bool>();
}

bool Var::operator && (const Var& other) const {
  if (IsEmpty() || other.IsEmpty()) {
    return false;
  }

  return Convert<bool>() && other.Convert<bool>();
}

void Var::Clear() {
#ifdef FUN_NO_SOO
  delete holder_;
  holder_ = nullptr;
#else
  if (placeholder_.IsLocal()) {
    this->~Var();
  } else {
    delete GetContent();
  }
  placeholder_.Clear();
#endif
}

Var& Var::GetAt(size_t n) {
  if (IsVector()) {
    return HolderImpl<std::vector<Var>,
              InvalidAccessException>("Not a vector.")->operator[](n);
  } else if (IsList()) {
    return HolderImpl<std::list<Var>,
              InvalidAccessException>("Not a list.")->operator[](n);
  } else if (IsDeque()) {
    return HolderImpl<std::deque<Var>,
              InvalidAccessException>("Not a deque.")->operator[](n);
  } else if (IsStruct()) {
#ifdef FUN_ENABLE_CPP11
    if (IsOrdered())
      return StructIndexOperator(HolderImpl<Struct<int, OrderedMap<int, Var>, OrderedSet<int> >,
              InvalidAccessException>("Not a struct."), static_cast<int>(n));
    else
#endif // FUN_ENABLE_CPP11
      return StructIndexOperator(HolderImpl<Struct<int, std::map<int, Var>, std::set<int> >,
              InvalidAccessException>("Not a struct."), static_cast<int>(n));
  }
  else if (!IsString() && !IsEmpty() && (n == 0)) {
    return *this;
  }

  throw RangeException("Index out of bounds.");
}

char& Var::At(size_t n) {
  if (IsString()) {
    return HolderImpl<String,
                InvalidAccessException>("Not a string.")->operator[](n);
  }

  throw InvalidAccessException("Not a string.");
}

Var& Var::GetAt(const String& name) {
  if (IsStruct()) {
#ifdef FUN_ENABLE_CPP11
    if (IsOrdered())
      return StructIndexOperator(HolderImpl<OrderedDynamicStruct, InvalidAccessException>("Not a struct."), name);
    else
#endif // FUN_ENABLE_CPP11
      return StructIndexOperator(HolderImpl<DynamicStruct, InvalidAccessException>("Not a struct."), name);
  }

  throw InvalidAccessException("Not a struct.");
}

Var Var::Parse(const String& val) {
  String::size_type t = 0;
  return Parse(val, t);
}

Var Var::Parse(const String& val, String::size_type& pos) {
  // { -> an Object==DynamicStruct
  // [ -> an array
  // '/" -> a string (strip '/")
  // other: also treat as string

  SkipWhitespaces(val, pos);

  if (pos < val.size()) {
    switch (val[pos]) {
      case '{':
        return ParseObject(val, pos);
      case '[':
        return ParseArray(val, pos);
      case '"':
        return ParseJsonString(val, pos);
      default: {
        String str = ParseString(val, pos);
        if (str == "false") {
          return false;
        }

        if (str == "true") {
          return true;
        }

        bool is_number = false;
        bool is_signed = false;
        int separators = 0;
        int frac = 0;
        int index = 0;
        size_t size = str.size();
        for (size_t i = 0; i < size ; ++i) {
          int ch = str[i];
          if ((ch == '-' || ch == '+') && index == 0) {
            if (ch == '-') {
              is_signed = true;
            }
          } else if (Ascii::IsDigit(ch)) {
            is_number |= true;
          } else if (ch == '.' || ch == ',') {
            frac = ch;
            ++separators;
            if (separators > 1) {
              return str;
            }
          } else {
            return str;
          }
          ++index;
        }

        if (frac && is_number) {
          const double number = NumberParser::ParseFloat(str, static_cast<char>(frac));
          return Var(number);
        } else if (frac == 0 && is_number && is_signed) {
          const fun::int64 number = NumberParser::Parse64(str);
          return number;
        } else if (frac == 0 && is_number && !is_signed) {
          const fun::uint64 number = NumberParser::ParseUnsigned64(str);
          return number;
        }

        return str;
      }
    }
  }

  String empty;
  return empty;
}

Var Var::ParseObject(const String& val, String::size_type& pos) {
  fun_assert_dbg(pos < val.size() && val[pos] == '{');
  ++pos;
  SkipWhitespaces(val, pos);
  DynamicStruct aStruct;
  while (val[pos] != '}' && pos < val.size()) {
    String key = ParseString(val, pos);
    SkipWhitespaces(val, pos);
    if (val[pos] != ':') {
      throw DataFormatException("Incorrect object, must contain: key : value pairs");
    }

    ++pos; // skip past :
    Var value = Parse(val, pos);
    aStruct.insert(key, value);
    SkipWhitespaces(val, pos);
    if (val[pos] == ',') {
      ++pos;
      SkipWhitespaces(val, pos);
    }
  }

  if (val[pos] != '}') {
    throw DataFormatException("Unterminated object");
  }

  ++pos;
  return aStruct;
}

Var Var::ParseArray(const String& val, String::size_type& pos) {
  fun_assert_dbg(pos < val.size() && val[pos] == '[');

  ++pos;
  SkipWhitespaces(val, pos);
  std::vector<Var> result;
  while (val[pos] != ']' && pos < val.size()) {
    result.push_back(Parse(val, pos));
    SkipWhitespaces(val, pos);
    if (val[pos] == ',') {
      ++pos;
      SkipWhitespaces(val, pos);
    }
  }

  if (val[pos] != ']') {
    throw DataFormatException("Unterminated array");
  }

  ++pos;
  return result;
}

String Var::ParseString(const String& val, String::size_type& pos) {
  fun_assert_dbg(pos < val.size());

  if (val[pos] == '"') {
    return ParseJsonString(val, pos);
  } else {
    String result;
    while (pos < val.size()
        && !fun::Ascii::isSpace(val[pos])
        && val[pos] != ','
        && val[pos] != ']'
        && val[pos] != '}') {
      result += val[pos++];
    }
    return result;
  }
}

String Var::ParseJsonString(const String& val, String::size_type& pos) {
  fun_assert_dbg(pos < val.size() && val[pos] == '"');
  ++pos;
  String result;
  bool done = false;
  while (pos < val.size() && !done) {
    switch (val[pos]) {
      case '"':
        done = true;
        ++pos;
        break;
      case '\\':
        if (pos < val.size()) {
          ++pos;
          switch (val[pos]) {
            case 'b':
              result += '\b';
              break;
            case 'f':
              result += '\f';
              break;
            case 'n':
              result += '\n';
              break;
            case 'r':
              result += '\r';
              break;
            case 't':
              result += '\t';
              break;
            default:
              result += val[pos];
              break;
          }
          break;
        } else {
          result += val[pos];
        }
        ++pos;
        break;
      default:
        result += val[pos++];
        break;
    }
  }

  if (!done) {
    throw fun::DataFormatException("unterminated JSON string");
  }

  return result;
}

void Var::SkipWhitespaces(const String& val, String::size_type& pos) {
  fun_assert_dbg(pos < val.size());

  while (std::isspace(val[pos]) && pos < val.size()) {
    ++pos;
  }
}

String Var::ToString(const Var& any) {
  String res;
  Impl::AppendJsonValue(res, any);
  return res;
}

/*
Var& Var::StructIndexOperator(VarHolderImpl<Struct<int> >* pStr, int n) const {
  return pStr->operator[](n);
}
*/

} // namespace dynamic
} // namespace fun
