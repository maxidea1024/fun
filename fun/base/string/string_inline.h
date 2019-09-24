#pragma once

//
// ByteStringView
//

template <typename str, ByteStringView::IfCompatibleByteStringFamily<str>>
inline ByteStringView::ByteStringView(const str& str) noexcept
  : ByteStringView(str.ConstData(), str.Len()) {}


//
// UStringView
//

template <typename str, UStringView::IfCompatibleUStringFamily<str>>
inline UStringView::UStringView(const str& str) noexcept
  : UStringView(str.ConstData(), str.Len()) {}

inline UString UStringView::ToString() const {
  // deep-copy
  return UString(ConstData(), Len());
}

inline bool UStringView::StartsWith(UStringView sub, CaseSensitivity casesense) const { return StringCmp::StartsWith(ConstData(), Len(), sub.ConstData(), sub.Len(), casesense); }
inline bool UStringView::StartsWith(AsciiString sub, CaseSensitivity casesense) const { return StringCmp::StartsWith(ConstData(), Len(), sub.ConstData(), sub.Len(), casesense); }
inline bool UStringView::StartsWith(UNICHAR ch, CaseSensitivity casesense) const { return StringCmp::StartsWith(ConstData(), Len(), &ch, 1, casesense); }

inline bool UStringView::EndsWith(UStringView sub, CaseSensitivity casesense) const { return StringCmp::EndsWith(ConstData(), Len(), sub.ConstData(), sub.Len(), casesense); }
inline bool UStringView::EndsWith(AsciiString sub, CaseSensitivity casesense) const { return StringCmp::EndsWith(ConstData(), Len(), sub.ConstData(), sub.Len(), casesense); }
inline bool UStringView::EndsWith(UNICHAR ch, CaseSensitivity casesense) const { return StringCmp::EndsWith(ConstData(), Len(), &ch, 1, casesense); }


//
// AsciiString
//

inline AsciiString::AsciiString(const ByteString& str)
  : data_(str.ConstData()), length_(str.Len()) {}


//
// ByteString
//

inline bool ByteString::IsAscii() const {
  bool is_ascii = true;
  for (const char* p = cbegin(); p != cend(); ++p) {
    if (uint8(*p) > 127) {
      is_ascii = false;
      break;
    }
  }
  return is_ascii;
}

