#pragma once

#include "fun/base/base.h"
#include "fun/base/string/char_traits.h"
#include "fun/base/string/cstring.h"

namespace fun {

template <typename _CharType>
class CStringTraits {
 public:
  typedef _CharType CharType;

  static bool IsPureAnsi(const CharType* str);

  static bool IsNumeric(const CharType* str) {
    return IsNumeric(str, str + Strlen(str));
  }

  static bool IsNumeric(const CharType* it, const CharType* end) {
    if (*it == '-' || *it == '+') {
      ++it;
    }

    bool has_dot = false;
    while (it != end) {
      if (*it == '\0') {
        // What the...
        return false;
      }

      if (*it == '.') {
        if (has_dot) {  // ".." 은 잘못된...
          return false;
        }
        has_dot = true;
      }
      // tail 'f'는 성공으로 취급해야할까??
      else if (!CharTraits<CharType>::IsDigit(*it)) {
        return false;
      }

      ++it;
    }

    return true;
  }

  static int32 Strlen(const CharType* str);

  static CharType* Strcpy(CharType* dst, size_t dst_len, const CharType* src);

  static CharType* Strncpy(CharType* dst, const CharType* src, int32 max_len);

  template <size_t dst_len>
  FUN_ALWAYS_INLINE static CharType* Strcpy(CharType (&dst)[dst_len],
                                            const CharType* src) {
    return Strcpy(dst, dst_len, src);
  }

  static CharType* Strcat(CharType* dst, size_t dst_len, const CharType* src);

  template <size_t dst_len>
  FUN_ALWAYS_INLINE static CharType* Strcat(CharType (&dst)[dst_len],
                                            const CharType* src) {
    return Strcat(dst, dst_len, src);
  }

  FUN_ALWAYS_INLINE static CharType* Strncat(CharType* dst, const CharType* src,
                                             int32 max_len) {
    const int32 len = Strlen(dst);
    CharType* new_dest = dst + len;
    if ((max_len -= len) > 0) {
      Strncpy(new_dest, src, max_len);
    }
    return dst;
  }

  static CharType* Strupr(CharType* dst, size_t dst_len);

  template <size_t dst_len>
  FUN_ALWAYS_INLINE static CharType* Strupr(CharType (&dst)[dst_len]) {
    return Strupr(dst, dst_len);
  }

  static int32 Strcmp(const CharType* str1, const CharType* str2);
  static int32 Strncmp(const CharType* str1, const CharType* str2, size_t len);
  static int32 Stricmp(const CharType* str1, const CharType* str2);
  static int32 Strnicmp(const CharType* str1, const CharType* str2, size_t len);

  static const CharType* Strfind(const CharType* str, const CharType* find);
  static const CharType* StrfindDelim(const CharType* str, const CharType* find,
                                      const CharType* delim = LITERAL(CharType,
                                                                      " \t,"));
  static const CharType* Stristr(const CharType* str, const CharType* find);
  static CharType* Stristr(CharType* str, const CharType* find) {
    return (CharType*)Stristr((const CharType*)str, find);
  }

  static const CharType* Strstr(const CharType* str, const CharType* find);
  static CharType* Strstr(CharType* str, const CharType* find);

  static const CharType* Strchr(const CharType* str, CharType ch);
  static CharType* Strchr(CharType* str, CharType ch);

  static const CharType* Strrchr(const CharType* str, CharType ch);
  static CharType* Strrchr(CharType* str, CharType ch);

  static const CharType* Strrstr(const CharType* str, const CharType* find);
  static CharType* Strrstr(CharType* str, const CharType* find);

  static CharType* Strtok(CharType* str, const CharType* delim,
                          CharType** context);

  static int32 Atoi(const CharType* str);
  static int64 Atoi64(const CharType* str);
  static float Atof(const CharType* str);
  static double Atod(const CharType* str);

  static bool ToBool(const CharType* str);

  static int32 Strtoi(const CharType* start, CharType** end, int32 base);
  static uint64 Strtoui64(const CharType* start, CharType** end, int32 base);

  static const CharType* Spc(int32 len);
  static const CharType* Tab(int32 len);

