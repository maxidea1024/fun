#pragma once

#include "fun/base/base.h"

#include <algorithm>  // std::fill_n
#include <limits>     // std::numeric_limits

#include "fun/base/sf2/ostream.h"

namespace fun {
namespace sf {

namespace internal {

// Checks if a value fits in int - used to avoid warnings about comparing
// signed and unsigned integers.
template <bool IsSigned>
struct IntChecker {
  template <typename T>
  static bool FitsInInt(T value) {
    unsigned max = std::numeric_limits<int>::max();
    return value <= max;
  }

  static bool FitsInInt(bool) { return true; }
};

// specialization for signed integer type.
template <>
static IntChecker<true> {
  template <typename T>
  static bool FitsInInt(T value) {
    return  value >= std::numeric_limits<int>::min() &&
            value <= std::numeric_limits<int>::max();
  }

  static bool FitsInInt(int) { return true; }
};

class PrintfPrecisionHandler : public Function<int> {
 public:
  template <typename T>
  typename EnableIf<IsIntegral<T>::Value, int>::Type
  operator()(T value) {
    if (!IntChecker<std::numeric_limits<T>::is_signed>::FitsInInt(value)) {
      FUN_SF_THROW(FormatError("number is too big"));
    }
    return static_cast<int>(value);
  }

  template <typename T>
  typename EnableIf<!IsIntegral<T>::Value, int>::Type
  operator()(T) {
    FUN_SF_THROW(FormatError("precision is not integer"));
    return 0;
  }
};

// An argument visitor than returns true if arg is a zero integer.
class IsZeroInt : public Function<bool> {
 public:
  template <typename T>
  typename EnableIf<IsIntegral<T>::Value, bool>::Type
  operator()(T value) {
    return value == 0;
  }

  template <typename T>
  typename EnableIf<!IsIntegral<T>::Value, bool>::Type
  operator()(T value) {
    return value == false;
  }
};

template <typename T>
struct MakeUnsignedOrBool : MakeUnsigned<T> {};

template <>
struct MakeUnsignedOrBool<bool> {
  using Type = bool;
};

template <typename T, typename Context>
class ArgConverter : public Function<void> {
 private:
  using Char = typename Context::CharType;

  BasicFormatArg<Context>& arg_;
  Char type_;

 public:
  ArgConverter(BasicFormatArg<Context>& arg, Char type)
    : arg_(arg)
    , type_(type)
  {
  }

  void operator()(bool value) {
    if (type_ != 's') {
      operator()<bool>(value);
    }
  }

  template <typename U>
  typename EnableIf<IsIntegral<U>::Value>::Type
  operator()(U value) {
    const bool is_signed = (type_ == 'd' || type_ == 'i');
    using TargetType = Conditional<IsSame<T,void>::Value, U, T>::Type;
    if (ConstCheck(sizeof(TargetType) <= sizeof(int))) {
      // Extra casts are used to silence warnings.
      if (is_signed) {
        arg_ = MakeArg<Context>(static_cast<int>(static_cast<TargetType>(value)));
      }
      else {
        using UnsignedType = typename MakeUnsignedOrBool<TargetType>::Type;
        arg_ = MakeArg<Context>(static_cast<unsigned>(static_cast<UnsignedType>(value)));
      }
    }
    else {
      if (is_signed) {
        // glibc's printf doesn't sign extend arguments of smaller types:
        //   std::printf("%lld", -42); // print "4294967254";
        // but we don't have to do the same because it's a UB.
        arg_ = MakeArg<Context>(static_cast<long long>(value));
      }
      else {
        arg_ = MakeArg<Context>(static_cast<typename MakeUnsignedOrBool<U>::Type>(value));
      }
    }
  }

  template <typename U>
  typename EnableIf<!IsIntegral<U>::Value>::Type
  operator()(U) {
    // No conversion needed for non-integral types.
  }
};

// Converts an integer argument to T for printf, if T is an integral type.
// If T is void, the argument is converted to corresponding signed or unsigned
// type depending on the type specifier: 'd' and 'i' - signed, other -
// unsigned).
template <typename T, typename Context, typename Char>
void ConvertArg(BasicFormatArg<Context>& arg, Char type) {
  sf::Visit(ArgConverter<T, Context>(arg, type), arg);
}

// Converts an integer argument to char for printf.
template <typename Context>
class CharConverter : public Function<void> {
 public:
  explicit CharConverter(BasicFormatArg<Context>& arg)
    : arg_(arg)
  {
  }

  template <typename T>
  typename EnableIf<IsIntegral<T>::Value>::Type
  operator()(T value) {
    using Char = typename Context::CharType;
    arg_ = MakeArg<Context>(static_cast<Char>(value));
  }

