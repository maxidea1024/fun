//TODO 초기화 전에 호출되는 형태로 구현되어 있는데 이는 순서의 문제가 발생할 수 있음?

//FAKE 어떻게 문제를 풀어야하나???
#define WINVER  0x0601

#include "fun/base/locale/locale.h"
#include "fun/base/ftl/shared_ptr.h"
#include "fun/base/container/array.h"
#include "fun/base/variant.h"
#include "fun/base/date_time.h"

#if FUN_PLATFORM_WINDOWS_FAMILY

#include "fun/base/locale/locale_private.h"

namespace fun {

#undef GetCurrencyFormat
#undef GetDateFormat
#undef GetTimeFormat
#undef GetLocaleInfo

static UString GetWinLocaleName(LCID id = LOCALE_USER_DEFAULT);
static UString WinIso639LangName(LCID id = LOCALE_USER_DEFAULT);
static UString WinIso3116CountryName(LCID id = LOCALE_USER_DEFAULT);

class SystemLocaleImpl {
 public:
  SystemLocaleImpl();

  UNICHAR GetZeroDigit();
  UNICHAR GetDecimalPoint();
  UNICHAR GetGroupSeparator();
  UNICHAR GetNegativeSign();
  UNICHAR GetPositiveSign();

  Variant GetDateFormat(Locale::FormatType);
  Variant GetTimeFormat(Locale::FormatType);
  Variant GetDateTimeFormat(Locale::FormatType);
  Variant GetDayName(DayOfWeekType, Locale::FormatType);
  Variant GetMonthName(int32, Locale::FormatType);
  Variant ToString(const Date&, Locale::FormatType);
  Variant ToString(const Time&, Locale::FormatType);
  Variant ToString(const DateTime&, Locale::FormatType);
  Variant GetMeasurementSystem();
  Variant GetAMText();
  Variant GetPMText();
  Variant GetFirstDayOfWeek();

  Variant GetCurrencySymbol(Locale::CurrencySymbolFormat);
#if !FUN_NO_SYSTEM_LOCALE
  Variant ToCurrency(const SystemLocale::CurrencyToStringArgument&);
#endif

  Variant GetUiLanguages();
  Variant GetEnglishLanguageName();
  Variant GetNativeLanguageName();
  Variant GetNativeCountryName();

  void Update();

 private:
  enum SubstitutionType {
    SUnknown,
    SContext,
    SAlways,
    SNever
  };

  // cached values:
  LCID locale_id_;
  SubstitutionType substitution_type_;
  UNICHAR zero_;

  int32 GetLocaleInfo(LCTYPE type, LPTSTR data, int32 size);
  UString GetLocaleInfo(LCTYPE type, int32 max_len = 0);
  int32 GetLocaleInfo_INT(LCTYPE type, int32 max_len = 0);
  UNICHAR GetLocaleInfo_CHAR(LCTYPE type);

  int32 GetCurrencyFormat(DWORD flags, LPCTSTR value, const CURRENCYFMTW* format, LPTSTR data, int32 size);
  int32 GetDateFormat(DWORD flags, const SYSTEMTIME* date, LPCTSTR format, LPTSTR data, int32 size);
  int32 GetTimeFormat(DWORD flags, const SYSTEMTIME* date, LPCTSTR format, LPTSTR data, int32 size);

  SubstitutionType GetSubstitution();
  UString& SubstituteDigits(UString& string);

  static UString WinToFunFormat(const UString& sys_fmt);
};

//TODO 멀티스레드 환경에서는 문제가 발생할 수 있음.  시작 시점에서 명시적으로 초기화한 후 진입하면 문제는 없을터...
static SharedPtr<SystemLocaleImpl> g_system_locale_private_instance;
static SystemLocaleImpl* GetSystemLocaleImpl() {
  if (!g_system_locale_private_instance.IsValid()) {
    g_system_locale_private_instance = MakeShareable(new SystemLocaleImpl);
  }

  return g_system_locale_private_instance.Get();
}

SystemLocaleImpl::SystemLocaleImpl()
  : substitution_type_(SUnknown) {
  locale_id_ = GetUserDefaultLCID();
}

FUN_ALWAYS_INLINE int32 SystemLocaleImpl::GetCurrencyFormat(DWORD flags, LPCTSTR value, const CURRENCYFMTW* format, LPTSTR data, int32 size) {
  return GetCurrencyFormatW(locale_id_, flags, value, format, data, size);
}

FUN_ALWAYS_INLINE int32 SystemLocaleImpl::GetDateFormat(DWORD flags, const SYSTEMTIME* date, LPCTSTR format, LPTSTR data, int32 size) {
  return GetDateFormatW(locale_id_, flags, date, format, data, size);
}

FUN_ALWAYS_INLINE int32 SystemLocaleImpl::GetTimeFormat(DWORD flags, const SYSTEMTIME* date, LPCTSTR format, LPTSTR data, int32 size) {
  return GetTimeFormatW(locale_id_, flags, date, format, data, size);
}

FUN_ALWAYS_INLINE int32 SystemLocaleImpl::GetLocaleInfo(LCTYPE type, LPTSTR data, int32 size) {
  return GetLocaleInfoW(locale_id_, type, data, size);
}

UString SystemLocaleImpl::GetLocaleInfo(LCTYPE type, int32 max_len) {
  Array<UNICHAR, InlineAllocator<128+1>> buf(128, NoInit);
  if (!GetLocaleInfo(type, buf.MutableData(), buf.Count())) {
    return UString();
  }

  if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
    const int32 num = GetLocaleInfo(type, 0, 0);
    if (num == 0) {
      return UString();
    }
    buf.Resize(num);
    if (!GetLocaleInfo(type, buf.MutableData(), buf.Count())) {
      return UString();
    }
  }

  return UString(buf.ConstData());
}

int32 SystemLocaleImpl::GetLocaleInfo_INT(LCTYPE type, int32 max_len) {
  const UString str = GetLocaleInfo(type, max_len);
  UNICHAR* end = nullptr;
  return CStringTraitsU::Strtoi(*str, &end, 10);
  //bool ok = NumberParser::TryParseInt(str, v);
  //return ok ? v : 0;
}