  /**
   * 대소문자 구분안함.
   *
   * '_' 문자는 무시함.  '_' 문자가 연달아 오는 경우에는 무시하지 않는다??
   *
   * "darkColor" 와 "dark_color"는 같은것으로 취급하기 위해서 사용함.
   */
  static int32 LenientlyCompare(const CharType* str1, const CharType* str2);

  // TODO 아래 함수는 가급적 GenericCString 쪽으로 옮겨 주도록 하자!
  // TODO 아래 함수는 가급적 GenericCString 쪽으로 옮겨 주도록 하자!
  // TODO 아래 함수는 가급적 GenericCString 쪽으로 옮겨 주도록 하자!
  // TODO 아래 함수는 가급적 GenericCString 쪽으로 옮겨 주도록 하자!
  // TODO 아래 함수는 가급적 GenericCString 쪽으로 옮겨 주도록 하자!

  // TODO

  // FUN_ALWAYS_INLINE static float Atof(const CharType* str, bool* ok =
  // nullptr)
  //{
  //  return Strtof(str, str + Strlen(str), nullptr, ok);
  //}
  // FUN_ALWAYS_INLINE static double Atod(const CharType* str, bool* ok =
  // nullptr)
  //{
  //  return Strtod(str, str + Strlen(str), nullptr, ok);
  //}
  //
  // FUN_ALWAYS_INLINE static float Strtof(const CharType* str, CharType**
  // end_ptr, bool* ok = nullptr)
  //{
  //  return Strtof(str, str + Strlen(str), end_ptr, ok);
  //}
  // FUN_ALWAYS_INLINE static double Strtod(const CharType* str, CharType**
  // end_ptr, bool* ok = nullptr)
  //{
  //  return Strtod(str, str + Strlen(str), end_ptr, ok);
  //}
  //
  // FUN_BASE_API static float Strtof(const CharType* str, const CharType* end,
  // CharType** end_ptr, bool* ok = nullptr); FUN_BASE_API static double
  // Strtod(const CharType* str, const CharType* end, CharType** end_ptr, bool*
  // ok = nullptr);
  //
  // FUN_ALWAYS_INLINE static int32 Atoi(const CharType* str, bool* ok =
  // nullptr)
  //{
  //  return Strtoi32(str, str + Strlen(str), nullptr, 0, ok);
  //}
  // FUN_ALWAYS_INLINE static int64 Atoi64(const CharType* str, bool* ok =
  // nullptr)
  //{
  //  return Strtoi64(str, str + Strlen(str), nullptr, 0, ok);
  //}
  // FUN_ALWAYS_INLINE static uint32 Atoui(const CharType* str, bool* ok =
  // nullptr)
  //{
  //  return Strtoui32(str, str + Strlen(str), nullptr, 0, ok);
  //}
  // FUN_ALWAYS_INLINE static uint64 Atoui64(const CharType* str, bool* ok =
  // nullptr)
  //{
  //  return Strtoui64(str, str + Strlen(str), nullptr, 0, ok);
  //}
  //
  // static int32 Strtoi(const CharType* str, CharType** end_ptr, int32 base,
  // bool* ok = nullptr); static int64 Strtoi64(const CharType* str, CharType**
  // end_ptr, int32 base, bool* ok = nullptr); static uint32 Strtoui(const
  // CharType* str, CharType** end_ptr, int32 base, bool* ok = nullptr); static
  // uint64 Strtoui64(const CharType* str, CharType** end_ptr, int32 base, bool*
  // ok = nullptr);
  //
  // static int32 Strtoi(const CharType* str, const CharType* end, CharType**
  // end_ptr, int32 base, bool* ok = nullptr); static uint32 Strtoui(const
  // CharType* str, const CharType* end, CharType** end_ptr, int32 base, bool* ok
  // = nullptr); static int64 Strtoi64(const CharType* str, const CharType* end,
  // CharType** end_ptr, int32 base, bool* ok = nullptr); static uint64
  // Strtoui64(const CharType* str, const CharType* end, CharType** end_ptr,
  // int32 base, bool* ok = nullptr);
  //
  // FUN_BASE_API static int64 Strtoi64Impl(const CharType* str, const CharType*
  // end, CharType** end_ptr, int32 base); FUN_BASE_API static int64
  // Strtoui64Impl(const CharType* str, const CharType* end, CharType** end_ptr,
  // int32 base);
};

typedef CStringTraits<char> CStringTraitsA;
typedef CStringTraits<UNICHAR> CStringTraitsU;

//
// Implementations
//

template <typename CharType>
struct CStringData {
  /** Number of characters to be stored in string. */
  static const int32 MAX_SPACES = 255;

