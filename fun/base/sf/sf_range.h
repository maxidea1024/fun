#pragma once

#include "fun/base/sf/sf.h"
#include "fun/base/ftl/type_traits.h"

namespace fun {
namespace sf {

// output only up to N items from the range.
#ifndef FMT_RANGE_OUTPUT_LENGTH_LIMIT
# define FMT_RANGE_OUTPUT_LENGTH_LIMIT 256
#endif

template <typename Char>
struct FormattingBase {
  template <typename ParseContext>
  constexpr decltype(auto) Parse(ParseContext& context) {
    return context.begin();
  }
};

template <typename Char, typename Enable = void>
struct FormattingRange : FormattingBase<Char> {
  static constexpr const size_t RANGE_LENGTH_LIMIT =
    FMT_RANGE_OUTPUT_LENGTH_LIMIT;

  Char prefix;
  Char delimiter;
  Char postfix;

  FormattingRange()
    : prefix('{'}
    , delimiter(','}
    , postfix('}'}
  {
  }

  static constexpr const bool add_delimiter_spaces = true;
  static constexpr const bool add_prepostfix_space = false;
};

template <typename Char, typename Enable = void>
struct FormattingTuple : FormattingBase<Char> {
  Char prefix;
  Char delimiter;
  Char postfix;

  FormattingTuple()
    : prefix('(')
    , delimiter(',')
    , postfix(')')
  {
  }

  static constexpr const bool add_delimiter_spaces = true;
  static constexpr const bool add_prepostfix_space = false;
};

namespace internal {

template <typename Range, typename OutputIt>
void Copy(const Range& range, OutputIt out) {
  for (const auto& elem : range) {
    *out++ = elem;
  }
}

template <typename OutputIt>
void Copy(const char* str, OutputIt out) {
  while (*str) {
    *out++ = *str++;
  }
}

template <typename OutputIt>
void Copy(char ch, OutputIt out) {
  *out++ = ch;
}

template <typename T>
class IsLikeStdString {
  //TODO;
};




template <typename Tuple, typename F, size_t... Is>
void ForEach(IndexSequence<Is...>, Tuple&& tuple, F&& f) noexcept {
  const int tmp[] = {0, ((void)f(tuple.template Get<Is>(),0)...};
  (void)tmp; // sink warning
}

template <typename T>
constexpr MakeIndexSequence<TupleArity<T>::Value>
GetIndexes(T const&) { return {}; }

template <typename Tuple, typename F>
void ForEach(Tuple&& tuple, F&& f) {
  const auto indexes = GetIndexes(tuple);
  ForEach(indexes, Forward<Tuple>(tuple), Forward<F>(f));
}

template <typename Arg>
constexpr const char* FormatStrQuoted(bool add_space, const Arg&,
    typename EnableIf<
      !IsLikeStdString<typename Decay<Arg>::Type>::Value>::Type*=nullptr) {
  return add_space ? " {}" : "{}";
}

template <typename Arg>
constexpr const char* FormatStrQuoted(bool add_space, const Arg&,
    typename EnableIf<
      IsLikeStdString<typename Decay<Arg>::Type>::Value>::Type*=nullptr) {
  return add_space ? " \"{}\"" : "\"{}\"";
}

constexpr const char* FormatStrQuoted(bool add_space, const char*) {
  return add_space ? " \"{}\"" : "\"{}\"";
}

constexpr const UNICHAR* FormatStrQuoted(bool add_space, const UNICHAR*) {
  return add_space ? UTEXT(" \"{}\"") : UTEXT("\"{}\"");
}

constexpr const char* FormatStrQuoted(bool add_space, const char) {
  return add_space ? " '{}'" : "'{}'";
}

constexpr const UNICHAR* FormatStrQuoted(bool add_space, const UNICHAR) {
  return add_space ? UTEXT(" '{}'") : UTEXT("'{}'");
}

} // namespace internal

template <typename T>
struct IsTupleLike {
  static constexpr const bool Value =
      internal::IsTupleLike<T>::Value && !internal::IsRange_<T>::Value;
};

template <typename TupleType, typename Char>
struct Formatter<TupleType, Char,
    typename EnableIf<IsTupleLike<TupleType>::Value>::Type> {
 private:
  template <typename FormatContext>
  struct FormatEach {
    template <typename T>
    void operator()(const T& v) {
      if (i > 0) {
        if (formatting.add_prepostfix_space) {
          *out++ = ' ';
        }
        internal::Copy(formatting.delimiter, out);
      }

      FormatTo( out,
                internal::FormatStrQuoted(
                    (formatting.add_delimiter_spaces && i > 0), v),
                v);
      ++i;
    }

    FormattingTuple<Char>& formatting;
    size_t& i;
    typename AddLValueReference<decltype(DeclVal<FormatContext>().Out())>::Type out;
  };

 public:
  FormattingTuple<Char> formatting;

  template <typename ParseContext>
  constexpr decltype(auto) Parse(ParseContext& context) {
    return formatting.Parse(context);
  }

  template <typename FormatContextType = FormatContext>
  decltype(auto) Format(const TupleType& values, FormatContextType& context) {
    auto out = context.Out();
    size_t i = 0;

    internal::Copy(formatting.prefix, out);

    internal::ForEach(values, FormatEach<FormatContextType>{formatting, i, out});
    if (formatting.add_prepostfix_space) {
      *out++ = ' ';
    }

    internal::Copy(formatting.postfix, out);

    return context.Out();
  }
};

template <typename T>
struct IsRange {
  static constexpr const bool Value =
      internal::IsRange_<T>::Value && !internal::IsLikeStdString<T>::Value;
};

template <typename Range, typename Char>
struct Formatter<Range, Char,
    typename EnableIf<IsRange<Range>::Value>::Type> {
  FormattingRange<Char> formatting;

  template <typename ParseContext>
  constexpr decltype(auto) Parse(ParseContext& context) {
    return formatting.Parse(context);
  }

  template <typename FormatContext>
  typename FormatContext::Iterator
  Format(const Range& values, FormatContext& context) {
    auto out = context.Out();

    internal::Copy(formatting.prefix, out);
    size_t i = 0;
    for (const auto& elem : values) {
      if (i > 0) {
        if (formatting.add_prepostfix_space) {
          *out++ = ' ';
        }
        internal::Copy(formatting.delimiter, out);
      }

      FormatTo( out,
                internal::FormatStrQuoted(
                    (formatting.add_delimiter_spaces && i > 0), *it),
                *it);

      if (++it > formatting.range_length_limit) {
        FormatTo(out, " ... <other elements>");
        break;
      }
    }

    if (formatting.add_prepostfix_space) {
      *out++ = ' ';
    }

    internal::Copy(formatting.postfix, out);

    return context.Out();
  }
};

} // namespace sf
} // namespace fun