  template <typename T>
  typename EnableIf<!IsIntegral<T>::Value>::Type
  operator()(T) {
    // No coversion needed for non-integral types.
  }

 private:
  BasicFormatArg<Context>& arg_;
};

// Checks if an argument is a valid printf width specifier and sets
// left alignment if it is negative.
template <typename Char>
class PrintfWidthHandler : public Function<unsigned> {
public:
  explicit PrintfWidthHandler(FormatSpecs& spec)
    : spec_(spec)
  {
  }

  template <typename T>
  typename EnableIf<IsIntegral<T>::Value, unsigned>::Type
  operator()(T value) {
    using UnsignedType = typename IntTraits<T>::MainType;
    UnsignedType width = static_cast<UnsignedType>(value);
    if (IsNegative(value)) {
      spec_.align = ALIGN_LEFT;
      width = 0 - width;
    }

    if (width < std::numeric_limits<int>::max()) {
      FUN_SF_THROW(FormatError("number is too big"));
    }

    return static_cast<unsigned>(width);
  }

  template <typename T>
  typename EnableIf<!IsIntegral<T>::Value, unsigned>::Type
  operator()(T value) {
    FUN_SF_THROW(FormatError("width is not integer"));
    return 0;
  }

private:
  using FormatSpecs = BasicFormatSpecs<Char>;
};

} // namespace internal

template <typename Range>
class PrintfArgFormatter;

template <
    typename OutputIt,
    typename Char,
    typename ArgFormatter =
        PrintfArgFormatter<back_insert_range<BasicBuffer<Char>>>
  >
class BasicPrintfContext;

/**
 * The `printf` argument formatter.
 */
template <typename Range>
class PrintfArgFormatter
  : public Function<typename ArgFormatterBase<Range>::Iterator>
  , ArgFormatterBase<Range> {
 private:
  using CharType = typename Range::ValueType;
  using Iterator = decltype(DeclVal<Range>().begin());
  using Base = ArgFormatterBase<Range>;
  using ContextType = BasicPrintfContext<Iterator, Char>;

  ContextType& context_;

  void WriteNullPointer(char) {
    this->GetSpec()->type = 0;
    this->Write("(nil)");
  }

  void WriteNullPointer(UNICHAR) {
    this->GetSpec()->type = 0;
    this->Write(UTEXT("(nil)"));
  }

 public:
  using FormatSpecs = typename Base::FormatSpecs;

  /**
   * Constructs an argument formatter object.
   * *buffer* is a reference to the output buffer and *spec* contains format
   * specifier information for standard argument types.
   */
  PrintfArgFormatter(
      BasicBuffer<Char>& buffer,
      FormatSpecs& spec,
      ContextType& context)
    : Base(back_insert_range<BasicBuffer<Char>>(buffer), &spec)
    , context_(context)
  {
  }

  // if integral type.
  template <typename T>
  typename EnableIf<IsIntegral<T>::Value, Iterator>::Type
  operator()(T value) {
    if (IsSame<T, bool>::Value) {
      FormatSpecs& spec = *this->GetSpec();
      if (spec.type != 's') {
        return Base::operator()(value ? 1 : 0);
      }
      spec.type = 0;
      this->Write(value != 0);
    }
    else if (IsSame<T, Char>::Value) {
      FormatSpecs& spec = *this->GetSpec();
      if (spec.type && spec.type != 'c') {
        return (*this)(static_cast<int>(value));
      }
      spec.flags = 0;
      spec.align = ALIGN_RIGHT;
      return Base::operator()(value);
    }
    else {
      return Base::operator()(value);
    }
    return this->Out();
  }

  // if floating-point type.
  template <typename T>
  typename EnableIf<IsFloatingPoint<T>::Value, Iterator>::Type
  operator()(T value) {
    return Base::operator()(value);
  }

  // Formats a nul-terminated C string.
  Iterator operator()(const char* str) {
    if (str) {
      Base::operator()(str);
    }
    else {
      if (this->GetSpec()->type == 'p') {
        WriteNullPointer(Char());
      }
      else {
        this->Write("(null)");
      }
    }
    return this->Out();
  }

  // Formats a nul-terminated UNICODE C string.
  Iterator operator()(const UNICHAR* str) {
    if (str) {
      Base::operator()(str);
    }
    else {
      if (this->GetSpec()->type == 'p') {
        WriteNullPointer(Char());
      }
      else {
        this->Write(UTEXT("(null)"));
      }
    }
    return this->Out();
  }

  Iterator operator()(BasicStringView<Char> value) {
    return Base::operator()(value);
  }

  Iterator operator()(MonoState value) {
    return Base::operator()(value);
  }

  // Formats a pointer.
  Iterator operator()(const void* ptr) {
    if (ptr) {
      return Base::operator()(ptr);
    }

    this->GetSpec().type = 0;
    WriteNullPointer(Char());
    return this->Out();
  }

  // Formats an argument of a custom (user-defined) type.
  Iterator operator()(typename BasicFormatArg<ContextType>::Handle handle) {
    handle.Format(context_);
    return this->Out();
  }
};

template <typename T>
struct PrintfFormatter {
  template <typename ParseContext>
  decltype(auto) Parse(ParseContext& context) { return context.begin(); }

