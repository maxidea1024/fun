#pragma once

#include "fun/base/locale/locale.h"
#include "fun/base/variant.h"
#include "fun/base/container/array.h"

namespace fun {

#if !FUN_NO_SYSTEM_LOCALE

class SystemLocale
{
 public:
  SystemLocale();
  virtual ~SystemLocale();

  struct CurrencyToStringArgument {
    CurrencyToStringArgument() {}

    CurrencyToStringArgument(const Variant& v, const UString& s)
      : value(v)
      , symbol(s)
    {
    }

    friend Archive& operator & (Archive& ar, CurrencyToStringArgument& v)
    {
      return ar & v.value & v.symbol;
    }

    Variant value;
    UString symbol;
  };

  enum QueryType {
    LanguageId, // uint32
    CountryId, // uint32
    DecimalPoint, // string
    GroupSeparator, // string
    ZeroDigit, // string
    NegativeSign, // string
    DateFormatLong, // string
    DateFormatShort, // string
    TimeFormatLong, // string
    TimeFormatShort, // string
    DayNameLong, // string, in: int32
    DayNameShort, // string, in: int32
    MonthNameLong, // string, in: int32
    MonthNameShort, // string, in: int32
    DateToStringLong, // string, in: Date
    DateToStringShort, // string in: Date
    TimeToStringLong, // string in: Time
    TimeToStringShort, // string in: Time
    DateTimeFormatLong, // string
    DateTimeFormatShort, // string
    DateTimeToStringLong, // string in: DateTime
    DateTimeToStringShort, // string in: DateTime
    MeasurementSystem, // uint32
    PositiveSign, // string
    AMText, // string
    PMText, // string
    FirstDayOfWeek, // DayOfWeek
    Weekdays, // Array<DayOfWeekType>    - not implemented!
    CurrencySymbol, // string in: CurrencyToStringArgument
    CurrencyToString, // string in: int64, uint64 or double
    UILanguages, // Array<string>
    StringToStandardQuotation, // string in: string to quote
    StringToAlternateQuotation, // string in: string to quote
    ScriptId, // uint32
    ListToSeparatedString, // string
    LocaleChanged, // System locale changed
    EnglishLanguageName, // string
    NativeLanguageName, // string
    NativeCountryName, // string
    StandaloneMonthNameLong, // string, in: int32
    StandaloneMonthNameShort // string, in: int32
  };

  virtual Variant Query(QueryType type, Variant in) const;
  virtual Locale GetFallbackUiLocale() const;

 private:
  SystemLocale(bool);
  friend class SystemLocaleSingleton;
};

// Variant support.
template <>
struct VariantTraits<SystemLocale::CurrencyToStringArgument>
{
  static const int32 Type = VariantTypes::Custom + 1234;
};

#endif // !FUN_NO_SYSTEM_LOCALE

struct LocaleId
{
  static FUN_ALWAYS_INLINE LocaleId FromIds(uint16 language, uint16 script, uint16 country) {
    const LocaleId locale_id { language, script, country };
    return locale_id;
  }

  FUN_ALWAYS_INLINE bool operator == (const LocaleId& rhs) const {
    return language_id == rhs.language_id && script_id == rhs.script_id && country_id == rhs.country_id;
  }

  FUN_ALWAYS_INLINE bool operator != (const LocaleId& rhs) const {
    return language_id != rhs.language_id || script_id != rhs.script_id || country_id != rhs.country_id;
  }

  LocaleId WithLikelySubtagsAdded() const;
  LocaleId WithLikelySubtagsRemoved() const;

  UString GetName(UNICHAR separator = '-') const;

  /** language id */
  uint16 language_id;
  /** script id */
  uint16 script_id;
  /** country id */
  uint16 country_id;
};


//WARNING: static table entry이므로 임의로 필드를 추가하거나, 타입을 변경해서는 안됨!
struct LocaleData
{
  static const LocaleData* FindLocaleData(Locale::Language language, Locale::Script script, Locale::Country country);
  static const LocaleData* C();

  // Maximum number of significant digits needed to represent a double.
  // We cannot use std::numeric_limits here without constexpr.
  static const int32 DoubleMantissaBits = 53;
  static const int32 Log10_2_100000 = 30103; // log10(2) * 100000
  // same as C++11 std::numeric_limits<T>::max_digits10
  static const int32 DoubleMaxSignificant = (DoubleMantissaBits * Log10_2_100000) / 100000 + 2;

  // Maximum number of digits before decimal point to represent a double
  // Same as std::numeric_limits<double>::max_exponent10 + 1
  static const int32 DoubleMaxDigitsBeforeDecimal = 309;

  enum DoubleForm {
    DFExponent = 0,
    DFDecimal,
    DFSignificantDigits,
    DFMax = DFSignificantDigits
  };

  enum Flags {
    NoFlags             = 0,
    AddTrailingZeroes   = 0x01,
    ZeroPadded          = 0x02,
    LeftAdjusted        = 0x04,
    BlankBeforePositive = 0x08,
    AlwaysShowSign      = 0x10,
    ThousandsGroup      = 0x20,
    CapitalEorX         = 0x40,

