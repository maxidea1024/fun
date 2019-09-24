#include "fun/base/string/cstring_traits.h"
#include "fun/base/string/string_conversion.h"

namespace fun {

template <>
const char CStringData<char>::SpcArray[MAX_SPACES + 1] =
  "                                                                "
  "                                                                "
  "                                                                "
  "                                                               ";

template <>
const UNICHAR CStringData<UNICHAR>::SpcArray[MAX_SPACES + 1] =
  UTEXT("                                                                ")
  UTEXT("                                                                ")
  UTEXT("                                                                ")
  UTEXT("                                                               ");

template <>
const char CStringData<char>::TabArray[MAX_TABS + 1] =
  "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"
  "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"
  "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"
  "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";

template <>
const UNICHAR CStringData<UNICHAR>::TabArray[MAX_TABS + 1] =
  UTEXT("\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t")
  UTEXT("\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t")
  UTEXT("\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t")
  UTEXT("\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t");

bool ToBoolHelper::FromString(const char* str) {
  return ToBoolHelper::FromString(UTF8_TO_UNICHAR(str));
}

bool ToBoolHelper::FromString(const UNICHAR* str) {
  if (CStringTraitsU::Stricmp(str, UTEXT("true")) == 0 ||
      CStringTraitsU::Stricmp(str, UTEXT("yes")) == 0 ||
      CStringTraitsU::Stricmp(str, UTEXT("on")) == 0
      //||  CStringTraitsU::Stricmp(str, *(GTrue.ToString())) == 0
      //||  CStringTraitsU::Stricmp(str, *(GYes.ToString())) == 0
  ) {
    return true;
  } else if (CStringTraitsU::Stricmp(str, UTEXT("false")) == 0 ||
             CStringTraitsU::Stricmp(str, UTEXT("no")) == 0 ||
             CStringTraitsU::Stricmp(str, UTEXT("off")) == 0
             //||  CStringTraitsU::Stricmp(str, *(GFalse.ToString())) == 0
             //||  CStringTraitsU::Stricmp(str, *(GNo.ToString())) == 0
  ) {
    return false;
  } else {
    return !!CStringTraitsU::Atoi(str);
  }
}

//
// StringCmp
//

namespace {
FUN_ALWAYS_INLINE UNICHAR FoldCase(UNICHAR ch) {
  return !CharTraitsU::IsLower(ch) ? CharTraitsU::ToUpper(ch) : ch;
}
}  // namespace

int32 StringCmp::Compare(const char* str1, const char* str2,
                         CaseSensitivity casesense) {
  if (casesense == CaseSensitivity::CaseSensitive) {
    return CStringTraitsA::Strcmp(str1, str2);
  } else {
    return CStringTraitsA::Stricmp(str1, str2);
  }
}

int32 StringCmp::Compare(const UNICHAR* str1, const UNICHAR* str2,
                         CaseSensitivity casesense) {
  if (casesense == CaseSensitivity::CaseSensitive) {
    return CStringTraitsU::Strcmp(str1, str2);
  } else {
    return CStringTraitsU::Stricmp(str1, str2);
  }
}

int32 StringCmp::Compare(const char* str1, const UNICHAR* str2,
                         CaseSensitivity casesense) {
  // TODO
  fun_check(0);
  return 0;
}

int32 StringCmp::Compare(const UNICHAR* str1, const char* str2,
                         CaseSensitivity casesense) {
  // TODO
  fun_check(0);
  return 0;
}

int32 StringCmp::Compare(const char* str1, int32 str1_len, const char* str2,
                         int32 str2_len, CaseSensitivity casesense) {
  fun_check(str1_len >= 0);
  fun_check(str2_len >= 0);

  if (str1 == nullptr || str2 == nullptr) {
    return ComparePointer(str1, str2);
  }

  const int32 len = str1_len < str2_len ? str1_len : str2_len;
  int32 diff;
  if (casesense == CaseSensitivity::CaseSensitive) {
    diff = UnsafeMemory::Memcmp(str1, str2, len);
  } else {
    const uint8* s1 = reinterpret_cast<const uint8*>(str1);
    const uint8* s2 = reinterpret_cast<const uint8*>(str2);
    for (int32 i = 0; i < len; ++i, ++s1, ++s2) {
      diff = FoldCase(*s1) - FoldCase(*s2);
      if (diff != 0) {
        break;
      }
    }
  }

  return diff ? diff : CompareLength(str1_len, str2_len);
}

int32 StringCmp::Compare(const char* str1, const char* str2, int32 len,
                         CaseSensitivity casesense) {
  fun_check(len >= 0);

  if (str1 == nullptr || str2 == nullptr) {
    return ComparePointer(str1, str2);
  }

  if (casesense == CaseSensitivity::CaseSensitive) {
    return UnsafeMemory::Memcmp(str1, str2, len);
  } else {
    int32 diff;
    const uint8* s1 = reinterpret_cast<const uint8*>(str1);
    const uint8* s2 = reinterpret_cast<const uint8*>(str2);
    for (int32 i = 0; i < len; ++i, ++s1, ++s2) {
      diff = FoldCase(*s1) - FoldCase(*s2);
      if (diff != 0) {
        return diff;
      }
    }
    return 0;  // equals
  }
}

int32 StringCmp::Compare(const UNICHAR* str1, int32 str1_len,
                         const UNICHAR* str2, int32 str2_len,
                         CaseSensitivity casesense) {
  fun_check(str1_len >= 0);
  fun_check(str2_len >= 0);

  if (str1 == nullptr || str2 == nullptr) {
    return ComparePointer(str1, str2);
  }

  const int32 len = str1_len < str2_len ? str1_len : str2_len;
  int32 diff;
  if (casesense == CaseSensitivity::CaseSensitive) {
    diff = CStringTraitsU::Strncmp(str1, str2, len);
  } else {
    for (int32 i = 0; i < len; ++i, ++str1, ++str2) {
      diff = FoldCase(*str1) - FoldCase(*str2);
      if (diff != 0) {
        break;
      }
    }
  }

  return diff ? diff : CompareLength(str1_len, str2_len);
}

int32 StringCmp::Compare(const UNICHAR* str1, const UNICHAR* str2, int32 len,
                         CaseSensitivity casesense) {
  fun_check(len >= 0);

  if (str1 == nullptr || str2 == nullptr) {
    return ComparePointer(str1, str2);
  }

  if (casesense == CaseSensitivity::CaseSensitive) {
    return CStringTraitsU::Strncmp(str1, str2, len);
  } else {
    int32 diff;
    for (int32 i = 0; i < len; ++i, ++str1, ++str2) {
      diff = FoldCase(*str1) - FoldCase(*str2);
      if (diff != 0) {
        return diff;
      }
    }
    return 0;  // equals
  }
}

int32 StringCmp::Compare(const UNICHAR* str1, int32 str1_len, const char* str2,
                         int32 str2_len, CaseSensitivity casesense) {
  fun_check(str1_len >= 0);
  fun_check(str2_len >= 0);

  if (str1 == nullptr || str2 == nullptr) {
    return ComparePointer(str1, str2);
  }

  const int32 len = str1_len < str2_len ? str1_len : str2_len;
  int32 diff;
  if (casesense == CaseSensitivity::CaseSensitive) {
    const UNICHAR* s1 = str1;
    const uint8* s2 = reinterpret_cast<const uint8*>(str2);
    for (int32 i = 0; i < len; ++i, ++s1, ++s2) {
      diff = *s1 - *s2;
      if (diff != 0) {
        break;
      }
    }
  } else {
    const UNICHAR* s1 = str1;
    const uint8* s2 = reinterpret_cast<const uint8*>(str2);
    for (int32 i = 0; i < len; ++i, ++s1, ++s2) {
      diff = FoldCase(*s1) - FoldCase(*s2);
      if (diff != 0) {
        break;
      }
    }
  }

  return diff ? diff : CompareLength(str1_len, str2_len);
}

int32 StringCmp::Compare(const UNICHAR* str1, const char* str2, int32 len,
                         CaseSensitivity casesense) {
  fun_check(len >= 0);

  if (str1 == nullptr || str2 == nullptr) {
    return ComparePointer(str1, str2);
  }

  int32 diff;

  if (casesense == CaseSensitivity::CaseSensitive) {
    const UNICHAR* s1 = str1;
    const uint8* s2 = reinterpret_cast<const uint8*>(str2);
    for (int32 i = 0; i < len; ++i, ++s1, ++s2) {
      diff = *s1 - *s2;
      if (diff != 0) {
        return diff;
      }
    }
  } else {
    const UNICHAR* s1 = str1;
    const uint8* s2 = reinterpret_cast<const uint8*>(str2);
    for (int32 i = 0; i < len; ++i, ++s1, ++s2) {
      diff = FoldCase(*s1) - FoldCase(*s2);
      if (diff != 0) {
        return diff;
      }
    }
  }

  return 0;  // equals
}

}  // namespace fun
