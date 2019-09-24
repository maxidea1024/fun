#pragma once

#include "fun/base/base.h"
//#include "fun/base/string/uni_string.h"
#include "fun/base/locale/locale_data.h"
#include "fun/base/locale/locale_private.h"

namespace fun {

class UString;

enum TrailingJunkMode {
  TrailingJunkProhibited,
  TrailingJunkAllowed
};


FUN_BASE_API
void DoubleToAscii( double d,
                    LocaleData::DoubleForm form,
                    int32 precision,
                    char* buf,
                    int32 buf_len,
                    bool& sign,
                    int32& length,
                    int32& dec_pt);

FUN_BASE_API
double AsciiToDouble( const char* num,
                      int32 num_len,
                      bool& ok,
                      int32& processed,
                      TrailingJunkMode trailing_junk_mode = TrailingJunkProhibited);

//QString qulltoa(qulonglong l, int base, const QChar _zero);
//QString qlltoa(qlonglong l, int base, const QChar zero);
//Q_CORE_EXPORT QString qdtoa(qreal d, int *decpt, int *sign);

enum PrecisionMode {
  //PM_DECIMAL_DIGITS
  PMDecimalDigits = 0x01,

  //PM_SIGNIFICIANT_DIGITS
  PMSignificantDigits = 0x02,

  //CHOP_TRAILING_ZEROS
  PMChopTrailingZeros = 0x03
};


FUN_BASE_API
UString& ExponentForm(UNICHAR zero,
                      UNICHAR decimal,
                      UNICHAR exponential,
                      UNICHAR group,
                      UNICHAR plus,
                      UNICHAR minus,
                      UString& digits,
                      int32 dec_pt,
                      int32 precision,
                      PrecisionMode pm,
                      bool always_show_dec_pt,
                      bool leading_zero_in_exponent);

FUN_BASE_API
UString& DecimalForm( UNICHAR zero,
                      UNICHAR decimal,
                      UNICHAR group,
                      UString& digits,
                      int32 dec_pt,
                      int32 precision,
                      PrecisionMode pm,
                      bool always_show_dec_pt,
                      bool thousands_group);


FUN_ALWAYS_INLINE bool IsZero(double d)
{
  const uint8* ch = (const uint8*)&d;
#if FUN_ARCH_BIG_ENDIAN
  return !(ch[0] & 0x7F || ch[1] || ch[2] || ch[3] || ch[4] || ch[5] || ch[6] || ch[7]);
#else
  return !(ch[7] & 0x7F || ch[6] || ch[5] || ch[4] || ch[3] || ch[2] || ch[1] || ch[0]);
#endif
}

//TODO 아래의 함수들은 char_traits에 위치시키면 될듯...
//TODO 아래의 함수들은 char_traits에 위치시키면 될듯...
//TODO 아래의 함수들은 char_traits에 위치시키면 될듯...

//Q_CORE_EXPORT double qstrtod(const char *s00, char const **se, bool *ok);
//Q_CORE_EXPORT double qstrntod(const char *s00, int len, char const **se, bool *ok);
//qlonglong qstrtoll(const char *nptr, const char **endptr, int base, bool *ok);
//qulonglong qstrtoull(const char *nptr, const char **endptr, int base, bool *ok);

} // namespace fun
