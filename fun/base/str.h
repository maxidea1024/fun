#pragma once

#include "fun/base/base.h"
#include "fun/base/string/cstring_traits.h"
#include "fun/base/string/string.h"

namespace fun {

template <typename S>
S TrimLeft(const S& str) {
  typename S::ConstIterator i = str.begin();
  typename S::ConstIterator e = str.end();

  while (i != e && CharTraitsA::IsSpace(*i)) {
    ++i;
  }
  return S(i, e);
}

template <typename S>
S& TrimLeftInPlace(S& str) {
  typename S::ConstIterator i = str.begin();
  typename S::ConstIterator e = str.end();

  while (i != e && CharTraitsA::IsSpace(*i)) {
    ++i;
  }
  str.Remove(0, e - i);
  return str;
}

template <typename S>
S TrimRight(const S& str) {
  int32 i = str.Len() - 1;
  while (i >= 0 && CharTraitsA::IsSpace(str[i])) {
    --i;
  }
  return S(str, 0, i + 1);
}

/**
 * Removes all trailing whitespace in str.
 */
template <typename S>
S& TrimRightInPlace(S& str) {
  int32 pos = int32(str.Len()) - 1;

  while (pos >= 0 && CharTraitsA::IsSpace(str[pos])) {
    --pos;
  }
  str.Resize(pos + 1);

  return str;
}

/**
 * Returns a copy of str with all leading and
 * trailing whitespace removed.
 */
template <typename S>
S Trim(const S& str) {
  int32 first = 0;
  int32 last = int32(str.Len()) - 1;

  while (first <= last && CharTraitsA::IsSpace(str[first])) {
    ++first;
  }
  while (last >= first && CharTraitsA::IsSpace(str[last])) {
    --last;
  }

  return S(str, first, last - first + 1);
}

/**
 * Removes all leading and trailing whitespace in str.
 */
template <typename S>
S& TrimInPlace(S& str) {
  int32 first = 0;
  int32 last = int32(str.Len()) - 1;

  while (first <= last && CharTraitsA::IsSpace(str[first])) {
    ++first;
  }
  while (last >= first && CharTraitsA::IsSpace(str[last])) {
    --last;
  }

  str.Resize(last + 1);
  str.Remove(0, first);

  return str;
}

/**
 * Returns a copy of str containing all upper-case characters.
 */
template <typename S>
S ToUpper(const S& str) {
  typename S::ConstIterator it = str.begin();
  typename S::ConstIterator end = str.end();

  S result;
  result.Reserve(str.Len());
  while (it != end) {
    result += static_cast<typename S::CharType>(CharTraitsA::ToUpper(*it++));
  }
  return result;
}

/**
 * Replaces all characters in str with their upper-case counterparts.
 */
template <typename S>
S& ToUpperInPlace(S& str) {
  typename S::iterator it = str.begin();
  typename S::iterator end = str.end();

  while (it != end) {
    *it = static_cast<typename S::CharType>(CharTraitsA::ToUpper(*it));
    ++it;
  }
  return str;
}

/**
 * Returns a copy of str containing all lower-case characters.
 */
template <typename S>
S ToLower(const S& str) {
  typename S::ConstIterator it = str.begin();
  typename S::ConstIterator end = str.end();

  S result;
  result.Reserve(str.Len());
  while (it != end) {
    result += static_cast<typename S::CharType>(CharTraitsA::ToLower(*it++));
  }
  return result;
}

/**
 * Replaces all characters in str with their lower-case counterparts.
 */
template <typename S>
S& ToLowerInPlace(S& str) {
  typename S::iterator it = str.begin();
  typename S::iterator end = str.end();

  while (it != end) {
    *it = static_cast<typename S::CharType>(CharTraitsA::ToLower(*it));
    ++it;
  }
  return str;
}

#if !defined(FUN_NO_TEMPLATE_ICOMPARE)

/**
 * Case-insensitive string comparison
 */
template <typename S, typename It>
int32 icompare(const S& str, int32 pos, int32 n, It it2, It end2) {
  int32 sz = str.Len();
  if (pos > sz) {
    pos = sz;
  }
  if (pos + n > sz) {
    n = sz - pos;
  }
  It it1 = str.begin() + pos;
  It end1 = str.begin() + pos + n;
  while (it1 != end1 && it2 != end2) {
    typename S::CharType c1(
        static_cast<typename S::CharType>(CharTraitsA::ToLower(*it1)));
    typename S::CharType c2(
        static_cast<typename S::CharType>(CharTraitsA::ToLower(*it2)));
    if (c1 < c2) {
      return -1;
    } else if (c1 > c2) {
      return 1;
    }
    ++it1;
    ++it2;
  }

  if (it1 == end1) {
    return it2 == end2 ? 0 : -1;
  } else {
    return 1;
  }
}

/**
 * A special optimization for an often used case.
 */
template <typename S>
int32 icompare(const S& str1, const S& str2) {
  typename S::ConstIterator it1(str1.begin());
  typename S::ConstIterator end1(str1.end());
  typename S::ConstIterator it2(str2.begin());
  typename S::ConstIterator end2(str2.end());
  while (it1 != end1 && it2 != end2) {
    typename S::CharType c1(
        static_cast<typename S::CharType>(CharTraitsA::ToLower(*it1)));
    typename S::CharType c2(
        static_cast<typename S::CharType>(CharTraitsA::ToLower(*it2)));
    if (c1 < c2) {
      return -1;
    } else if (c1 > c2) {
      return 1;
    }
    ++it1;
    ++it2;
  }

  if (it1 == end1) {
    return it2 == end2 ? 0 : -1;
  } else {
    return 1;
  }
}

template <typename S>
int32 icompare(const S& str1, int32 n1, const S& str2, int32 n2) {
  if (n2 > str2.Len()) {
    n2 = str2.Len();
  }
  return icompare(str1, 0, n1, str2.begin(), str2.begin() + n2);
}

template <typename S>
int32 icompare(const S& str1, int32 n, const S& str2) {
  if (n > str2.Len()) {
    n = str2.Len();
  }
  return icompare(str1, 0, n, str2.begin(), str2.begin() + n);
}

template <typename S>
int32 icompare(const S& str1, int32 pos, int32 n, const S& str2) {
  return icompare(str1, pos, n, str2.begin(), str2.end());
}

template <typename S>
int32 icompare(const S& str1, int32 pos1, int32 n1, const S& str2, int32 pos2,
               int32 n2) {
  int32 sz2 = str2.Len();
  if (pos2 > sz2) {
    pos2 = sz2;
  }
  if (pos2 + n2 > sz2) {
    n2 = sz2 - pos2;
  }
  return icompare(str1, pos1, n1, str2.begin() + pos2,
                  str2.begin() + pos2 + n2);
}

template <typename S>
int32 icompare(const S& str1, int32 pos1, int32 n, const S& str2, int32 pos2) {
  int32 sz2 = str2.Len();
  if (pos2 > sz2) {
    pos2 = sz2;
  }
  if (pos2 + n > sz2) {
    n = sz2 - pos2;
  }
  return icompare(str1, pos1, n, str2.begin() + pos2, str2.begin() + pos2 + n);
}

template <typename S>
int32 icompare(const S& str, int32 pos, int32 n,
               const typename S::CharType* ptr) {
  fun_check_ptr(ptr);
  int32 sz = str.Len();
  if (pos > sz) {
    pos = sz;
  }
  if (pos + n > sz) {
    n = sz - pos;
  }
  typename S::ConstIterator it = str.begin() + pos;
  typename S::ConstIterator end = str.begin() + pos + n;
  while (it != end && *ptr) {
    typename S::CharType c1(
        static_cast<typename S::CharType>(CharTraitsA::ToLower(*it)));
    typename S::CharType c2(
        static_cast<typename S::CharType>(CharTraitsA::ToLower(*ptr)));
    if (c1 < c2) {
      return -1;
    } else if (c1 > c2) {
      return 1;
    }
    ++it;
    ++ptr;
  }

  if (it == end) {
    return *ptr == 0 ? 0 : -1;
  } else {
    return 1;
  }
}

template <typename S>
int32 icompare(const S& str, int32 pos, const typename S::CharType* ptr) {
  return icompare(str, pos, str.Len() - pos, ptr);
}

template <typename S>
int32 icompare(const S& str, const typename S::CharType* ptr) {
  return icompare(str, 0, str.Len(), ptr);
}

#else

int32 FUN_BASE_API icompare(const String& str, int32 pos, int32 n,
                            String::ConstIterator it2,
                            String::ConstIterator end2);
int32 FUN_BASE_API icompare(const String& str1, const String& str2);
int32 FUN_BASE_API icompare(const String& str1, int32 n1, const String& str2,
                            int32 n2);
int32 FUN_BASE_API icompare(const String& str1, int32 n, const String& str2);
int32 FUN_BASE_API icompare(const String& str1, int32 pos, int32 n,
                            const String& str2);
int32 FUN_BASE_API icompare(const String& str1, int32 pos1, int32 n1,
                            const String& str2, int32 pos2, int32 n2);
int32 FUN_BASE_API icompare(const String& str1, int32 pos1, int32 n,
                            const String& str2, int32 pos2);
int32 FUN_BASE_API icompare(const String& str, int32 pos, int32 n,
                            const String::CharType* ptr);
int32 FUN_BASE_API icompare(const String& str, int32 pos,
                            const String::CharType* ptr);
int32 FUN_BASE_API icompare(const String& str, const String::CharType* ptr);

#endif

/**
 * Returns a copy of str with all characters in
 * from replaced by the corresponding (by position)
 * characters in to. If there is no corresponding
 * character in to, the character is removed from
 * the copy.
 */
template <typename S>
S Translate(const S& str, const S& from, const S& to) {
  S result;
  result.Reserve(str.Len());
  typename S::ConstIterator it = str.begin();
  typename S::ConstIterator end = str.end();
  int32 to_size = to.Len();
  while (it != end) {
    int32 pos = from.IndexOf(*it);
    if (pos == INVALID_INDEX) {
      result += *it;
    } else {
      if (pos < to_size) {
        result += to[pos];
      }
    }
    ++it;
  }
  return result;
}

template <typename S>
S Translate(const S& str, const typename S::CharType* from,
            const typename S::CharType* to) {
  fun_check_ptr(from);
  fun_check_ptr(to);
  return Translate(str, S(from), S(to));
}

/**
 * Replaces in str all occurrences of characters in from
 * with the corresponding (by position) characters in to.
 * If there is no corresponding character, the character
 * is removed.
 */
template <typename S>
S& TranslateInPlace(S& str, const S& from, const S& to) {
  str = Translate(str, from, to);
  return str;
}

template <typename S>
S TranslateInPlace(S& str, const typename S::CharType* from,
                   const typename S::CharType* to) {
  fun_check_ptr(from);
  fun_check_ptr(to);
  str = Translate(str, S(from), S(to));
#if defined(__SUNPRO_CC)
  // Fix around the RVO bug in SunStudio 12.4
  S ret(str);
  return ret;
#else
  return str;
#endif
}

#if !defined(FUN_NO_TEMPLATE_ICOMPARE)

template <typename S>
S& ReplaceInPlace(S& str, const S& from, const S& to, int32 start = 0) {
  fun_check(from.Len() > 0);

  S result;
  int32 pos = 0;
  result.Append(str, 0, start);
  do {
    pos = str.IndexOf(from, start);
    if (pos != INVALID_INDEX) {
      result.Append(str, start, pos - start);
      result.Append(to);
      start = pos + from.Len();
    } else {
      result.Append(str, start, str.Len() - start);
    }
  } while (pos != INVALID_INDEX);
  str.Swap(result);
  return str;
}

template <typename S>
S& ReplaceInPlace(S& str, const typename S::CharType* from,
                  const typename S::CharType* to, int32 start = 0) {
  fun_check(*from);

  S result;
  int32 pos = 0;
  int32 from_len = std::strlen(from);
  result.Append(str, 0, start);
  do {
    pos = str.find(from, start);
    if (pos != INVALID_INDEX) {
      result.Append(str, start, pos - start);
      result.Append(to);
      start = pos + from_len;
    } else {
      result.Append(str, start, str.Len() - start);
    }
  } while (pos != INVALID_INDEX);
  str.Swap(result);
  return str;
}

template <typename S>
S& ReplaceInPlace(S& str, const typename S::CharType from,
                  const typename S::CharType to = 0, int32 start = 0) {
  if (from == to) {
    return str;
  }

  int32 pos = 0;
  do {
    pos = str.IndexOf(from, start);
    if (pos != INVALID_INDEX) {
      if (to) {
        str[pos] = to;
      } else {
        str.Remove(pos, 1);
      }
    }
  } while (pos != INVALID_INDEX);

  return str;
}

template <typename S>
S& RemoveInPlace(S& str, const typename S::CharType ch, int32 start = 0) {
  return ReplaceInPlace(str, ch, 0, start);
}

/**
 * Replace all occurrences of from (which must not be the empty string)
 * in str with to, starting at position start.
 */
template <typename S>
S Replace(const S& str, const S& from, const S& to, int32 start = 0) {
  S result(str);
  ReplaceInPlace(result, from, to, start);
  return result;
}

template <typename S>
S Replace(const S& str, const typename S::CharType* from,
          const typename S::CharType* to, int32 start = 0) {
  S result(str);
  ReplaceInPlace(result, from, to, start);
  return result;
}

template <typename S>
S Replace(const S& str, const typename S::CharType from,
          const typename S::CharType to = 0, int32 start = 0) {
  S result(str);
  ReplaceInPlace(result, from, to, start);
  return result;
}

template <typename S>
S Remove(const S& str, const typename S::CharType ch, int32 start = 0) {
  S result(str);
  ReplaceInPlace(result, ch, 0, start);
  return result;
}

#else

FUN_BASE_API String Replace(const String& str, const String& from,
                            const String& to, int32 start = 0);
FUN_BASE_API String Replace(const String& str, const String::CharType* from,
                            const String::CharType* to, int32 start = 0);
FUN_BASE_API String Replace(const String& str, const String::CharType from,
                            const String::CharType to = 0, int32 start = 0);
FUN_BASE_API String Remove(const String& str, const String::CharType ch,
                           int32 start = 0);
FUN_BASE_API String& ReplaceInPlace(String& str, const String& from,
                                    const String& to, int32 start = 0);
FUN_BASE_API String& ReplaceInPlace(String& str, const String::CharType* from,
                                    const String::CharType* to,
                                    int32 start = 0);
FUN_BASE_API String& ReplaceInPlace(String& str, const String::CharType from,
                                    const String::CharType to = 0,
                                    int32 start = 0);
FUN_BASE_API String& RemoveInPlace(String& str, const String::CharType ch,
                                   int32 start = 0);

#endif

/**
 * Concatenates two strings.
 */
template <typename S>
S Cat(const S& s1, const S& s2) {
  S result = s1;
  result.Reserve(s1.Len() + s2.Len());
  result.Append(s2);
  return result;
}

/**
 * Concatenates three strings.
 */
template <typename S>
S Cat(const S& s1, const S& s2, const S& s3) {
  S result = s1;
  result.Reserve(s1.Len() + s2.Len() + s3.Len());
  result.Append(s2);
  result.Append(s3);
  return result;
}

/**
 * Concatenates four strings.
 */
template <typename S>
S Cat(const S& s1, const S& s2, const S& s3, const S& s4) {
  S result = s1;
  result.Reserve(s1.Len() + s2.Len() + s3.Len() + s4.Len());
  result.Append(s2);
  result.Append(s3);
  result.Append(s4);
  return result;
}

/**
 * Concatenates five strings.
 */
template <typename S>
S Cat(const S& s1, const S& s2, const S& s3, const S& s4, const S& s5) {
  S result = s1;
  result.Reserve(s1.Len() + s2.Len() + s3.Len() + s4.Len() + s5.Len());
  result.Append(s2);
  result.Append(s3);
  result.Append(s4);
  result.Append(s5);
  return result;
}

/**
 * Concatenates six strings.
 */
template <typename S>
S Cat(const S& s1, const S& s2, const S& s3, const S& s4, const S& s5,
      const S& s6) {
  S result = s1;
  result.Reserve(s1.Len() + s2.Len() + s3.Len() + s4.Len() + s5.Len() +
                 s6.Len());
  result.Append(s2);
  result.Append(s3);
  result.Append(s4);
  result.Append(s5);
  result.Append(s6);
  return result;
}

/**
 * Concatenates a sequence of strings, delimited
 * by the string given in delim.
 */
template <typename S, typename It>
S Cat(const S& delim, const It& begin, const It& end) {
  S result;
  for (It it = begin; it != end; ++it) {
    if (!result.empty()) {
      result.Append(delim);
    }
    result += *it;
  }
  return result;
}

/**
 * Tests whether the string starts with the given prefix.
 */
template <typename S>
bool StartsWith(const S& str, const S& prefix) {
  return str.Len() >= prefix.Len() &&
         equal(prefix.begin(), prefix.end(), str.begin());
}

/**
 * Tests whether the string ends with the given suffix.
 */
template <typename S>
bool EndsWith(const S& str, const S& suffix) {
  return str.Len() >= suffix.Len() &&
         equal(suffix.rbegin(), suffix.rend(), str.rbegin());
}

//
// case-insensitive string equality
//

template <typename charT>
struct i_char_traits : public std::char_traits<charT> {
  FUN_ALWAYS_INLINE static bool eq(charT c1, charT c2) {
    return CharTraitsA::ToLower(c1) == CharTraitsA::ToLower(c2);
  }

