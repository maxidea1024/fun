#include "fun/base/smart_format.h"


//TODO 내부 버퍼를 사용한 후 복사를 하는 형태로 밖에는 사용이 안되는데 말이지..
//     뭐 크게 상관 없으려나...??

namespace fun {

namespace {

#ifndef _MSC_VER
  #define SF_SNPRINTF  snprintf
#else
inline int sf_snprintf(char* buffer, size_t len, const char* format, ...)
{
  va_list args;
  va_start(args, format);
  int result = vsnprintf_s(buffer, len, _TRUNCATE, format, args);
  va_end(args);
  return result;
}
# define SF_SNPRINTF  sf_snprintf
#endif

#if defined(_WIN32) && defined(__MINGW32__) && !defined(__NO_ISOCEXT)
  #define SF_SWPRINTF  snwprintf
#else
  #define SF_SWPRINTF  swprintf
#endif

} // namespace


const char RESET_COLOR[] = "\x1b[0m";

//typedef void (*FormatFunc)(CWriter&, int, CStringRef);

namespace sf {

void internal::ReportUnknownType(char code, const char* type)
{
  //TODO
  fun_check(0);
}


template <typename T>
int internal::CharTraits<char>::FormatFloat(char* buffer,
                                            size_t len,
                                            const char* format,
                                            uint32 width,
                                            int32 precision,
                                            T value)
{
  if (width == 0)
  {
    return precision < 0 ?
                SF_SNPRINTF(buffer, len, format, value) :
                SF_SNPRINTF(buffer, len, format, precision, value);
  }

  return precision < 0 ?
                SF_SNPRINTF(buffer, len, format, width, value) :
                SF_SNPRINTF(buffer, len, format, width, precision, value);
}


template <typename T>
int32 internal::CharTraits<wchar_t>::FormatFloat( wchar_t* buffer,
                                                  size_t len,
                                                  const wchar_t* format,
                                                  uint32 width,
                                                  int32 precision,
                                                  T value)
{
  //TODO wchar_t가 아닌, uint16 형으로 처리할 경우에는 변환이 필요할 수 있음.
  if (sizeof(wchar_t) == 4)
  {
    size_t format_len = CStringTraitsU::Strlen(format);
    wchar_t* tmp_format = (wchar_t*)ALLOCA((format_len + 1) * sizeof(wchar_t));
    for (size_t i = 0; i < format_len; ++i)
    {
      tmp_format[i] = format[i];
    }
    tmp_format[format_len] = '\0';

    wchar_t* tmp_buffer = (wchar_t*)ALLOCA(len * sizeof(wchar_t));

    int32 ret;
    if (width == 0)
    {
      ret = precision < 0 ?
              SF_SWPRINTF(tmp_buffer, len, tmp_format, value) :
              SF_SWPRINTF(tmp_buffer, len, tmp_format, precision, value);
    }
    else
    {
      ret = precision < 0 ?
              SF_SWPRINTF(tmp_buffer, len, tmp_format, width, value) :
              SF_SWPRINTF(tmp_buffer, len, tmp_format, width, precision, value);
    }

    if (ret >= 0)
    {
      for (int32 i = 0; i < ret; ++i)
      {
        buffer[i] = tmp_buffer[i];
      }
      buffer[ret] = '\0';
    }

    return ret;
  }
  else
  {
    if (width == 0)
    {
      return precision < 0 ?
        SF_SWPRINTF((wchar_t*)buffer, len, (const wchar_t*)format, value) :
        SF_SWPRINTF((wchar_t*)buffer, len, (const wchar_t*)format, precision, value);
    }

    return precision < 0 ?
        SF_SWPRINTF((wchar_t*)buffer, len, (const wchar_t*)format, width, value) :
        SF_SWPRINTF((wchar_t*)buffer, len, (const wchar_t*)format, width, precision, value);
  }
}


template <typename T>
const char internal::BasicData<T>::DIGITS[] =
  "0001020304050607080910111213141516171819"
  "2021222324252627282930313233343536373839"
  "4041424344454647484950515253545556575859"
  "6061626364656667686970717273747576777879"
  "8081828384858687888990919293949596979899";

#define SF_POWERS_OF_10(factor) \
  factor * 10, \
  factor * 100, \
  factor * 1000, \
  factor * 10000, \
  factor * 100000, \
  factor * 1000000, \
  factor * 10000000, \
  factor * 100000000, \
  factor * 1000000000

template <typename T>
const uint32_t internal::BasicData<T>::POWERS_OF_10_32[] = {
  0, SF_POWERS_OF_10(1)
};

template <typename T>
const uint64_t internal::BasicData<T>::POWERS_OF_10_64[] = {
  0,
  SF_POWERS_OF_10(1),
  SF_POWERS_OF_10(ULongLong(1000000000)),
  // Multiply several constants instead of using a single long long constant
  // to avoid warnings about C++98 not supporting long long.
  ULongLong(1000000000) * ULongLong(1000000000) * 10
};


template <typename Char>
void internal::ArgMap<Char>::Init(const ArgList& args)
{
  if (!map_.empty())
  {
    return;
  }

  typedef internal::NamedArg<Char> NamedArg;
  const NamedArg* named_arg = nullptr;
  bool use_values = args.Type(ArgList::MAX_PACKED_ARGS - 1) == internal::Arg::NONE;
  if (use_values)
  {
    for (uint32 i = 0; ; ++i)
    {
      internal::Arg::Type arg_type = args.Type(i);
      switch (arg_type) {
      case internal::Arg::NONE:
        return;
      case internal::Arg::NAMED_ARG:
        named_arg = static_cast<const NamedArg*>(args.values_[i].pointer);
        map_.Add(Pair(named_arg->name, *named_arg));
        break;
      default:
      /*nothing*/;
      }
    }
    return;
  }

  for (uint32 i = 0; i != ArgList::MAX_PACKED_ARGS; ++i)
  {
    internal::Arg::Type arg_type = args.Type(i);
    if (arg_type == internal::Arg::NAMED_ARG)
    {
      named_arg = static_cast<const NamedArg*>(args.args_[i].pointer);
      map_.Add(Pair(named_arg->name, *named_arg));
    }
  }

  for (uint32 i = ArgList::MAX_PACKED_ARGS;/*nothing*/; ++i)
  {
    switch (args.args_[i].type) {
    case internal::Arg::NONE:
      return;
    case internal::Arg::NAMED_ARG:
      named_arg = static_cast<const NamedArg*>(args.args_[i].pointer);
      map_.Add(Pair(named_arg->name, *named_arg));
      break;
    default:
      /*nothing*/;
    }
  }
}


template <typename Char>
void internal::FixedBuffer<Char>::Grow(size_t)
{
  SF_THROW(std::runtime_error("buffer overflow"));
}


internal::Arg internal::FormatterBase::DoGetArg(uint32 arg_index, const char*& error)
{
  internal::Arg arg = args_[arg_index];
  switch (arg.type) {
  case internal::Arg::NONE:
    error = "argument index out of range";
    break;
  case internal::Arg::NAMED_ARG:
    arg = *static_cast<const internal::Arg*>(arg.pointer);
    break;
  default:
    /*nothing*/;
  }
  return arg;
}


void Print(FILE* fp, CStringRefA format_str, ArgList args)
{
  MemoryWriterA writer;
  writer.Write(format_str, args);
  std::fwrite(writer.ConstData(), 1, writer.Len(), fp);
}


void Print(CStringRefA format_str, ArgList args)
{
  Print(stdout, format_str, args);
}


void PrintColored(Color c, CStringRefA format, ArgList args)
{
  char escape[] = "\x1b[30m";
  escape[3] = static_cast<char>('0' + c);
  std::fputs(escape, stdout);
  Print(format, args);
  std::fputs(RESET_COLOR, stdout);
}

template struct internal::BasicData<void>;

// Explicit instantiations for char.

template void internal::FixedBuffer<char>::Grow(size_t);

template void internal::ArgMap<char>::Init(const ArgList &args);

template FUN_BASE_API int32 internal::CharTraits<char>::FormatFloat(char* buffer, size_t len, const char* format, uint32 width, int32 precision, double value);
template FUN_BASE_API int32 internal::CharTraits<char>::FormatFloat(char* buffer, size_t len, const char* format, uint32 width, int32 precision, long double value);

// Explicit instantiations for wchar_t.

template void internal::FixedBuffer<wchar_t>::Grow(size_t);

template void internal::ArgMap<wchar_t>::Init(const ArgList& args);

template FUN_BASE_API int32 internal::CharTraits<wchar_t>::FormatFloat(wchar_t* buffer, size_t len, const wchar_t* format, uint32 width, int32 precision, double value);
template FUN_BASE_API int32 internal::CharTraits<wchar_t>::FormatFloat(wchar_t* buffer, size_t len, const wchar_t* format, uint32 width, int32 precision, long double value);



//
// Printf
//

int32 fprintf(std::FILE* fp, const CStringRefA& format_str, const ArgList& args)
{
  MemoryWriterA writer;
  printf(writer, format_str, args);
  const auto len = writer.Len();
  return std::fwrite(writer.ConstData(), 1, len, fp) < len ? -1 : static_cast<int32>(len);
}

template void PrintfFormatter<char   >::Format(const CStringRefA& format);
template void PrintfFormatter<wchar_t>::Format(const CStringRefW& format);


} // namespace sf
} // namespace fun