  /** Number of tabs to be stored in string. */
  static const int32 MAX_TABS = 255;

  static FUN_BASE_API const CharType SpcArray[MAX_SPACES + 1];
  static FUN_BASE_API const CharType TabArray[MAX_TABS + 1];
};

template <typename T>
const typename CStringTraits<T>::CharType* CStringTraits<T>::Spc(int32 len) {
  fun_check(len >= 0 && len <= CStringData<T>::MAX_SPACES);
  return CStringData<T>::SpcArray + CStringData<T>::MAX_SPACES - len;
}

template <typename T>
const typename CStringTraits<T>::CharType* CStringTraits<T>::Tab(int32 len) {
  fun_check(len >= 0 && len <= CStringData<T>::MAX_TABS);
  return CStringData<T>::TabArray + CStringData<T>::MAX_TABS - len;
}

struct FUN_BASE_API ToBoolHelper {
  static bool FromString(const char* str);
  static bool FromString(const UNICHAR* str);
};

template <typename T>
int32 CStringTraits<T>::LenientlyCompare(const CharType* str1,
                                         const CharType* str2) {
  while (true) {
    while (*str1 && *str1 == LITERAL(CharType, '_')) {
      ++str1;
    }
    while (*str2 && *str2 == LITERAL(CharType, '_')) {
      ++str2;
    }

    const int32 diff = CharTraits<CharType>::ToUpper(*str2) -
                       CharTraits<CharType>::ToUpper(*str1);
    if (diff != 0) {
      return diff < 0 ? -1 : (diff > 0 ? 0 : +1);
    }

    ++str1;
    ++str2;

    while (*str1 && *str1 == LITERAL(CharType, '_')) {
      ++str1;
    }
    while (*str2 && *str2 == LITERAL(CharType, '_')) {
      ++str2;
    }

    if (*str1 == '\0' && *str2 != '\0') {
      return -1;
    } else if (*str1 != '\0' && *str2 == '\0') {
      return +1;
    } else if (*str1 == '\0' && *str2 == '\0') {
      break;
    }
  }

  return 0;
}

/**
 * find string in string, case insensitive, requires non-alphanumeric lead-in.
 */
template <typename T>
const typename CStringTraits<T>::CharType* CStringTraits<T>::Strfind(
    const CharType* str, const CharType* find) {
  if (find == nullptr || str == nullptr) {
    return nullptr;
  }

  bool is_alnum = false;
  const CharType f =
      (*find < LITERAL(CharType, 'a') || *find > LITERAL(CharType, 'z'))
          ? (*find)
          : (*find + LITERAL(CharType, 'A') - LITERAL(CharType, 'a'));
  const int32 len = Strlen(find++) - 1;
  CharType ch = *str++;
  while (ch) {
    if (ch >= LITERAL(CharType, 'a') && ch <= LITERAL(CharType, 'z')) {
      ch += LITERAL(CharType, 'A') - LITERAL(CharType, 'a');
    }

    if (!is_alnum && ch == f && Strnicmp(str, find, len) == 0) {
      return str - 1;
    }

    is_alnum = (ch >= LITERAL(CharType, 'A') && ch <= LITERAL(CharType, 'Z')) ||
               (ch >= LITERAL(CharType, '0') && ch <= LITERAL(CharType, '9'));
    ch = *str++;
  }

  return nullptr;
}

template <typename T>
const typename CStringTraits<T>::CharType* CStringTraits<T>::StrfindDelim(
    const CharType* str, const CharType* find, const CharType* delim) {
  if (find == nullptr || str == nullptr) {
    return nullptr;
  }

  const int32 len = Strlen(find);
  const T* found = Stristr(str, find);
  if (found) {
    // check if this occurrence is delimited correctly
    if ((found == str ||
         Strchr(delim, found[-1]) !=
             nullptr) &&  // either first char, or following a delim
        (found[len] == LITERAL(CharType, '\0') ||
         Strchr(delim, found[len]) !=
             nullptr)) {  // either last or with a delim following
      return found;
    }

    // start searching again after the first matched character
    for (;;) {
      str = found + 1;
      found = Stristr(str, find);
      if (found == nullptr) {
        return nullptr;
      }

      // check if the next occurrence is delimited correctly
      if ((Strchr(delim, found[-1]) !=
           nullptr) &&  // match is following a delim
          (found[len] == LITERAL(CharType, '\0') ||
           Strchr(delim, found[len]) !=
               nullptr)) {  // either last or with a delim following
        return found;
      }
    }
  }

  return nullptr;
}

template <typename T>
const typename CStringTraits<T>::CharType* CStringTraits<T>::Stristr(
    const CharType* str, const CharType* find) {
  // both strings must be valid
  if (find == nullptr || str == nullptr) {
    return nullptr;
  }

  // Get upper-case first letter of the find string
  // (to reduce the number of full strnicmps)
  CharType find_initial = CharTraits<CharType>::ToUpper(*find);
  // Get length of find string, and increment past first letter
  const int32 len = Strlen(find++) - 1;
  // Get the first letter of the search string, and increment past it
  CharType str_char = *str++;
  // While we aren't at end of string...
  while (str_char) {
    // Make sure it's upper-case
    str_char = CharTraits<CharType>::ToUpper(str_char);
    // If it matches the first letter of the find string,
    // do a case-insensitive string compare for the length of the find string
    if (str_char == find_initial && !Strnicmp(str, find, len)) {
      // If we found the string, then return a pointer to
      // the beginning of it in the search string
      return str - 1;
    }
    // Go to next letter
    str_char = *str++;
  }

  // If nothing was found, return NULL
  return nullptr;
}

template <typename T>
FUN_ALWAYS_INLINE typename CStringTraits<T>::CharType* CStringTraits<T>::Strcpy(
    CharType* dst, size_t dst_len, const CharType* src) {
  return CString::Strcpy(dst, dst_len, src);
}

template <typename T>
FUN_ALWAYS_INLINE typename CStringTraits<T>::CharType*
CStringTraits<T>::Strncpy(CharType* dst, const CharType* src, int32 max_len) {
  fun_check(max_len > 0);
  CString::Strncpy(dst, src, max_len);
  return dst;
}

template <typename T>
FUN_ALWAYS_INLINE typename CStringTraits<T>::CharType* CStringTraits<T>::Strcat(
    CharType* dst, size_t dst_len, const CharType* src) {
  return CString::Strcat(dst, dst_len, src);
}

template <typename T>
FUN_ALWAYS_INLINE typename CStringTraits<T>::CharType* CStringTraits<T>::Strupr(
    CharType* dst, size_t dst_len) {
  return CString::Strupr(dst, dst_len);
}

template <typename T>
FUN_ALWAYS_INLINE int32 CStringTraits<T>::Strcmp(const CharType* str1,
                                                 const CharType* str2) {
  return CString::Strcmp(str1, str2);
}

template <typename T>
FUN_ALWAYS_INLINE int32 CStringTraits<T>::Strncmp(const CharType* str1,
                                                  const CharType* str2,
                                                  size_t len) {
  return CString::Strncmp(str1, str2, len);
}

template <typename T>
FUN_ALWAYS_INLINE int32 CStringTraits<T>::Stricmp(const CharType* str1,
                                                  const CharType* str2) {
  return CString::Stricmp(str1, str2);
}

template <typename T>
FUN_ALWAYS_INLINE int32 CStringTraits<T>::Strnicmp(const CharType* str1,
                                                   const CharType* str2,
                                                   size_t len) {
  return CString::Strnicmp(str1, str2, len);
}

template <typename T>
FUN_ALWAYS_INLINE int32 CStringTraits<T>::Strlen(const CharType* str) {
  return CString::Strlen(str);
}

template <typename T>
FUN_ALWAYS_INLINE const typename CStringTraits<T>::CharType*
CStringTraits<T>::Strstr(const CharType* str, const CharType* find) {
  return CString::Strstr(str, find);
}

template <typename T>
FUN_ALWAYS_INLINE typename CStringTraits<T>::CharType* CStringTraits<T>::Strstr(
    CharType* str, const CharType* find) {
  return (CharType*)CString::Strstr(str, find);
}

template <typename T>
FUN_ALWAYS_INLINE const typename CStringTraits<T>::CharType*
CStringTraits<T>::Strchr(const CharType* str, CharType ch) {
  return CString::Strchr(str, ch);
}

template <typename T>
FUN_ALWAYS_INLINE typename CStringTraits<T>::CharType* CStringTraits<T>::Strchr(
    CharType* str, CharType ch) {
  return (CharType*)CString::Strchr(str, ch);
}

template <typename T>
FUN_ALWAYS_INLINE const typename CStringTraits<T>::CharType*
CStringTraits<T>::Strrchr(const CharType* str, CharType ch) {
  return CString::Strrchr(str, ch);
}

template <typename T>
FUN_ALWAYS_INLINE typename CStringTraits<T>::CharType*
CStringTraits<T>::Strrchr(CharType* str, CharType ch) {
  return (CharType*)CString::Strrchr(str, ch);
}

template <typename T>
FUN_ALWAYS_INLINE const typename CStringTraits<T>::CharType*
CStringTraits<T>::Strrstr(const CharType* str, const CharType* find) {
  return Strrstr((CharType*)str, find);
}

template <typename T>
FUN_ALWAYS_INLINE typename CStringTraits<T>::CharType*
CStringTraits<T>::Strrstr(CharType* str, const CharType* find) {
  if (*find == (CharType)0) {
    return str + Strlen(str);
  }

  CharType* result = nullptr;
  for (;;) {
    CharType* found = Strstr(str, find);
    if (found == nullptr) {
      return result;
    }

    result = found;
    str = found + 1;
  }
}

template <typename T>
FUN_ALWAYS_INLINE int32 CStringTraits<T>::Atoi(const CharType* str) {
  return CString::Atoi(str);
}

template <typename T>
FUN_ALWAYS_INLINE int64 CStringTraits<T>::Atoi64(const CharType* str) {
  return CString::Atoi64(str);
}

template <typename T>
FUN_ALWAYS_INLINE float CStringTraits<T>::Atof(const CharType* str) {
  return CString::Atof(str);
}

template <typename T>
FUN_ALWAYS_INLINE double CStringTraits<T>::Atod(const CharType* str) {
  return CString::Atod(str);
}

template <typename T>
FUN_ALWAYS_INLINE int32 CStringTraits<T>::Strtoi(const CharType* start,
                                                 CharType** end, int32 base) {
  return CString::Strtoi(start, end, base);
}

template <typename T>
FUN_ALWAYS_INLINE uint64 CStringTraits<T>::Strtoui64(const CharType* start,
                                                     CharType** end,
                                                     int32 base) {
  return CString::Strtoui64(start, end, base);
}

template <typename T>
FUN_ALWAYS_INLINE typename CStringTraits<T>::CharType* CStringTraits<T>::Strtok(
    CharType* str, const CharType* delim, CharType** context) {
  return CString::Strtok(str, delim, context);
}

//
// CStringTraits<UNICHAR> specializations
//

template <>
FUN_ALWAYS_INLINE bool CStringTraits<UNICHAR>::IsPureAnsi(const UNICHAR* str) {
  for (; *str; ++str) {
    if (*str > 0x7f) {
      return false;
    }
  }
  return true;
}

template <>
FUN_ALWAYS_INLINE bool CStringTraits<UNICHAR>::ToBool(const UNICHAR* str) {
  // TODO
  return false;
  // return ToBoolHelper::FromString(str);
}

//
// CStringTraits<char> specializations
//

template <>
FUN_ALWAYS_INLINE bool CStringTraits<char>::IsPureAnsi(const char* str) {
  // TODO UTF8 베이스일 경우에는 문자 하나하나를 체크하는게 좋을듯...
  const uint8* ustr = (const uint8*)str;
  for (; *ustr; ++ustr) {
    if (*ustr > 0x7f) {
      return false;
    }
  }
  return true;
}

template <>
FUN_ALWAYS_INLINE bool CStringTraits<char>::ToBool(const char* str) {
  return ToBoolHelper::FromString(str);
}

//주의 : 인코딩은 고려안함. (내부적으로 인코딩 변환없이 캐스팅 후 비교함)
struct StringCmp {
  FUN_BASE_API static int32 Compare(
      const char* str1, int32 str1_len, const char* str2, int32 str2_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive);
  FUN_BASE_API static int32 Compare(
      const char* str1, const char* str2, int32 len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive);

