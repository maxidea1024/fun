#include "fun/base/locale/locale_tool.h"
#include "fun/base/bundle/v8/double-conversion.h"

namespace fun {

void DoubleToAscii( double d,
                    LocaleData::DoubleForm form,
                    int32 precision,
                    char* buf,
                    char buf_len,
                    bool& sign,
                    int32& len,
                    int32& dec_pt)
{
  if (buf_len == 0) {
    dec_pt = 0;
    sign = d < 0;
    len = 0;
    return;
  }

  // Detect special numbers (nan, +/-inf)
  // We cannot use the high-level API of libdouble-conversion as we need to apply locale-specific
  // formatting, such as decimal points, thousands-separators, etc. Because of this, we have to
  // check for infinity and NaN before calling DoubleToAscii.
  if (std::isinf(d)) {
    sign = d < 0;
    if (buf_len >= 3) {
      buf[0] = 'i';
      buf[1] = 'n';
      buf[2] = 'f';
      len = 3;
    }
    else {
      len = 0;
    }

    return;
  }
  else if (std::isnan(d)) {
    if (buf_len >= 3) {
      buf[0] = 'n';
      buf[1] = 'a';
      buf[2] = 'n';
      len = 3;
    }
    else {
      len = 0;
    }

    return;
  }

  if (form == LocaleData::DFSignificantDigits && precision == 0) {
    precision = 1; // 0 significant digits is silently converted to 1
  }

  // one digit before the decimal dot, counts as significant digit for DoubleToStringConverter
  if (form == LocaleData::DFExponent && precision >= 0) {
    ++precision;
  }

  double_conversion::DoubleToStringConverter::DtoaMode mode;
  if (precision == Locale::FloatingPointShortest) {
    mode = double_conversion::DoubleToStringConverter::SHORTEST;
  }
  else if (form == LocaleData::DFSignificantDigits || form == LocaleData::DFExponent) {
    mode = double_conversion::DoubleToStringConverter::PRECISION;
  }
  else {
    mode = double_conversion::DoubleToStringConverter::FIXED;
  }

  double_conversion::DoubleToStringConverter::DoubleToAscii(d, mode, precision, buf, buf_len, &sign, &len, &dec_pt);
  while (len > 1 && buf[len - 1] == '0') { // drop trailing zeroes
    --len;
  }
}

double AsciiToDouble( const char* num,
                      int32 num_len,
                      bool& ok,
                      int32& processed,
                      TrailingJunkMode trailing_junk_mode)
{
  if (*num == '\0') {
    ok = false;
    processed = 0;
    return 0.0;
  }

  ok = true;

  // We have to catch NaN before because we need NaN as marker for "garbage" in the
  // libdouble-conversion case and, in contrast to libdouble-conversion or sscanf, we don't allow
  // "-nan" or "+nan"
  if (CStringTraitsA::Strncmp(num, "nan", 3) == 0) {
    processed = 3;
    return std::numeric_limits<double>::signaling_NaN();
  }
  else if ((num[0] == '-' || num[0] == '+') && CStringTraitsA::Strncmp(num + 1, "nan", 3) == 0) {
    processed = 0;
    ok = false;
    return 0.0;
  }

  // Infinity values are implementation defined in the sscanf case. in the libdouble-conversion
  // case we need infinity as overflow marker.
  if (CStringTraitsA::Strncmp(num, "+inf", 4) == 0) {
    processed = 4;
    return std::numeric_limits<double>::infinity();
  }
  else if (CStringTraitsA::Strncmp(num, "inf", 3) == 0) {
    processed = 3;
    return std::numeric_limits<double>::infinity();
  }
  else if (CStringTraitsA::Strncmp(num, "-inf", 4) == 0) {
    processed = 4;
    return -std::numeric_limits<double>::infinity();
  }

  double d = 0.0;
  int32 conv_flags = (trailing_junk_mode == TrailingJunkAllowed) ?
              double_conversion::StringToDoubleConverter::ALLOW_TRAILING_JUNK :
              double_conversion::StringToDoubleConverter::NO_FLAGS;
  double_conversion::StringToDoubleConverter cnv(conv_flags, 0.0, std::numeric_limits<double>::signaling_NaN(), 0, 0);
  d = cnv.StringToDouble(num, num_len, &processed);

  if (!std::isfinite(d)) {
    ok = false;
    if (std::isnan(d)) {
      // Garbage found. We don't accept it and return 0.
      processed = 0;
      return 0.0;
    }
    else {
      // Overflow. That's not OK, but we still return infinity.
      return d;
    }
  }

  // Otherwise we would have gotten NaN or sorted it out above.
  fun_check(trailing_junk_mode == TrailingJunkAllowed || processed == num_len);

  // Check if underflow has occurred.
  if (IsZero(d)) {
    for (int32 i = 0; i < processed; ++i) {
      if (num[i] >= '1' && num[i] <= '9') {
        // if a digit before any 'e' is not 0, then a non-zero number was intended.
        ok = false;
        return 0.0;
      }
      else if (num[i] == 'e') {
        break;
      }
    }
  }

  return d;
}

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
                      bool leading_zero_in_exponent)
{
  const int32 exp = dec_pt - 1;

  if (pm == PMDecimalDigits) {
    for (int32 i = digits.Len(); i < precision + 1; ++i) {
      digits.Append(zero);
    }
  }
  else if (pm == PMSignificantDigits) {
    for (int32 i = digits.Len(); i < precision; ++i) {
      digits.Append(zero);
    }
  }
  else { // pm == PMChopTrailingZeros
  }

  if (always_show_dec_pt || digits.Len() > 1) {
    digits.Insert(1, decimal);
  }

  digits.Append(exponential);
  digits.Append(LocaleData::Int64ToString(zero, group, plus, minus,
                                          exp,
                                          leading_zero_in_exponent ? 2 : 1, 10, -1,
                                          LocaleData::AlwaysShowSign));

  return digits;
}

UString& DecimalForm( UNICHAR zero,
                      UNICHAR decimal,
                      UNICHAR group,
                      UString& digits,
                      int32 dec_pt,
                      int32 precision,
                      PrecisionMode pm,
                      bool always_show_dec_pt,
                      bool thousands_group)
{
  if (dec_pt < 0) {
    for (int32 i = 0; i < -dec_pt; ++i) {
      digits.Prepend(zero);
    }
    dec_pt = 0;
  }
  else if (dec_pt > digits.Len()) {
    for (int32 i = digits.Len(); i < dec_pt; ++i) {
      digits.Append(zero);
    }
  }

  if (pm == PMDecimalDigits) {
    int32 decimal_digit_count = digits.Len() - dec_pt;
    for (int32 i = decimal_digit_count; i < precision; ++i) {
      digits.Append(zero);
    }
  }
  else if (pm == PMSignificantDigits) {
    for (int32 i = digits.Len(); i < precision; ++i) {
      digits.Append(zero);
    }
  }
  else { // pm == PMChopTrailingZeros
  }

  if (always_show_dec_pt || dec_pt < digits.Len()) {
    digits.Insert(dec_pt, decimal);
  }

  if (thousands_group) {
    for (int32 i = dec_pt - 3; i > 0; i -= 3) {
      digits.Insert(i, group);
    }
  }

  if (dec_pt == 0) {
    digits.Prepend(zero);
  }

  return digits;
}

} // namespace fun
