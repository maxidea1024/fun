#include "fun/base/str.h"

namespace fun {

#if defined(FUN_NO_TEMPLATE_ICOMPARE)

int32 icompare(const String& str, int32 pos, int32 n, String::ConstIterator it2,
               String::ConstIterator end2) {
  int32 sz = str.Len();
  if (pos > sz) {
    pos = sz;
  }
  if (pos + n > sz) {
    n = sz - pos;
  }

  String::ConstIterator it1 = str.begin() + pos;
  String::ConstIterator end1 = str.begin() + pos + n;
  while (it1 != end1 && it2 != end2) {
    String::CharType c1 = CharTraitsA::ToLower(*it1);
    String::CharType c2 = CharTraitsA::ToLower(*it2);
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

int32 icompare(const String& str1, const String& str2) {
  return icompare(str1, 0, str1.Len(), str2.begin(), str2.end());
}

int32 icompare(const String& str1, int32 n1, const String& str2, int32 n2) {
  if (n2 > str2.Len()) {
    n2 = str2.Len();
  }
  return icompare(str1, 0, n1, str2.begin(), str2.begin() + n2);
}

int32 icompare(const String& str1, int32 n, const String& str2) {
  if (n > str2.Len()) {
    n = str2.Len();
  }
  return icompare(str1, 0, n, str2.begin(), str2.begin() + n);
}

int32 icompare(const String& str1, int32 pos, int32 n, const String& str2) {
  return icompare(str1, pos, n, str2.begin(), str2.end());
}

int32 icompare(const String& str1, int32 pos1, int32 n1, const String& str2,
               int32 pos2, int32 n2) {
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

int32 icompare(const String& str1, int32 pos1, int32 n, const String& str2,
               int32 pos2) {
  int32 sz2 = str2.Len();
  if (pos2 > sz2) {
    pos2 = sz2;
  }
  if (pos2 + n > sz2) {
    n = sz2 - pos2;
  }
  return icompare(str1, pos1, n, str2.begin() + pos2, str2.begin() + pos2 + n);
}

int32 icompare(const String& str, int32 pos, int32 n,
               const String::CharType* ptr) {
  fun_check_ptr(ptr);
  int32 sz = str.Len();
  if (pos > sz) {
    pos = sz;
  }
  if (pos + n > sz) {
    n = sz - pos;
  }
  String::ConstIterator it = str.begin() + pos;
  String::ConstIterator end = str.begin() + pos + n;
  while (it != end && *ptr) {
    String::CharType c1 = CharTraitsA::ToLower(*it);
    String::CharType c2 = CharTraitsA::ToLower(*ptr);
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

int32 icompare(const String& str, int32 pos, const String::CharType* ptr) {
  return icompare(str, pos, str.Len() - pos, ptr);
}

int32 icompare(const String& str, const String::CharType* ptr) {
  return icompare(str, 0, str.Len(), ptr);
}

String Replace(const String& str, const String& from, const String& to,
               int32 start) {
  String result(str);
  ReplaceInPlace(result, from, to, start);
  return result;
}

String Replace(const String& str, const String::CharType* from,
               const String::CharType* to, int32 start) {
  String result(str);
  ReplaceInPlace(result, from, to, start);
  return result;
}

String Replace(const String& str, const String::CharType from,
               const String::CharType to, int32 start) {
  String result(str);
  ReplaceInPlace(result, from, to, start);
  return result;
}

String Remove(const String& str, const String::CharType ch, int32 start) {
  String result(str);
  ReplaceInPlace(result, ch, 0, start);
  return result;
}

String& ReplaceInPlace(String& str, const String& from, const String& to,
                       int32 start) {
  fun_check(from.Len() > 0);

  String result;
  int32 pos = 0;
  result.Append(str, 0, start);
  do {
    pos = str.find(from, start);
    if (pos != String::npos) {
      result.Append(str, start, pos - start);
      result.Append(to);
      start = pos + from.length();
    } else {
      result.Append(str, start, str.Len() - start);
    }
  } while (pos != String::npos);
  str.Swap(result);
  return str;
}

String& ReplaceInPlace(String& str, const String::CharType* from,
                       const String::CharType* to, int32 start) {
  fun_check(*from);

  String result;
  int32 pos = 0;
  int32 from_len = std::strlen(from);
  result.Append(str, 0, start);
  do {
    pos = str.find(from, start);
    if (pos != String::npos) {
      result.Append(str, start, pos - start);
      result.Append(to);
      start = pos + from_len;
    } else {
      result.Append(str, start, str.Len() - start);
    }
  } while (pos != String::npos);
  str.Swap(result);
  return str;
}

String& ReplaceInPlace(String& str, const String::CharType from,
                       const String::CharType to, int32 start) {
  if (from == to) {
    return str;
  }

  int32 pos = 0;
  do {
    pos = str.find(from, start);
    if (pos != String::npos) {
      if (to) {
        str[pos] = to;
      } else {
        str.erase(pos, 1);
      }
    }
  } while (pos != String::npos);

  return str;
}

String& RemoveInPlace(String& str, const String::CharType ch, int32 start) {
  return ReplaceInPlace(str, ch, 0, start);
}

#endif

}  // namespace fun
