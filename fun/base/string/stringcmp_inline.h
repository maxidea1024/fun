#pragma once

//-----------------------------------------------------------------------------
// ByteString
//-----------------------------------------------------------------------------

//ByteString and ByteString
inline int32 ByteString::Compare(const ByteString& str, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), str.ConstData(), str.Len(), casesense); }
inline bool ByteString::Equals(const ByteString& str, CaseSensitivity casesense) const { return Len() == str.Len() && Compare(str, casesense) == 0; }
inline bool ByteString::operator == (const ByteString& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool ByteString::operator != (const ByteString& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool ByteString::operator <  (const ByteString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool ByteString::operator <= (const ByteString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool ByteString::operator >  (const ByteString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool ByteString::operator >= (const ByteString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//ByteString and ByteStringRef
inline int32 ByteString::Compare(const ByteStringRef& str, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), str.ConstData(), str.Len(), casesense); }
inline bool ByteString::Equals(const ByteStringRef& str, CaseSensitivity casesense) const { return Len() == str.Len() && Compare(str, casesense) == 0; }
inline bool ByteString::operator == (const ByteStringRef& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool ByteString::operator != (const ByteStringRef& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool ByteString::operator <  (const ByteStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool ByteString::operator <= (const ByteStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool ByteString::operator >  (const ByteStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool ByteString::operator >= (const ByteStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//ByteString and ByteStringView
inline int32 ByteString::Compare(const ByteStringView& str, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), str.ConstData(), str.Len(), casesense); }
inline bool ByteString::Equals(const ByteStringView& str, CaseSensitivity casesense) const { return Len() == str.Len() && Compare(str, casesense) == 0; }
inline bool ByteString::operator == (const ByteStringView& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool ByteString::operator != (const ByteStringView& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool ByteString::operator <  (const ByteStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool ByteString::operator <= (const ByteStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool ByteString::operator >  (const ByteStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool ByteString::operator >= (const ByteStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//ByteString and AsciiString
inline int32 ByteString::Compare(const AsciiString& str, CaseSensitivity casesense) const {
  //ByteStringRef를 UTF8로 가정하므로, UTF8을 TCHAR로 변환 후 ANSI와 비교하는게 정상적으로 보임.
  //하지만 경우에 따라서는 변환해야할 대상이 클수도 있음.
  //고민이 필요한 부분인듯...
  UtfConversionBuffer<char,UNICHAR> ThisAsTCHAR(ConstData(), Len());
  return StringCmp::Compare(ThisAsTCHAR.ConstData(), ThisAsTCHAR.Len(), str.ConstData(), str.Len(), casesense);
}
inline bool ByteString::Equals(const AsciiString& str, CaseSensitivity casesense) const { return Compare(str, casesense) == 0; }
inline bool ByteString::operator == (const AsciiString& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool ByteString::operator != (const AsciiString& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool ByteString::operator <  (const AsciiString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool ByteString::operator <= (const AsciiString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool ByteString::operator >  (const AsciiString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool ByteString::operator >= (const AsciiString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//ByteString and UString
inline int32 ByteString::Compare(const UString& str, CaseSensitivity casesense) const {
  UtfConversionBuffer<UNICHAR,char> str_as_utf8(str.ConstData(), str.Len());
  return StringCmp::Compare(ConstData(), Len(), str_as_utf8.ConstData(), str_as_utf8.Len(), casesense);
}
inline bool ByteString::Equals(const UString& str, CaseSensitivity casesense) const { return Compare(str, casesense) == 0; }
inline bool ByteString::operator == (const UString& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool ByteString::operator != (const UString& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool ByteString::operator <  (const UString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool ByteString::operator <= (const UString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool ByteString::operator >  (const UString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool ByteString::operator >= (const UString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//ByteString and UStringRef
inline int32 ByteString::Compare(const UStringRef& str, CaseSensitivity casesense) const {
  UtfConversionBuffer<UNICHAR,char> str_as_utf8(str.ConstData(), str.Len());
  return StringCmp::Compare(ConstData(), Len(), str_as_utf8.ConstData(), str_as_utf8.Len(), casesense);
}
inline bool ByteString::Equals(const UStringRef& str, CaseSensitivity casesense) const { return Compare(str, casesense) == 0; }
inline bool ByteString::operator == (const UStringRef& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool ByteString::operator != (const UStringRef& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool ByteString::operator <  (const UStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool ByteString::operator <= (const UStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool ByteString::operator >  (const UStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool ByteString::operator >= (const UStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//ByteString and UStringView
inline int32 ByteString::Compare(const UStringView& str, CaseSensitivity casesense) const {
  UtfConversionBuffer<UNICHAR,char> str_as_utf8(str.ConstData(), str.Len());
  return StringCmp::Compare(ConstData(), Len(), str_as_utf8.ConstData(), str_as_utf8.Len(), casesense);
}
inline bool ByteString::Equals(const UStringView& str, CaseSensitivity casesense) const { return Compare(str, casesense) == 0; }
inline bool ByteString::operator == (const UStringView& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool ByteString::operator != (const UStringView& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool ByteString::operator <  (const UStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool ByteString::operator <= (const UStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool ByteString::operator >  (const UStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool ByteString::operator >= (const UStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//ByteString and char
inline int32 ByteString::Compare(char ch, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), &ch, 1, casesense); }
inline bool ByteString::Equals(char ch, CaseSensitivity casesense) const { return Len() == 1 && Compare(ch, casesense) == 0; }
inline bool ByteString::operator == (char ch) const { return Equals(ch, CaseSensitivity::CaseSensitive); }
inline bool ByteString::operator != (char ch) const { return Equals(ch, CaseSensitivity::CaseSensitive) == false; }
inline bool ByteString::operator <  (char ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) <  0; }
inline bool ByteString::operator <= (char ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) <= 0; }
inline bool ByteString::operator >  (char ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) >  0; }
inline bool ByteString::operator >= (char ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) >= 0; }
inline bool operator == (char ch, const ByteString& str) { return (str == ch); }
inline bool operator != (char ch, const ByteString& str) { return (str != ch); }
inline bool operator <  (char ch, const ByteString& str) { return (str >  ch); }
inline bool operator <= (char ch, const ByteString& str) { return (str >= ch); }
inline bool operator >  (char ch, const ByteString& str) { return (str <  ch); }
inline bool operator >= (char ch, const ByteString& str) { return (str <= ch); }

//ByteString and const char*
inline int32 ByteString::Compare(const char* str, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), str, CStringTraitsA::Strlen(str), casesense); }
inline bool ByteString::Equals(const char* str, CaseSensitivity casesense) const { return Compare(str, casesense) == 0; }
inline bool ByteString::operator == (const char* str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool ByteString::operator != (const char* str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool ByteString::operator <  (const char* str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool ByteString::operator <= (const char* str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool ByteString::operator >  (const char* str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool ByteString::operator >= (const char* str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }
inline bool operator == (const char* str1, const ByteString& str2) { return (str2 == str1); }
inline bool operator != (const char* str1, const ByteString& str2) { return (str2 != str1); }
inline bool operator <  (const char* str1, const ByteString& str2) { return (str2 >  str1); }
inline bool operator <= (const char* str1, const ByteString& str2) { return (str2 >= str1); }
inline bool operator >  (const char* str1, const ByteString& str2) { return (str2 <  str1); }
inline bool operator >= (const char* str1, const ByteString& str2) { return (str2 <= str1); }

//ByteString and UNICHAR
inline int32 ByteString::Compare(UNICHAR ch, CaseSensitivity casesense) const {
  UtfConversionBuffer<UNICHAR,char> str2(&ch,1);
  return StringCmp::Compare(ConstData(), Len(), str2.ConstData(), str2.Len(), casesense);
}
inline bool ByteString::Equals(UNICHAR ch, CaseSensitivity casesense) const { return Compare(ch, casesense) == 0; }
inline bool ByteString::operator == (UNICHAR ch) const { return Equals(ch, CaseSensitivity::CaseSensitive); }
inline bool ByteString::operator != (UNICHAR ch) const { return Equals(ch, CaseSensitivity::CaseSensitive) == false; }
inline bool ByteString::operator <  (UNICHAR ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) <  0; }
inline bool ByteString::operator <= (UNICHAR ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) <= 0; }
inline bool ByteString::operator >  (UNICHAR ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) >  0; }
inline bool ByteString::operator >= (UNICHAR ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) >= 0; }
inline bool operator == (UNICHAR ch, const ByteString& str) { return (str == ch); }
inline bool operator != (UNICHAR ch, const ByteString& str) { return (str != ch); }
inline bool operator <  (UNICHAR ch, const ByteString& str) { return (str >  ch); }
inline bool operator <= (UNICHAR ch, const ByteString& str) { return (str >= ch); }
inline bool operator >  (UNICHAR ch, const ByteString& str) { return (str <  ch); }
inline bool operator >= (UNICHAR ch, const ByteString& str) { return (str <= ch); }

//ByteString and const UNICHAR*
inline int32 ByteString::Compare(const UNICHAR* str, CaseSensitivity casesense) const {
  UtfConversionBuffer<UNICHAR,char> str_as_utf8(str);
  return StringCmp::Compare(ConstData(), Len(), str_as_utf8.ConstData(), str_as_utf8.Len(), casesense);
}
inline bool ByteString::Equals(const UNICHAR* str, CaseSensitivity casesense) const { return Compare(str, casesense) == 0; }
inline bool ByteString::operator == (const UNICHAR* str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool ByteString::operator != (const UNICHAR* str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool ByteString::operator <  (const UNICHAR* str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool ByteString::operator <= (const UNICHAR* str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool ByteString::operator >  (const UNICHAR* str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool ByteString::operator >= (const UNICHAR* str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }
inline bool operator == (const UNICHAR* str1, const ByteString& str2) { return (str2 == str1); }
inline bool operator != (const UNICHAR* str1, const ByteString& str2) { return (str2 != str1); }
inline bool operator <  (const UNICHAR* str1, const ByteString& str2) { return (str2 >  str1); }
inline bool operator <= (const UNICHAR* str1, const ByteString& str2) { return (str2 >= str1); }
inline bool operator >  (const UNICHAR* str1, const ByteString& str2) { return (str2 <  str1); }
inline bool operator >= (const UNICHAR* str1, const ByteString& str2) { return (str2 <= str1); }



//-----------------------------------------------------------------------------
// ByteStringRef
//-----------------------------------------------------------------------------

//ByteStringRef and ByteStringRef
inline int32 ByteStringRef::Compare(const ByteStringRef& str, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), str.ConstData(), str.Len(), casesense); }
inline bool ByteStringRef::Equals(const ByteStringRef& str, CaseSensitivity casesense) const { return Len() == str.Len() && Compare(str, casesense) == 0; }
inline bool ByteStringRef::operator == (const ByteStringRef& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool ByteStringRef::operator != (const ByteStringRef& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool ByteStringRef::operator <  (const ByteStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool ByteStringRef::operator <= (const ByteStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool ByteStringRef::operator >  (const ByteStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool ByteStringRef::operator >= (const ByteStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//ByteStringRef and ByteString
inline int32 ByteStringRef::Compare(const ByteString& str, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), str.ConstData(), str.Len(), casesense); }
inline bool ByteStringRef::Equals(const ByteString& str, CaseSensitivity casesense) const { return Len() == str.Len() && Compare(str, casesense) == 0; }
inline bool ByteStringRef::operator == (const ByteString& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool ByteStringRef::operator != (const ByteString& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool ByteStringRef::operator <  (const ByteString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool ByteStringRef::operator <= (const ByteString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool ByteStringRef::operator >  (const ByteString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool ByteStringRef::operator >= (const ByteString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//ByteStringRef and ByteStringView
inline int32 ByteStringRef::Compare(const ByteStringView& str, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), str.ConstData(), str.Len(), casesense); }
inline bool ByteStringRef::Equals(const ByteStringView& str, CaseSensitivity casesense) const { return Len() == str.Len() && Compare(str, casesense) == 0; }
inline bool ByteStringRef::operator == (const ByteStringView& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool ByteStringRef::operator != (const ByteStringView& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool ByteStringRef::operator <  (const ByteStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool ByteStringRef::operator <= (const ByteStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool ByteStringRef::operator >  (const ByteStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool ByteStringRef::operator >= (const ByteStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//ByteStringRef and AsciiString
inline int32 ByteStringRef::Compare(const AsciiString& str, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), str.ConstData(), str.Len(), casesense); }
inline bool ByteStringRef::Equals(const AsciiString& str, CaseSensitivity casesense) const { return Len() == str.Len() && Compare(str, casesense) == 0; }
inline bool ByteStringRef::operator == (const AsciiString& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool ByteStringRef::operator != (const AsciiString& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool ByteStringRef::operator <  (const AsciiString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool ByteStringRef::operator <= (const AsciiString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool ByteStringRef::operator >  (const AsciiString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool ByteStringRef::operator >= (const AsciiString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//ByteStringRef and UString
inline int32 ByteStringRef::Compare(const UString& str, CaseSensitivity casesense) const {
  UtfConversionBuffer<UNICHAR,char> str_as_uni(str.ConstData(), str.Len());
  return StringCmp::Compare(ConstData(), Len(), str_as_uni.ConstData(), str_as_uni.Len(), casesense) == 0;
}
inline bool ByteStringRef::Equals(const UString& str, CaseSensitivity casesense) const { return Compare(str, casesense) == 0; }
inline bool ByteStringRef::operator == (const UString& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool ByteStringRef::operator != (const UString& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool ByteStringRef::operator <  (const UString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool ByteStringRef::operator <= (const UString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool ByteStringRef::operator >  (const UString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool ByteStringRef::operator >= (const UString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//ByteStringRef and UStringRef
inline int32 ByteStringRef::Compare(const UStringRef& str, CaseSensitivity casesense) const {
  UtfConversionBuffer<UNICHAR,char> str_as_uni(str.ConstData(), str.Len());
  return StringCmp::Compare(ConstData(), Len(), str_as_uni.ConstData(), str_as_uni.Len(), casesense);
}
inline bool ByteStringRef::Equals(const UStringRef& str, CaseSensitivity casesense) const { return Compare(str, casesense) == 0; }
inline bool ByteStringRef::operator == (const UStringRef& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool ByteStringRef::operator != (const UStringRef& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool ByteStringRef::operator <  (const UStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool ByteStringRef::operator <= (const UStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool ByteStringRef::operator >  (const UStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool ByteStringRef::operator >= (const UStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//ByteStringRef and UStringView
inline int32 ByteStringRef::Compare(const UStringView& str, CaseSensitivity casesense) const {
  UtfConversionBuffer<UNICHAR,char> str_as_uni(str.ConstData(), str.Len());
  return StringCmp::Compare(ConstData(), Len(), str_as_uni.ConstData(), str_as_uni.Len(), casesense) == 0;
}
inline bool ByteStringRef::Equals(const UStringView& str, CaseSensitivity casesense) const { return Compare(str, casesense) == 0; }
inline bool ByteStringRef::operator == (const UStringView& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool ByteStringRef::operator != (const UStringView& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool ByteStringRef::operator <  (const UStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool ByteStringRef::operator <= (const UStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool ByteStringRef::operator >  (const UStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool ByteStringRef::operator >= (const UStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//ByteStringRef and char
inline int32 ByteStringRef::Compare(char ch, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), &ch, 1, casesense); }
inline bool ByteStringRef::Equals(char ch, CaseSensitivity casesense) const { return Len() == 1 && Compare(ch, casesense) == 0; }
inline bool ByteStringRef::operator == (char ch) const { return Equals(ch, CaseSensitivity::CaseSensitive); }
inline bool ByteStringRef::operator != (char ch) const { return Equals(ch, CaseSensitivity::CaseSensitive) == false; }
inline bool ByteStringRef::operator <  (char ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) <  0; }
inline bool ByteStringRef::operator <= (char ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) <= 0; }
inline bool ByteStringRef::operator >  (char ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) >  0; }
inline bool ByteStringRef::operator >= (char ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) >= 0; }
inline bool operator == (char ch, const ByteStringRef& str) { return (str == ch); }
inline bool operator != (char ch, const ByteStringRef& str) { return (str != ch); }
inline bool operator <  (char ch, const ByteStringRef& str) { return (str >  ch); }
inline bool operator <= (char ch, const ByteStringRef& str) { return (str >= ch); }
inline bool operator >  (char ch, const ByteStringRef& str) { return (str <  ch); }
inline bool operator >= (char ch, const ByteStringRef& str) { return (str <= ch); }

//ByteStringRef and const char*
inline int32 ByteStringRef::Compare(const char* str, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), str, CStringTraitsA::Strlen(str), casesense); }
inline bool ByteStringRef::Equals(const char* str, CaseSensitivity casesense) const { return Compare(str, casesense) == 0; }
inline bool ByteStringRef::operator == (const char* str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool ByteStringRef::operator != (const char* str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool ByteStringRef::operator <  (const char* str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool ByteStringRef::operator <= (const char* str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool ByteStringRef::operator >  (const char* str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool ByteStringRef::operator >= (const char* str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }
inline bool operator == (const char* str1, const ByteStringRef& str2) { return (str2 == str1); }
inline bool operator != (const char* str1, const ByteStringRef& str2) { return (str2 != str1); }
inline bool operator <  (const char* str1, const ByteStringRef& str2) { return (str2 >  str1); }
inline bool operator <= (const char* str1, const ByteStringRef& str2) { return (str2 >= str1); }
inline bool operator >  (const char* str1, const ByteStringRef& str2) { return (str2 <  str1); }
inline bool operator >= (const char* str1, const ByteStringRef& str2) { return (str2 <= str1); }

//ByteStringRef and UNICHAR
inline int32 ByteStringRef::Compare(UNICHAR ch, CaseSensitivity casesense) const {
  UtfConversionBuffer<UNICHAR,char> str2(&ch,1);
  return StringCmp::Compare(ConstData(), Len(), str2.ConstData(), str2.Len(), casesense);
}
inline bool ByteStringRef::Equals(UNICHAR ch, CaseSensitivity casesense) const { return Compare(ch, casesense) == 0; }
inline bool ByteStringRef::operator == (UNICHAR ch) const { return Equals(ch, CaseSensitivity::CaseSensitive); }
inline bool ByteStringRef::operator != (UNICHAR ch) const { return Equals(ch, CaseSensitivity::CaseSensitive) == false; }
inline bool ByteStringRef::operator <  (UNICHAR ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) <  0; }
inline bool ByteStringRef::operator <= (UNICHAR ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) <= 0; }
inline bool ByteStringRef::operator >  (UNICHAR ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) >  0; }
inline bool ByteStringRef::operator >= (UNICHAR ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) >= 0; }
inline bool operator == (UNICHAR ch, const ByteStringRef& str) { return (str == ch); }
inline bool operator != (UNICHAR ch, const ByteStringRef& str) { return (str != ch); }
inline bool operator <  (UNICHAR ch, const ByteStringRef& str) { return (str >  ch); }
inline bool operator <= (UNICHAR ch, const ByteStringRef& str) { return (str >= ch); }
inline bool operator >  (UNICHAR ch, const ByteStringRef& str) { return (str <  ch); }
inline bool operator >= (UNICHAR ch, const ByteStringRef& str) { return (str <= ch); }

//ByteStringRef and const UNICHAR*
inline int32 ByteStringRef::Compare(const UNICHAR* str, CaseSensitivity casesense) const {
  UtfConversionBuffer<UNICHAR,char> str_as_utf8(str);
  return StringCmp::Compare(ConstData(), Len(), str_as_utf8.ConstData(), str_as_utf8.Len(), casesense);
}
inline bool ByteStringRef::Equals(const UNICHAR* str, CaseSensitivity casesense) const { return Compare(str, casesense) == 0; }
inline bool ByteStringRef::operator == (const UNICHAR* str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool ByteStringRef::operator != (const UNICHAR* str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool ByteStringRef::operator <  (const UNICHAR* str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool ByteStringRef::operator <= (const UNICHAR* str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool ByteStringRef::operator >  (const UNICHAR* str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool ByteStringRef::operator >= (const UNICHAR* str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }
inline bool operator == (const UNICHAR* str1, const ByteStringRef& str2) { return (str2 == str1); }
inline bool operator != (const UNICHAR* str1, const ByteStringRef& str2) { return (str2 != str1); }
inline bool operator <  (const UNICHAR* str1, const ByteStringRef& str2) { return (str2 >  str1); }
inline bool operator <= (const UNICHAR* str1, const ByteStringRef& str2) { return (str2 >= str1); }
inline bool operator >  (const UNICHAR* str1, const ByteStringRef& str2) { return (str2 <  str1); }
inline bool operator >= (const UNICHAR* str1, const ByteStringRef& str2) { return (str2 <= str1); }



//-----------------------------------------------------------------------------
// ByteStringView
//-----------------------------------------------------------------------------

//ByteStringView and ByteStringView
inline int32 ByteStringView::Compare(const ByteStringView& str, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), str.ConstData(), str.Len(), casesense); }
inline bool ByteStringView::Equals(const ByteStringView& str, CaseSensitivity casesense) const { return Len() == str.Len() && Compare(str, casesense) == 0; }
inline bool ByteStringView::operator == (const ByteStringView& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool ByteStringView::operator != (const ByteStringView& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool ByteStringView::operator <  (const ByteStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool ByteStringView::operator <= (const ByteStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool ByteStringView::operator >  (const ByteStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool ByteStringView::operator >= (const ByteStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//ByteStringView and ByteString
inline int32 ByteStringView::Compare(const ByteString& str, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), str.ConstData(), str.Len(), casesense); }
inline bool ByteStringView::Equals(const ByteString& str, CaseSensitivity casesense) const { return Len() == str.Len() && Compare(str, casesense) == 0; }
inline bool ByteStringView::operator == (const ByteString& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool ByteStringView::operator != (const ByteString& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool ByteStringView::operator <  (const ByteString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool ByteStringView::operator <= (const ByteString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool ByteStringView::operator >  (const ByteString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool ByteStringView::operator >= (const ByteString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//ByteStringView and ByteStringRef
inline int32 ByteStringView::Compare(const ByteStringRef& str, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), str.ConstData(), str.Len(), casesense); }
inline bool ByteStringView::Equals(const ByteStringRef& str, CaseSensitivity casesense) const { return Len() == str.Len() && Compare(str, casesense) == 0; }
inline bool ByteStringView::operator == (const ByteStringRef& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool ByteStringView::operator != (const ByteStringRef& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool ByteStringView::operator <  (const ByteStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool ByteStringView::operator <= (const ByteStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool ByteStringView::operator >  (const ByteStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool ByteStringView::operator >= (const ByteStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//ByteStringView and char
inline int32 ByteStringView::Compare(char ch, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), &ch, 1, casesense); }
inline bool ByteStringView::Equals(char ch, CaseSensitivity casesense) const { return Len() == 1 && Compare(ch, casesense) == 0; }
inline bool ByteStringView::operator == (char ch) const { return Equals(ch, CaseSensitivity::CaseSensitive); }
inline bool ByteStringView::operator != (char ch) const { return Equals(ch, CaseSensitivity::CaseSensitive) == false; }
inline bool ByteStringView::operator <  (char ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) <  0; }
inline bool ByteStringView::operator <= (char ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) <= 0; }
inline bool ByteStringView::operator >  (char ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) >  0; }
inline bool ByteStringView::operator >= (char ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) >= 0; }
inline bool operator == (char ch, const ByteStringView& str) { return (str == ch); }
inline bool operator != (char ch, const ByteStringView& str) { return (str != ch); }
inline bool operator <  (char ch, const ByteStringView& str) { return (str >  ch); }
inline bool operator <= (char ch, const ByteStringView& str) { return (str >= ch); }
inline bool operator >  (char ch, const ByteStringView& str) { return (str <  ch); }
inline bool operator >= (char ch, const ByteStringView& str) { return (str <= ch); }

//ByteStringView and const char*
inline int32 ByteStringView::Compare(const char* str, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), str, CStringTraitsA::Strlen(str), casesense); }
inline bool ByteStringView::Equals(const char* str, CaseSensitivity casesense) const { return Compare(str, casesense) == 0; }
inline bool ByteStringView::operator == (const char* str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool ByteStringView::operator != (const char* str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool ByteStringView::operator <  (const char* str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool ByteStringView::operator <= (const char* str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool ByteStringView::operator >  (const char* str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool ByteStringView::operator >= (const char* str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }
inline bool operator == (const char* str1, const ByteStringView& str2) { return (str2 == str1); }
inline bool operator != (const char* str1, const ByteStringView& str2) { return (str2 != str1); }
inline bool operator <  (const char* str1, const ByteStringView& str2) { return (str2 >  str1); }
inline bool operator <= (const char* str1, const ByteStringView& str2) { return (str2 >= str1); }
inline bool operator >  (const char* str1, const ByteStringView& str2) { return (str2 <  str1); }
inline bool operator >= (const char* str1, const ByteStringView& str2) { return (str2 <= str1); }

//ByteStringView and AsciiString
inline int32 ByteStringView::Compare(const AsciiString& str, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), str.ConstData(), str.Len(), casesense); }
inline bool ByteStringView::Equals(const AsciiString& str, CaseSensitivity casesense) const { return Len() == str.Len() && Compare(str, casesense) == 0; }
inline bool ByteStringView::operator == (const AsciiString& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool ByteStringView::operator != (const AsciiString& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool ByteStringView::operator <  (const AsciiString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool ByteStringView::operator <= (const AsciiString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool ByteStringView::operator >  (const AsciiString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool ByteStringView::operator >= (const AsciiString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//ByteStringView and UString
inline int32 ByteStringView::Compare(const UString& str, CaseSensitivity casesense) const {
  UtfConversionBuffer<UNICHAR,char> str_as_utf8(str.ConstData(), str.Len());
  return StringCmp::Compare(ConstData(), Len(), str_as_utf8.ConstData(), str_as_utf8.Len(), casesense);
}
inline bool ByteStringView::Equals(const UString& str, CaseSensitivity casesense) const { return Compare(str, casesense) == 0; }
inline bool ByteStringView::operator == (const UString& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool ByteStringView::operator != (const UString& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool ByteStringView::operator <  (const UString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool ByteStringView::operator <= (const UString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool ByteStringView::operator >  (const UString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool ByteStringView::operator >= (const UString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//ByteStringView and UStringRef
inline int32 ByteStringView::Compare(const UStringRef& str, CaseSensitivity casesense) const {
  UtfConversionBuffer<UNICHAR,char> str_as_utf8(str.ConstData(), str.Len());
  return StringCmp::Compare(ConstData(), Len(), str_as_utf8.ConstData(), str_as_utf8.Len(), casesense);
}
inline bool ByteStringView::Equals(const UStringRef& str, CaseSensitivity casesense) const { return Compare(str, casesense) == 0; }
inline bool ByteStringView::operator == (const UStringRef& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool ByteStringView::operator != (const UStringRef& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool ByteStringView::operator <  (const UStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool ByteStringView::operator <= (const UStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool ByteStringView::operator >  (const UStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool ByteStringView::operator >= (const UStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//ByteStringView and UStringView
inline int32 ByteStringView::Compare(const UStringView& str, CaseSensitivity casesense) const {
  UtfConversionBuffer<UNICHAR,char> str_as_utf8(str.ConstData(), str.Len());
  return StringCmp::Compare(ConstData(), Len(), str_as_utf8.ConstData(), str_as_utf8.Len(), casesense);
}
inline bool ByteStringView::Equals(const UStringView& str, CaseSensitivity casesense) const { return Compare(str, casesense) == 0; }
inline bool ByteStringView::operator == (const UStringView& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool ByteStringView::operator != (const UStringView& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool ByteStringView::operator <  (const UStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool ByteStringView::operator <= (const UStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool ByteStringView::operator >  (const UStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool ByteStringView::operator >= (const UStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//ByteStringView and UNICHAR
inline int32 ByteStringView::Compare(UNICHAR ch, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), &ch, 1, casesense); }
inline bool ByteStringView::Equals(UNICHAR ch, CaseSensitivity casesense) const { return Len() == 1 && Compare(ch, casesense) == 0; }
inline bool ByteStringView::operator == (UNICHAR ch) const { return Equals(ch, CaseSensitivity::CaseSensitive); }
inline bool ByteStringView::operator != (UNICHAR ch) const { return Equals(ch, CaseSensitivity::CaseSensitive) == false; }
inline bool ByteStringView::operator <  (UNICHAR ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) <  0; }
inline bool ByteStringView::operator <= (UNICHAR ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) <= 0; }
inline bool ByteStringView::operator >  (UNICHAR ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) >  0; }
inline bool ByteStringView::operator >= (UNICHAR ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) >= 0; }
inline bool operator == (UNICHAR ch, const ByteStringView& str) { return (str == ch); }
inline bool operator != (UNICHAR ch, const ByteStringView& str) { return (str != ch); }
inline bool operator <  (UNICHAR ch, const ByteStringView& str) { return (str >  ch); }
inline bool operator <= (UNICHAR ch, const ByteStringView& str) { return (str >= ch); }
inline bool operator >  (UNICHAR ch, const ByteStringView& str) { return (str <  ch); }
inline bool operator >= (UNICHAR ch, const ByteStringView& str) { return (str <= ch); }

//ByteStringView and const UNICHAR*
inline int32 ByteStringView::Compare(const UNICHAR* str, CaseSensitivity casesense) const {
  UtfConversionBuffer<UNICHAR,char> str_as_utf8(str);
  return StringCmp::Compare(ConstData(), Len(), str_as_utf8.ConstData(), str_as_utf8.Len(), casesense);
}
inline bool ByteStringView::Equals(const UNICHAR* str, CaseSensitivity casesense) const { return Compare(str, casesense) == 0; }
inline bool ByteStringView::operator == (const UNICHAR* str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool ByteStringView::operator != (const UNICHAR* str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool ByteStringView::operator <  (const UNICHAR* str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool ByteStringView::operator <= (const UNICHAR* str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool ByteStringView::operator >  (const UNICHAR* str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool ByteStringView::operator >= (const UNICHAR* str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }
inline bool operator == (const UNICHAR* str1, const ByteStringView& str2) { return (str2 == str1); }
inline bool operator != (const UNICHAR* str1, const ByteStringView& str2) { return (str2 != str1); }
inline bool operator <  (const UNICHAR* str1, const ByteStringView& str2) { return (str2 >  str1); }
inline bool operator <= (const UNICHAR* str1, const ByteStringView& str2) { return (str2 >= str1); }
inline bool operator >  (const UNICHAR* str1, const ByteStringView& str2) { return (str2 <  str1); }
inline bool operator >= (const UNICHAR* str1, const ByteStringView& str2) { return (str2 <= str1); }



//-----------------------------------------------------------------------------
// AsciiString
//-----------------------------------------------------------------------------

//AsciiString and ByteString
inline int32 AsciiString::Compare(const ByteString& str, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), str.ConstData(), str.Len(), casesense); }
inline bool AsciiString::Equals(const ByteString& str, CaseSensitivity casesense) const { return Len() == str.Len() && Compare(str, casesense) == 0; }
inline bool AsciiString::operator == (const ByteString& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool AsciiString::operator != (const ByteString& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool AsciiString::operator <  (const ByteString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool AsciiString::operator <= (const ByteString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool AsciiString::operator >  (const ByteString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool AsciiString::operator >= (const ByteString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//AsciiString and ByteStringRef
inline int32 AsciiString::Compare(const ByteStringRef& str, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), str.ConstData(), str.Len(), casesense); }
inline bool AsciiString::Equals(const ByteStringRef& str, CaseSensitivity casesense) const { return Len() == str.Len() && Compare(str, casesense) == 0; }
inline bool AsciiString::operator == (const ByteStringRef& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool AsciiString::operator != (const ByteStringRef& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool AsciiString::operator <  (const ByteStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool AsciiString::operator <= (const ByteStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool AsciiString::operator >  (const ByteStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool AsciiString::operator >= (const ByteStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//AsciiString and ByteStringView
inline int32 AsciiString::Compare(const ByteStringView& str, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), str.ConstData(), str.Len(), casesense); }
inline bool AsciiString::Equals(const ByteStringView& str, CaseSensitivity casesense) const { return Len() == str.Len() && Compare(str, casesense) == 0; }
inline bool AsciiString::operator == (const ByteStringView& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool AsciiString::operator != (const ByteStringView& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool AsciiString::operator <  (const ByteStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool AsciiString::operator <= (const ByteStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool AsciiString::operator >  (const ByteStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool AsciiString::operator >= (const ByteStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//AsciiString and UString
inline int32 AsciiString::Compare(const UString& str, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), str.ConstData(), str.Len(), casesense); }
inline bool AsciiString::Equals(const UString& str, CaseSensitivity casesense) const { return Len() == str.Len() && Compare(str, casesense) == 0; }
inline bool AsciiString::operator == (const UString& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool AsciiString::operator != (const UString& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool AsciiString::operator <  (const UString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool AsciiString::operator <= (const UString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool AsciiString::operator >  (const UString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool AsciiString::operator >= (const UString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//AsciiString and UStringRef
inline int32 AsciiString::Compare(const UStringRef& str, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), str.ConstData(), str.Len(), casesense); }
inline bool AsciiString::Equals(const UStringRef& str, CaseSensitivity casesense) const { return Len() == str.Len() && Compare(str, casesense) == 0; }
inline bool AsciiString::operator == (const UStringRef& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool AsciiString::operator != (const UStringRef& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool AsciiString::operator <  (const UStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool AsciiString::operator <= (const UStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool AsciiString::operator >  (const UStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool AsciiString::operator >= (const UStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//AsciiString and UStringView
inline int32 AsciiString::Compare(const UStringView& str, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), str.ConstData(), str.Len(), casesense); }
inline bool AsciiString::Equals(const UStringView& str, CaseSensitivity casesense) const { return Len() == str.Len() && Compare(str, casesense) == 0; }
inline bool AsciiString::operator == (const UStringView& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool AsciiString::operator != (const UStringView& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool AsciiString::operator <  (const UStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool AsciiString::operator <= (const UStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool AsciiString::operator >  (const UStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool AsciiString::operator >= (const UStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//AsciiString and char
inline int32 AsciiString::Compare(char ch, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), &ch, 1, casesense); }
inline bool AsciiString::Equals(char ch, CaseSensitivity casesense) const { return Compare(ch, casesense) == 0; }
inline bool AsciiString::operator == (char ch) const { return Equals(ch, CaseSensitivity::CaseSensitive); }
inline bool AsciiString::operator != (char ch) const { return Equals(ch, CaseSensitivity::CaseSensitive) == false; }
inline bool AsciiString::operator <  (char ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) <  0; }
inline bool AsciiString::operator <= (char ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) <= 0; }
inline bool AsciiString::operator >  (char ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) >  0; }
inline bool AsciiString::operator >= (char ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) >= 0; }
inline bool operator == (char ch, AsciiString str) { return (str == ch); }
inline bool operator != (char ch, AsciiString str) { return (str != ch); }
inline bool operator <  (char ch, AsciiString str) { return (str >  ch); }
inline bool operator <= (char ch, AsciiString str) { return (str >= ch); }
inline bool operator >  (char ch, AsciiString str) { return (str <  ch); }
inline bool operator >= (char ch, AsciiString str) { return (str <= ch); }

//AsciiString and const char*
inline int32 AsciiString::Compare(const char* str, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), str, CStringTraitsA::Strlen(str), casesense); }
inline bool AsciiString::Equals(const char* str, CaseSensitivity casesense) const { return Compare(str, casesense) == 0; }
inline bool AsciiString::operator == (const char* str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool AsciiString::operator != (const char* str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool AsciiString::operator <  (const char* str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool AsciiString::operator <= (const char* str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool AsciiString::operator >  (const char* str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool AsciiString::operator >= (const char* str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }
inline bool operator == (const char* str1, AsciiString str2) { return (str2 == str1); }
inline bool operator != (const char* str1, AsciiString str2) { return (str2 != str1); }
inline bool operator <  (const char* str1, AsciiString str2) { return (str2 >  str1); }
inline bool operator <= (const char* str1, AsciiString str2) { return (str2 >= str1); }
inline bool operator >  (const char* str1, AsciiString str2) { return (str2 <  str1); }
inline bool operator >= (const char* str1, AsciiString str2) { return (str2 <= str1); }

//AsciiString and UNICHAR
inline int32 AsciiString::Compare(UNICHAR ch, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), &ch, 1, casesense); }
inline bool AsciiString::Equals(UNICHAR ch, CaseSensitivity casesense) const { return Compare(ch, casesense) == 0; }
inline bool AsciiString::operator == (UNICHAR ch) const { return Equals(ch, CaseSensitivity::CaseSensitive); }
inline bool AsciiString::operator != (UNICHAR ch) const { return Equals(ch, CaseSensitivity::CaseSensitive) == false; }
inline bool AsciiString::operator <  (UNICHAR ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) <  0; }
inline bool AsciiString::operator <= (UNICHAR ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) <= 0; }
inline bool AsciiString::operator >  (UNICHAR ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) >  0; }
inline bool AsciiString::operator >= (UNICHAR ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) >= 0; }
inline bool operator == (UNICHAR ch, AsciiString str) { return (str == ch); }
inline bool operator != (UNICHAR ch, AsciiString str) { return (str != ch); }
inline bool operator <  (UNICHAR ch, AsciiString str) { return (str >  ch); }
inline bool operator <= (UNICHAR ch, AsciiString str) { return (str >= ch); }
inline bool operator >  (UNICHAR ch, AsciiString str) { return (str <  ch); }
inline bool operator >= (UNICHAR ch, AsciiString str) { return (str <= ch); }

//AsciiString and const UNICHAR*
inline int32 AsciiString::Compare(const UNICHAR* str, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), str, CStringTraitsU::Strlen(str), casesense); }
inline bool AsciiString::Equals(const UNICHAR* str, CaseSensitivity casesense) const { return Compare(str, casesense) == 0; }
inline bool AsciiString::operator == (const UNICHAR* str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool AsciiString::operator != (const UNICHAR* str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool AsciiString::operator <  (const UNICHAR* str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool AsciiString::operator <= (const UNICHAR* str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool AsciiString::operator >  (const UNICHAR* str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool AsciiString::operator >= (const UNICHAR* str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }
inline bool operator == (const UNICHAR* str1, AsciiString str2) { return (str2 == str1); }
inline bool operator != (const UNICHAR* str1, AsciiString str2) { return (str2 != str1); }
inline bool operator <  (const UNICHAR* str1, AsciiString str2) { return (str2 >  str1); }
inline bool operator <= (const UNICHAR* str1, AsciiString str2) { return (str2 >= str1); }
inline bool operator >  (const UNICHAR* str1, AsciiString str2) { return (str2 <  str1); }
inline bool operator >= (const UNICHAR* str1, AsciiString str2) { return (str2 <= str1); }



//-----------------------------------------------------------------------------
// UString
//-----------------------------------------------------------------------------

//UString and UString
inline int32 UString::Compare(const UString& str, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), str.ConstData(), str.Len(), casesense); }
inline bool UString::Equals(const UString& str, CaseSensitivity casesense) const { return Len() == str.Len() && Compare(str, casesense) == 0; }
inline bool UString::operator == (const UString& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool UString::operator != (const UString& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool UString::operator <  (const UString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool UString::operator <= (const UString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool UString::operator >  (const UString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool UString::operator >= (const UString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//UString and UStringRef
inline int32 UString::Compare(const UStringRef& str, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), str.ConstData(), str.Len(), casesense); }
inline bool UString::Equals(const UStringRef& str, CaseSensitivity casesense) const { return Len() == str.Len() && Compare(str, casesense) == 0; }
inline bool UString::operator == (const UStringRef& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool UString::operator != (const UStringRef& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool UString::operator <  (const UStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool UString::operator <= (const UStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool UString::operator >  (const UStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool UString::operator >= (const UStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//UString and UStringView
inline int32 UString::Compare(const UStringView& str, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), str.ConstData(), str.Len(), casesense); }
inline bool UString::Equals(const UStringView& str, CaseSensitivity casesense) const { return Len() == str.Len() && Compare(str, casesense) == 0; }
inline bool UString::operator == (const UStringView& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool UString::operator != (const UStringView& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool UString::operator <  (const UStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool UString::operator <= (const UStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool UString::operator >  (const UStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool UString::operator >= (const UStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//UString and AsciiString
inline int32 UString::Compare(const AsciiString& str, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), str.ConstData(), str.Len(), casesense); }
inline bool UString::Equals(const AsciiString& str, CaseSensitivity casesense) const { return Len() == str.Len() && Compare(str, casesense) == 0; }
inline bool UString::operator == (const AsciiString& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool UString::operator != (const AsciiString& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool UString::operator <  (const AsciiString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool UString::operator <= (const AsciiString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool UString::operator >  (const AsciiString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool UString::operator >= (const AsciiString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//UString and ByteString
inline int32 UString::Compare(const ByteString& str, CaseSensitivity casesense) const {
  UtfConversionBuffer<char,UNICHAR> str_as_uni(str.ConstData(), str.Len());
  return StringCmp::Compare(ConstData(), Len(), str_as_uni.ConstData(), str_as_uni.Len(), casesense);
}
inline bool UString::Equals(const ByteString& str, CaseSensitivity casesense) const { return Compare(str, casesense) == 0; }
inline bool UString::operator == (const ByteString& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool UString::operator != (const ByteString& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool UString::operator <  (const ByteString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool UString::operator <= (const ByteString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool UString::operator >  (const ByteString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool UString::operator >= (const ByteString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//UString and ByteStringRef
inline int32 UString::Compare(const ByteStringRef& str, CaseSensitivity casesense) const {
  UtfConversionBuffer<char,UNICHAR> str_as_uni(str.ConstData(), str.Len());
  return StringCmp::Compare(ConstData(), Len(), str_as_uni.ConstData(), str_as_uni.Len(), casesense);
}
inline bool UString::Equals(const ByteStringRef& str, CaseSensitivity casesense) const { return Compare(str, casesense) == 0; }
inline bool UString::operator == (const ByteStringRef& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool UString::operator != (const ByteStringRef& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool UString::operator <  (const ByteStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool UString::operator <= (const ByteStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool UString::operator >  (const ByteStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool UString::operator >= (const ByteStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//UString and ByteStringView
inline int32 UString::Compare(const ByteStringView& str, CaseSensitivity casesense) const {
  UtfConversionBuffer<char,UNICHAR> str_as_uni(str.ConstData(), str.Len());
  return StringCmp::Compare(ConstData(), Len(), str_as_uni.ConstData(), str_as_uni.Len(), casesense);
}
inline bool UString::Equals(const ByteStringView& str, CaseSensitivity casesense) const { return Compare(str, casesense) == 0; }
inline bool UString::operator == (const ByteStringView& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool UString::operator != (const ByteStringView& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool UString::operator <  (const ByteStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool UString::operator <= (const ByteStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool UString::operator >  (const ByteStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool UString::operator >= (const ByteStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//UString and char
inline int32 UString::Compare(char ch, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), &ch, 1, casesense); }
inline bool UString::Equals(char ch, CaseSensitivity casesense) const { return Len() == 1 && Compare(ch, casesense) == 0; }
inline bool UString::operator == (char ch) const { return Equals(ch, CaseSensitivity::CaseSensitive); }
inline bool UString::operator != (char ch) const { return Equals(ch, CaseSensitivity::CaseSensitive) == false; }
inline bool UString::operator <  (char ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) <  0; }
inline bool UString::operator <= (char ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) <= 0; }
inline bool UString::operator >  (char ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) >  0; }
inline bool UString::operator >= (char ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) >= 0; }
inline bool operator == (char ch, const UString& str) { return (str == ch); }
inline bool operator != (char ch, const UString& str) { return (str != ch); }
inline bool operator <  (char ch, const UString& str) { return (str >  ch); }
inline bool operator <= (char ch, const UString& str) { return (str >= ch); }
inline bool operator >  (char ch, const UString& str) { return (str <  ch); }
inline bool operator >= (char ch, const UString& str) { return (str <= ch); }

//UString and const char*
inline int32 UString::Compare(const char* str, CaseSensitivity casesense) const {
  UtfConversionBuffer<char,UNICHAR> str_as_uni(str);
  return StringCmp::Compare(ConstData(), Len(), str_as_uni.ConstData(), str_as_uni.Len(), casesense);
}
inline bool UString::Equals(const char* str, CaseSensitivity casesense) const { return Compare(str, casesense) == 0; }
inline bool UString::operator == (const char* str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool UString::operator != (const char* str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool UString::operator <  (const char* str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool UString::operator <= (const char* str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool UString::operator >  (const char* str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool UString::operator >= (const char* str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }
inline bool operator == (const char* str1, const UString& str2) { return (str2 == str1); }
inline bool operator != (const char* str1, const UString& str2) { return (str2 != str1); }
inline bool operator <  (const char* str1, const UString& str2) { return (str2 >  str1); }
inline bool operator <= (const char* str1, const UString& str2) { return (str2 >= str1); }
inline bool operator >  (const char* str1, const UString& str2) { return (str2 <  str1); }
inline bool operator >= (const char* str1, const UString& str2) { return (str2 <= str1); }

//UString and UNICHAR
inline int32 UString::Compare(UNICHAR ch, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), &ch, 1, casesense); }
inline bool UString::Equals(UNICHAR ch, CaseSensitivity casesense) const { return Len() == 1 && Compare(ch, casesense) == 0; }
inline bool UString::operator == (UNICHAR ch) const { return Equals(ch, CaseSensitivity::CaseSensitive); }
inline bool UString::operator != (UNICHAR ch) const { return Equals(ch, CaseSensitivity::CaseSensitive) == false; }
inline bool UString::operator <  (UNICHAR ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) <  0; }
inline bool UString::operator <= (UNICHAR ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) <= 0; }
inline bool UString::operator >  (UNICHAR ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) >  0; }
inline bool UString::operator >= (UNICHAR ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) >= 0; }
inline bool operator == (UNICHAR ch, const UString& str) { return (str == ch); }
inline bool operator != (UNICHAR ch, const UString& str) { return (str != ch); }
inline bool operator <  (UNICHAR ch, const UString& str) { return (str >  ch); }
inline bool operator <= (UNICHAR ch, const UString& str) { return (str >= ch); }
inline bool operator >  (UNICHAR ch, const UString& str) { return (str <  ch); }
inline bool operator >= (UNICHAR ch, const UString& str) { return (str <= ch); }

//UString and const UNICHAR*
inline int32 UString::Compare(const UNICHAR* str, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), str, CStringTraitsU::Strlen(str), casesense); }
inline bool UString::Equals(const UNICHAR* str, CaseSensitivity casesense) const { return Compare(str, casesense) == 0; }
inline bool UString::operator == (const UNICHAR* str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool UString::operator != (const UNICHAR* str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool UString::operator <  (const UNICHAR* str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool UString::operator <= (const UNICHAR* str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool UString::operator >  (const UNICHAR* str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool UString::operator >= (const UNICHAR* str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }
inline bool operator == (const UNICHAR* str1, const UString& str2) { return (str2 == str1); }
inline bool operator != (const UNICHAR* str1, const UString& str2) { return (str2 != str1); }
inline bool operator <  (const UNICHAR* str1, const UString& str2) { return (str2 >  str1); }
inline bool operator <= (const UNICHAR* str1, const UString& str2) { return (str2 >= str1); }
inline bool operator >  (const UNICHAR* str1, const UString& str2) { return (str2 <  str1); }
inline bool operator >= (const UNICHAR* str1, const UString& str2) { return (str2 <= str1); }



//-----------------------------------------------------------------------------
// UStringRef
//-----------------------------------------------------------------------------

inline int32 UStringRef::Compare(const UStringRef& str, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), str.ConstData(), str.Len(), casesense); }
inline bool UStringRef::Equals(const UStringRef& str, CaseSensitivity casesense) const { return Len() == str.Len() && Compare(str, casesense) == 0; }
inline bool UStringRef::operator == (const UStringRef& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool UStringRef::operator != (const UStringRef& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool UStringRef::operator <  (const UStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool UStringRef::operator <= (const UStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool UStringRef::operator >  (const UStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool UStringRef::operator >= (const UStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//UStringRef and UString
inline int32 UStringRef::Compare(const UString& str, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), str.ConstData(), str.Len(), casesense); }
inline bool UStringRef::Equals(const UString& str, CaseSensitivity casesense) const { return Len() == str.Len() && Compare(str, casesense) == 0; }
inline bool UStringRef::operator == (const UString& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool UStringRef::operator != (const UString& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool UStringRef::operator <  (const UString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool UStringRef::operator <= (const UString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool UStringRef::operator >  (const UString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool UStringRef::operator >= (const UString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//UStringRef and UStringView
inline int32 UStringRef::Compare(const UStringView& str, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), str.ConstData(), str.Len(), casesense); }
inline bool UStringRef::Equals(const UStringView& str, CaseSensitivity casesense) const { return Len() == str.Len() && Compare(str, casesense) == 0; }
inline bool UStringRef::operator == (const UStringView& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool UStringRef::operator != (const UStringView& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool UStringRef::operator <  (const UStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool UStringRef::operator <= (const UStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool UStringRef::operator >  (const UStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool UStringRef::operator >= (const UStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//UStringRef and AsciiString
inline int32 UStringRef::Compare(const AsciiString& str, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), str.ConstData(), str.Len(), casesense); }
inline bool UStringRef::Equals(const AsciiString& str, CaseSensitivity casesense) const { return Len() == str.Len() && Compare(str, casesense) == 0; }
inline bool UStringRef::operator == (const AsciiString& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool UStringRef::operator != (const AsciiString& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool UStringRef::operator <  (const AsciiString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool UStringRef::operator <= (const AsciiString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool UStringRef::operator >  (const AsciiString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool UStringRef::operator >= (const AsciiString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//UStringRef and ByteString
inline int32 UStringRef::Compare(const ByteString& str, CaseSensitivity casesense) const {
  UtfConversionBuffer<char,UNICHAR> str_as_uni(str.ConstData(), str.Len());
  return StringCmp::Compare(ConstData(), Len(), str_as_uni.ConstData(), str_as_uni.Len(), casesense);
}
inline bool UStringRef::Equals(const ByteString& str, CaseSensitivity casesense) const { return Compare(str, casesense) == 0; }
inline bool UStringRef::operator == (const ByteString& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool UStringRef::operator != (const ByteString& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool UStringRef::operator <  (const ByteString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool UStringRef::operator <= (const ByteString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool UStringRef::operator >  (const ByteString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool UStringRef::operator >= (const ByteString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//UStringRef and ByteStringRef
inline int32 UStringRef::Compare(const ByteStringRef& str, CaseSensitivity casesense) const {
  UtfConversionBuffer<char,UNICHAR> str_as_uni(str.ConstData(), str.Len());
  return StringCmp::Compare(ConstData(), Len(), str_as_uni.ConstData(), str_as_uni.Len(), casesense);
}
inline bool UStringRef::Equals(const ByteStringRef& str, CaseSensitivity casesense) const { return Compare(str, casesense) == 0; }
inline bool UStringRef::operator == (const ByteStringRef& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool UStringRef::operator != (const ByteStringRef& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool UStringRef::operator <  (const ByteStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool UStringRef::operator <= (const ByteStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool UStringRef::operator >  (const ByteStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool UStringRef::operator >= (const ByteStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//UStringRef and ByteStringView
inline int32 UStringRef::Compare(const ByteStringView& str, CaseSensitivity casesense) const {
  UtfConversionBuffer<char,UNICHAR> str_as_uni(str.ConstData(), str.Len());
  return StringCmp::Compare(ConstData(), Len(), str_as_uni.ConstData(), str_as_uni.Len(), casesense);
}
inline bool UStringRef::Equals(const ByteStringView& str, CaseSensitivity casesense) const { return Compare(str, casesense) == 0; }
inline bool UStringRef::operator == (const ByteStringView& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool UStringRef::operator != (const ByteStringView& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool UStringRef::operator <  (const ByteStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool UStringRef::operator <= (const ByteStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool UStringRef::operator >  (const ByteStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool UStringRef::operator >= (const ByteStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//UStringRef and char
inline int32 UStringRef::Compare(char ch, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), &ch, 1, casesense); }
inline bool UStringRef::Equals(char ch, CaseSensitivity casesense) const { return Len() == 1 && Compare(ch, casesense) == 0; }
inline bool UStringRef::operator == (char ch) const { return Equals(ch, CaseSensitivity::CaseSensitive); }
inline bool UStringRef::operator != (char ch) const { return Equals(ch, CaseSensitivity::CaseSensitive) == false; }
inline bool UStringRef::operator <  (char ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) <  0; }
inline bool UStringRef::operator <= (char ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) <= 0; }
inline bool UStringRef::operator >  (char ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) >  0; }
inline bool UStringRef::operator >= (char ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) >= 0; }
inline bool operator == (char ch, const UStringRef& str) { return (ch == str); }
inline bool operator != (char ch, const UStringRef& str) { return (ch != str); }
inline bool operator <  (char ch, const UStringRef& str) { return (ch >  str); }
inline bool operator <= (char ch, const UStringRef& str) { return (ch >= str); }
inline bool operator >  (char ch, const UStringRef& str) { return (ch <  str); }
inline bool operator >= (char ch, const UStringRef& str) { return (ch <= str); }

//UStringRef and const char*
inline int32 UStringRef::Compare(const char* str, CaseSensitivity casesense) const {
  UtfConversionBuffer<char,UNICHAR> str_as_uni(str);
  return StringCmp::Compare(ConstData(), Len(), str_as_uni.ConstData(), str_as_uni.Len(), casesense);
}
inline bool UStringRef::Equals(const char* str, CaseSensitivity casesense) const { return Compare(str, casesense) == 0; }
inline bool UStringRef::operator == (const char* str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool UStringRef::operator != (const char* str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool UStringRef::operator <  (const char* str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool UStringRef::operator <= (const char* str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool UStringRef::operator >  (const char* str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool UStringRef::operator >= (const char* str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }
inline bool operator == (const char* str1, const UStringRef& str2) { return (str2 == str1); }
inline bool operator != (const char* str1, const UStringRef& str2) { return (str2 != str1); }
inline bool operator <  (const char* str1, const UStringRef& str2) { return (str2 >  str1); }
inline bool operator <= (const char* str1, const UStringRef& str2) { return (str2 >= str1); }
inline bool operator >  (const char* str1, const UStringRef& str2) { return (str2 <  str1); }
inline bool operator >= (const char* str1, const UStringRef& str2) { return (str2 <= str1); }

//UStringRef and UNICHAR
inline int32 UStringRef::Compare(UNICHAR ch, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), &ch, 1, casesense); }
inline bool UStringRef::Equals(UNICHAR ch, CaseSensitivity casesense) const { return Len() == 1 && Compare(ch, casesense) == 0; }
inline bool UStringRef::operator == (UNICHAR ch) const { return Equals(ch, CaseSensitivity::CaseSensitive); }
inline bool UStringRef::operator != (UNICHAR ch) const { return Equals(ch, CaseSensitivity::CaseSensitive) == false; }
inline bool UStringRef::operator <  (UNICHAR ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) <  0; }
inline bool UStringRef::operator <= (UNICHAR ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) <= 0; }
inline bool UStringRef::operator >  (UNICHAR ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) >  0; }
inline bool UStringRef::operator >= (UNICHAR ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) >= 0; }
inline bool operator == (UNICHAR ch, const UStringRef& str) { return (ch == str); }
inline bool operator != (UNICHAR ch, const UStringRef& str) { return (ch != str); }
inline bool operator <  (UNICHAR ch, const UStringRef& str) { return (ch >  str); }
inline bool operator <= (UNICHAR ch, const UStringRef& str) { return (ch >= str); }
inline bool operator >  (UNICHAR ch, const UStringRef& str) { return (ch <  str); }
inline bool operator >= (UNICHAR ch, const UStringRef& str) { return (ch <= str); }

//UStringRef and const UNICHAR*
inline int32 UStringRef::Compare(const UNICHAR* str, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), str, CStringTraitsU::Strlen(str), casesense); }
inline bool UStringRef::Equals(const UNICHAR* str, CaseSensitivity casesense) const { return Compare(str, casesense) == 0; }
inline bool UStringRef::operator == (const UNICHAR* str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool UStringRef::operator != (const UNICHAR* str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool UStringRef::operator <  (const UNICHAR* str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool UStringRef::operator <= (const UNICHAR* str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool UStringRef::operator >  (const UNICHAR* str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool UStringRef::operator >= (const UNICHAR* str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }
inline bool operator == (const UNICHAR* str1, const UStringRef& str2) { return (str2 == str1); }
inline bool operator != (const UNICHAR* str1, const UStringRef& str2) { return (str2 != str1); }
inline bool operator <  (const UNICHAR* str1, const UStringRef& str2) { return (str2 >  str1); }
inline bool operator <= (const UNICHAR* str1, const UStringRef& str2) { return (str2 >= str1); }
inline bool operator >  (const UNICHAR* str1, const UStringRef& str2) { return (str2 <  str1); }
inline bool operator >= (const UNICHAR* str1, const UStringRef& str2) { return (str2 <= str1); }



//-----------------------------------------------------------------------------
// UStringView
//-----------------------------------------------------------------------------

//UStringView and UStringView
inline int32 UStringView::Compare(const UStringView& str, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), str.ConstData(), str.Len(), casesense); }
inline bool UStringView::Equals(const UStringView& str, CaseSensitivity casesense) const { return Len() == str.Len() && Compare(str, casesense) == 0; }
inline bool UStringView::operator == (const UStringView& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool UStringView::operator != (const UStringView& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool UStringView::operator <  (const UStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool UStringView::operator <= (const UStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool UStringView::operator >  (const UStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool UStringView::operator >= (const UStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//UStringView and ByteString
inline int32 UStringView::Compare(const ByteString& str, CaseSensitivity casesense) const {
  UtfConversionBuffer<char,UNICHAR> str_as_uni(str.ConstData(), str.Len());
  return StringCmp::Compare(ConstData(), Len(), str_as_uni.ConstData(), str_as_uni.Len(), casesense);
}
inline bool UStringView::Equals(const ByteString& str, CaseSensitivity casesense) const { return Compare(str, casesense) == 0; }
inline bool UStringView::operator == (const ByteString& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool UStringView::operator != (const ByteString& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool UStringView::operator <  (const ByteString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool UStringView::operator <= (const ByteString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool UStringView::operator >  (const ByteString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool UStringView::operator >= (const ByteString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//UStringView and ByteStringRef
inline int32 UStringView::Compare(const ByteStringRef& str, CaseSensitivity casesense) const {
  UtfConversionBuffer<char,UNICHAR> str_as_uni(str.ConstData(), str.Len());
  return StringCmp::Compare(ConstData(), Len(), str_as_uni.ConstData(), str_as_uni.Len(), casesense);
}
inline bool UStringView::Equals(const ByteStringRef& str, CaseSensitivity casesense) const { return Compare(str, casesense) == 0; }
inline bool UStringView::operator == (const ByteStringRef& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool UStringView::operator != (const ByteStringRef& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool UStringView::operator <  (const ByteStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool UStringView::operator <= (const ByteStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool UStringView::operator >  (const ByteStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool UStringView::operator >= (const ByteStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//UStringView and ByteStringView
inline int32 UStringView::Compare(const ByteStringView& str, CaseSensitivity casesense) const {
  UtfConversionBuffer<char,UNICHAR> str_as_uni(str.ConstData(), str.Len());
  return StringCmp::Compare(ConstData(), Len(), str_as_uni.ConstData(), str_as_uni.Len(), casesense);
}
inline bool UStringView::Equals(const ByteStringView& str, CaseSensitivity casesense) const { return Compare(str, casesense) == 0; }
inline bool UStringView::operator == (const ByteStringView& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool UStringView::operator != (const ByteStringView& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool UStringView::operator <  (const ByteStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool UStringView::operator <= (const ByteStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool UStringView::operator >  (const ByteStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool UStringView::operator >= (const ByteStringView& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//UStringView and AsciiString
inline int32 UStringView::Compare(const AsciiString& str, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), str.ConstData(), str.Len(), casesense); }
inline bool UStringView::Equals(const AsciiString& str, CaseSensitivity casesense) const { return Len() == str.Len() && Compare(str, casesense) == 0; }
inline bool UStringView::operator == (const AsciiString& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool UStringView::operator != (const AsciiString& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool UStringView::operator <  (const AsciiString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool UStringView::operator <= (const AsciiString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool UStringView::operator >  (const AsciiString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool UStringView::operator >= (const AsciiString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//UStringView and UString
inline int32 UStringView::Compare(const UString& str, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), str.ConstData(), str.Len(), casesense); }
inline bool UStringView::Equals(const UString& str, CaseSensitivity casesense) const { return Len() == str.Len() && Compare(str, casesense) == 0; }
inline bool UStringView::operator == (const UString& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool UStringView::operator != (const UString& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool UStringView::operator <  (const UString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool UStringView::operator <= (const UString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool UStringView::operator >  (const UString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool UStringView::operator >= (const UString& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//UStringView and UStringRef
inline int32 UStringView::Compare(const UStringRef& str, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), str.ConstData(), str.Len(), casesense); }
inline bool UStringView::Equals(const UStringRef& str, CaseSensitivity casesense) const { return Len() == str.Len() && Compare(str, casesense) == 0; }
inline bool UStringView::operator == (const UStringRef& str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool UStringView::operator != (const UStringRef& str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool UStringView::operator <  (const UStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool UStringView::operator <= (const UStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool UStringView::operator >  (const UStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool UStringView::operator >= (const UStringRef& str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }

//UStringView and char
inline int32 UStringView::Compare(char ch, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), &ch, 1, casesense); }
inline bool UStringView::Equals(char ch, CaseSensitivity casesense) const { return Len() == 1 && Compare(ch, casesense) == 0; }
inline bool UStringView::operator == (char ch) const { return Equals(ch, CaseSensitivity::CaseSensitive); }
inline bool UStringView::operator != (char ch) const { return Equals(ch, CaseSensitivity::CaseSensitive) == false; }
inline bool UStringView::operator <  (char ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) <  0; }
inline bool UStringView::operator <= (char ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) <= 0; }
inline bool UStringView::operator >  (char ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) >  0; }
inline bool UStringView::operator >= (char ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) >= 0; }
inline bool operator == (UNICHAR ch, const UStringView& str) { return (str == ch); }
inline bool operator != (UNICHAR ch, const UStringView& str) { return (str != ch); }
inline bool operator <  (UNICHAR ch, const UStringView& str) { return (str >  ch); }
inline bool operator <= (UNICHAR ch, const UStringView& str) { return (str >= ch); }
inline bool operator >  (UNICHAR ch, const UStringView& str) { return (str <  ch); }
inline bool operator >= (UNICHAR ch, const UStringView& str) { return (str <= ch); }

//UStringView and const char*
inline int32 UStringView::Compare(const char* str, CaseSensitivity casesense) const {
  UtfConversionBuffer<char,UNICHAR> str_as_uni(str);
  return StringCmp::Compare(ConstData(), Len(), str_as_uni.ConstData(), str_as_uni.Len(), casesense);
}
inline bool UStringView::Equals(const char* str, CaseSensitivity casesense) const { return Compare(str, casesense) == 0; }
inline bool UStringView::operator == (const char* str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool UStringView::operator != (const char* str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool UStringView::operator <  (const char* str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool UStringView::operator <= (const char* str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool UStringView::operator >  (const char* str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool UStringView::operator >= (const char* str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }
inline bool operator == (const char* str1, const UStringView& str2) { return (str2 == str1); }
inline bool operator != (const char* str1, const UStringView& str2) { return (str2 != str1); }
inline bool operator <  (const char* str1, const UStringView& str2) { return (str2 >  str1); }
inline bool operator <= (const char* str1, const UStringView& str2) { return (str2 >= str1); }
inline bool operator >  (const char* str1, const UStringView& str2) { return (str2 <  str1); }
inline bool operator >= (const char* str1, const UStringView& str2) { return (str2 <= str1); }

//UStringView and UNICHAR
inline int32 UStringView::Compare(UNICHAR ch, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), &ch, 1, casesense); }
inline bool UStringView::Equals(UNICHAR ch, CaseSensitivity casesense) const { return Len() == 1 && Compare(ch, casesense) == 0; }
inline bool UStringView::operator == (UNICHAR ch) const { return Equals(ch, CaseSensitivity::CaseSensitive); }
inline bool UStringView::operator != (UNICHAR ch) const { return Equals(ch, CaseSensitivity::CaseSensitive) == false; }
inline bool UStringView::operator <  (UNICHAR ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) <  0; }
inline bool UStringView::operator <= (UNICHAR ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) <= 0; }
inline bool UStringView::operator >  (UNICHAR ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) >  0; }
inline bool UStringView::operator >= (UNICHAR ch) const { return Compare(ch, CaseSensitivity::CaseSensitive) >= 0; }
inline bool operator == (char ch, const UStringView& str) { return (str == ch); }
inline bool operator != (char ch, const UStringView& str) { return (str != ch); }
inline bool operator <  (char ch, const UStringView& str) { return (str >  ch); }
inline bool operator <= (char ch, const UStringView& str) { return (str >= ch); }
inline bool operator >  (char ch, const UStringView& str) { return (str <  ch); }
inline bool operator >= (char ch, const UStringView& str) { return (str <= ch); }

//UStringView and const UNICHAR*
inline int32 UStringView::Compare(const UNICHAR* str, CaseSensitivity casesense) const { return StringCmp::Compare(ConstData(), Len(), str, CStringTraitsU::Strlen(str), casesense); }
inline bool UStringView::Equals(const UNICHAR* str, CaseSensitivity casesense) const { return Compare(str, casesense) == 0; }
inline bool UStringView::operator == (const UNICHAR* str) const { return Equals(str, CaseSensitivity::CaseSensitive); }
inline bool UStringView::operator != (const UNICHAR* str) const { return Equals(str, CaseSensitivity::CaseSensitive) == false; }
inline bool UStringView::operator <  (const UNICHAR* str) const { return Compare(str, CaseSensitivity::CaseSensitive) <  0; }
inline bool UStringView::operator <= (const UNICHAR* str) const { return Compare(str, CaseSensitivity::CaseSensitive) <= 0; }
inline bool UStringView::operator >  (const UNICHAR* str) const { return Compare(str, CaseSensitivity::CaseSensitive) >  0; }
inline bool UStringView::operator >= (const UNICHAR* str) const { return Compare(str, CaseSensitivity::CaseSensitive) >= 0; }
inline bool operator == (const UNICHAR* str1, const UStringView& str2) { return (str2 == str1); }
inline bool operator != (const UNICHAR* str1, const UStringView& str2) { return (str2 != str1); }
inline bool operator <  (const UNICHAR* str1, const UStringView& str2) { return (str2 >  str1); }
inline bool operator <= (const UNICHAR* str1, const UStringView& str2) { return (str2 >= str1); }
inline bool operator >  (const UNICHAR* str1, const UStringView& str2) { return (str2 <  str1); }
inline bool operator >= (const UNICHAR* str1, const UStringView& str2) { return (str2 <= str1); }