    ShowBase            = 0x80,
    UppercaseBase       = 0x100,
    ZeroPadExponent     = 0x200,
    ForcePoint          = 0x400
  };

  enum NumberMode {
    IntegerMode,
    DoubleStandardMode,
    DoubleScientificMode
  };

  static UString DoubleToString(UNICHAR zero,
                                UNICHAR plus,
                                UNICHAR minus,
                                UNICHAR Exponent,
                                UNICHAR group,
                                UNICHAR decimal,
                                double value,
                                int32 precision,
                                DoubleForm form,
                                int32 width,
                                uint32 flags);

  static UString Int64ToString( UNICHAR zero,
                                UNICHAR group,
                                UNICHAR plus,
                                UNICHAR minus,
                                int64 value,
                                int32 precision,
                                int32 base,
                                int32 width,
                                uint32 flags);

  static UString UInt64ToString(const UNICHAR zero,
                                const UNICHAR group,
                                const UNICHAR plus,
                                uint64 value,
                                int32 precision,
                                int32 base,
                                int32 width,
                                uint32 flags);

  UString DoubleToString( double value,
                          int32 precision = -1,
                          DoubleForm form = DFSignificantDigits,
                          int32 width = -1,
                          uint32 flags = NoFlags) const;

  UString Int64ToString(int64 value,
                        int32 precision = -1,
                        int32 base = 10,
                        int32 width = -1,
                        uint32 flags = NoFlags) const;

  UString UInt64ToString( uint64 value,
                          int32 precision = -1,
                          int32 base = 10,
                          int32 width = -1,
                          uint32 flags = NoFlags) const;

  // this function is meant to be called with the result of StringToDouble or ByteStringToDouble
  static float ConvertDoubleToFloat(double value, bool* ok)
  {
    //TODO double 버젼 추가하자.
    //if (MathBase::IsInfinite(value)) {
    if (MathBase::IsInfinite((float)value)) {
      return float(value);
    }

    if (MathBase::Abs(value) > float_MAX) {
      if (ok) {
        *ok = false;
      }
      return 0.0f;
    }

    return float(value);
  }

  double StringToDouble(const UNICHAR* str,
                        int32 len,
                        bool* ok,
                        Locale::NumberOptions options) const;

  int64 StringToInt64(const UNICHAR* str,
                      int32 len,
                      int32 base,
                      bool* ok,
                      Locale::NumberOptions options) const;

  uint64 StringToUInt64(const UNICHAR* str,
                        int32 len,
                        int32 base,
                        bool* ok,
                        Locale::NumberOptions options) const;


  typedef Array<char, InlineAllocator<128>> AsciiBuffer;

  bool NumberToCLocale( const UNICHAR* str,
                        int32 len,
                        Locale::NumberOptions number_options,
                        AsciiBuffer& result) const;
  FUN_ALWAYS_INLINE UNICHAR DigitToCLocale(UNICHAR ch) const;


  static double ByteStringToDouble(const char* str, int32 len, bool* ok);
  static int64 ByteStringToInt64(const char* str, int32 len, int32 base, bool* ok);
  static int64 ByteStringToUInt64(const char* str, int32 len, int32 base, bool* ok);


  // 다른 곳에서 유효한 문자인지 체크 하기 위해서 사용함.(UI input validation?)
  /*FUN_BASE_API*/
  bool ValidateChars( const UNICHAR* str,
                      int32 len,
                      NumberMode num_mode,
                      UString& buf,
                      int32 dec_digits = -1,
                      Locale::NumberOptions number_options = Locale::DefaultNumberOptions) const;

 public:
  uint16 language_id, script_id, country_id;

  uint16 decimal, group, list, percent, zero, minus, plus, exponential;
  uint16 quotation_start, quotation_end;
  uint16 alternate_quotation_start, alternate_quotation_end;

  uint16 list_pattern_part_start_idx, list_pattern_part_start_len;
  uint16 list_pattern_part_mid_idx, list_pattern_part_mid_len;
  uint16 list_pattern_part_end_idx, list_pattern_part_end_len;
  uint16 list_pattern_part_two_idx, list_pattern_part_two_len;
  uint16 short_date_format_idx, short_date_format_size;
  uint16 long_date_format_idx, long_date_format_size;
  uint16 short_time_format_idx, short_time_format_len;
  uint16 long_time_format_idx, long_time_format_len;
  uint16 standalone_short_month_names_idx, standalone_short_month_names_len;
  uint16 standalone_long_month_names_idx, standalone_long_month_names_len;
  uint16 standalone_narrow_month_names_idx, standalone_narrow_month_names_len;
  uint16 short_month_names_idx, short_month_names_len;
  uint16 long_month_names_idx, long_month_names_len;
  uint16 narrow_month_names_idx, narrow_month_names_len;
  uint16 standalone_short_day_names_idx, standalone_short_day_names_len;
  uint16 standalone_long_day_names_idx, standalone_long_day_names_len;
  uint16 standalone_narrow_day_names_idx, standalone_narrow_day_names_len;
  uint16 short_day_names_idx, short_day_names_len;
  uint16 long_day_names_idx, long_day_names_len;
  uint16 narrow_day_names_idx, narrow_day_names_len;
  uint16 am_idx, am_len;
  uint16 pm_idx, pm_len;
  char currency_iso_code[3];
  uint16 currency_symbol_idx, currency_symbol_len;
  uint16 currency_native_name_idx, currency_native_name_len;
  uint8 currency_format_idx, currency_format_len;
  uint8 currency_negative_format_idx, currency_negative_format_len;
  uint16 language_endonym_idx, language_endonym_size;
  uint16 country_endonym_idx, country_endonym_size;
  uint16 currency_digits : 2;
  uint16 currency_rounding : 3;
  uint16 first_day_of_week : 3;
  uint16 weekend_start : 3;
  uint16 weekend_end : 3;
};


class LocaleImpl
{
 public:
  static LocaleImpl* Create(const LocaleData* data,
                            Locale::NumberOptions number_options = Locale::DefaultNumberOptions)
  {
    LocaleImpl* ret = new LocaleImpl;
    ret->data = data;
    ret->ref.counter_ = 0;
    ret->number_options = number_options;
    return ret;
  }