UNICHAR SystemLocaleImpl::GetLocaleInfo_CHAR(LCTYPE type) {
  const UString str = GetLocaleInfo(type);
  return str.IsEmpty() ? UNICHAR() : str[0];
}

SystemLocaleImpl::SubstitutionType SystemLocaleImpl::GetSubstitution() {
  if (substitution_type_ == SUnknown) {
    //#define LOCALE_IDIGITSUBSTITUTION 0x00001014 // 0 = context, 1 = none, 2 = national
    UNICHAR buf[8];
    if (!GetLocaleInfo(LOCALE_IDIGITSUBSTITUTION, buf, 8)) {
      substitution_type_ = SystemLocaleImpl::SNever;
      return substitution_type_;
    }

    if (buf[0] == '0') {
      substitution_type_ = SystemLocaleImpl::SContext;
    }
    else if (buf[0] == '1') {
      substitution_type_ = SystemLocaleImpl::SNever;
    }
    else if (buf[0] == '2') {
      substitution_type_ = SystemLocaleImpl::SAlways;
    }
    else {
      //#define LOCALE_SNATIVEDIGITS 0x00000013 // native digits for 0-9, eg "0123456789"
      UNICHAR digits[11];
      if (!GetLocaleInfo(LOCALE_SNATIVEDIGITS, digits, 11)) {
        substitution_type_ = SystemLocaleImpl::SNever;
        return substitution_type_;
      }

      const UNICHAR zero = digits[0];
      if (buf[0] == zero + 2) {
        substitution_type_ = SystemLocaleImpl::SAlways;
      }
      else {
        substitution_type_ = SystemLocaleImpl::SNever;
      }
    }
  }

  return substitution_type_;
}

UString& SystemLocaleImpl::SubstituteDigits(UString& str) {
  const UNICHAR zero = GetZeroDigit();
  for (int32 i = 0; i < str.Len(); ++i) {
    if (CharTraitsU::IsDigit(str[i])) {
      str[i] = zero + (str[i] - '0');
    }
  }

  return str;
}

UNICHAR SystemLocaleImpl::GetZeroDigit() {
  //#define LOCALE_SNATIVEDIGITS 0x00000013 // native digits for 0-9, eg "0123456789"
  if (zero_ == 0) {
    zero_ = GetLocaleInfo_CHAR(LOCALE_SNATIVEDIGITS);
  }

  return zero_;
}

UNICHAR SystemLocaleImpl::GetDecimalPoint() {
  //#define LOCALE_SDECIMAL 0x0000000E // decimal separator, eg "." for 1,234.00
  return GetLocaleInfo_CHAR(LOCALE_SDECIMAL);
}

UNICHAR SystemLocaleImpl::GetGroupSeparator() {
  //#define LOCALE_STHOUSAND 0x0000000F // thousand separator, eg "," for 1,234.00
  return GetLocaleInfo_CHAR(LOCALE_STHOUSAND);
}

UNICHAR SystemLocaleImpl::GetNegativeSign() {
  // #define LOCALE_SNEGATIVESIGN 0x00000051 // negative sign, eg "-"
  return GetLocaleInfo_CHAR(LOCALE_SNEGATIVESIGN);
}

UNICHAR SystemLocaleImpl::GetPositiveSign() {
  //#define LOCALE_SPOSITIVESIGN 0x00000050 // positive sign, eg ""
  return GetLocaleInfo_CHAR(LOCALE_SPOSITIVESIGN);
}

Variant SystemLocaleImpl::GetDateFormat(Locale::FormatType type) {
  switch (type) {
  //#define LOCALE_SSHORTDATE 0x0000001F // short date format UString, eg "MM/dd/yyyy"
  case Locale::ShortFormat:
    return WinToFunFormat(GetLocaleInfo(LOCALE_SSHORTDATE));

  //#define LOCALE_SLONGDATE 0x00000020 // long date format UString, eg "dddd, MMMM dd, yyyy"
  case Locale::LongFormat:
    return WinToFunFormat(GetLocaleInfo(LOCALE_SLONGDATE));

  case Locale::NarrowFormat:
    break;
  }

  return Variant();
}

Variant SystemLocaleImpl::GetTimeFormat(Locale::FormatType type) {
  switch (type) {
  //#define LOCALE_SSHORTTIME 0x00000079 // Returns the preferred short time format (ie: no seconds, just h:mm)
  case Locale::ShortFormat:
    //if (SysInfo::WindowsVersion() >= SysInfo::WV_WINDOWS7) {
      return WinToFunFormat(GetLocaleInfo(LOCALE_SSHORTTIME));
    //}
    // fall through
  //#define LOCALE_STIMEFORMAT 0x00001003 // time format UString, eg "HH:mm:ss"
  case Locale::LongFormat:
    return WinToFunFormat(GetLocaleInfo(LOCALE_STIMEFORMAT));

  case Locale::NarrowFormat:
    break;
  }

  return Variant();
}

Variant SystemLocaleImpl::GetDateTimeFormat(Locale::FormatType type) {
  return UString(GetDateFormat(type).ToUString() + UTEXT(' ') + GetTimeFormat(type).ToUString());
}