  FUN_BASE_API static int32 Compare(
      const UNICHAR* str1, int32 str1_len, const UNICHAR* str2, int32 str2_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive);
  FUN_BASE_API static int32 Compare(
      const UNICHAR* str1, const UNICHAR* str2, int32 len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive);

  FUN_BASE_API static int32 Compare(
      const UNICHAR* str1, int32 str1_len, const char* str2, int32 str2_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive);
  FUN_BASE_API static int32 Compare(
      const UNICHAR* str1, const char* str2, int32 len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive);

  FUN_BASE_API static int32 Compare(
      const char* str1, const char* str2,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive);
  FUN_BASE_API static int32 Compare(
      const UNICHAR* str1, const UNICHAR* str2,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive);

  FUN_BASE_API static int32 Compare(
      const char* str1, const UNICHAR* str2,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive);
  FUN_BASE_API static int32 Compare(
      const UNICHAR* str1, const char* str2,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive);

  static FUN_ALWAYS_INLINE int32
  Compare(const char* str1, int32 str1_len, const UNICHAR* str2, int32 str2_len,
          CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return -Compare(str2, str2_len, str1, str1_len, casesense);
  }
  static FUN_ALWAYS_INLINE int32
  Compare(const char* str1, const UNICHAR* str2, int32 len,
          CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return -Compare(str2, str1, len, casesense);
  }

