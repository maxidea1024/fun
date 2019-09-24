#if 0


#include "fun/base/base.h"


namespace fun {


typedef int32_t (*Ptr_u_strToCase)(UChar *dst, int32_t destCapacity, const UChar *src, int32_t srcLength, const char *locale, UErrorCode *pErrorCode);


{

  bool StrToCase(const UString& str, UString* out, const char* locale_id, Ptr_u_strToCase case_func)
  {
    fun_check_ptr(out);

    int32_t len = str.Len();
    len += len >> 2; // add 25% for possible expansions
    UString result(len, Uninitialized);

    UErrorCode status = U_ZERO_ERROR;

    len = case_func(reinterpret_cast<UChar*>(result.MutableData()), result.Len(),
        reinterpret_cast<const UChar*>(str.ConstData()), str.Len(),
        locale_id, &status);

    if (U_FAILURE(status) && status != U_BUFFER_OVERFLOW_ERROR)
    {
      return false;
    }

    if (len < result.Len())
    {
      result.Resize(len);
    }
    else if (len > result.Len())
    {
      // the resulting string is larger than our source string
      result.Resize(len);

      status = U_ZERO_ERROR;
      len = case_func(reinterpret_cast<UChar*>(result.MutableData()), result.Len(),
        reinterpret_cast<const UChar*>(str.ConstData()), str.Len(),
        locale_id, &status);

      if (U_FAILURE(status))
      {
        return false;
      }

      // if the sizes don't match now, we give up.
      if (len != result.Len())
      {
        return false;
      }
    }

    *out = result;
    return true;
  }


} // namespace


UString Icu::ToUpper(const ByteArray& locale_id, const UString& str, bool* ok)
{
  UString out;
  bool result = StrToCase(str, &out, locale_id, u_strToUpper);
  if (ok)
  {
    *ok = result;
  }
  return out;
}


UString Icu::ToLower(const ByteArray& locale_id, const UString& str, bool* ok)
{
  UString out;
  bool result = StrToCase(str, &out, locale_id, u_strToLower);
  if (ok)
  {
    *ok = result;
  }
  return out;
}


} // namespace fun


#endif