Variant SystemLocaleImpl::GetDayName(DayOfWeekType day, Locale::FormatType type) {
  if (day < DayOfWeekType::Sunday || day > DayOfWeekType::Saturday) {
    return UString();
  }

//#define LOCALE_SABBREVDAYNAME1        0x00000031   // abbreviated name for Monday
//#define LOCALE_SABBREVDAYNAME2        0x00000032   // abbreviated name for Tuesday
//#define LOCALE_SABBREVDAYNAME3        0x00000033   // abbreviated name for Wednesday
//#define LOCALE_SABBREVDAYNAME4        0x00000034   // abbreviated name for Thursday
//#define LOCALE_SABBREVDAYNAME5        0x00000035   // abbreviated name for Friday
//#define LOCALE_SABBREVDAYNAME6        0x00000036   // abbreviated name for Saturday
//#define LOCALE_SABBREVDAYNAME7        0x00000037   // abbreviated name for Sunday
//
//#define LOCALE_SDAYNAME1              0x0000002A   // long name for Monday
//#define LOCALE_SDAYNAME2              0x0000002B   // long name for Tuesday
//#define LOCALE_SDAYNAME3              0x0000002C   // long name for Wednesday
//#define LOCALE_SDAYNAME4              0x0000002D   // long name for Thursday
//#define LOCALE_SDAYNAME5              0x0000002E   // long name for Friday
//#define LOCALE_SDAYNAME6              0x0000002F   // long name for Saturday
//#define LOCALE_SDAYNAME7              0x00000030   // long name for Sunday
//
//#define LOCALE_SABBREVDAYNAME1        0x00000031   // abbreviated name for Monday
//#define LOCALE_SABBREVDAYNAME2        0x00000032   // abbreviated name for Tuesday
//#define LOCALE_SABBREVDAYNAME3        0x00000033   // abbreviated name for Wednesday
//#define LOCALE_SABBREVDAYNAME4        0x00000034   // abbreviated name for Thursday
//#define LOCALE_SABBREVDAYNAME5        0x00000035   // abbreviated name for Friday
//#define LOCALE_SABBREVDAYNAME6        0x00000036   // abbreviated name for Saturday
//#define LOCALE_SABBREVDAYNAME7        0x00000037   // abbreviated name for Sunday

  static const LCTYPE short_day_map[] = {
    LOCALE_SABBREVDAYNAME1, LOCALE_SABBREVDAYNAME2,
    LOCALE_SABBREVDAYNAME3, LOCALE_SABBREVDAYNAME4, LOCALE_SABBREVDAYNAME5,
    LOCALE_SABBREVDAYNAME6, LOCALE_SABBREVDAYNAME7 };

  static const LCTYPE long_day_map[] = {
    LOCALE_SDAYNAME1, LOCALE_SDAYNAME2,
    LOCALE_SDAYNAME3, LOCALE_SDAYNAME4, LOCALE_SDAYNAME5,
    LOCALE_SDAYNAME6, LOCALE_SDAYNAME7 };

  static const LCTYPE narrow_day_map[] = {
    LOCALE_SSHORTESTDAYNAME1, LOCALE_SSHORTESTDAYNAME2,
    LOCALE_SSHORTESTDAYNAME3, LOCALE_SSHORTESTDAYNAME4,
    LOCALE_SSHORTESTDAYNAME5, LOCALE_SSHORTESTDAYNAME6,
    LOCALE_SSHORTESTDAYNAME7 };

  const int32 idx = (int32)day == 0 ? 6 : ((int32)day - 1);

  if (type == Locale::LongFormat) {
    return GetLocaleInfo(long_day_map[idx]);
  }
  else if (type == Locale::NarrowFormat) { // && SysInfo::WindowsVersion() >= SysInfo::WV_VISTA)
    return GetLocaleInfo(narrow_day_map[idx]);
  }
  else {
    return GetLocaleInfo(short_day_map[idx]);
  }
}

Variant SystemLocaleImpl::GetMonthName(int32 month, Locale::FormatType type) {
//#define LOCALE_SABBREVMONTHNAME1      0x00000044   // abbreviated name for January
//#define LOCALE_SABBREVMONTHNAME2      0x00000045   // abbreviated name for February
//#define LOCALE_SABBREVMONTHNAME3      0x00000046   // abbreviated name for March
//#define LOCALE_SABBREVMONTHNAME4      0x00000047   // abbreviated name for April
//#define LOCALE_SABBREVMONTHNAME5      0x00000048   // abbreviated name for May
//#define LOCALE_SABBREVMONTHNAME6      0x00000049   // abbreviated name for June
//#define LOCALE_SABBREVMONTHNAME7      0x0000004A   // abbreviated name for July
//#define LOCALE_SABBREVMONTHNAME8      0x0000004B   // abbreviated name for August
//#define LOCALE_SABBREVMONTHNAME9      0x0000004C   // abbreviated name for September
//#define LOCALE_SABBREVMONTHNAME10     0x0000004D   // abbreviated name for October
//#define LOCALE_SABBREVMONTHNAME11     0x0000004E   // abbreviated name for November
//#define LOCALE_SABBREVMONTHNAME12     0x0000004F   // abbreviated name for December
//
//#define LOCALE_SMONTHNAME1            0x00000038   // long name for January
//#define LOCALE_SMONTHNAME2            0x00000039   // long name for February
//#define LOCALE_SMONTHNAME3            0x0000003A   // long name for March
//#define LOCALE_SMONTHNAME4            0x0000003B   // long name for April
//#define LOCALE_SMONTHNAME5            0x0000003C   // long name for May
//#define LOCALE_SMONTHNAME6            0x0000003D   // long name for June
//#define LOCALE_SMONTHNAME7            0x0000003E   // long name for July
//#define LOCALE_SMONTHNAME8            0x0000003F   // long name for August
//#define LOCALE_SMONTHNAME9            0x00000040   // long name for September
//#define LOCALE_SMONTHNAME10           0x00000041   // long name for October
//#define LOCALE_SMONTHNAME11           0x00000042   // long name for November
//#define LOCALE_SMONTHNAME12           0x00000043   // long name for December

  static const LCTYPE short_month_map[] = {
    LOCALE_SABBREVMONTHNAME1, LOCALE_SABBREVMONTHNAME2, LOCALE_SABBREVMONTHNAME3,
    LOCALE_SABBREVMONTHNAME4, LOCALE_SABBREVMONTHNAME5, LOCALE_SABBREVMONTHNAME6,
    LOCALE_SABBREVMONTHNAME7, LOCALE_SABBREVMONTHNAME8, LOCALE_SABBREVMONTHNAME9,
    LOCALE_SABBREVMONTHNAME10, LOCALE_SABBREVMONTHNAME11, LOCALE_SABBREVMONTHNAME12 };

  static const LCTYPE long_month_map[] = {
    LOCALE_SMONTHNAME1, LOCALE_SMONTHNAME2, LOCALE_SMONTHNAME3,
    LOCALE_SMONTHNAME4, LOCALE_SMONTHNAME5, LOCALE_SMONTHNAME6,
    LOCALE_SMONTHNAME7, LOCALE_SMONTHNAME8, LOCALE_SMONTHNAME9,
    LOCALE_SMONTHNAME10, LOCALE_SMONTHNAME11, LOCALE_SMONTHNAME12 };

  month -= 1;
  if (month < 0 || month > 11) {
    return UString();
  }

  const LCTYPE lctype = (type == Locale::ShortFormat || type == Locale::NarrowFormat) ? short_month_map[month] : long_month_map[month];
  return GetLocaleInfo(lctype);
}