  static LocaleImpl* Get(Locale& locale) { return locale.impl_.Get(); }
  static const LocaleImpl* Get(const Locale& locale) { return locale.impl_.Get(); }

  //FIXME 문자로 하는게 좋을지, 문자열로 하는게 좋을지...?
  UNICHAR GetDecimal() const { return UNICHAR(data->decimal); }
  UNICHAR GetGroup() const { return UNICHAR(data->group); }
  UNICHAR GetList() const { return UNICHAR(data->list); }
  UNICHAR GetPercent() const { return UNICHAR(data->percent); }
  UNICHAR GetZero() const { return UNICHAR(data->zero); }
  UNICHAR GetPlus() const { return UNICHAR(data->plus); }
  UNICHAR GetMinus() const { return UNICHAR(data->minus); }
  UNICHAR GetExponential() const { return UNICHAR(data->exponential); }

  uint16 GetLanguageId() const { return data->language_id; }
  uint16 GetCountryId() const { return data->country_id; }

  UString GetBcp47Name(UNICHAR separator = '-') const;

  FUN_ALWAYS_INLINE UString GetLanguageCode() const { return LocaleImpl::LanguageToCode(Locale::Language(data->language_id)); }
  FUN_ALWAYS_INLINE UString GetScriptCode() const { return LocaleImpl::ScriptToCode(Locale::Script(data->script_id)); }
  FUN_ALWAYS_INLINE UString GetCountryCode() const { return LocaleImpl::CountryToCode(Locale::Country(data->country_id)); }

  static UString LanguageToCode(Locale::Language language);
  static UString ScriptToCode(Locale::Script script);
  static UString CountryToCode(Locale::Country country);
  static Locale::Language CodeToLanguage(const UString& code);
  static Locale::Script CodeToScript(const UString& code);
  static Locale::Country CodeToCountry(const UString& code);

  static void GetLangAndCountry(const UString& name,
                                Locale::Language& lang,
                                Locale::Script& script,
                                Locale::Country& country);

  Locale::MeasurementSystem GetMeasurementSystem() const;

  static void UpdateSystemImpl();

  UString DateTimeToString( const UString& format,
                            const DateTime* date_time,
                            const Date* date_part,
                            const Time* time_part,
                            const Locale* q) const;

  const LocaleData* data;
  //AtomicCounter32 ref;
  RefCounter ref;
  Locale::NumberOptions number_options;

  //딱히 필요 없는데 AtomicCounter32가 초기화가 안되는 이슈로 일단은 이렇게 해둠.
  //차후에 제거하고 원활하게 돌아가도록 하자.
  //LocaleImpl() {}
  //LocaleImpl(const LocaleData* data, AtomicCounter32 ref, Locale::NumberOptions number_options)
  //  : data(data)
  //  , ref(ref)
  //  , number_options(number_options)
  //{
  //}

  friend class Locale;
};


//
// inlines
//

FUN_ALWAYS_INLINE UNICHAR LocaleData::DigitToCLocale(UNICHAR ch) const
{
  if (ch >= zero && ch < (zero + 10)) {
    return '0' + ch - zero;
  }

  if (ch >= '0' && ch <= '9') {
    return ch;
  }

  if (ch == plus || ch == '+') {
    return '+';
  }

  if (ch == minus || ch == '-' || ch == UNICHAR(0x2212)) {
    return '-';
  }

  if (ch == decimal) {
    return '.';
  }

  if (ch == group) {
    return ',';
  }

  if (ch == exponential || ch == CharTraitsU::ToUpper(exponential)) {
    return 'e';
  }

  // in several languages group() is the char 0xA0, which looks like a space.
  // People use a regular space instead of it and complain it doesn't work.
  if (group == 0xA0 && ch == ' ') {
    return ',';
  }

  return 0;
}

UString ReadEscapedFormatString(const UString& format, int32* idx);

bool SplitLocaleName( const UString& name,
                      UString& language,
                      UString& script,
                      UString& country);

int32 RepeatCount(const UString& str);

} // namespace fun