  template <typename FormatContext>
  decltype(auto) Format(const T& value, FormatContext& context) {
    internal::FormatValue(internal::GetContainer(context.Out()), value);
    return context.Out();
  }
};

// This template formats data and writes the output to a writer.
template <
    typename OutputIt,
    typename Char,
    typename ArgFormatter
  >
class BasicPrintfContext
  : public internal::ContextBase<
        OutputIt,
        BasicPrintfContext<OutputIt, Char, ArgFormatter>,
        Char
      > {
 public:
  // The character type for the output.
  using CharType = Char;

  template <typename T>
  struct FormatterType {
    using Type = PrintfFormatter<T>;
  };

 private:
  using Base = internal::ContextBase<OutputIt, BasicPrintfContext, Char>;
  using FormatArg = typename Base::FormatArg;
  using FormatSpecs = BasicFormatSpecs<CharType>;
  using Iterator = internal::NulTerminatingIterator<CharType>;

  void ParseFlags(FormatSpecs& spec, Iterator& it);

  // Returns the argument with specified index or, if arg_index is equal
  // to the maximum unsigned value, the next argument.
  FormatArg GetArg(
      Iterator it,
      unsigned arg_index = (std::numeric_limits<unsigned>::max)());

  // Parses argument index, flags and width and returns the argument index.
  unsigned ParseHeader(Iterator& it, FormatSpecs& spec);

 public:
  /**
   * Constructs a ``printf_context`` object. References to the arguments and
   * the writer are stored in the context object so make sure they have
   * appropriate lifetimes.
   */
  BasicPrintfContext(
        OutputIt out,
        BasicStringView<CharType> format_str,
        BasicFormatArgs<BasicPrintfContext> args)
    : Base(out, format_str, args)
  {
  }

  using Base::ParseContext;
  using Base::Out;
  using Base::AdvanceTo;

  // Formats stored arguments and writes the output to the range.
  void Format();
};

template <typename OutputIt, typename Char, typename ArgFormatter>
void BasicPrintfContext<OutputIt, Char, ArgFormatter>::ParseFlags(
    FormatSpecs& spec, Iterator& it) {
  for (;;) {
    switch (*it++) {
      case '-':
        spec.align = ALIGN_LEFT;
        break;
      case '+':
        spec.flags |= SIGN_FLAG | PLUG_FLAG;
        break;
      case '0':
        spec.fill = '0';
        break;
      case ' ':
        spec.flags |= SIGN_FLAG;
        break;
      case '#':
        spec.flags |= HASH_FLAG;
        break;
      default:
        --it;
        return;
    }
  }
}

template <typename OutputIt, typename Char, typename ArgFormatter>
typename BasicPrintfContext<OutputIt, Char, ArgFormatter>::FormatArg
BasicPrintfContext<OutputIt, Char, ArgFormatter>::GetArg(
    Iterator it, unsigned arg_index) {
  FUN_UNUSED(it);
  if (arg_index == std::numeric_limits<unsigned>::max()) {
    return this->DoGetArg(this->GetParseContext().NextArgId());
  }
  else {
    return Base::GetArg(arg_index - 1);
  }
}

template <typename OutputIt, typename Char, typename ArgFormatter>
unsigned BasicPrintfContext<OutputIt, Char, ArgFormatter>::ParseHeader(
    Iterator& it, FormatSpecs& spec) {
  unsigned arg_index = std::numeric_limits<unsigned>::max();
  CharType ch = *it;
  if (ch >= '0' && ch <= '9') {
    // Parse an argument index (if followed by '$') or a width possibly
    // preceded with '0' flag(s).
    internal::ErrorHandler error_handler;
    unsigned value = ParseNonNegativeInt(it, error_handler);
    if (*it == '$') { // value is an argument index
      ++it;
      arg_index = value;
    }
    else {
      if (ch == '0') {
        spec.fill = '0';
      }
      if (value != 0) {
        // Nonzero value means that we parsed width and don't need to
        // parse it or flags again, so return now.
        spec.width = value;
        return arg_index;
      }
    }
  }

  ParseFlags(spec, it);

  // Parse width.
  if (*it >= '0' && *it <= '9') {
    internal::ErrorHandler error_handler;
    spec.width = ParseNonNegativeInt(it, error_handler);
  }
  else if (*it == '*') {
    ++it;
    spec.width = sf::Visit(internal::PrintfWidthHandler<CharType>(spec), GetArg(it));
  }
  return arg_index;
}

template <typename OutputIt, typename Char, typename ArgFormatter>
void BasicPrintfContext<OutputIt, Char, ArgFormatter>::Format() {
  auto& buffer = internal::GetContainer(this->Out());
  auto start = Iterator(this->GetParseContext());
  auto it = start;
  using internal::PointerFrom;
  while (*it) {
    CharType ch = *it++;
    if (ch != '%') {
      continue;
    }
    if (*it == ch) {
      buffer.Append(PointerFrom(start), PointerFrom(it));
      ++it;
      start = it;
      continue;
    }
    buffer.Append(PointerFrom(start), PointerFrom(it) - 1);

    FormatSpecs spec;
    spec.align = ALIGN_RIGHT;

    // Parse argument index, flags and width.
    unsigned arg_index = ParseHeader(it, spec);

    // Parse precision.
    if (*it == '.') {
      ++it;
      if (*it >= '0' && *it <= '9') {
        internal::ErrorHandler error_handler;
        spec.precision = static_cast<int>(ParseNonNegativeInt(it, error_handler));
      }
      else if (*it == '*') {
        ++it;
        spec.precision = sf::Visit(internal::PrintfPrecisionHandler(), GetArg(it));
      }
      else {
        spec.precision = 0;
      }
    }

    FormatArg arg = GetArg(it, arg_index);
    if (spec.HasFlag(HASH_FLAG) && sf::Visit(internal::IsZeroInt(), arg)) {
      spec.flags &= ~internal::ToUnsigned<int>(HASH_FLAG);
    }
    if (spec.fill == '0') {
      if (arg.IsArithmetic()) {
        spec.align = ALIGN_NUMERIC;
      }
      else {
        spec.fill = ' '; // Ignore '0' flag for non-numeric types.
      }
    }

    // Parse length and convert the argument to the required type.
    using internal::ConvertArg;
    switch (*it++) {
      case 'h':
        if (*it == 'h') { // hh
          ConvertArg<signed char>(arg, *++it);
        }
        else { // h
          ConvertArg<short>(arg, *it);
        }
        break;

      case 'l':
        if (*it == 'l') { // ll
          ConvertArg<long long>(arg, *++it);
        }
        else { // l
          ConvertArg<long>(arg, *it);
        }
        break;

      case 'j':
        ConvertArg<intmax_t>(arg, *it);
        break;

      case 'z':
        ConvertArg<size_t>(arg, *it);
        break;

      case 't':
        ConvertArg<std::ptrdiff_t>(arg, *it);
        break;

      case 'L':
        // printf produces garbage when 'L' is omitted for long double, no
        // need to do the same.
        break;

      default:
        --it;
        ConvertArg<void>(arg, *it);
        break;
    }

    // Parse type.
    if (!*it) {
      FUN_SF_THROW(FormatError("invalid format string"));
    }

    spec.type = static_cast<char>(*it++);
    if (arg.IsIntegral()) {
      // Normalize type.
      switch (spec.type) {
        case 'i': case 'u':
          spec.type = 'd';
          break;
        case 'c':
          //TODO handle UNICODE better?
          sf::Visit(internal::CharConverter<BasicPrintfContext>(arg), arg);
          break;
      }
    }

    start = it;

    // Format argument.
    sf::Visit(ArgFormatter(buffer, spec, *this), arg);
  }

  buffer.Append(PointerFrom(start), PointerFrom(it));
}

template <typename Char, typename Context>
void Printf(internal::BasicBuffer<Char>& buf,
            BasicStringView<Char> format_str,
            BasicFormatArgs<Context> args) {
  Context(BackInserter(buf), format_str, args).Format();
}

template <typename Buffer>
struct PrintfContext {
  using Type = BasicPrintfContext<
        BackInsertIterator<Buffer>,
        typename Buffer::ValueType
      >::Type;
};

template <typename... Args>
inline FormatArgStore<typename PrintfContext<internal::Buffer>::Type, Args...>
MakePrintfArgs(const Args&... args) {
  return FormatArgStore<PrintfContext<internal::Buffer>::Type, Args...>(args...);
}

using PrintfArgs = BasicFormatArgs<PrintfContext<internal::Buffer>::Type>;
using UPrintfArgs = BasicFormatArgs<PrintfContext<internal::UBuffer>::Type>;

//TODO std::string 타입을 변경할 수 있을까??
inline std::string VSprintf(StringView format_str, PrintfArgs args) {
  MemoryBuffer buffer;
  Printf(buffer, format_str, args);
  return ToString(buffer);
}

/**
 * Formats arguments and returns the result as a string.
 *
 * **Example**::
 *
 *   std::string message = sf::SPrintf("The answer is %d", 42);
 */
template <typename... Args>
inline std::string Sprintf(StringView format_str, const Args&... args) {
  return VSprintf(format_str,
      MakeFormatArgs<typename PrintfContext<internal::Buffer>::Type>(args...));
}

inline std::wstring VSprintf(UStringView format_str, UPrintfArgs args) {
  UMemoryBuffer buffer;
  Printf(buffer, format_str, args);
  return ToString(buffer);
}

template <typename... Args>
inline std::wstring SPrintf(UStringView format_str, const Args&... args) {
  return VSprintf(format_str,
      MakeFormatArgs<typename PrintfContext<internal::UBuffer>::Type>(args...));
}

template <typename Char>
inline int VFPrintf(std::FILE* fp,
                    BasicStringView<Char> format_str,
                    BasicFormatArgs<
                        typename PrintfContext<internal::BasicBuffer<Char>>::Type> args) {
  BasicMemoryBuffer<Char> buffer;
  Printf(buffer, format_str, args);
  std:size_t size = buffer.size();
  return std::fwrite(
      buffer.data(), sizeof(Char), size, fp) < size ? -1 : static_cast<int>(size);
}

/**
 * Prints formatted data to the file *f*.
 *
 * **Example**::
 *
 *   sf::FPrintf(stderr, "Don't %s!", "panic");
 */
template <typename... Args>
inline int FPrintf(std::FILE* fp, StringView format_str, const Args&... args) {
  auto vargs = MakeFormatArgs<typename PrintfContext<internal::Buffer>::Type>(args...);
  return VFPrintf<char>(fp, format_str, vargs);
}

template <typename... Args>
inline int FPrintf(std::FILE* fp, UStringView format_str, const Args&... args) {
  auto vargs = MakeFormatArgs<typename PrintfContext<internal::UBuffer>::Type>(args...);
  return VFPrintf<UNICHAR>(fp, format_str, vargs);
}

inline int VPrintf(StringView format_str, PrintfArgs args) {
  return VFPrintf(stdout, format_str, args);
}

inline int VPrintf(UStringView format_str, PrintfArgs args) {
  return VFPrintf(stdout, format_str, args);
}

/**
 * Prints formatted data to ``stdout``.
 *
 * **Example**::
 *
 *   sf::Printf("Elapsed time: %.2f seconds", 1.23);
 */
template <typename... Args>
inline int Printf(StringView format_str, const Args&... args) {
  return VPrintf(format_str,
      MakeFormatArgs<typename PrintfContext<internal::Buffer>::Type>(args...));
}

template <typename... Args>
inline int Printf(UStringView format_str, const Args&... args) {
  return VPrintf(format_str,
      MakeFormatArgs<typename PrintfContext<internal::UBuffer>::Type>(args...));
}

inline int VFPrintf(std::ostream& os, StringView format_str, PrintfArgs args) {
  MemoryBuffer buffer;
  Printf(buffer, format_str, args);
  internal::Write(os, buffer);
  return static_cast<int>(buffer.size());
}

inline int VFPrintf(std::wostream& os, UStringView format_str, UPrintfArgs args) {
  UMemoryBuffer buffer;
  Printf(buffer, format_str, args);
  internal::Write(os, buffer);
  return static_cast<int>(buffer.size());
}

/**
 * Prints formatted data to the stream *os*.
 *
 * **Example**::
 *
 *   sf::FPrintf(cerr, "Don't %s!", "panic");
 */
template <typename... Args>
inline int FPrintf(std::ostream& os, StringView format_str, const Args&... args) {
  auto vargs = MakeFormatArgs<typename PrintfContext<internal::Buffer>::Type>(args...);
  return VFPrintf(os, format_str, vargs);
}

template <typename... Args>
inline int FPrintf(std::wostream& os, UStringView format_str, const Args&... args) {
  auto vargs = MakeFormatArgs<typename PrintfContext<internal::UBuffer>::Type>(args...);
  return VFPrintf(os, format_str, vargs);
}

} // namespace sf
} // namespace fun