Variant SystemLocaleImpl::ToString(const Date& date, Locale::FormatType type) {
  SYSTEMTIME st;
  UnsafeMemory::Memzero(&st, sizeof(SYSTEMTIME));
  st.wYear = date.Year();
  st.wMonth = date.Month();
  st.wDay = date.Day();

  //#define DATE_SHORTDATE 0x00000001 // use short date picture
  //#define DATE_LONGDATE 0x00000002 // use long date picture

  DWORD flags = (type == Locale::LongFormat ? DATE_LONGDATE : DATE_SHORTDATE);
  UNICHAR buf[255];
  if (GetDateFormat(flags, &st, nullptr, buf, 255)) {
    UString fmt = buf;
    if (GetSubstitution() == SAlways) {
      SubstituteDigits(fmt);
    }

    return fmt;
  }

  return UString();
}

Variant SystemLocaleImpl::ToString(const Time& time, Locale::FormatType type) {
  SYSTEMTIME st;
  UnsafeMemory::Memzero(&st, sizeof(SYSTEMTIME));
  st.wHour = time.Hour();
  st.wMinute = time.Minute();
  st.wSecond = time.Second();
  st.wMilliseconds = 0;

  DWORD flags = 0;
  // keep the same conditional as TimeFormat() above
  if (type == Locale::ShortFormat) { // && SysInfo::WindowsVersion() >= SysInfo::WV_WINDOWS7)
    flags = TIME_NOSECONDS;
  }

  UNICHAR buf[255];
  if (GetTimeFormat(flags, &st, nullptr, buf, 255)) {
    UString fmt = buf;
    if (GetSubstitution() == SAlways) {
      SubstituteDigits(fmt);
    }

    return fmt;
  }

  return UString();
}

Variant SystemLocaleImpl::ToString(const DateTime& date_time, Locale::FormatType type) {
  return ToString(date_time.GetDate(), type).ToUString() + UString(UTEXT(" ")) + ToString(date_time.GetTime(), type).ToUString();
}

Variant SystemLocaleImpl::GetMeasurementSystem() {
  //#define LOCALE_IMEASURE 0x0000000D // 0 = metric, 1 = US measurement system
  UNICHAR buf[2];
  if (GetLocaleInfo(LOCALE_IMEASURE, buf, 2)) {
    UString measure = buf;
    if (measure == UTEXT("1")) {
      //FIXME Variant type issue
      return (int32)Locale::ImperialSystem;
    }
  }

  //FIXME Variant type issue
  return (int32)Locale::MetricSystem;
}

Variant SystemLocaleImpl::GetAMText() {
  //#define LOCALE_S1159 0x00000028 // AM designator, eg "AM"
  UNICHAR buf[15]; // maximum length including  terminating zero character for Win2003+
  return GetLocaleInfo(LOCALE_S1159, buf, 15) ? Variant(UString(buf)) : Variant();
}

Variant SystemLocaleImpl::GetPMText() {
  //#define LOCALE_S2359 0x00000029 // pm designator, eg "pm"
  UNICHAR buf[15]; // maximum length including  terminating zero character for Win2003+
  return GetLocaleInfo(LOCALE_S2359, buf, 15) ? Variant(UString(buf)) : Variant();
}

//https://msdn.microsoft.com/en-us/library/windows/desktop/dd373771(v=vs.85).aspx
//0 LOCALE_SDAYNAME1(Monday)
//1 LOCALE_SDAYNAME2(Tuesday)
//2 LOCALE_SDAYNAME3(Wednesday)
//3 LOCALE_SDAYNAME4(Thursday)
//4 LOCALE_SDAYNAME5(Friday)
//5 LOCALE_SDAYNAME6(Saturday)
//6 LOCALE_SDAYNAME7(Sunday)
Variant SystemLocaleImpl::GetFirstDayOfWeek() {
  UNICHAR buf[4]; // maximum length including  terminating zero character for Win2003+

  if (GetLocaleInfo(LOCALE_IFIRSTDAYOFWEEK, buf, 4)) {
    //TODO 0=sunday인지??
    UNICHAR* end = nullptr;
    const uint32 n = CStringTraitsU::Strtoi(buf, &end, 10);
    return (DayOfWeekType)((n + 1) % 7);
  }

  return (DayOfWeekType)0;
}