  FUN_ALWAYS_INLINE static bool ne(charT c1, charT c2) { return !eq(c1, c2); }

  FUN_ALWAYS_INLINE static bool lt(charT c1, charT c2) {
    return CharTraitsA::ToLower(c1) < CharTraitsA::ToLower(c2);
  }

  static int32 compare(const charT* s1, const charT* s2, std::size_t n) {
    for (int32 i = 0; i < n && s1 && s2; ++i, ++s1, ++s2) {
      if (CharTraitsA::ToLower(*s1) == CharTraitsA::ToLower(*s2)) {
        continue;
      } else if (CharTraitsA::ToLower(*s1) < CharTraitsA::ToLower(*s2)) {
        return -1;
      } else {
        return 1;
      }
    }

    return 0;
  }

  static const charT* find(const charT* s, int32 n, charT a) {
    while (n-- > 0 && CharTraitsA::ToLower(*s) != CharTraitsA::ToLower(a)) {
      ++s;
    }
    return s;
  }
};

/**
 * Case-insensitive String counterpart.
 */
typedef std::basic_string<char, i_char_traits<char> > istring;

/**
 * Case-insensitive substring; searches for a substring
 * without regards to case.
 */
template <typename T>
std::size_t isubstr(const T& str, const T& sought) {
  typename T::ConstIterator it =
      std::search(str.begin(), str.end(), sought.begin(), sought.end(),
                  i_char_traits<typename T::CharType>::eq);

  if (it != str.end()) {
    return it - str.begin();
  } else {
    return static_cast<std::size_t>(T::npos);
  }
}

/**
 * Case-insensitive less-than functor; useful for standard maps
 * and sets with std::strings keys and case-insensitive ordering
 * requirement.
 */
struct CILess {
  FUN_ALWAYS_INLINE bool operator()(const String& s1, const String& s2) const {
    return icompare(s1, s2) < 0;
  }
};

}  // namespace fun