  // 두 길이중 작은 길이만큼 비교를 하였으나, 동일할 경우 단순 길이 비교하는데
  // 쓰임.
  static FUN_ALWAYS_INLINE int32 CompareLength(int32 len1, int32 len2) {
    return len1 == len2 ? 0 : (len1 > len2 ? +1 : -1);
  }
  // 두 포인터중 null이 있을 경우 단순 포인터 비교에 쓰임.
  static FUN_ALWAYS_INLINE int32 ComparePointer(const void* ptr1,
                                                const void* ptr2) {
    return ptr1 ? +1 : (ptr2 ? -1 : 0);
  }

  static bool StartsWith(
      const UNICHAR* str, int32 str_len, const UNICHAR* prefix,
      int32 prefix_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive);
  static bool StartsWith(
      const UNICHAR* str, int32 str_len, const char* prefix, int32 prefix_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive);
  static bool StartsWith(
      const char* str, int32 str_len, const UNICHAR* prefix, int32 prefix_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive);
  static bool StartsWith(
      const char* str, int32 str_len, const char* prefix, int32 prefix_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive);
  static bool EndsWith(
      const UNICHAR* str, int32 str_len, const UNICHAR* suffix,
      int32 suffix_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive);
  static bool EndsWith(
      const UNICHAR* str, int32 str_len, const char* suffix, int32 suffix_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive);
  static bool EndsWith(
      const char* str, int32 str_len, const UNICHAR* suffix, int32 suffix_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive);
  static bool EndsWith(
      const char* str, int32 str_len, const char* suffix, int32 suffix_len,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive);