Variant SystemLocaleImpl::GetCurrencySymbol(Locale::CurrencySymbolFormat format) {
  UNICHAR buf[13];

  switch (format) {
    //#define LOCALE_SCURRENCY 0x00000014 // local monetary symbol, eg "$"
    case Locale::CSFCurrencySymbol:
      if (GetLocaleInfo(LOCALE_SCURRENCY, buf, 13)) {
        return UString(buf);
      }
      break;

    //#define LOCALE_SINTLSYMBOL 0x00000015 // intl monetary symbol, eg "USD"
    case Locale::CSFCurrencyIsoCode:
      if (GetLocaleInfo(LOCALE_SINTLSYMBOL, buf, 9)) {
        return UString(buf);
      }
      break;

    //#define LOCALE_SENGCURRNAME 0x00001007 // english name of currency, eg "Euro"
    case Locale::CSFCurrencyEnglishName: {
      Array<UNICHAR> buf;
      buf.AddUninitialized(64);
      if (!GetLocaleInfo(LOCALE_SENGCURRNAME, buf.MutableData(), buf.Count())) {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
          break;
        }
        buf.Resize(255); // should be large enough, right?
        if (!GetLocaleInfo(LOCALE_SENGCURRNAME, buf.MutableData(), buf.Count())) {
          break;
        }
      }
      return UString(buf.ConstData());
    }

    //#define LOCALE_SNATIVECURRNAME 0x00001008 // native name of currency, eg "euro"
    case Locale::CSFCurrencyNativeName: {
      Array<UNICHAR,InlineAllocator<64+1>> buf(64, NoInit);
      if (!GetLocaleInfo(LOCALE_SNATIVECURRNAME, buf.MutableData(), buf.Count())) {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
          break;
        }
        buf.Resize(255); // should be large enough, right?
        if (!GetLocaleInfo(LOCALE_SNATIVECURRNAME, buf.MutableData(), buf.Count())) {
          break;
        }
      }
      return UString(buf.ConstData());
    }

    default:
      break;
  }

  return Variant();
}

#if !FUN_NO_SYSTEM_LOCALE
Variant SystemLocaleImpl::ToCurrency(const SystemLocale::CurrencyToStringArgument& arg) {
  UString value;
  switch (arg.value.Type()) {
    case VariantTypes::Int32:
      value = LocaleData::Int64ToString(UTEXT('0'), UTEXT(','), UTEXT('+'), UTEXT('-'), arg.value.ToInt32(), -1, 10, -1, Locale::OmitGroupSeparator);
      break;
    case VariantTypes::UInt32:
      value = LocaleData::UInt64ToString(UTEXT('0'), UTEXT(','), UTEXT('+'), arg.value.ToUInt32(), -1, 10, -1, Locale::OmitGroupSeparator);
      break;
    case VariantTypes::Double:
      value = LocaleData::DoubleToString(UTEXT('0'), UTEXT('+'), UTEXT('-'), UTEXT(' '), UTEXT(','), UTEXT('.'), arg.value.ToDouble(), -1, LocaleData::DFDecimal, -1, Locale::OmitGroupSeparator);
      break;
    case VariantTypes::Int64:
      value = LocaleData::Int64ToString(UTEXT('0'), UTEXT(','), UTEXT('+'), UTEXT('-'), arg.value.ToInt64(), -1, 10, -1, Locale::OmitGroupSeparator);
      break;
    case VariantTypes::UInt64:
      value = LocaleData::UInt64ToString(UTEXT('0'), UTEXT(','), UTEXT('+'), arg.value.ToUInt64(), -1, 10, -1, Locale::OmitGroupSeparator);
      break;
    default:
      fun_check(0);
      return Variant();
  }

  Array<UNICHAR, InlineAllocator<128+1>> buf(128, NoInit);

  UString decimal_sep;
  UString thousand_sep;
  CURRENCYFMT format;
  CURRENCYFMT* currency_fmt = nullptr;
  if (!arg.symbol.IsEmpty()) {
    //#define LOCALE_ICURRDIGITS            0x00000019   // # local monetary digits, eg 2 for $1.00
    format.NumDigits = GetLocaleInfo_INT(LOCALE_ICURRDIGITS);
    //#define LOCALE_ILZERO                 0x00000012   // leading zeros for decimal, 0 for .97, 1 for 0.97
    format.LeadingZero = GetLocaleInfo_INT(LOCALE_ILZERO);
    //#define LOCALE_SMONDECIMALSEP         0x00000016   // monetary decimal separator, eg "." for $1,234.00
    decimal_sep = GetLocaleInfo(LOCALE_SMONDECIMALSEP);
    format.lpDecimalSep = (UNICHAR*)*decimal_sep;
    //#define LOCALE_SMONTHOUSANDSEP        0x00000017   // monetary thousand separator, eg "," for $1,234.00
    thousand_sep = GetLocaleInfo(LOCALE_SMONTHOUSANDSEP);
    format.lpThousandSep = (UNICHAR*)*thousand_sep;
    //#define LOCALE_INEGCURR               0x0000001C   // negative currency mode, 0-15, see documentation
    format.NegativeOrder = GetLocaleInfo_INT(LOCALE_INEGCURR);
    //#define LOCALE_ICURRENCY              0x0000001B   // positive currency mode, 0-3, see documenation
    format.PositiveOrder = GetLocaleInfo_INT(LOCALE_ICURRENCY);
    format.lpCurrencySymbol = (UNICHAR*)*arg.symbol;

    // grouping is complicated and ugly:
    // int32(0)  == "123456789.00"    == UString("0")
    // int32(3)  == "123,456,789.00"  == UString("3;0")
    // int32(30) == "123456,789.00"   == UString("3;0;0")
    // int32(32) == "12,34,56,789.00" == UString("3;2;0")
    // int32(320)== "1234,56,789.00"  == UString("3;2")
    //#define LOCALE_SMONGROUPING           0x00000018   // monetary grouping, eg "3;0" for $1,000,000.00
    UString grouping_str = GetLocaleInfo(LOCALE_SMONGROUPING);
    grouping_str.FindAndRemove(UTEXT(';'));
    format.Grouping = CStringTraitsU::Atoi(*grouping_str);

    if (format.Grouping % 10 == 0) { // magic
      format.Grouping /= 10;
    }
    else {
      format.Grouping *= 10;
    }
    currency_fmt = &format;
  }

  int32 ret = GetCurrencyFormat(0, *value, currency_fmt, buf.MutableData(), buf.Count());
  if (ret == 0 && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
    ret = GetCurrencyFormat(0, *value, currency_fmt, buf.MutableData(), 0);
    buf.Resize(ret);
    GetCurrencyFormat(0, *value, currency_fmt, buf.MutableData(), buf.Count());
  }

  value = UString(buf.ConstData());
  if (GetSubstitution() == SAlways) {
    SubstituteDigits(value);
  }
  return value;
}
#endif //!FUN_NO_SYSTEM_LOCALE