inline int32 ByteString::IndexOfAny(ByteStringView chars, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const {
  //TODO from(offset) adjustment code를 별도의 유틸로 빼주는게 좋을듯...
  if (from < 0) {
    from = MathBase::Max(from + Len(), 0);
  }

  if (from < Len()) {
    //TODO optimize
    ByteString chars2(chars);
    for (int32 i = from; i < Len(); ++i) {
      if (chars2.Contains(ConstData()[i], casesense, 0, matched_len)) {
        if (matched_index) {
          *matched_index = i;
        }
        return i;
      }
    }
  }
  if (matched_index) {
    *matched_index = INVALID_INDEX;
  }
  return INVALID_INDEX;
}

inline int32 ByteString::IndexOfAny(AsciiString chars, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const {
  //TODO from(offset) adjustment code를 별도의 유틸로 빼주는게 좋을듯...
  if (from < 0) {
    from = MathBase::Max(from + Len(), 0);
  }

  if (from < Len()) {
    //TODO optimize
    ByteString chars2(chars);
    for (int32 i = from; i < Len(); ++i) {
      if (chars2.Contains(ConstData()[i], casesense, 0, matched_len)) {
        if (matched_index) {
          *matched_index = i;
        }
        return i;
      }
    }
  }
  if (matched_index) {
    *matched_index = INVALID_INDEX;
  }
  return INVALID_INDEX;
}


inline int32 ByteString::LastIndexOfAny(ByteStringView chars, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const {
  //TODO from(offset) adjustment code를 별도의 유틸로 빼주는게 좋을듯...
  if (Len() <= 0) {
    return INVALID_INDEX;
  }

  if (from < 0) {
    from += Len();
  } else if (from > Len()) {
    from = Len() - 1;
  }

  for (int32 i = from; i >= 0; --i) {
    //TODO optimize
    ByteString chars2(chars);
    if (chars2.Contains(ConstData()[i], casesense, 0, matched_len)) {
      if (matched_index) {
        *matched_index = i;
      }
      return i;
    }
  }
  if (matched_index) {
    *matched_index = INVALID_INDEX;
  }
  return INVALID_INDEX;
}

inline int32 ByteString::LastIndexOfAny(AsciiString chars, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const {
  //TODO from(offset) adjustment code를 별도의 유틸로 빼주는게 좋을듯...
  if (Len() <= 0) {
    return INVALID_INDEX;
  }

  if (from < 0) {
    from += Len();
  } else if (from > Len()) {
    from = Len() - 1;
  }

  for (int32 i = from; i >= 0; --i) {
    //TODO optimize
    ByteString chars2(chars);
    if (chars2.Contains(ConstData()[i], casesense, 0, matched_len)) {
      if (matched_index) {
        *matched_index = i;
      }
      return i;
    }
  }
  if (matched_index) {
    *matched_index = INVALID_INDEX;
  }
  return INVALID_INDEX;
}


//
// ByteStringRef
//

inline bool ByteStringRef::IsAscii() const {
  bool is_ascii = true;
  for (const char* p = cbegin(); p != cend(); ++p) {
    if (uint8(*p) > 127) {
      is_ascii = false;
      break;
    }
  }
  return is_ascii;
}

template <typename Predicate>
int32 ByteStringRef::IndexOfIf(const Predicate& pred, int32 from) const {
  if (from < 0) {
    from = MathBase::Max(from + Len(), 0);
  }

  if (from < Len()) {
    const char* p = ConstData() + from - 1;
    const char* e = ConstData() + Len();
    for (; p != e; ++p) {
      if (pred(*p)) {
        return p - ConstData();
      }
    }
  }
  return INVALID_INDEX;
}

template <typename Predicate>
int32 ByteStringRef::LastIndexOfIf(const Predicate& pred, int32 from) const {
  //if (!AdjustFromForReverse(from)) return INVALID_INDEX;

  if (Len() <= 0) {
    return INVALID_INDEX;
  }

  if (from < 0) {
    from += Len();
  } else if (from > Len()) {
    from = Len() - 1;
  }

  if (from >= 0) {
    const char* b = ConstData();
    const char* p = ConstData() + from + 1;
    for (; p != b; --p) {
      if (pred(*p)) {
        return p - ConstData();
      }
    }
  }
  return INVALID_INDEX;
}

inline int32 ByteStringRef::IndexOfAny(ByteStringView chars, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const {
  //TODO from(offset) adjustment code를 별도의 유틸로 빼주는게 좋을듯...
  if (from < 0) {
    from = MathBase::Max(from + Len(), 0);
  }

  if (from < Len()) {
    //TODO optimize
    ByteString chars2(chars);
    for (int32 i = from; i < Len(); ++i) {
      if (chars2.Contains(ConstData()[i], casesense, 0, matched_len)) {
        if (matched_index) {
          *matched_index = i;
        }
        return i;
      }
    }
  }
  if (matched_index) {
    *matched_index = INVALID_INDEX;
  }
  return INVALID_INDEX;
}

inline int32 ByteStringRef::IndexOfAny(AsciiString chars, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const {
  //TODO from(offset) adjustment code를 별도의 유틸로 빼주는게 좋을듯...
  if (from < 0) {
    from = MathBase::Max(from + Len(), 0);
  }

  if (from < Len()) {
    //TODO optimize
    ByteString chars2(chars);
    for (int32 i = from; i < Len(); ++i) {
      if (chars2.Contains(ConstData()[i], casesense, 0, matched_len)) {
        if (matched_index) {
          *matched_index = i;
        }
        return i;
      }
    }
  }
  if (matched_index) {
    *matched_index = INVALID_INDEX;
  }
  return INVALID_INDEX;
}


inline int32 ByteStringRef::LastIndexOfAny(ByteStringView chars, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const {
  //TODO from(offset) adjustment code를 별도의 유틸로 빼주는게 좋을듯...
  if (Len() <= 0) {
    return INVALID_INDEX;
  }

  if (from < 0) {
    from += Len();
  } else if (from > Len()) {
    from = Len() - 1;
  }

  for (int32 i = from; i >= 0; --i) {
    //TODO optimize
    ByteString chars2(chars);
    if (chars2.Contains(ConstData()[i], casesense, 0, matched_len)) {
      if (matched_index) {
        *matched_index = i;
      }
      return i;
    }
  }
  if (matched_index) {
    *matched_index = INVALID_INDEX;
  }
  return INVALID_INDEX;
}

inline int32 ByteStringRef::LastIndexOfAny(AsciiString chars, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const {
  //TODO from(offset) adjustment code를 별도의 유틸로 빼주는게 좋을듯...
  if (Len() <= 0) {
    return INVALID_INDEX;
  }

  if (from < 0) {
    from += Len();
  } else if (from > Len()) {
    from = Len() - 1;
  }

  for (int32 i = from; i >= 0; --i) {
    //TODO optimize
    ByteString chars2(chars);
    if (chars2.Contains(ConstData()[i], casesense, 0, matched_len)) {
      if (matched_index) {
        *matched_index = i;
      }
      return i;
    }
  }
  if (matched_index) {
    *matched_index = INVALID_INDEX;
  }
  return INVALID_INDEX;
}


//
// UString
//

inline bool UString::IsAscii() const {
  bool is_ascii = true;
  for (const UNICHAR* p = cbegin(); p != cend(); ++p) {
    if (*p > 127) {
      is_ascii = false;
      break;
    }
  }
  return is_ascii;
}


template <typename Predicate>
inline int32 UString::IndexOfIf(const Predicate& pred, int32 from) const {
  if (from < 0) {
    from = MathBase::Max(from + Len(), 0);
  }

  if (from < Len()) {
    const UNICHAR* p = ConstData() + from - 1;
    const UNICHAR* e = ConstData() + Len();
    for (; p != e; ++p) {
      if (pred(*p)) {
        return p - ConstData();
      }
    }
  }
  return INVALID_INDEX;
}

template <typename Predicate>
int32 UString::LastIndexOfIf(const Predicate& pred, int32 from) const {
  if (Len() <= 0) {
    return INVALID_INDEX;
  }

  if (from < 0) {
    from += Len();
  } else if (from > Len()) {
    from = Len() - 1;
  }

  if (from >= 0) {
    const UNICHAR* b = ConstData();
    const UNICHAR* p = ConstData() + from + 1;
    for (; p != b; --p) {
      if (pred(*p)) {
        return p - ConstData();
      }
    }
  }
  return INVALID_INDEX;
}

inline int32 UString::IndexOfAny(UStringView chars, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const {
  if (matched_len) {
    *matched_len = 1;
  }

  //TODO from(offset) adjustment code를 별도의 유틸로 빼주는게 좋을듯...
  if (from < 0) {
    from = MathBase::Max(from + Len(), 0);
  }

  if (from < Len()) {
    //TODO optimize
    UString chars2(chars);
    for (int32 i = from; i < Len(); ++i) {
      if (chars2.Contains(ConstData()[i], casesense, 0, matched_len)) {
        if (matched_index) {
          *matched_index = i;
        }
        return i;
      }
    }
  }
  if (matched_index) {
    *matched_index = INVALID_INDEX;
  }
  return INVALID_INDEX;
}

inline int32 UString::IndexOfAny(AsciiString chars, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const {
  if (matched_len) {
    *matched_len = 1;
  }

  //TODO from(offset) adjustment code를 별도의 유틸로 빼주는게 좋을듯...
  if (from < 0) {
    from = MathBase::Max(from + Len(), 0);
  }

  if (from < Len()) {
    //TODO optimize
    UString chars2(chars);
    for (int32 i = from; i < Len(); ++i) {
      if (chars2.Contains(ConstData()[i], casesense, 0, matched_len)) {
        if (matched_index) {
          *matched_index = i;
        }
        return i;
      }
    }
  }
  if (matched_index) {
    *matched_index = INVALID_INDEX;
  }
  return INVALID_INDEX;
}

inline int32 UString::LastIndexOfAny(UStringView chars, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const {
  if (matched_len) {
    *matched_len = 1;
  }

  //TODO from(offset) adjustment code를 별도의 유틸로 빼주는게 좋을듯...
  if (Len() <= 0) {
    return INVALID_INDEX;
  }

  if (from < 0) {
    from += Len();
  } else if (from > Len()) {
    from = Len() - 1;
  }

  for (int32 i = from; i >= 0; --i) {
    //TODO optimize
    UString chars2(chars);
    if (chars2.Contains(ConstData()[i], casesense, 0, matched_len)) {
      if (matched_index) {
        *matched_index = i;
      }
      return i;
    }
  }
  if (matched_index) {
    *matched_index = INVALID_INDEX;
  }
  return INVALID_INDEX;
}

inline int32 UString::LastIndexOfAny(AsciiString chars, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const {
  if (matched_len) {
    *matched_len = 1;
  }

  //TODO from(offset) adjustment code를 별도의 유틸로 빼주는게 좋을듯...
  if (Len() <= 0) {
    return INVALID_INDEX;
  }

  if (from < 0) {
    from += Len();
  } else if (from > Len()) {
    from = Len() - 1;
  }

  for (int32 i = from; i >= 0; --i) {
    //TODO optimize
    UString chars2(chars);
    if (chars2.Contains(ConstData()[i], casesense, 0, matched_len)) {
      if (matched_index) {
        *matched_index = i;
      }
      return i;
    }
  }
  if (matched_index) {
    *matched_index = INVALID_INDEX;
  }
  return INVALID_INDEX;
}


//
// UStringRef
//

inline bool UStringRef::IsAscii() const {
  bool is_ascii = true;
  for (const UNICHAR* p = cbegin(); p != cend(); ++p) {
    if (*p > 127) {
      is_ascii = false;
      break;
    }
  }
  return is_ascii;
}

template <typename Predicate>
int32 UStringRef::IndexOfIf(const Predicate& pred, int32 from) const {
  if (from < 0) {
    from = MathBase::Max(from + Len(), 0);
  }

  if (from < Len()) {
    const UNICHAR* p = ConstData() + from - 1;
    const UNICHAR* e = ConstData() + Len();
    for (; p != e; ++p) {
      if (pred(*p)) {
        return p - ConstData();
      }
    }
  }
  return INVALID_INDEX;
}

template <typename Predicate>
int32 UStringRef::LastIndexOfIf(const Predicate& pred, int32 from) const {
  if (Len() <= 0) {
    return INVALID_INDEX;
  }

  if (from < 0) {
    from += Len();
  } else if (from > Len()) {
    from = Len() - 1;
  }

  if (from >= 0) {
    const UNICHAR* b = ConstData();
    const UNICHAR* p = ConstData() + from + 1;
    for (; p != b; --p) {
      if (pred(*p)) {
        return p - ConstData();
      }
    }
  }
  return INVALID_INDEX;
}

inline int32 UStringRef::IndexOfAny(UStringView chars, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const {
  //TODO from(offset) adjustment code를 별도의 유틸로 빼주는게 좋을듯...
  if (from < 0) {
    from = MathBase::Max(from + Len(), 0);
  }

  if (from < Len()) {
    //TODO optimize
    UString chars2(chars);
    for (int32 i = from; i < Len(); ++i) {
      if (chars2.Contains(ConstData()[i], casesense, 0, matched_len)) {
        if (matched_index) {
          *matched_index = i;
        }
        return i;
      }
    }
  }
  if (matched_index) {
    *matched_index = INVALID_INDEX;
  }
  return INVALID_INDEX;
}

inline int32 UStringRef::IndexOfAny(AsciiString chars, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const {
  //TODO from(offset) adjustment code를 별도의 유틸로 빼주는게 좋을듯...
  if (from < 0) {
    from = MathBase::Max(from + Len(), 0);
  }

  if (from < Len()) {
    //TODO optimize
    UString chars2(chars);
    for (int32 i = from; i < Len(); ++i) {
      if (chars2.Contains(ConstData()[i], casesense, 0, matched_len)) {
        if (matched_index) {
          *matched_index = i;
        }
        return i;
      }
    }
  }
  if (matched_index) {
    *matched_index = INVALID_INDEX;
  }
  return INVALID_INDEX;
}

inline int32 UStringRef::LastIndexOfAny(UStringView chars, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const {
  //TODO from(offset) adjustment code를 별도의 유틸로 빼주는게 좋을듯...
  if (Len() <= 0) {
    return INVALID_INDEX;
  }

  if (from < 0) {
    from += Len();
  } else if (from > Len()) {
    from = Len() - 1;
  }

  for (int32 i = from; i >= 0; --i) {
    //TODO optimize
    UString chars2(chars);
    if (chars2.Contains(ConstData()[i], casesense, 0, matched_len)) {
      if (matched_index) {
        *matched_index = i;
      }
      return i;
    }
  }
  if (matched_index) {
    *matched_index = INVALID_INDEX;
  }
  return INVALID_INDEX;
}

inline int32 UStringRef::LastIndexOfAny(AsciiString chars, CaseSensitivity casesense, int32 from, int32* matched_index, int32* matched_len) const {
  //TODO from(offset) adjustment code를 별도의 유틸로 빼주는게 좋을듯...
  if (Len() <= 0) {
    return INVALID_INDEX;
  }

  if (from < 0) {
    from += Len();
  } else if (from > Len()) {
    from = Len() - 1;
  }

  for (int32 i = from; i >= 0; --i) {
    //TODO optimize
    UString chars2(chars);
    if (chars2.Contains(ConstData()[i], casesense, 0, matched_len)) {
      if (matched_index) {
        *matched_index = i;
      }
      return i;
    }
  }
  if (matched_index) {
    *matched_index = INVALID_INDEX;
  }
  return INVALID_INDEX;
}

#include "fun/base/string/stringcmp_inline.h"