  FUN_ALWAYS_INLINE static bool StartsWith(
      const UNICHAR* str, const UNICHAR* prefix,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return StartsWith(str, -1, prefix, -1, casesense);
  }
  FUN_ALWAYS_INLINE static bool StartsWith(
      const UNICHAR* str, const char* prefix,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return StartsWith(str, -1, prefix, -1, casesense);
  }
  FUN_ALWAYS_INLINE static bool StartsWith(
      const char* str, const UNICHAR* prefix,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return StartsWith(str, -1, prefix, -1, casesense);
  }
  FUN_ALWAYS_INLINE static bool StartsWith(
      const char* str, const char* prefix,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return StartsWith(str, -1, prefix, -1, casesense);
  }
  FUN_ALWAYS_INLINE static bool EndsWith(
      const UNICHAR* str, const UNICHAR* suffix,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return StartsWith(str, -1, suffix, -1, casesense);
  }
  FUN_ALWAYS_INLINE static bool EndsWith(
      const UNICHAR* str, const char* suffix,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return StartsWith(str, -1, suffix, -1, casesense);
  }
  FUN_ALWAYS_INLINE static bool EndsWith(
      const char* str, const UNICHAR* suffix,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return StartsWith(str, -1, suffix, -1, casesense);
  }
  FUN_ALWAYS_INLINE static bool EndsWith(
      const char* str, const char* suffix,
      CaseSensitivity casesense = CaseSensitivity::CaseSensitive) {
    return StartsWith(str, -1, suffix, -1, casesense);
  }
};

FUN_ALWAYS_INLINE bool StringCmp::StartsWith(const UNICHAR* str, int32 str_len,
                                             const UNICHAR* prefix,
                                             int32 prefix_len,
                                             CaseSensitivity casesense) {
  // Cause some violation if length is specified when pointer is null.
  fun_check(str != nullptr || str_len == 0);
  fun_check(prefix != nullptr || prefix_len == 0);

  if (str_len < 0) {
    str_len = CStringTraits<UNICHAR>::Strlen(str);
  }

  if (prefix_len < 0) {
    prefix_len = CStringTraits<UNICHAR>::Strlen(prefix);
  }

  if (str_len == 0) {
    return prefix_len == 0;
  }

  if (str_len < prefix_len) {
    return false;
  }

  const int32 cmp_len = prefix_len;
  return Compare(str, cmp_len, prefix, cmp_len, casesense) == 0;
}

FUN_ALWAYS_INLINE bool StringCmp::StartsWith(const UNICHAR* str, int32 str_len,
                                             const char* prefix,
                                             int32 prefix_len,
                                             CaseSensitivity casesense) {
  // Cause some violation if length is specified when pointer is null.
  fun_check(str != nullptr || str_len == 0);
  fun_check(prefix != nullptr || prefix_len == 0);

  if (str_len < 0) {
    str_len = CStringTraits<UNICHAR>::Strlen(str);
  }

  if (prefix_len < 0) {
    prefix_len = CStringTraits<char>::Strlen(prefix);
  }

  if (str_len == 0) {
    return prefix_len == 0;
  }

  if (str_len < prefix_len) {
    return false;
  }

  const int32 cmp_len = prefix_len;
  return Compare(str, cmp_len, prefix, cmp_len, casesense) == 0;
}

FUN_ALWAYS_INLINE bool StringCmp::StartsWith(const char* str, int32 str_len,
                                             const UNICHAR* prefix,
                                             int32 prefix_len,
                                             CaseSensitivity casesense) {
  // Cause some violation if length is specified when pointer is null.
  fun_check(str != nullptr || str_len == 0);
  fun_check(prefix != nullptr || prefix_len == 0);

  if (str_len < 0) {
    str_len = CStringTraits<char>::Strlen(str);
  }

  if (prefix_len < 0) {
    prefix_len = CStringTraits<UNICHAR>::Strlen(prefix);
  }

  if (str_len == 0) {
    return prefix_len == 0;
  }

  if (str_len < prefix_len) {
    return false;
  }

  const int32 cmp_len = prefix_len;
  return Compare(str, cmp_len, prefix, cmp_len, casesense) == 0;
}

FUN_ALWAYS_INLINE bool StringCmp::StartsWith(const char* str, int32 str_len,
                                             const char* prefix,
                                             int32 prefix_len,
                                             CaseSensitivity casesense) {
  // Cause some violation if length is specified when pointer is null.
  fun_check(str != nullptr || str_len == 0);
  fun_check(prefix != nullptr || prefix_len == 0);

  if (str_len < 0) {
    str_len = CStringTraits<char>::Strlen(str);
  }

  if (prefix_len < 0) {
    prefix_len = CStringTraits<char>::Strlen(prefix);
  }

  if (str_len == 0) {
    return prefix_len == 0;
  }

  if (str_len < prefix_len) {
    return false;
  }

  const int32 cmp_len = prefix_len;
  return Compare(str, cmp_len, prefix, cmp_len, casesense) == 0;
}

FUN_ALWAYS_INLINE bool StringCmp::EndsWith(const UNICHAR* str, int32 str_len,
                                           const UNICHAR* suffix,
                                           int32 suffix_len,
                                           CaseSensitivity casesense) {
  // Cause some violation if length is specified when pointer is null.
  fun_check(str != nullptr || str_len == 0);
  fun_check(suffix != nullptr || suffix_len == 0);

  if (str_len < 0) {
    str_len = CStringTraits<UNICHAR>::Strlen(str);
  }

  if (suffix_len < 0) {
    suffix_len = CStringTraits<UNICHAR>::Strlen(suffix);
  }

  if (str_len == 0) {
    return suffix_len == 0;
  }

  if (str_len < suffix_len) {
    return false;
  }

  const int32 cmp_len = suffix_len;
  return Compare(str + str_len - suffix_len, cmp_len, suffix, cmp_len,
                 casesense) == 0;
}

FUN_ALWAYS_INLINE bool StringCmp::EndsWith(const UNICHAR* str, int32 str_len,
                                           const char* suffix, int32 suffix_len,
                                           CaseSensitivity casesense) {
  // Cause some violation if length is specified when pointer is null.
  fun_check(str != nullptr || str_len == 0);
  fun_check(suffix != nullptr || suffix_len == 0);

  if (str_len < 0) {
    str_len = CStringTraits<UNICHAR>::Strlen(str);
  }

  if (suffix_len < 0) {
    suffix_len = CStringTraits<char>::Strlen(suffix);
  }

  if (str_len == 0) {
    return suffix_len == 0;
  }

  if (str_len < suffix_len) {
    return false;
  }

  const int32 cmp_len = suffix_len;
  return Compare(str + str_len - suffix_len, cmp_len, suffix, cmp_len,
                 casesense) == 0;
}

FUN_ALWAYS_INLINE bool StringCmp::EndsWith(const char* str, int32 str_len,
                                           const UNICHAR* suffix,
                                           int32 suffix_len,
                                           CaseSensitivity casesense) {
  // Cause some violation if length is specified when pointer is null.
  fun_check(str != nullptr || str_len == 0);
  fun_check(suffix != nullptr || suffix_len == 0);

  if (str_len < 0) {
    str_len = CStringTraits<char>::Strlen(str);
  }

  if (suffix_len < 0) {
    suffix_len = CStringTraits<UNICHAR>::Strlen(suffix);
  }

  if (str_len == 0) {
    return suffix_len == 0;
  }

  if (str_len < suffix_len) {
    return false;
  }

  const int32 cmp_len = suffix_len;
  return Compare(str + str_len - suffix_len, cmp_len, suffix, cmp_len,
                 casesense) == 0;
}

FUN_ALWAYS_INLINE bool StringCmp::EndsWith(const char* str, int32 str_len,
                                           const char* suffix, int32 suffix_len,
                                           CaseSensitivity casesense) {
  // Cause some violation if length is specified when pointer is null.
  fun_check(str != nullptr || str_len == 0);
  fun_check(suffix != nullptr || suffix_len == 0);

  if (str_len < 0) {
    str_len = CStringTraits<char>::Strlen(str);
  }

  if (suffix_len < 0) {
    suffix_len = CStringTraits<char>::Strlen(suffix);
  }

  if (str_len == 0) {
    return suffix_len == 0;
  }

  if (str_len < suffix_len) {
    return false;
  }

  const int32 cmp_len = suffix_len;
  return Compare(str + str_len - suffix_len, cmp_len, suffix, cmp_len,
                 casesense) == 0;
}

}  // namespace fun