Variant SystemLocaleImpl::GetUiLanguages() {
  //TODO
  //return Array<UString>();
  return Variant();

//#ifndef Q_OS_WINRT
//    unsigned long cnt = 0;
//    QVarLengthArray<UNICHAR, 64> buf(64);
//#  if !defined(QT_BOOTSTRAPPED) && !defined(QT_BUILD_QMAKE) // Not present in MinGW 4.9/bootstrap builds.
//    unsigned long size = buf.size();
//    if (!GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &cnt, buf.data(), &size)) {
//        size = 0;
//        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
//                GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &cnt, NULL, &size)) {
//            buf.resize(size);
//            if (!GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &cnt, buf.data(), &size))
//                return QStringList();
//        }
//    }
//#  endif // !QT_BOOTSTRAPPED && !QT_BUILD_QMAKE
//    QStringList result;
//    result.reserve(cnt);
//    const UNICHAR *str = buf.ConstData();
//    for (; cnt > 0; --cnt) {
//        UString s = UString::fromWCharArray(str);
//        if (s.IsEmpty())
//            break; // something is wrong
//        result.append(s);
//        str += s.size() + 1;
//    }
//    return result;
//#else // !Q_OS_WINRT
//    QStringList result;
//
//    ComPtr<IGlobalizationPreferencesStatics> preferences;
//    HRESULT hr = GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_System_UserProfile_GlobalizationPreferences).Get(), &preferences);
//    if (FAILED(hr)) {
//        fun_log(Warning, "Could not obtain ApplicationLanguagesStatic");
//        return QStringList();
//    }
//
//    ComPtr<ABI::Windows::Foundation::Collections::IVectorView<HSTRING> > languageList;
//    // Languages is a ranked list of "long names" (e.g. en-US) of preferred languages
//    hr = preferences->get_Languages(&languageList);
//    Q_ASSERT_SUCCEEDED(hr);
//    unsigned int32 size;
//    hr = languageList->get_Size(&size);
//    Q_ASSERT_SUCCEEDED(hr);
//    result.reserve(size);
//    for (unsigned int32 i = 0; i < size; ++i) {
//        HString language;
//        hr = languageList->GetAt(i, language.GetAddressOf());
//        Q_ASSERT_SUCCEEDED(hr);
//        UINT32 length;
//        PCWSTR rawString = language.GetRawBuffer(&length);
//        result << UString::fromWCharArray(rawString, length);
//    }
//
//    return result;
//#endif // Q_OS_WINRT
}

Variant SystemLocaleImpl::GetEnglishLanguageName() {
  #define LOCALE_SENGLISHLANGUAGENAME   0x00001001   // English name of language, eg "German"
  //if (SysInfo::WindowsVersion() < SysInfo::WV_WINDOWS7) {
  //    return GetLocaleInfo(LOCALE_SNATIVELANGNAME);
  //}

  return GetLocaleInfo(LOCALE_SENGLISHLANGUAGENAME);
}

Variant SystemLocaleImpl::GetNativeLanguageName() {
  //#define LOCALE_SNATIVELANGUAGENAME    0x00000004   // native name of language, eg "Deutsch"
  //if (SysInfo::WindowsVersion() < SysInfo::WV_WINDOWS7) {
  //  return GetLocaleInfo(LOCALE_SNATIVELANGNAME);
  //}

  return GetLocaleInfo(LOCALE_SNATIVELANGUAGENAME);
}

Variant SystemLocaleImpl::GetNativeCountryName() {
  //#define LOCALE_SNATIVECOUNTRYNAME     0x00000008   // native name of country/region, eg "Deutschland"
  //if (SysInfo::WindowsVersion() < SysInfo::WV_WINDOWS7) {
  //  return GetLocaleInfo(LOCALE_SNATIVECTRYNAME);
  //}

  return GetLocaleInfo(LOCALE_SNATIVECOUNTRYNAME);
}

void SystemLocaleImpl::Update() {
  locale_id_ = GetUserDefaultLCID();
  substitution_type_ = SUnknown;
  zero_ = UNICHAR(0);
}

//윈도우즈에서 사용하는 날짜/시간 포맷팅과 약간 다른 부분이 있다.
//통일 시켜주는쪽이 좋을듯 싶은데...
UString SystemLocaleImpl::WinToFunFormat(const UString& sys_fmt) {
  UString result;
  int32 i = 0;

  while (i < sys_fmt.Len()) {
    if (sys_fmt[i] == UTEXT('\'')) {
      const UString text = ReadEscapedFormatString(sys_fmt, &i);
      if (text == UTEXT("'")) {
        result += UTEXT("''");
      }
      else {
        result += UTEXT('\'') + text + UTEXT('\'');
      }
      continue;
    }

    const UNICHAR ch = sys_fmt[i];
    int32 repeat = RepeatCount(sys_fmt.Mid(i));

    switch (ch) {
      // date
      case 'y':
        if (repeat > 5) {
          repeat = 5;
        }
        else if (repeat == 3) {
          repeat = 2;
        }

        switch (repeat) {
        case 1:
          result += UTEXT("yy"); // "y" unsupported by FUN, use "yy"
          break;
        case 5:
          result += UTEXT("yyyy"); // "yyyyy" same as "yyyy" on Windows
          break;
        default:
          result += UString(repeat, UTEXT('y'));
          break;
        }
        break;

      case 'g':
        if (repeat > 2) {
          repeat = 2;
        }

        switch (repeat) {
        case 2:
          break; // no equivalent of "gg" in FUN
        default:
          result += UTEXT('g');
          break;
        }
        break;

      case 't':
        if (repeat > 2) {
          repeat = 2;
        }

        result += UTEXT("AP"); // "t" unsupported, use "AP"
        break;

      default:
        result += UString(repeat, ch);
        break;
    }

    i += repeat;
  }

  return MoveTemp(result);
}

#if !FUN_NO_SYSTEM_LOCALE
Locale SystemLocale::GetFallbackUiLocale() const {
  return Locale(GetWinLocaleName());
}

Variant SystemLocale::Query(QueryType type, Variant in) const {
  auto impl = GetSystemLocaleImpl();

  switch (type) {
  case DecimalPoint:
    return impl->GetDecimalPoint();

  case GroupSeparator:
    return impl->GetGroupSeparator();

  case NegativeSign:
    return impl->GetNegativeSign();

  case PositiveSign:
    return impl->GetPositiveSign();

  case DateFormatLong:
    return impl->GetDateFormat(Locale::LongFormat);

  case DateFormatShort:
    return impl->GetDateFormat(Locale::ShortFormat);

  case TimeFormatLong:
    return impl->GetTimeFormat(Locale::LongFormat);

  case TimeFormatShort:
    return impl->GetTimeFormat(Locale::ShortFormat);

  case DateTimeFormatLong:
    return impl->GetDateTimeFormat(Locale::LongFormat);

  case DateTimeFormatShort:
    return impl->GetDateTimeFormat(Locale::ShortFormat);

  case DayNameLong:
    return impl->GetDayName(in.Value<DayOfWeekType>(), Locale::LongFormat);

  case DayNameShort:
    return impl->GetDayName(in.Value<DayOfWeekType>(), Locale::ShortFormat);

  case MonthNameLong:
  case StandaloneMonthNameLong:
    return impl->GetMonthName(in.Value<int32>(), Locale::LongFormat);

  case MonthNameShort:
  case StandaloneMonthNameShort:
    return impl->GetMonthName(in.Value<int32>(), Locale::ShortFormat);

  case DateToStringShort:
    return impl->ToString(in.Value<Date>(), Locale::ShortFormat);

  case DateToStringLong:
    return impl->ToString(in.Value<Date>(), Locale::LongFormat);

  case TimeToStringShort:
    return impl->ToString(in.Value<Time>(), Locale::ShortFormat);

  case TimeToStringLong:
    return impl->ToString(in.Value<Time>(), Locale::LongFormat);

  case DateTimeToStringShort:
    return impl->ToString(in.Value<DateTime>(), Locale::ShortFormat);

  case DateTimeToStringLong:
    return impl->ToString(in.Value<DateTime>(), Locale::LongFormat);

  case ZeroDigit:
    return impl->GetZeroDigit();

  case LanguageId:
  case CountryId: {
    UString locale = GetWinLocaleName();
    Locale::Language language;
    Locale::Script script;
    Locale::Country country;
    LocaleImpl::GetLangAndCountry(locale, language, script, country);
    if (type == LanguageId) {
      return language;
    }
    if (country == Locale::AnyCountry) {
      return GetFallbackUiLocale().GetCountry();
    }
    return country;
  }

  case ScriptId:
    return Variant(Locale::AnyScript);

  case MeasurementSystem:
    //FIXME Variant type issue
    return impl->GetMeasurementSystem();

  case AMText:
    return impl->GetAMText();

  case PMText:
    return impl->GetPMText();

  case FirstDayOfWeek:
    return impl->GetFirstDayOfWeek();

  case CurrencySymbol:
    return impl->GetCurrencySymbol(Locale::CurrencySymbolFormat(in.Value<int32>()));

  case CurrencyToString:
    return impl->ToCurrency(in.Value<SystemLocale::CurrencyToStringArgument>());

  case UILanguages:
    return impl->GetUiLanguages();

  case LocaleChanged:
    impl->Update();
    break;

  case EnglishLanguageName:
    return impl->GetEnglishLanguageName();
  case NativeLanguageName:
    return impl->GetNativeLanguageName();

  case NativeCountryName:
    return impl->GetNativeCountryName();

  default:
    break;
  }
  return Variant();
}

#endif //!FUN_NO_SYSTEM_LOCALE


//TODO 이하 코드는 실제로 사용되는지 여부를 확인해 봐야함.

struct WindowsToIsoListElement {
  uint16 windows_code;
  char iso_name[6];
};

// NOTE: This array should be sorted by the first column!
static const WindowsToIsoListElement WINDOWS_TO_ISO_TABLE[] = {
  { 0x0401, "ar_SA" },
  { 0x0402, "bg\0  " },
  { 0x0403, "ca\0  " },
  { 0x0404, "zh_TW" },
  { 0x0405, "cs\0  " },
  { 0x0406, "da\0  " },
  { 0x0407, "de\0  " },
  { 0x0408, "el\0  " },
  { 0x0409, "en_US" },
  { 0x040a, "es\0  " },
  { 0x040b, "fi\0  " },
  { 0x040c, "fr\0  " },
  { 0x040d, "he\0  " }, { 0x040e, "hu\0  " }, { 0x040f, "is\0  " }, { 0x0410, "it\0  " }, { 0x0411, "ja\0  " }, { 0x0412, "ko\0  " }, { 0x0413, "nl\0  " }, { 0x0414, "no\0  " }, { 0x0415, "pl\0  " }, { 0x0416, "pt_BR" }, { 0x0418, "ro\0  " }, { 0x0419, "ru\0  " }, { 0x041a, "hr\0  " }, { 0x041c, "sq\0  " }, { 0x041d, "sv\0  " }, { 0x041e, "th\0  " }, { 0x041f, "tr\0  " }, { 0x0420, "ur\0  " }, { 0x0421, "in\0  " }, { 0x0422, "uk\0  " }, { 0x0423, "be\0  " }, { 0x0425, "et\0  " }, { 0x0426, "lv\0  " }, { 0x0427, "lt\0  " }, { 0x0429, "fa\0  " }, { 0x042a, "vi\0  " }, { 0x042d, "eu\0  " }, { 0x042f, "mk\0  " }, { 0x0436, "af\0  " }, { 0x0438, "fo\0  " }, { 0x0439, "hi\0  " }, { 0x043e, "ms\0  " }, { 0x0458, "mt\0  " }, { 0x0801, "ar_IQ" }, { 0x0804, "zh_CN" }, { 0x0807, "de_CH" }, { 0x0809, "en_GB" }, { 0x080a, "es_MX" }, { 0x080c, "fr_BE" }, { 0x0810, "it_CH" }, { 0x0812, "ko\0  " }, { 0x0813, "nl_BE" }, { 0x0814, "no\0  " }, { 0x0816, "pt\0  " }, { 0x081a, "sr\0  " }, { 0x081d, "sv_FI" }, { 0x0c01, "ar_EG" }, { 0x0c04, "zh_HK" }, { 0x0c07, "de_AT" }, { 0x0c09, "en_AU" }, { 0x0c0a, "es\0  " }, { 0x0c0c, "fr_CA" }, { 0x0c1a, "sr\0  " }, { 0x1001, "ar_LY" }, { 0x1004, "zh_SG" }, { 0x1007, "de_LU" }, { 0x1009, "en_CA" }, { 0x100a, "es_GT" }, { 0x100c, "fr_CH" }, { 0x1401, "ar_DZ" }, { 0x1407, "de_LI" }, { 0x1409, "en_NZ" }, { 0x140a, "es_CR" }, { 0x140c, "fr_LU" }, { 0x1801, "ar_MA" }, { 0x1809, "en_IE" }, { 0x180a, "es_PA" }, { 0x1c01, "ar_TN" }, { 0x1c09, "en_ZA" }, { 0x1c0a, "es_DO" }, { 0x2001, "ar_OM" }, { 0x2009, "en_JM" }, { 0x200a, "es_VE" }, { 0x2401, "ar_YE" }, { 0x2409, "en\0  " }, { 0x240a, "es_CO" }, { 0x2801, "ar_SY" }, { 0x2809, "en_BZ" }, { 0x280a, "es_PE" }, { 0x2c01, "ar_JO" }, { 0x2c09, "en_TT" }, { 0x2c0a, "es_AR" }, { 0x3001, "ar_LB" }, { 0x300a, "es_EC" }, { 0x3401, "ar_KW" }, { 0x340a, "es_CL" }, { 0x3801, "ar_AE" }, { 0x380a, "es_UY" }, { 0x3c01, "ar_BH" }, { 0x3c0a, "es_PY" }, { 0x4001, "ar_QA" }, { 0x400a, "es_BO" }, { 0x440a, "es_SV" }, { 0x480a, "es_HN" }, { 0x4c0a, "es_NI" }, { 0x500a, "es_PR" } };

static const char* WinLangCodeToIsoName(int32 code) {
  const int32 cmp = code - WINDOWS_TO_ISO_TABLE[0].windows_code;
  if (cmp < 0) {
    return nullptr;
  }

  if (cmp == 0) {
    return WINDOWS_TO_ISO_TABLE[0].iso_name;
  }

  int32 begin = 0;
  int32 end = countof(WINDOWS_TO_ISO_TABLE);

  while (end - begin > 1) {
    const uint32 mid = (begin + end) / 2;

    const WindowsToIsoListElement* element = WINDOWS_TO_ISO_TABLE + mid;
    const int32 cmp = code - element->windows_code;
    if (cmp < 0) {
      end = mid;
    }
    else if (cmp > 0) {
      begin = mid;
    }
    else {
      return element->iso_name;
    }
  }

  return nullptr;
}

//TODO

//LCID IsoNameToLCID(const char *name)
//{
//    // handle norwegian manually, the list above will fail
//    if (!strncmp(name, "nb", 2))
//        return 0x0414;
//    else if (!strncmp(name, "nn", 2))
//        return 0x0814;
//
//    char n[64];
//    strncpy(n, name, sizeof(n));
//    n[sizeof(n)-1] = 0;
//    char *c = n;
//    while (*c) {
//        if (*c == '-')
//            *c = '_';
//        ++c;
//    }
//
//    for (int32 i = 0; i < windows_to_iso_count; ++i) {
//        if (!strcmp(n, windows_to_iso_list[i].iso_name))
//            return windows_to_iso_list[i].windows_code;
//    }
//    return LOCALE_USER_DEFAULT;
//}

static UString WinIso639LangName(LCID id) {
  UString result;

  //#define LOCALE_ILANGUAGE 0x00000001 // language id, LOCALE_SNAME preferred
  UString lang_code;
  UNICHAR buf[256];
  if (GetLocaleInfoW(id, LOCALE_ILANGUAGE, buf, 255)) {
    lang_code = buf;
  }

  if (lang_code.Len() != 0) {
    UNICHAR* end = nullptr;
    uint32 code = CStringTraitsU::Strtoi(*lang_code, &end, 16);
    if (code != 0 || *end == '\0') {
      switch (code) {
      case 0x814:
        result = UTEXT("nn"); // Nynorsk
        break;
      default:
        break;
      }
    }
  }

  if (result.Len() != 0) {
    return result;
  }

  //#define LOCALE_SISO639LANGNAME 0x00000059 // ISO abbreviated language name, eg "en"
  if (GetLocaleInfoW(id, LOCALE_SISO639LANGNAME, buf, 255)) {
    result = buf;
  }

  return result;
}

static UString WinIso3116CountryName(LCID id) {
  //#define LOCALE_SISO3166CTRYNAME 0x0000005A // ISO abbreviated country/region name, eg "US"
  UString result;
  UNICHAR buf[256];
  if (GetLocaleInfoW(id, LOCALE_SISO3166CTRYNAME, buf, 255)) {
    result = buf;
  }
  return result;
}

static UString GetWinLocaleName(LCID id) {
  //TODO
  //우선 환경 변수에 설정되어 있는 경우에는 그것을 먼저 사용함.

  if (id == LOCALE_USER_DEFAULT) {
    id = GetUserDefaultLCID();
  }

  UString result_usage = WinIso639LangName(id);
  UString country = WinIso3116CountryName(id);
  if (!country.IsEmpty()) {
    result_usage += UTEXT("_") + country;
  }
  return result_usage;
}

//FUN_BASE_API Locale LocaleFromLCID(LCID id)
//{
//  return Locale(GetWinLocaleName(id));
//}

} // namespace fun

#endif // FUN_PLATFORM_WINDOWS_FAMILY
