//TODO 텍스트 인코딩 체크해야함.

#include "fun/base/locale/locale_private.h"
#include "fun/base/locale/locale_data.h"
#include "fun/base/date_time_parser.h"
//TODO private에 위치시키는게 좋을지...
#include "fun/base/locale/locale_tool.h"

#include "fun/base/bundle/v8/double-conversion.h"
#include "fun/base/bundle/v8/strtod.h"

//FIXME
//기본 시스템 로케일에서 조회한 시간/날짜 형식과, "ko"에서 조회된 형식이 다르다.
//형식 테이블을 변경해주어야 할런지?

namespace fun {

#if !FUN_NO_SYSTEM_LOCALE
static SystemLocale* g_system_locale = nullptr;

//TODO 스레드 이슈는 없는지 확인해야함!
class SystemLocaleSingleton : public SystemLocale
{
 public:
  SystemLocaleSingleton()
    : SystemLocale(true) {
  }

  static SystemLocale& Get() {
    return instance_;
  }

 private:
  static SystemLocale instance_;
};

SystemLocale SystemLocaleSingleton::instance_(true);
#endif //!FUN_NO_SYSTEM_LOCALE

static LocaleData* g_system_data = nullptr;
static LocaleData g_locale_data;

Locale::Language LocaleImpl::CodeToLanguage(const UString& code)
{
  const int32 len = code.Len();

  if (!(len == 2 || len == 3)) {
    return Locale::C;
  }

  const UNICHAR c0 = CharTraitsU::ToLower(code[0]);
  const UNICHAR c1 = CharTraitsU::ToLower(code[1]);
  const UNICHAR c2 = (len > 2 ? CharTraitsU::ToLower(code[2]) : 0);

  //TODO optimize...
  const char* code_ptr = _LANGUAGE_CODE_LIST;
  for (; *code_ptr; code_ptr += 3) {
    if (c0 == code_ptr[0] && c1 == code_ptr[1] && c2 == code_ptr[2]) {
      return (Locale::Language)((code_ptr - _LANGUAGE_CODE_LIST) / 3);
    }
  }

  // legacy codes
  if (c0 == 'n' && c1 == 'o' && c2 == 0) { // no -> nb
    static_assert(Locale::Norwegian == Locale::NorwegianBokmal, "Locale::Norwegian == Locale::NorwegianBokmal");
    return Locale::Norwegian;
  }
  if (c0 == 't' && c1 == 'l' && c2 == 0) { // tl -> fil
    static_assert(Locale::Tagalog == Locale::Filipino, "Locale::Tagalog == Locale::Filipino");
    return Locale::Tagalog;
  }
  if (c0 == 's' && c1 == 'h' && c2 == 0) { // sh -> sr[_Latn]
    static_assert(Locale::SerboCroatian == Locale::Serbian, "Locale::SerboCroatian == Locale::Serbian");
    return Locale::SerboCroatian;
  }
  if (c0 == 'm' && c1 == 'o' && c2 == 0) { // mo -> ro
    static_assert(Locale::Moldavian == Locale::Romanian, "Locale::Moldavian == Locale::Romanian");
    return Locale::Moldavian;
  }

  // Android uses the following deprecated codes.
  if (c0 == 'i' && c1 == 'w' && c2 == 0) { // iw -> he
    return Locale::Hebrew;
  }
  if (c0 == 'i' && c1 == 'n' && c2 == 0) { // in -> id
    return Locale::Indonesian;
  }
  if (c0 == 'j' && c1 == 'i' && c2 == 0) { // ji -> yi
    return Locale::Yiddish;
  }

  // Fallback
  return Locale::C;
}

Locale::Script LocaleImpl::CodeToScript(const UString& code)
{
  const int32 len = code.Len();
  if (len != 4) {
    return Locale::AnyScript;
  }

  const UNICHAR c0 = CharTraitsU::ToUpper(code[0]);
  const UNICHAR c1 = CharTraitsU::ToLower(code[1]);
  const UNICHAR c2 = CharTraitsU::ToLower(code[2]);
  const UNICHAR c3 = CharTraitsU::ToLower(code[3]);

  //TODO optimize binarysearch?
  const uint8* code_ptr = _SCRIPT_CODE_LIST;
  for (int32 i = 0; i < Locale::LastScript; ++i, code_ptr += 4) {
    if (c0 == code_ptr[0] && c1 == code_ptr[1] && c2 == code_ptr[2] && c3 == code_ptr[3]) {
      return (Locale::Script)i;
    }
  }

  // Fallback
  return Locale::AnyScript;
}

Locale::Country LocaleImpl::CodeToCountry(const UString& code)
{
  const int32 len = code.Len();
  if (!(len == 2 || len == 3)) {
    return Locale::AnyCountry;
  }

  const UNICHAR c0 = CharTraitsU::ToUpper(code[0]);
  const UNICHAR c1 = CharTraitsU::ToUpper(code[1]);
  const UNICHAR c2 = len > 2 ? CharTraitsU::ToUpper(code[2]) : 0;

  //TODO optimize binarysearch?
  const uint8* code_ptr = _COUNTRY_CODE_LIST;
  for (; *code_ptr; code_ptr += 3) {
    if (c0 == code_ptr[0] && c1 == code_ptr[1] && c2 == code_ptr[2]) {
      return (Locale::Country)((code_ptr - _COUNTRY_CODE_LIST) / 3);
    }
  }

  // Fallback
  return Locale::AnyCountry;
}

UString LocaleImpl::LanguageToCode(Locale::Language language)
{
  if (language == Locale::AnyLanguage) {
    return UString();
  }

  if (language == Locale::C) {
    return UString(AsciiString("C"));
  }

  //TODO 범위체크
  const char* code_ptr = _LANGUAGE_CODE_LIST + (uint32)language * 3;
  if (code_ptr[2] == 0) { // 2 characters
    return UString(AsciiString(code_ptr, 2));
  }
  else { // 3 characters
    return UString(AsciiString(code_ptr, 3));
  }
}

UString LocaleImpl::ScriptToCode(Locale::Script script)
{
  if (script == Locale::AnyScript) {
    return UString();
  }

  if (script > Locale::LastScript) {
    fun_check(0);
    return UString();
  }

  return UString(AsciiString(_SCRIPT_CODE_LIST + (uint32)script * 4, 4));
}

UString LocaleImpl::CountryToCode(Locale::Country country)
{
  if (country == Locale::AnyCountry) {
    return UString();
  }

  //TODO 범위체크
  const uint8* code = _COUNTRY_CODE_LIST + (uint32)country * 3;
  if (code[2] == 0) { // 2 characters
    return UString(AsciiString(code, 2));
  }
  else { // 3 characters
    return UString(AsciiString(code, 3));
  }
}


//
//
//

static bool AddLikelySubtags(LocaleId& locale_id)
{
  // ### optimize with bsearch
  const int32 LIKELY_SUB_TAGS_COUNT = countof(_LIKELY_SUB_TAGS);
  const LocaleId* ptr = _LIKELY_SUB_TAGS;
  const LocaleId* const end = ptr + LIKELY_SUB_TAGS_COUNT;
  for ( ; ptr < end; ptr += 2) {
    if (locale_id == ptr[0]) {
      locale_id = ptr[1];
      return true;
    }
  }

  return false;
}

LocaleId LocaleId::WithLikelySubtagsAdded() const
{
  // language_script_region
  if (language_id || script_id || country_id) {
    LocaleId id = LocaleId::FromIds(language_id, script_id, country_id);
    if (AddLikelySubtags(id)) {
      return id;
    }
  }

  // language_script
  if (country_id) {
    LocaleId id = LocaleId::FromIds(language_id, script_id, 0);
    if (AddLikelySubtags(id)) {
      id.country_id = country_id;
      return id;
    }
  }

  // language_region
  if (script_id) {
    LocaleId id = LocaleId::FromIds(language_id, 0, country_id);
    if (AddLikelySubtags(id)) {
      id.script_id = script_id;
      return id;
    }
  }

  // language
  if (script_id && country_id) {
    LocaleId id = LocaleId::FromIds(language_id, 0, 0);
    if (AddLikelySubtags(id)) {
      id.script_id = script_id;
      id.country_id = country_id;
      return id;
    }
  }

  return *this;
}

LocaleId LocaleId::WithLikelySubtagsRemoved() const
{
  const LocaleId max = WithLikelySubtagsAdded();

  // language
  {
    const LocaleId id = LocaleId::FromIds(language_id, 0, 0);
    if (id.WithLikelySubtagsAdded() == max) {
      return id;
    }
  }

  // language_region
  if (country_id) {
    const LocaleId id = LocaleId::FromIds(language_id, 0, country_id);
    if (id.WithLikelySubtagsAdded() == max) {
      return id;
    }
  }

  // language_script
  if (script_id) {
    const LocaleId id = LocaleId::FromIds(language_id, script_id, 0);
    if (id.WithLikelySubtagsAdded() == max) {
      return id;
    }
  }

  return max;
}

UString LocaleId::GetName(UNICHAR separator) const
{
  if (language_id == Locale::AnyLanguage) {
    return UString();
  }

  if (language_id == Locale::C) {
    return AsciiString("C");
  }

  const char* lang = _LANGUAGE_CODE_LIST + 3 * language_id;
  const uint8* script = (script_id != Locale::AnyScript ? _SCRIPT_CODE_LIST + 4 * script_id : 0);
  const uint8* country = (country_id != Locale::AnyCountry ? _COUNTRY_CODE_LIST + 3 * country_id : 0);

  UNICHAR name[5];
  UNICHAR* ptr = name;

  *ptr++ = lang[0];
  *ptr++ = lang[1];
  if (lang[2] != 0) {
    *ptr++ = lang[2];
  }

  if (script) {
    *ptr++ = separator;
    *ptr++ = script[0];
    *ptr++ = script[1];
    *ptr++ = script[2];
    *ptr++ = script[3];
  }

  if (country) {
    *ptr++ = separator;
    *ptr++ = country[0];
    *ptr++ = country[1];
    if (country[2] != 0) {
      *ptr++ = country[2];
    }
  }

  *ptr = 0; // null-term

  return UString(name);
}

UString LocaleImpl::GetBcp47Name(UNICHAR separator) const
{
  if (data->language_id == Locale::AnyLanguage) {
    return UString();
  }

  if (data->language_id == Locale::C) {
    return AsciiString("en");
  }

  const LocaleId locale_id = LocaleId::FromIds(data->language_id, data->script_id, data->country_id);
  return locale_id.WithLikelySubtagsRemoved().GetName(separator);
}

static const LocaleData* FindLocaleDataById(const LocaleId& locale_id)
{
  const uint32 idx = _LOCALE_INDEX[locale_id.language_id];

  const LocaleData* data = _LOCALE_DATA + idx;

  if (idx == 0) { // default language has no associated script or country
    return data;
  }

  fun_check(data->language_id == locale_id.language_id);

  if (locale_id.script_id == Locale::AnyScript && locale_id.country_id == Locale::AnyCountry) {
    return data;
  }

  if (locale_id.script_id == Locale::AnyScript) {
    do {
      if (data->country_id == locale_id.country_id) {
        return data;
      }
      ++data;
    } while (data->language_id && data->language_id == locale_id.language_id);
  }
  else if (locale_id.country_id == Locale::AnyCountry) {
    do {
      if (data->script_id == locale_id.script_id) {
        return data;
      }
      ++data;
    } while (data->language_id && data->language_id == locale_id.language_id);
  }
  else {
    do {
      if (data->script_id == locale_id.script_id && data->country_id == locale_id.country_id) {
        return data;
      }
      ++data;
    } while (data->language_id && data->language_id == locale_id.language_id);
  }

  return 0;
}

const LocaleData* LocaleData::FindLocaleData( Locale::Language language,
                                              Locale::Script script,
                                              Locale::Country country)
{
  LocaleId locale_id = LocaleId::FromIds(language, script, country);
  locale_id = locale_id.WithLikelySubtagsAdded();

  const uint32 idx = _LOCALE_INDEX[locale_id.language_id];

  // Try a straight match
  if (const LocaleData* const data = FindLocaleDataById(locale_id)) {
    return data;
  }

  Array<LocaleId> tried;
  tried.Add(locale_id);

  // No match; try again with likely country
  locale_id = LocaleId::FromIds(language, script, Locale::AnyCountry);
  locale_id = locale_id.WithLikelySubtagsAdded();
  if (!tried.Contains(locale_id)) {
    if (const LocaleData* const data = FindLocaleDataById(locale_id)) {
      return data;
    }
    tried.Add(locale_id);
  }

  // No match; try again with any country
  locale_id = LocaleId::FromIds(language, script, Locale::AnyCountry);
  if (!tried.Contains(locale_id)) {
    if (const LocaleData* const data = FindLocaleDataById(locale_id)) {
      return data;
    }
    tried.Add(locale_id);
  }

  // No match; try again with likely script
  locale_id = LocaleId::FromIds(language, Locale::AnyScript, country);
  locale_id = locale_id.WithLikelySubtagsAdded();
  if (!tried.Contains(locale_id)) {
    if (const LocaleData* const data = FindLocaleDataById(locale_id)) {
      return data;
    }
    tried.Add(locale_id);
  }

  // No match; try again with any script
  locale_id = LocaleId::FromIds(language, Locale::AnyScript, country);
  if (!tried.Contains(locale_id)) {
    if (const LocaleData* const data = FindLocaleDataById(locale_id)) {
      return data;
    }
    tried.Add(locale_id);
  }

  // No match; return data at original index
  return _LOCALE_DATA + idx;
}

static bool ParseLocaleTag( const UString& input,
                            int32& i,
                            UString* result,
                            const UString& separators)
{
  *result = UString();

  UNICHAR dst_buf[8+1];
  UNICHAR* dst = dst_buf;
  const UNICHAR* src = *input + i;
  const int32 len = input.Len();
  for (; i < len && (dst - dst_buf) < 8; ++i) {
    if (separators.Contains(*src)) {
      break;
    }

    if (!((*src >= 'a' && *src <= 'z') ||
          (*src >= 'A' && *src <= 'Z') ||
          (*src >= '0' && *src <= '9'))) {
      return false;
    }
    *dst++ = *src++;
  }
  *dst++ = 0;

  *result = UString(dst_buf);
  return true;
}

bool SplitLocaleName( const UString& name,
                      UString& language,
                      UString& script,
                      UString& country)
{
  const int32 length = name.Len();

  language = script = country = UString();

  const UString separators = AsciiString("_-.@");
  enum ParserState { NoState, LangState, ScriptState, CountryState };
  ParserState state = LangState;
  for (int32 i = 0; i < length && state != NoState; ) {
    UString value;
    if (!ParseLocaleTag(name, i, &value, separators) || value.IsEmpty()) {
      break;
    }

    const UNICHAR sep = i < length ? name[i] : UNICHAR(0);
    switch (state) {
    case LangState:
      if (sep != 0 && !separators.Contains(sep)) {
        state = NoState;
        break;
      }
      language = value;
      if (i == length) {
        // just language was specified
        state = NoState;
        break;
      }
      state = ScriptState;
      break;

    case ScriptState: {
      UString scripts = AsciiString(_SCRIPT_CODE_LIST, countof(_SCRIPT_CODE_LIST)-1); //TODO 이거 뭐하는짓이지??
      if (value.Len() == 4 && scripts.IndexOf(value) % 4 == 0) {
        // script name is always 4 characters
        script = value;
        state = CountryState;
      }
      else {
        // it wasn't a script, maybe it is a country then?
        country = value;
        state = NoState;
      }
      break;
    }

    case CountryState:
      country = value;
      state = NoState;
      break;

    case NoState:
      // shouldn't happen
      //fun_log(Warning, "Locale: This should never happen");
      break;
    }
    ++i;
  }

  return language.Len() == 2 || language.Len() == 3;
}

void LocaleImpl::GetLangAndCountry( const UString& name,
                                    Locale::Language& language,
                                    Locale::Script& script,
                                    Locale::Country& country)
{
  language = Locale::C;
  script = Locale::AnyScript;
  country = Locale::AnyCountry;

  UString lang_code;
  UString script_code;
  UString country_code;
  if (!SplitLocaleName(name, lang_code, script_code, country_code)) {
    return;
  }

  language = LocaleImpl::CodeToLanguage(lang_code);
  if (language == Locale::C) {
    return;
  }

  script = LocaleImpl::CodeToScript(script_code);
  country = LocaleImpl::CodeToCountry(country_code);
}

static const LocaleData* FindLocaleData(const UString& name)
{
  Locale::Language language;
  Locale::Script script;
  Locale::Country country;
  LocaleImpl::GetLangAndCountry(name, language, script, country);

  return LocaleData::FindLocaleData(language, script, country);
}

UString ReadEscapedFormatString(const UString& format, int32* idx)
{
  int32& i = *idx;

  fun_check(format[i] == '\'');
  ++i;
  if (i == format.Len()) {
    return UString();
  }

  if (format[i] == '\'') { // "''" outside of a quoted stirng
    ++i;
    return AsciiString("'");
  }

  UString result;
  while (i < format.Len()) {
    if (format[i] == '\'') {
      if (i + 1 < format.Len() && format[i + 1] == '\'') {
        // "''" inside of a quoted UString
        result += AsciiString('\'');
        i += 2;
      }
      else {
        break;
      }
    }
    else {
      result += format[i++];
    }
  }
  if (i < format.Len()) {
    ++i;
  }

  return result;
}


int32 RepeatCount(const UString& str)
{
  if (str.IsEmpty()) {
    return 0;
  }

  const UNICHAR first_char = str[0];
  int32 idx = 0;
  while (idx < str.Len() && str[idx] == first_char) {
    ++idx;
  }
  return idx;
}


static const LocaleData* g_default_data = nullptr;
static Locale::NumberOptions g_default_number_options = Locale::DefaultNumberOptions;

static const LocaleData* const C_Data = _LOCALE_DATA;

//static LocaleImpl* GetPrivateANSI()
//{
//  //TODO 두번째 필드(참조계수)는 atomic이어야함.
//  static LocaleImpl g_ansi_locale = { C_Data, 1, Locale::OmitGroupSeparator };
//  return &g_ansi_locale;
//}

static ScopedPtr<LocaleImpl> GetPrivateANSI()
{
  //TODO 두번째 필드(참조계수)는 atomic이어야함.
  //static LocaleImpl g_ansi_locale = { C_Data, 1, Locale::OmitGroupSeparator };
  //return ScopedPtr<LocaleImpl>(&g_ansi_locale);
  static ScopedPtr<LocaleImpl> instance;
  if (!instance.IsValid()) {
    instance = ScopedPtr<LocaleImpl>(new LocaleImpl{ C_Data, 1, Locale::OmitGroupSeparator });
  }
  return instance;
}


#if !FUN_NO_SYSTEM_LOCALE

SystemLocale::SystemLocale()
{
  g_system_locale = this;

  if (g_system_data) {
    g_system_data->language_id = 0;
  }
}

SystemLocale::SystemLocale(bool)
{
}

SystemLocale::~SystemLocale()
{
  if (g_system_locale == this) {
    g_system_locale = nullptr;

    if (g_system_data) {
      g_system_data->language_id = 0;
    }
  }
}

static const SystemLocale* GetSystemLocale()
{
  if (g_system_locale) {
    return g_system_locale;
  }

  return &SystemLocaleSingleton::Get();
}

void LocaleImpl::UpdateSystemImpl()
{
  const SystemLocale* sys_locale = GetSystemLocale();

  if (g_system_data == nullptr) {
    g_system_data = &g_locale_data;
  }

  // tell the object that the System locale has changed.
  sys_locale->Query(SystemLocale::LocaleChanged, Variant());

  *g_system_data = *sys_locale->GetFallbackUiLocale().impl_->data;

  Variant result;

  result = sys_locale->Query(SystemLocale::LanguageId, Variant());
  if (!result.IsNull()) {
    g_system_data->language_id = result.Value<Locale::Language>();
    g_system_data->script_id = Locale::AnyScript; // default for compatibility
  }

  result = sys_locale->Query(SystemLocale::CountryId, Variant());
  if (!result.IsNull()) {
    g_system_data->country_id = result.Value<Locale::Country>();
    g_system_data->script_id = Locale::AnyScript; // default for compatibility
  }

  result = sys_locale->Query(SystemLocale::ScriptId, Variant());
  if (!result.IsNull()) {
    g_system_data->script_id = result.Value<Locale::Script>();
  }

  result = sys_locale->Query(SystemLocale::DecimalPoint, Variant());
  if (!result.IsNull()) {
    g_system_data->decimal = result.Value<UNICHAR>();
  }

  result = sys_locale->Query(SystemLocale::GroupSeparator, Variant());
  if (!result.IsNull()) {
    g_system_data->group = result.Value<UNICHAR>();
  }

  result = sys_locale->Query(SystemLocale::ZeroDigit, Variant());
  if (!result.IsNull()) {
    g_system_data->zero = result.Value<UNICHAR>();
  }

  result = sys_locale->Query(SystemLocale::NegativeSign, Variant());
  if (!result.IsNull()) {
    g_system_data->minus = result.Value<UNICHAR>();
  }

  result = sys_locale->Query(SystemLocale::PositiveSign, Variant());
  if (!result.IsNull()) {
    g_system_data->plus = result.Value<UNICHAR>();
  }
}
#endif //!FUN_NO_SYSTEM_LOCALE

static const LocaleData* GetSystemData()
{
#if !FUN_NO_SYSTEM_LOCALE
  // copy over the information from the fallback locale and modify
  if (g_system_data == nullptr || g_system_data->language_id == 0) {
    LocaleImpl::UpdateSystemImpl();
  }

  return g_system_data;
#else
  return _LOCALE_DATA;
#endif
}

static const LocaleData* GetDefaultData()
{
  if (g_default_data == nullptr) {
    g_default_data = GetSystemData();
  }

  return g_default_data;
}

const LocaleData* LocaleData::C()
{
  fun_check(_LOCALE_INDEX[Locale::C] == 0);
  return C_Data;
}

static FUN_ALWAYS_INLINE UString GetLocaleData(const uint16* data, int32 len)
{
  return len > 0 ? UString::FromRawData((const UNICHAR*)data, len) : UString();
}

static UString GetLocaleListData(const uint16* data, int32 len, int32 index)
{
  static const UNICHAR separator = ';';
  while (index && len > 0) {
    while (*data != separator) {
      ++data;
      --len;
    }

    --index;
    ++data;
    --len;
  }

  const uint16* end = data;
  while (len > 0 && *end != separator) {
    ++end;
    --len;
  }

  return GetLocaleData(data, (int32)(end - data));
}


static const int32 _LOCALE_DATA_COUNT = countof(_LOCALE_DATA);

//TODO
//Q_GLOBAL_STATIC_WITH_ARGS(QSharedDataPointer<LocaleImpl>, DefaultLocalePrivate, (LocaleImpl::Create(GetDefaultData(), DefaultNumberOptions)))

static ScopedPtr<LocaleImpl> g_default_locale_private(LocaleImpl::Create(GetDefaultData(), g_default_number_options));

static ScopedPtr<LocaleImpl> LocalePrivateByName(const UString& name)
{
  if (name == AsciiString("C")) {
    return GetPrivateANSI();
  }

  const LocaleData* data = FindLocaleData(name);
  return ScopedPtr<LocaleImpl>(LocaleImpl::Create(data, data->language_id == Locale::C ? Locale::OmitGroupSeparator : Locale::DefaultNumberOptions));
}

static ScopedPtr<LocaleImpl> FindLocalePrivate(Locale::Language language, Locale::Script script, Locale::Country country)
{
  if (language == Locale::C) {
    return GetPrivateANSI();
  }

  const LocaleData* data = LocaleData::FindLocaleData(language, script, country);

  Locale::NumberOptions number_options = Locale::DefaultNumberOptions;

  // If not found, should default to System
  if (data->language_id == Locale::C && language != Locale::C) {
    number_options = g_default_number_options;
    data = GetDefaultData();
  }
  return ScopedPtr<LocaleImpl>(LocaleImpl::Create(data, number_options));
}


//
// Locale
//

Locale::Locale(LocaleImpl* impl)
  : impl_(impl)
{
}

Locale::Locale(const UString& name)
  : impl_(LocalePrivateByName(name))
{
}

Locale::Locale()
  : impl_(g_default_locale_private)
{
}

Locale::Locale(Language language, Country country)
  : impl_(FindLocalePrivate(language, Locale::AnyScript, country))
{
}

Locale::Locale(Language language, Script script, Country country)
  : impl_(FindLocalePrivate(language, script, country))
{
}

Locale::Locale(const Locale& rhs)
  : impl_(rhs.impl_)
{
}

Locale::~Locale()
{
}

Locale& Locale::operator = (const Locale& rhs)
{
  impl_ = rhs.impl_;
  return *this;
}

bool Locale::operator == (const Locale& rhs) const
{
  return impl_->data == rhs.impl_->data && impl_->number_options == rhs.impl_->number_options;
}

bool Locale::operator != (const Locale& rhs) const
{
  return impl_->data != rhs.impl_->data || impl_->number_options != rhs.impl_->number_options;
}

//TODO 이하 비교에 NumberOptions를 적용해야할듯...?

bool Locale::operator < (const Locale& rhs) const
{
  return GetName() < rhs.GetName();
}

bool Locale::operator <= (const Locale& rhs) const
{
  return GetName() <= rhs.GetName();
}

bool Locale::operator >  (const Locale& rhs) const
{
  return GetName() > rhs.GetName();
}

bool Locale::operator >= (const Locale& rhs) const
{
  return GetName() >= rhs.GetName();
}

//TODO hash
// 우선, hash는 이름으로 하도록 해놓긴 했는데, NumberOptions때문에 하...
// hash(impl_->data) + hash(impl_->number_options)

Locale::NumberOptions Locale::GetNumberOptions() const
{
  return impl_->number_options;
}

void Locale::SetNumberOptions(NumberOptions options)
{
  impl_->number_options = options;
}

UString Locale::QuoteString(const UString& str, QuotationStyle style) const
{
#if !FUN_NO_SYSTEM_LOCALE
  if (impl_->data == GetSystemData()) {
    Variant result;
    if (style == Locale::AlternateQuotation) {
      result = GetSystemLocale()->Query(SystemLocale::StringToAlternateQuotation, Variant(str));
    }
    if (result.IsNull() || style == Locale::StandardQuotation) {
      result = GetSystemLocale()->Query(SystemLocale::StringToStandardQuotation, Variant(str));
    }
    if (!result.IsNull()) {
      return result.Value<UString>();
    }
  }
#endif //!FUN_NO_SYSTEM_LOCALE

  if (style == Locale::StandardQuotation) {
    return UNICHAR(impl_->data->quotation_start) + str + UNICHAR(impl_->data->quotation_end);
  }
  else {
    return UNICHAR(impl_->data->alternate_quotation_start) + str + UNICHAR(impl_->data->alternate_quotation_end);
  }
}

//TODO 포맷 문자열을 변경해주어야함.
//그냥 매번 replace를 해도 되긴하겠지만... 흠...
//시스템 로케일에서도 %1 %2 형태로 돌려주기 때문에 어쩔수 없이 replace를 해주어야할듯...
UString Locale::CreateSeparatedList(const Array<UString>& list) const
{
#if !FUN_NO_SYSTEM_LOCALE
  if (impl_->data == GetSystemData()) {
    const Variant result = GetSystemLocale()->Query(SystemLocale::ListToSeparatedString, Variant(list));
    if (!result.IsNull()) {
      return result.Value<UString>();
    }
  }
#endif

  const int32 num = list.Count();
  if (num == 1) {
    return list[0];
  }
  else if (num == 2) {
    UString format = GetLocaleData(_LIST_PATTERN_PART_DATA + impl_->data->list_pattern_part_two_idx, impl_->data->list_pattern_part_two_len);
    //TODO
    //return format.arg(list[0], list[1]);
    //fun_log(LogCore, Info, L"format: %s", *format);
    return AsciiString("");
  }
  else if (num > 2) {
    UString format_start = GetLocaleData(_LIST_PATTERN_PART_DATA + impl_->data->list_pattern_part_start_idx, impl_->data->list_pattern_part_start_len);
    UString format_mid = GetLocaleData(_LIST_PATTERN_PART_DATA + impl_->data->list_pattern_part_mid_idx, impl_->data->list_pattern_part_mid_len);
    UString format_end = GetLocaleData(_LIST_PATTERN_PART_DATA + impl_->data->list_pattern_part_end_idx, impl_->data->list_pattern_part_end_len);
    //TODO
    //UString result = format_start.arg(list[0], list[1]);
    //for (int32 i = 2; i < num - 1; ++i) {
    //  result = format_mid.arg(result, list[i]);
    //}
    //result = format_end.arg(result, list[num - 1]);
    //return result;

    //fun_log(LogCore, Info, L"format_start: %s", *format_start);
    //fun_log(LogCore, Info, L"format_mid: %s", *format_mid);
    //fun_log(LogCore, Info, L"format_end: %s", *format_end);
    return AsciiString("");
  }

  return UString();
}

Locale Locale::GetDefault()
{
  return Locale();
}

void Locale::SetDefault(const Locale& locale)
{
  g_default_data = locale.impl_->data;
  g_default_number_options = locale.GetNumberOptions();

  if (g_default_locale_private) {
    // update the cached private
    g_default_locale_private = locale.impl_;
  }
}

Locale::Language Locale::GetLanguage() const
{
  return (Language)impl_->GetLanguageId();
}

Locale::Script Locale::GetScript() const
{
  return (Script)impl_->data->script_id;
}

Locale::Country Locale::GetCountry() const
{
  return (Country)impl_->GetCountryId();
}

UString Locale::GetName() const
{
  const Language lang = GetLanguage();
  if (lang == C) {
    return impl_->GetLanguageCode();
  }

  const Country country = GetCountry();
  if (country == AnyCountry) {
    return impl_->GetLanguageCode();
  }

  return impl_->GetLanguageCode() + AsciiString("_") + impl_->GetCountryCode();
}

UString Locale::GetBcp47Name() const
{
  return impl_->GetBcp47Name();
}

UString Locale::LanguageToString(Language language)
{
  if (uint32(language) > uint32(Locale::LastLanguage)) {
    return AsciiString("Unknown");
  }

  //TODO Latin1String으로 해주어야함.
  return AsciiString(_LANGUAGE_NAME_LIST + _LANGUAGE_NAME_INDEX[language]);
}

UString Locale::CountryToString(Locale::Country country)
{
  if (uint32(country) > uint32(Locale::LastCountry)) {
    return AsciiString("Unknown");
  }

  //TODO Latin1String으로 해주어야함.
  return AsciiString(_COUNTRY_NAME_LIST + _COUNTRY_NAME_INDEX[country]);
}

UString Locale::ScriptToString(Locale::Script script)
{
  if (uint32(script) > uint32(Locale::LastScript)) {
    return AsciiString("Unknown");
  }

  //TODO Latin1String으로 해주어야함.
  return AsciiString(_SCRIPT_NAME_LIST + _SCRIPT_NAME_INDEX[script]);
}


static int64 ToIntegral_helper( const LocaleData* d,
                                const UNICHAR* str,
                                int32 len,
                                bool* ok,
                                Locale::NumberOptions mode,
                                int64)
{
  return d->StringToInt64(str, len, 10, ok, mode);
}


static uint64 ToIntegral_helper(const LocaleData* d,
                                const UNICHAR* str,
                                int32 len,
                                bool* ok,
                                Locale::NumberOptions mode,
                                uint64)
{
  return d->StringToUInt64(str, len, 10, ok, mode);
}

template <typename T> static FUN_ALWAYS_INLINE
T ToIntegral_helper(const LocaleImpl* impl,
                    const UNICHAR* str,
                    int32 len,
                    bool* ok)
{
  const bool is_unsigned = T(0) < T(-1);
  typedef typename Conditional<is_unsigned, uint64, int64>::Result Int64Type;

  // we select the right overload by the last, unused parameter
  Int64Type val = ToIntegral_helper(impl->data, str, len, ok, impl->number_options, Int64Type(0));
  if (T(val) != val) {
    if (ok) {
      *ok = false;
    }
    val = 0;
  }

  return T(val);
}

int16 Locale::ToInt16(const UNICHAR* str, int32 len, bool* ok) const
{
  return ToIntegral_helper<int16>(impl_.Get(), str, len, ok);
}

uint16 Locale::ToUInt16(const UNICHAR* str, int32 len, bool* ok) const
{
  return ToIntegral_helper<uint16>(impl_.Get(), str, len, ok);
}

int32 Locale::ToInt32(const UNICHAR* str, int32 len, bool* ok) const
{
  return ToIntegral_helper<int32>(impl_.Get(), str, len, ok);
}

uint32 Locale::ToUInt32(const UNICHAR* str, int32 len, bool* ok) const
{
  return ToIntegral_helper<uint32>(impl_.Get(), str, len, ok);
}

int64 Locale::ToInt64(const UNICHAR* str, int32 len, bool* ok) const
{
  return ToIntegral_helper<int64>(impl_.Get(), str, len, ok);
}

uint64 Locale::ToUInt64(const UNICHAR* str, int32 len, bool* ok) const
{
  return ToIntegral_helper<uint64>(impl_.Get(), str, len, ok);
}

float Locale::ToFloat(const UNICHAR* str, int32 len, bool* ok) const
{
  return LocaleData::ConvertDoubleToFloat(ToDouble(str, len, ok), ok);
}

double Locale::ToDouble(const UNICHAR* str, int32 len, bool* ok) const
{
  return impl_->data->StringToDouble(str, len, ok, impl_->number_options);
}

UString Locale::ToString(int64 value) const
{
  const int32 flags = impl_->number_options & OmitGroupSeparator ? 0 : LocaleData::ThousandsGroup;
  return impl_->data->Int64ToString(value, -1, 10, -1, flags);
}

UString Locale::ToString(uint64 value) const
{
  const int32 flags = impl_->number_options & OmitGroupSeparator ? 0 : LocaleData::ThousandsGroup;
  return impl_->data->UInt64ToString(value, -1, 10, -1, flags);
}

UString Locale::ToString(const Date& date, const UString& format) const
{
  return impl_->DateTimeToString(format, nullptr, &date, nullptr, this);
}

UString Locale::ToString(const Date& date, FormatType format) const
{
  if (!date.IsValid()) {
    return UString();
  }

#if !FUN_NO_SYSTEM_LOCALE
  if (impl_->data == GetSystemData()) {
    const Variant result = GetSystemLocale()->Query(format == LongFormat ? SystemLocale::DateToStringLong : SystemLocale::DateToStringShort, date);
    if (!result.IsNull()) {
      return result.Value<UString>();
    }
  }
#endif

  UString format_str = GetDateFormat(format);
  return ToString(date, format_str);
}

UString Locale::ToString(const Time& time, const UString& format) const
{
  return impl_->DateTimeToString(format, nullptr, nullptr, &time, this);
}

UString Locale::ToString(const Time& time, FormatType format) const
{
  if (!time.IsValid()) {
    return UString();
  }

#if !FUN_NO_SYSTEM_LOCALE
  if (impl_->data == GetSystemData()) {
    const Variant result = GetSystemLocale()->Query(format == LongFormat ? SystemLocale::TimeToStringLong : SystemLocale::TimeToStringShort, time);
    if (!result.IsNull()) {
      return result.Value<UString>();
    }
  }
#endif

  UString format_str = GetTimeFormat(format);
  return ToString(time, format_str);
}

UString Locale::ToString(const DateTime& date_time, const UString& format) const
{
  return impl_->DateTimeToString(format, &date_time, nullptr, nullptr, this);
}

UString Locale::ToString(const DateTime& date_time, FormatType format) const
{
  if (!date_time.IsValid()) {
    return UString();
  }

#if !FUN_NO_SYSTEM_LOCALE
  if (impl_->data == GetSystemData()) {
    const Variant result = GetSystemLocale()->Query(format == LongFormat ? SystemLocale::DateTimeToStringLong : SystemLocale::DateTimeToStringShort, date_time);
    if (!result.IsNull()) {
      const UString string = result.Value<UString>();
      return string;
    }
  }
#endif

  const UString format_str = GetDateTimeFormat(format);
  return ToString(date_time, format_str);
}

UString Locale::GetDateFormat(FormatType format) const
{
#if !FUN_NO_SYSTEM_LOCALE
  if (impl_->data == GetSystemData()) {
    const Variant result = GetSystemLocale()->Query(format == LongFormat ? SystemLocale::DateFormatLong : SystemLocale::DateFormatShort, Variant());
    if (!result.IsNull()) {
      return result.Value<UString>();
    }
  }
#endif

  uint32 idx, len;
  switch (format) {
  case LongFormat:
    idx = impl_->data->long_date_format_idx;
    len = impl_->data->long_date_format_size;
    break;
  default:
    idx = impl_->data->short_date_format_idx;
    len = impl_->data->short_date_format_size;
    break;
  }

  return GetLocaleData(_DATE_FORMAT_DATA + idx, len);
}

UString Locale::GetTimeFormat(FormatType format) const
{
#if !FUN_NO_SYSTEM_LOCALE
  if (impl_->data == GetSystemData()) {
    const Variant result = GetSystemLocale()->Query(format == LongFormat ? SystemLocale::TimeFormatLong : SystemLocale::TimeFormatShort, Variant());
    if (!result.IsNull()) {
      return result.Value<UString>();
    }
  }
#endif

  uint32 idx, len;
  switch (format) {
  case LongFormat:
    idx = impl_->data->long_time_format_idx;
    len = impl_->data->long_time_format_len;
    break;
  default:
    idx = impl_->data->short_time_format_idx;
    len = impl_->data->short_time_format_len;
    break;
  }

  return GetLocaleData(_TIME_FORMAT_DATA + idx, len);
}

UString Locale::GetDateTimeFormat(FormatType format) const
{
#if !FUN_NO_SYSTEM_LOCALE
  if (impl_->data == GetSystemData()) {
    const Variant result = GetSystemLocale()->Query(format == LongFormat ? SystemLocale::DateTimeFormatLong : SystemLocale::DateTimeFormatShort, Variant());
    if (!result.IsNull()) {
      return result.Value<UString>();
    }
  }
#endif

  return GetDateFormat(format) + AsciiString(' ') + GetTimeFormat(format);
}

#if !FUN_NO_DATESTRING
Time Locale::ToTime(const UString& string, FormatType format) const
{
  return ToTime(string, GetTimeFormat(format));
}
#endif

#if !FUN_NO_DATESTRING
Date Locale::ToDate(const UString& string, FormatType format) const
{
  return ToDate(string, GetDateFormat(format));
}
#endif

#if !FUN_NO_DATESTRING
DateTime Locale::ToDateTime(const UString& string, FormatType format) const
{
  return ToDateTime(string, GetDateTimeFormat(format));
}
#endif

#if !FUN_NO_DATESTRING
Time Locale::ToTime(const UString& string, const UString& format) const
{
  Time time{ 0,0,0,0 };
  DateTimeParser praser(VariantTypes::Time, DateTimeParser::CONTEXT_FromString);
  praser.SetDefaultLocale(*this);
  if (praser.ParseFormat(format.ToUtf8())) {
    praser.FromString(string.ToUtf8(), 0, &time);
  }

  return time;
}
#endif

#if !FUN_NO_DATESTRING
Date Locale::ToDate(const UString& string, const UString& format) const
{
  Date date{ 0,0,0 };
  DateTimeParser praser(VariantTypes::Date, DateTimeParser::CONTEXT_FromString);
  praser.SetDefaultLocale(*this);
  if (praser.ParseFormat(format.ToUtf8())) {
    praser.FromString(string.ToUtf8(), &date, 0);
  }

  return date;
}
#endif

#if !FUN_NO_DATESTRING
DateTime Locale::ToDateTime(const UString& string, const UString& format) const
{
  Time time;
  Date date;

  DateTimeParser praser(VariantTypes::DateTime, DateTimeParser::CONTEXT_FromString);
  praser.SetDefaultLocale(*this);
  if (praser.ParseFormat(format.ToUtf8()) && praser.FromString(string.ToUtf8(), &date, &time)) {
    return DateTime(date, time);
  }

  return DateTime(Date(), Time());
}
#endif

UNICHAR Locale::GetDecimalPoint() const
{
  return impl_->GetDecimal();
}

UNICHAR Locale::GetGroupSeparator() const
{
  return impl_->GetGroup();
}

UNICHAR Locale::GetPercent() const
{
  return impl_->GetPercent();
}

UNICHAR Locale::GetZeroDigit() const
{
  return impl_->GetZero();
}

UNICHAR Locale::GetNegativeSign() const
{
  return impl_->GetMinus();
}

UNICHAR Locale::GetPositiveSign() const
{
  return impl_->GetPlus();
}

UNICHAR Locale::GetExponential() const
{
  return impl_->GetExponential();
}

UString Locale::ToString(double value, UNICHAR f, int32 precision) const
{
  LocaleData::DoubleForm form = LocaleData::DFDecimal;
  uint32 flags = 0;

  if (CharTraitsU::IsUpper(f)) {
    flags = LocaleData::CapitalEorX;
  }
  f = CharTraitsU::ToLower(f);

  switch (f) {
  case 'f': form = LocaleData::DFDecimal; break;
  case 'e': form = LocaleData::DFExponent; break;
  case 'g': form = LocaleData::DFSignificantDigits; break;
  default: break;
  }

  if (!(impl_->number_options & OmitGroupSeparator)) {
    flags |= LocaleData::ThousandsGroup;
  }

  if (!(impl_->number_options & OmitLeadingZeroInExponent)) {
    flags |= LocaleData::ZeroPadExponent;
  }

  if (impl_->number_options & IncludeTrailingZeroesAfterDot) {
    flags |= LocaleData::AddTrailingZeroes;
  }

  return impl_->data->DoubleToString(value, precision, form, -1, flags);
}

//TODO 함수명에 대해서 고민해봐야함.
Locale Locale::System()
{
  return Locale(LocaleImpl::Create(GetSystemData()));
}

Array<Locale> Locale::MatchingLocales( Locale::Language language,
                                        Locale::Script script,
                                        Locale::Country country)
{
  if (uint32(language)  > Locale::LastLanguage ||
      uint32(script)    > Locale::LastScript ||
      uint32(country)   > Locale::LastCountry) {
    return Array<Locale>();
  }

  if (language == Locale::C) {
    Array<Locale> result;
    result.Add(Locale(Locale::C));
    return result;
  }

  Array<Locale> result;
  if (language  == Locale::AnyLanguage &&
      script    == Locale::AnyScript &&
      country   == Locale::AnyCountry) {
    result.Reserve(_LOCALE_DATA_COUNT);
  }

  const LocaleData* data = _LOCALE_DATA + _LOCALE_INDEX[language];
  while ( (data != _LOCALE_DATA + _LOCALE_DATA_COUNT)
      && (language == Locale::AnyLanguage || data->language_id == uint32(language))) {
    if ((script == Locale::AnyScript || data->script_id == uint32(script))
        && (country == Locale::AnyCountry || data->country_id == uint32(country))) {
      //TODO LocaleImpl::Create에서 메모리 릭이 발생할것 같은데??
      result.Add(Locale((data->language_id == C ? GetPrivateANSI().Get() : LocaleImpl::Create(data))));
    }
    ++data;
  }

  return result;
}

Array<Locale::Country> Locale::CountriesForLanguage(Language language)
{
  Array<Country> result;
  if (language == C) {
    result.Add(AnyCountry);
    return result;
  }

  const uint32 language_id = language;
  const LocaleData* data = _LOCALE_DATA + _LOCALE_INDEX[language_id];
  while (data->language_id == language_id) {
    const Locale::Country country = static_cast<Country>(data->country_id);
    if (!result.Contains(country)) {
      result.Add(country);
    }
    ++data;
  }

  return result;
}

UString Locale::GetMonthName(int32 month, FormatType type) const
{
  if (month < 1 || month > 12) {
    return UString();
  }

#if !FUN_NO_SYSTEM_LOCALE
  if (impl_->data == GetSystemData()) {
    Variant result = GetSystemLocale()->Query(type == LongFormat ? SystemLocale::MonthNameLong : SystemLocale::MonthNameShort, month);
    if (!result.IsNull()) {
      return result.Value<UString>();
    }
  }
#endif

  uint32 idx, len;
  switch (type) {
  case Locale::LongFormat:
    idx = impl_->data->long_month_names_idx;
    len = impl_->data->long_month_names_len;
    break;
  case Locale::ShortFormat:
    idx = impl_->data->short_month_names_idx;
    len = impl_->data->short_month_names_len;
    break;
  case Locale::NarrowFormat:
    idx = impl_->data->narrow_month_names_idx;
    len = impl_->data->narrow_month_names_len;
    break;
  default:
    return UString();
  }

  return GetLocaleListData(_MONTHS_DATA + idx, len, month - 1);
}

UString Locale::GetStandaloneMonthName(int32 month, FormatType type) const
{
  if (month < 1 || month > 12) {
    return UString();
  }

#if !FUN_NO_SYSTEM_LOCALE
  if (impl_->data == GetSystemData()) {
    const Variant result = GetSystemLocale()->Query(type == LongFormat ? SystemLocale::StandaloneMonthNameLong : SystemLocale::StandaloneMonthNameShort, month);
    if (!result.IsNull()) {
      return result.Value<UString>();
    }
  }
#endif

  uint32 idx, len;
  switch (type) {
  case Locale::LongFormat:
    idx = impl_->data->standalone_long_month_names_idx;
    len = impl_->data->standalone_long_month_names_len;
    break;
  case Locale::ShortFormat:
    idx = impl_->data->standalone_short_month_names_idx;
    len = impl_->data->standalone_short_month_names_len;
    break;
  case Locale::NarrowFormat:
    idx = impl_->data->standalone_narrow_month_names_idx;
    len = impl_->data->standalone_narrow_month_names_len;
    break;
  default:
    return UString();
  }

  const UString name = GetLocaleListData(_MONTHS_DATA + idx, len, month - 1);
  if (name.Len() == 0) {
    return GetMonthName(month, type);
  }

  return name;
}

//TODO 값이 정상적인지 확인해야함.
UString Locale::GetDayName(DayOfWeekType day, FormatType type) const
{
  if (day < DayOfWeekType::Sunday || day > DayOfWeekType::Saturday) {
    return UString();
  }

#if !FUN_NO_SYSTEM_LOCALE
  if (impl_->data == GetSystemData()) {
    const Variant result = GetSystemLocale()->Query(type == LongFormat ? SystemLocale::DayNameLong : SystemLocale::DayNameShort, day);
    if (!result.IsNull()) {
      return result.Value<UString>();
    }
  }
#endif

  uint32 idx, len;
  switch (type) {
  case Locale::LongFormat:
    idx = impl_->data->long_day_names_idx;
    len = impl_->data->long_day_names_len;
    break;
  case Locale::ShortFormat:
    idx = impl_->data->short_day_names_idx;
    len = impl_->data->short_day_names_len;
    break;
  case Locale::NarrowFormat:
    idx = impl_->data->narrow_day_names_idx;
    len = impl_->data->narrow_day_names_len;
    break;
  default:
    return UString();
  }

  return GetLocaleListData(_DAYS_DATA + idx, len, (int32)day);
}

//TODO 값이 정상적인지 확인해야함.
UString Locale::GetStandaloneDayName(DayOfWeekType day, FormatType type) const
{
  if (day < DayOfWeekType::Sunday || day > DayOfWeekType::Saturday) {
    return UString();
  }

#if !FUN_NO_SYSTEM_LOCALE
  if (impl_->data == GetSystemData()) {
    const Variant result = GetSystemLocale()->Query(type == LongFormat ? SystemLocale::DayNameLong : SystemLocale::DayNameShort, day);
    if (!result.IsNull()) {
      return result.Value<UString>();
    }
  }
#endif

  uint32 idx, len;
  switch (type) {
  case Locale::LongFormat:
    idx = impl_->data->standalone_long_day_names_idx;
    len = impl_->data->standalone_long_day_names_len;
    break;

  case Locale::ShortFormat:
    idx = impl_->data->standalone_short_day_names_idx;
    len = impl_->data->standalone_short_day_names_len;
    break;

  case Locale::NarrowFormat:
    idx = impl_->data->standalone_narrow_day_names_idx;
    len = impl_->data->standalone_narrow_day_names_len;
    break;

  default:
    return UString();
  }

  const UString name = GetLocaleListData(_DAYS_DATA + idx, len, (int32)day);
  if (name.IsEmpty()) {
    return GetDayName(day, type);
  }

  return name;
}

DayOfWeekType Locale::GetFirstDayOfWeek() const
{
#if !FUN_NO_SYSTEM_LOCALE
  if (impl_->data == GetSystemData()) {
    Variant result = GetSystemLocale()->Query(SystemLocale::FirstDayOfWeek, Variant());
    if (!result.IsNull()) {
      return result.Value<DayOfWeekType>();
    }
  }
#endif

  //Translate
  //  data: 1=Monday, 7=Sunday
  //  Our : 0=Sunday, 6=Saturday
  int32 day_of_week = (int32)impl_->data->first_day_of_week - 1;
  if (day_of_week < 0) {
    day_of_week += 7;
  }

  return static_cast<DayOfWeekType>(day_of_week);
}

//평일 목록만 가져옴.
//TODO 값이 정상적인지 확인해야함.
Array<DayOfWeekType> Locale::GetWeekdays() const
{
//TODO optimize 현재 구현이 안된 기능이기 때문에, 어짜피 실패함.
#if !FUN_NO_SYSTEM_LOCALE
  if (impl_->data == GetSystemData()) {
    Variant result = GetSystemLocale()->Query(SystemLocale::Weekdays, Variant());
    if (!result.IsNull()) {
      //TODO enum 배열을 variant에서 처리할 수 없으므로, 상당히 비효율적임.
            //return static_cast<Array<DayOfWeekType> >(result.Value<Array<DayOfWeekType>>());
      Array<int32> int_array = result.Value<Array<int32>>();
      Array<DayOfWeekType> dow_array;
      dow_array.Reserve(int_array.Count());
      for (const auto& i : int_array) {
        dow_array.Add((DayOfWeekType)i);
      }
      return dow_array;
    }
  }
#endif

  Array<DayOfWeekType> weekdays;
  const int32 weekend_start = impl_->data->weekend_start  == 7 ? 0 : impl_->data->weekend_start;
  const int32 weekend_end   = impl_->data->weekend_end    == 7 ? 0 : impl_->data->weekend_end;
  for (int32 day = (int32)DayOfWeekType::Sunday; day <= (int32)DayOfWeekType::Saturday; ++day) {
    if ((weekend_end >= weekend_start && (day < weekend_start || day > weekend_end)) ||
        (weekend_end <  weekend_start && (day > weekend_end   && day < weekend_start))) {
      weekdays.Add(static_cast<DayOfWeekType>(day));
    }
  }

  return weekdays;
}

UString Locale::GetAMText() const
{
#if !FUN_NO_SYSTEM_LOCALE
  if (impl_->data == GetSystemData()) {
    Variant result = GetSystemLocale()->Query(SystemLocale::AMText, Variant());
    if (!result.IsNull()) {
      return result.Value<UString>();
    }
  }
#endif

  return GetLocaleData(_AM_DATA + impl_->data->am_idx, impl_->data->am_len);
}

UString Locale::GetPMText() const
{
#if !FUN_NO_SYSTEM_LOCALE
  if (impl_->data == GetSystemData()) {
    Variant result = GetSystemLocale()->Query(SystemLocale::PMText, Variant());
    if (!result.IsNull()) {
      return result.Value<UString>();
    }
  }
#endif

  return GetLocaleData(_PM_DATA + impl_->data->pm_idx, impl_->data->pm_len);
}

static bool TimeFormatContainsAP(const UString& format)
{
  int32 i = 0;
  while (i < format.Len()) {
    if (format[i] == '\'') {
      ReadEscapedFormatString(format, &i);
      continue;
    }

    // A or a
    if (format[i] == 'A' || format[i] == 'a') {
      return true;
    }

    ++i;
  }

  return false;
}

UString LocaleImpl::DateTimeToString( const UString& format,
                                      const DateTime* date_time,
                                      const Date* date_part,
                                      const Time* time_part,
                                      const Locale* q) const
{
  Date date;
  Time time;
  bool format_date = false;
  bool format_time = false;
  if (date_time) {
    date = date_time->GetDate();
    time = date_time->GetTime();
    format_date = true;
    format_time = true;
  }
  else if (date_part) {
    date = *date_part;
    format_date = true;
  }
  else if (time_part) {
    time = *time_part;
    format_time = true;
  }
  else {
    return UString();
  }

  UString result;

  int32 i = 0;
  while (i < format.Len()) {
    if (format[i] == '\'') {
      result.Append(ReadEscapedFormatString(format, &i));
      continue;
    }

    const UNICHAR c = format[i];
    int32 repeat = RepeatCount(format.Mid(i));
    bool used = false;
    if (format_date) {
      switch (c) {
      case 'y': // yyyy or yy
        used = true;
        if (repeat >= 4) {
          repeat = 4;
        }
        else if (repeat >= 2) {
          repeat = 2;
        }

        switch (repeat) {
        case 4: {
          const int32 year = date.Year();
          const int32 len = (year < 0) ? 5 : 4;
          result.Append(data->Int64ToString(year, -1, 10, len, LocaleData::ZeroPadded));
          break;
        }
        case 2:
          result.Append(data->Int64ToString(date.Year() % 100, -1, 10, 2, LocaleData::ZeroPadded));
          break;
        default:
          repeat = 1;
          result.Append(c);
          break;
        }
        break;

      case 'M':
        used = true;
        repeat = MathBase::Min(repeat, 4);
        switch (repeat) {
        case 1: result.Append(data->Int64ToString(date.Month())); break;
        case 2: result.Append(data->Int64ToString(date.Month(), -1, 10, 2, LocaleData::ZeroPadded)); break;
        case 3: result.Append(q->GetMonthName(date.Month(), Locale::ShortFormat)); break;
        case 4: result.Append(q->GetMonthName(date.Month(), Locale::LongFormat)); break;
        }
        break;

      case 'd':
        used = true;
        repeat = MathBase::Min(repeat, 4);
        switch (repeat) {
        case 1: result.Append(data->Int64ToString(date.Day())); break;
        case 2: result.Append(data->Int64ToString(date.Day(), -1, 10, 2, LocaleData::ZeroPadded)); break;
        case 3: result.Append(q->GetDayName(DateTime(Date(date.Year(), date.Month(), date.Day())).DayOfWeek(), Locale::ShortFormat)); break;
        case 4: result.Append(q->GetDayName(DateTime(Date(date.Year(), date.Month(), date.Day())).DayOfWeek(), Locale::LongFormat)); break;
        }
        break;

      default:
        break;
      }
    }

    if (!used && format_time) {
      switch (c) {
      case 'h': {
        used = true;
        repeat = MathBase::Min(repeat, 2);
        int32 hour = time.Hour();
        if (TimeFormatContainsAP(format)) {
          if (hour > 12) {
            hour -= 12;
          }
          else if (hour == 0) {
            hour = 12;
          }
        }

        switch (repeat) {
        case 1: result.Append(data->Int64ToString(hour)); break;
        case 2: result.Append(data->Int64ToString(hour, -1, 10, 2, LocaleData::ZeroPadded)); break;
        }
        break;
      }

      case 'H':
        used = true;
        repeat = MathBase::Min(repeat, 2);
        switch (repeat) {
        case 1: result.Append(data->Int64ToString(time.Hour())); break;
        case 2: result.Append(data->Int64ToString(time.Hour(), -1, 10, 2, LocaleData::ZeroPadded)); break;
        }
        break;

      case 'm':
        used = true;
        repeat = MathBase::Min(repeat, 2);
        switch (repeat) {
        case 1: result.Append(data->Int64ToString(time.Minute())); break;
        case 2: result.Append(data->Int64ToString(time.Minute(), -1, 10, 2, LocaleData::ZeroPadded)); break;
        }
        break;

      case 's':
        used = true;
        repeat = MathBase::Min(repeat, 2);
        switch (repeat) {
        case 1: result.Append(data->Int64ToString(time.Second())); break;
        case 2: result.Append(data->Int64ToString(time.Second(), -1, 10, 2, LocaleData::ZeroPadded)); break;
        }
        break;

      // 1/10  1/100  1/1000 ... sec.
      //TODO f ff fff ffff fffff ffffff
      //TODO f FF FFF FFFF FFFFF FFFFFF

      //TODO g gg 서기 또는 연대

      // ap / AP 를 통해 오전/오후를 표시하고 있지만,
      // C# 에서는 tt 라고 표기함.
      // t는 첫글자
      // tt는 온전한 오전/오후

      case 'a':
        used = true;
        if (i + 1 < format.Len() && format[i + 1] == 'p') {
          repeat = 2;
        }
        else {
          repeat = 1;
        }
        result.Append(time.Hour() < 12 ? q->GetAMText().ToLower() : q->GetPMText().ToLower());
        break;

      case 'A':
        used = true;
        if (i + 1 < format.Len() && format[i + 1] == 'P') {
          repeat = 2;
        }
        else {
          repeat = 1;
        }
        result.Append(time.Hour() < 12 ? q->GetAMText().ToUpper() : q->GetPMText().ToUpper());
        break;

      //
      case 'z':
        used = true;
        if (repeat >= 3) {
          repeat = 3;
        }
        else {
          repeat = 1;
        }
        switch (repeat) {
        case 1: result.Append(data->Int64ToString(time.Millisecond())); break;
        case 3: result.Append(data->Int64ToString(time.Millisecond(), -1, 10, 3, LocaleData::ZeroPadded)); break;
        }
        break;

      // C#에서는 "K"
      case 't':
        used = true;
        repeat = 1;
        // If we have a DateTime use the time spec otherwise use the current system tzname
        if (format_date) {
          result.Append(UTF8_TO_UNICHAR(date_time->GetTimeZoneAbbreviation().c_str()));
        }
        else {
          result.Append(UTF8_TO_UNICHAR(DateTime::Now().GetTimeZoneAbbreviation().c_str()));
        }
        break;

      default:
        break;
      }
    }

    if (!used) {
      result.Append(UString(repeat, c));
    }

    i += repeat;
  }

  return result;
}

Locale::MeasurementSystem LocaleImpl::GetMeasurementSystem() const
{
  //TODO optimize
  for (int32 i = 0; i < _IMPERIAL_MEASUREMENT_SYSTEM_COUNT; ++i) {
    if (_IMPERIAL_MEASUREMENT_SYSTEMS[i].language_id == data->language_id &&
        _IMPERIAL_MEASUREMENT_SYSTEMS[i].country_id  == data->country_id) {
      return _IMPERIAL_MEASUREMENT_SYSTEMS[i].system;
    }
  }

  return Locale::MetricSystem;
}

Locale::MeasurementSystem Locale::GetMeasurementSystem() const
{
#if !FUN_NO_SYSTEM_LOCALE
  if (impl_->data == GetSystemData()) {
    Variant result = GetSystemLocale()->Query(SystemLocale::MeasurementSystem, Variant());
    if (!result.IsNull()) {
      return MeasurementSystem(result.Value<int32>());
    }
  }
#endif

  return impl_->GetMeasurementSystem();
}

Locale::LayoutDirection Locale::GetTextDirection() const
{
  switch (GetScript()) {
  case Locale::AdlamScript:
  case Locale::ArabicScript:
  case Locale::AvestanScript:
  case Locale::CypriotScript:
  case Locale::HatranScript:
  case Locale::HebrewScript:
  case Locale::ImperialAramaicScript:
  case Locale::InscriptionalPahlaviScript:
  case Locale::InscriptionalParthianScript:
  case Locale::KharoshthiScript:
  case Locale::LydianScript:
  case Locale::MandaeanScript:
  case Locale::ManichaeanScript:
  case Locale::MendeKikakuiScript:
  case Locale::MeroiticCursiveScript:
  case Locale::MeroiticScript:
  case Locale::NabataeanScript:
  case Locale::NkoScript:
  case Locale::OldHungarianScript:
  case Locale::OldNorthArabianScript:
  case Locale::OldSouthArabianScript:
  case Locale::OrkhonScript:
  case Locale::PalmyreneScript:
  case Locale::PhoenicianScript:
  case Locale::PsalterPahlaviScript:
  case Locale::SamaritanScript:
  case Locale::SyriacScript:
  case Locale::ThaanaScript:
    return RightToLeft;
  default:
    break;
  }

  return LeftToRight;
}

UString Locale::ToUpper(const UString& str) const
{
//#if FUN_WITH_ICU
//  bool ok = true;
//  UString result = Icu::ToUpper(impl_->Bcp47Name('_'), str, &ok);
//  if (ok) {
//    return result;
//  }
//#endif

  return str.ToUpper();
}

UString Locale::ToLower(const UString& str) const
{
//#if FUN_WITH_ICU
//  bool ok = true;
//  const UString result = Icu::ToLower(impl_->Bcp47Name('_'), str, &ok);
//  if (ok) {
//    return result;
//  }
//#endif

  return str.ToLower();
}


//
// numeric
//

namespace {

static UString _ulltoa(uint64 l, int32 base, const UNICHAR zero)
{
  UNICHAR buff[65];
  UNICHAR* p = buff + 65;

  if (base != 10 || zero == '0') {
    while (l != 0) {
      const int32 c = l % base;
      *(--p) = (c < 10) ? ('0' + c) : (c - 10 + 'a');
      l /= base;
    }
  }
  else {
    while (l != 0) {
      const int32 c = l % base;
      (*--p) = zero + c;
      l /= base;
    }
  }

  return UString(p, 65 - int32(p - buff));
}


static UString _lltoa(int64 l, int32 base, const UNICHAR zero)
{
  if (l < 0) {
    return AsciiString("-") + _ulltoa(-l, base, zero);
  }
  else {
    return _ulltoa(l, base, zero);
  }
}

} // namespace

UString LocaleData::DoubleToString( double value,
                                    int32 precision,
                                    DoubleForm form,
                                    int32 width,
                                    uint32 flags) const
{
  return DoubleToString(zero, plus, minus, exponential, group,
                        decimal, value, precision, form, width, flags);
}

UString LocaleData::DoubleToString( const UNICHAR in_zero,
                                    const UNICHAR plus,
                                    const UNICHAR minus,
                                    const UNICHAR exponential,
                                    const UNICHAR group,
                                    const UNICHAR decimal,
                                    double value,
                                    int32 precision,
                                    DoubleForm form,
                                    int32 width,
                                    uint32 flags)
{
  if (precision != Locale::FloatingPointShortest && precision < 0) {
    precision = 6;
  }

  if (width < 0) {
    width = 0;
  }

  bool is_negative = false;
  UString num_str;

  int32 dec_pt;
  int32 buf_len = 1;
  if (precision == Locale::FloatingPointShortest) {
    buf_len += DoubleMaxSignificant;
  }
  else if (form == DFDecimal) { // optimize for numbers between -512k and 512k
    buf_len += ((value > (1 << 19) || value < -(1 << 19)) ? DoubleMaxDigitsBeforeDecimal : 6) + precision;
  }
  else { // Add extra digit due to different interpretations of precision. Also, "nan" has to fit.
    buf_len += MathBase::Max(2, precision) + 1;
  }

  Array<char, InlineAllocator<65>> buf;
  buf.AddUninitialized(buf_len);
  int32 length;
  DoubleToAscii(value, form, precision, buf.MutableData(), buf_len, is_negative, length, dec_pt);

  if (CStringTraitsA::Strncmp(buf.ConstData(), "inf", 3) == 0 ||
      CStringTraitsA::Strncmp(buf.ConstData(), "nan", 3) == 0) {
    num_str = UString::FromAscii(buf.ConstData(), length);
  }
  else {
    // Handle normal numbers
    UString digits = UString::FromAscii(buf.ConstData(), length);

    if (in_zero != '0') {
      UNICHAR z = in_zero - '0';
      for (int32 i = 0; i < digits.Len(); ++i) {
        const_cast<UNICHAR*>(*digits)[i] += z;
      }
    }

    const bool always_show_dec_pt = !!(flags & ForcePoint);
    switch (form) {
      case DFExponent:
        num_str = ExponentForm( in_zero,
                                decimal,
                                exponential,
                                group,
                                plus,
                                minus,
                                digits,
                                dec_pt,
                                precision,
                                PMDecimalDigits,
                                always_show_dec_pt,
                                !!(flags & ZeroPadExponent));
        break;

      case DFDecimal:
        num_str = DecimalForm(in_zero,
                              decimal,
                              group,
                              digits,
                              dec_pt,
                              precision,
                              PMDecimalDigits,
                              always_show_dec_pt,
                              !!(flags & ThousandsGroup));
        break;

      case DFSignificantDigits: {
        const PrecisionMode mode = (flags & AddTrailingZeroes) ? PMSignificantDigits : PMChopTrailingZeros;

        int32 cutoff = precision < 0 ? 6 : precision;
        // Find out which representation is shorter
        if (precision == Locale::FloatingPointShortest && dec_pt > 0) {
          cutoff = digits.Len() + 4; // 'e', '+'/'-', one digit Exponent
          if (dec_pt <= 10) {
            ++cutoff;
          }
          else {
            cutoff += dec_pt > 100 ? 2 : 1;
          }
          if (!always_show_dec_pt && digits.Len() > dec_pt) {
            ++cutoff; // dec_pt shown in Exponent form, but not in decimal form
          }
        }

        if (dec_pt != digits.Len() && (dec_pt <= -4 || dec_pt > cutoff)) {
          num_str = ExponentForm( in_zero,
                                  decimal,
                                  exponential,
                                  group,
                                  plus,
                                  minus,
                                  digits,
                                  dec_pt,
                                  precision,
                                  mode,
                                  always_show_dec_pt,
                                  !!(flags & ZeroPadExponent));
        }
        else {
          num_str = DecimalForm(in_zero,
                                decimal,
                                group, digits,
                                dec_pt,
                                precision,
                                mode,
                                always_show_dec_pt,
                                !!(flags & ThousandsGroup));
        }
        break;
      }
    }

    if (IsZero(value)) {
      is_negative = false;
    }

    // pad with zeros. LeftAdjusted overrides this flag). Also, we don't
    // pad special numbers
    if (flags & LocaleData::ZeroPadded && !(flags & LocaleData::LeftAdjusted)) {
      int32 pad_chars_count = width - num_str.Len();
      // leave space for the sign
      if (is_negative || flags & LocaleData::AlwaysShowSign || flags & LocaleData::BlankBeforePositive) {
        --pad_chars_count;
      }

      for (int32 i = 0; i < pad_chars_count; ++i) {
        num_str.Prepend(in_zero);
      }
    }
  }

  // add sign
  if (is_negative) {
    num_str.Prepend(minus);
  }
  else if (flags & LocaleData::AlwaysShowSign) {
    num_str.Prepend(plus);
  }
  else if (flags & LocaleData::BlankBeforePositive) {
    num_str.Prepend(' ');
  }

  if (flags & LocaleData::CapitalEorX) {
    num_str.MakeUpper();
  }

  return num_str;
}

UString LocaleData::Int64ToString(int64 value,
                                  int32 precision,
                                  int32 base,
                                  int32 width,
                                  uint32 flags) const
{
  return Int64ToString(zero, group, plus, minus, value, precision, base, width, flags);
}

UString LocaleData::Int64ToString(const UNICHAR zero,
                                  const UNICHAR group,
                                  const UNICHAR plus,
                                  const UNICHAR minus,
                                  int64 value,
                                  int32 precision,
                                  int32 base,
                                  int32 width,
                                  uint32 flags)
{
  bool precision_not_specified = false;
  if (precision == -1) {
    precision_not_specified = true;
    precision = 1;
  }

  bool is_negative = value < 0;
  if (base != 10) {
    // these are not supported by sprintf for octal and hex
    flags &= ~AlwaysShowSign;
    flags &= ~BlankBeforePositive;
    is_negative = false; // neither are is_negative numbers
  }

  UString num_str;
  if (base == 10) {
    num_str = _lltoa(value, base, zero);
  }
  else {
    num_str = _ulltoa(value, base, zero);
  }

  uint32 thousand_sep_count = 0;
  if (flags & ThousandsGroup && base == 10) {
    for (int32 i = num_str.Len() - 3; i > 0; i -= 3) {
      num_str.Insert(i, group);
      ++thousand_sep_count;
    }
  }

  for (int32 i = num_str.Len()/* - thousand_sep_count*/; i < precision; ++i) {
    num_str.Prepend(base == 10 ? zero : '0');
  }

  if ((flags & ShowBase) && base == 8 && (num_str.IsEmpty() || num_str[0] != UTEXT('0'))) {
    num_str.Prepend(UTEXT('0'));
  }

  // LeftAdjusted overrides this flag ZeroPadded. sprintf only padds
  // when precision is not specified in the format UString
  const bool zero_padded = flags & ZeroPadded && !(flags & LeftAdjusted) && precision_not_specified;
  if (zero_padded) {
    int32 pad_chars_count = width - num_str.Len();

    // leave space for the sign
    if (is_negative || flags & AlwaysShowSign || flags & BlankBeforePositive) {
      --pad_chars_count;
    }

    // leave space for optional '0x' in hex form
    if (base == 16 && (flags & ShowBase)) {
      pad_chars_count -= 2;
    }
    // leave space for optional '0b' in binary form
    else if (base == 2 && (flags & ShowBase)) {
      pad_chars_count -= 2;
    }

    for (int32 i = 0; i < pad_chars_count; ++i) {
      num_str.Prepend(base == 10 ? zero : UTEXT('0'));
    }
  }

  if (flags & CapitalEorX) {
    num_str.MakeUpper();
  }

  if (base == 16 && (flags & ShowBase)) {
    num_str.Prepend(AsciiString(flags & UppercaseBase ? "0X" : "0x"));
  }
  if (base == 2 && (flags & ShowBase)) {
    num_str.Prepend(AsciiString(flags & UppercaseBase ? "0B" : "0b"));
  }

  // add sign
  if (is_negative) {
    num_str.Prepend(minus);
  }
  else if (flags & AlwaysShowSign) {
    num_str.Prepend(plus);
  }
  else if (flags & BlankBeforePositive) {
    num_str.Prepend(' ');
  }

  return num_str;
}

UString LocaleData::UInt64ToString( uint64 value,
                                    int32 precision,
                                    int32 base,
                                    int32 width,
                                    uint32 flags) const
{
  return UInt64ToString(zero, group, plus, value, precision, base, width, flags);
}

UString LocaleData::UInt64ToString( const UNICHAR zero,
                                    const UNICHAR group,
                                    const UNICHAR plus,
                                    uint64 value,
                                    int32 precision,
                                    int32 base,
                                    int32 width,
                                    uint32 flags)
{
  const UNICHAR result_zero = base == 10 ? zero : UTEXT('0');
  UString num_str = value ? _ulltoa(value, base, zero) : UString(result_zero);

  bool precision_not_specified = false;
  if (precision == -1) {
    if (flags == NoFlags) {
      return num_str; // fast-path: nothing below applies, so we're done.
    }

    precision_not_specified = true;
    precision = 1;
  }

  uint32 thousand_sep_count = 0;
  if (flags & ThousandsGroup && base == 10) {
    for (int32 i = num_str.Len() - 3; i > 0; i -= 3) {
      num_str.Insert(i, group);
      ++thousand_sep_count;
    }
  }

  const int32 zero_padding = precision - num_str.Len()/* + thousand_sep_count*/;
  if (zero_padding > 0) {
    num_str.Prepend(UString(zero_padding, result_zero));
  }

  if ((flags & ShowBase) && base == 8 && (num_str.IsEmpty() || num_str[0] != UTEXT('0'))) {
    num_str.Prepend(UTEXT('0'));
  }

  // LeftAdjusted overrides this flag ZeroPadded. sprintf only padds
  // when precision is not specified in the format UString
  const bool zero_padded = flags & ZeroPadded && !(flags & LeftAdjusted) && precision_not_specified;
  if (zero_padded) {
    int32 pad_chars_count = width - num_str.Len();

    // leave space for optional '0x' in hex form
    if (base == 16 && flags & ShowBase) {
      pad_chars_count -= 2;
    }
    // leave space for optional '0b' in binary form
    else if (base == 2 && flags & ShowBase) {
      pad_chars_count -= 2;
    }

    if (pad_chars_count > 0) {
      num_str.Prepend(UString(pad_chars_count, result_zero));
    }
  }

  if (flags & CapitalEorX) {
    num_str.MakeUpper();
  }

  if (base == 16 && flags & ShowBase) {
    num_str.Prepend(AsciiString(flags & UppercaseBase ? "0X" : "0x"));
  }
  else if (base == 2 && flags & ShowBase) {
    num_str.Prepend(AsciiString(flags & UppercaseBase ? "0B" : "0b"));
  }

    // add sign
  if (flags & AlwaysShowSign) {
    num_str.Prepend(plus);
  }
  else if (flags & BlankBeforePositive) {
    num_str.Prepend(' ');
  }

  return num_str;
}

bool LocaleData::NumberToCLocale( const UNICHAR* str,
                                  int32 len,
                                  Locale::NumberOptions number_options,
                                  AsciiBuffer& result) const
{
  result.Clear();

  const UNICHAR* str_chars = str;
  int32 idx = 0;

  // Skip whitespace
  while (idx < len && CharTraitsU::IsWhitespace(str_chars[idx])) {
    ++idx;
  }

  if (idx == len) {
    return false;
  }

  // Check trailing whitespace
  for (; idx < len; --len) {
    if (!CharTraitsU::IsWhitespace(str_chars[len - 1])) {
      break;
    }
  }

  int32 group_count = 0; // counts number of group chars
  int32 dec_pt_idx = -1;
  int32 last_separator_idx = -1;
  int32 start_of_digits_idx = -1;
  int32 exponent_idx = -1;

  while (idx < len) {
    const UNICHAR in = str_chars[idx];

    char out = (char)DigitToCLocale(in);
    if (out == 0) {
      if (in == this->list) {
        out = ';';
      }
      else if (in == this->percent) {
        out = '%';
      }
      // for handling base-x numbers
      else if (in >= 'A' && in <= 'Z') {
        out = (char)CharTraitsU::ToLower(in);
      }
      else if (in >= 'a' && in <= 'z') {
        out = (char)in;
      }
      else {
        break;
      }
    }
    else if (out == '.') {
      // Fail if more than one decimal point or point after e
      if (dec_pt_idx != -1 || exponent_idx != -1) {
        return false;
      }
      dec_pt_idx = idx;
    }
    else if (out == 'e' || out == 'E') {
      exponent_idx = idx;
    }

    if (number_options & Locale::RejectLeadingZeroInExponent) {
      if (exponent_idx != -1 && out == '0' && idx < len - 1) {
        // After the exponent there can only be '+', '-' or digits.
        // If we find a '0' directly after some non-digit, then that is a leading zero.
        if (result.Last() < '0' || result.Last() > '9') {
          return false;
        }
      }
    }

    if (number_options & Locale::RejectTrailingZeroesAfterDot) {
      // If we've seen a decimal point and the Last character after the exponent is 0, then
      // that is a trailing zero.
      if (dec_pt_idx >= 0 && idx == exponent_idx && result.Last() == '0') {
        return false;
      }
    }

    if (!(number_options & Locale::RejectGroupSeparator)) {
      if (start_of_digits_idx == -1 && out >= '0' && out <= '9') {
        start_of_digits_idx = idx;
      }
      else if (out == ',') {
        // Don't allow group chars after the decimal point or exponent
        if (dec_pt_idx != -1 || exponent_idx != -1) {
          return false;
        }

        // check distance from the Last separator or from the beginning of the digits
        // ### FIXME: Some locales allow other groupings! See https://en.wikipedia.org/wiki/Thousands_separator
        if (last_separator_idx != -1 && idx - last_separator_idx != 4) {
          return false;
        }

        if (last_separator_idx == -1 && (start_of_digits_idx == -1 || idx - start_of_digits_idx > 3)) {
          return false;
        }

        last_separator_idx = idx;
        ++group_count;

        // don't add the group separator
        ++idx;
        continue;
      }
      else if (out == '.' || out == 'e' || out == 'E') {
        // check distance from the Last separator
        // ### FIXME: Some locales allow other groupings! See https://en.wikipedia.org/wiki/Thousands_separator
        if (last_separator_idx != -1 && idx - last_separator_idx != 4) {
          return false;
        }

        // stop processing separators
        last_separator_idx = -1;
      }
    }

    result.Add(out);

    ++idx;
  }

  if (!(number_options & Locale::RejectGroupSeparator)) {
    // group separator post-processing
    // did we end in a separator?
    if (last_separator_idx + 1 == idx) {
      return false;
    }

    // were there enough digits since the Last separator?
    if (last_separator_idx != -1 && idx - last_separator_idx != 4) {
      return false;
    }
  }

  if (number_options & Locale::RejectTrailingZeroesAfterDot) {
    // in decimal form, the Last character can be a trailing zero if we've seen a decpt.
    if (dec_pt_idx != -1 && exponent_idx == -1 && result.Last() == '0') {
      return false;
    }
  }

  result.Add('\0'); // nul-term

  return idx == len;
}

bool LocaleData::ValidateChars( const UNICHAR* str,
                                int32 len,
                                NumberMode num_mode,
                                UString& buf,
                                int32 dec_digits,
                                Locale::NumberOptions number_options) const
{
  if (len < 0) {
    len = CStringTraitsU::Strlen(str);
  }

  buf.Clear(len);

  const bool is_scientific = num_mode == DoubleScientificMode;
  bool last_was_E = false;
  bool last_was_digit = false;
  int32 E_count = 0;
  int32 dec_pt_count = 0;
  bool is_decimal = false;
  int32 dec_digit_count = 0;

  for (int32 i = 0; i < len; ++i) {
    const UNICHAR ch = DigitToCLocale(str[i]);

    if (CharTraitsU::IsDigit(ch)) {
      if (num_mode != IntegerMode) {
        // If a double has too many digits after decpt, it shall be invalid.
        if (is_decimal && dec_digits != -1 && dec_digits < ++dec_digit_count) {
          return false;
        }
      }

      // The only non-digit character after the 'e' can be '+' or '-'.
      // If a zero is directly after that, then the exponent is zero-padded.
      if ((number_options & Locale::RejectLeadingZeroInExponent) && ch == '0' && E_count > 0 && !last_was_digit) {
        return false;
      }

      last_was_digit = true;
    }
    else {
      switch (ch) {
      case '.':
        if (num_mode == IntegerMode) {
          // If an integer has a decimal point, it shall be Invalid.
          return false;
        }
        else {
          // If a double has more than one decimal point, it shall be Invalid.
          if (++dec_pt_count > 1) {
            return false;
          }
#if 0
          // If a double with no decimal digits has a decimal point, it shall be Invalid.
          if (dec_digits == 0) {
            return false;
          }
#endif    // On second thoughts, it shall be Valid.

          is_decimal = true;
        }
        break;

      case '+':
      case '-':
        if (is_scientific) {
          // If a is_scientific has a sign that's not at the beginning or after
          // an 'e', it shall be Invalid.
          if (i != 0 && !last_was_E) {
            return false;
          }
        }
        else {
          // If a non-is_scientific has a sign that's not at the beginning,
          // it shall be Invalid.
          if (i != 0) {
            return false;
          }
        }
        break;

      case ',':
        //it can only be placed after a digit which is before the decimal point
        if ((number_options & Locale::RejectGroupSeparator) || !last_was_digit || dec_pt_count > 0) {
          return false;
        }
        break;

      case 'e':
        if (is_scientific) {
          // If a is_scientific has more than one 'e', it shall be Invalid.
          if (++E_count > 1) {
            return false;
          }
          is_decimal = false;
        }
        else {
          // If a non-is_scientific has an 'e', it shall be Invalid.
          return false;
        }
        break;

      default:
        // If it's not a valid digit, it shall be Invalid.
        return false;
      }
      last_was_digit = false;
    }

    last_was_E = ch == 'e';
    if (ch != ',') {
      buf.Append(ch);
    }
  }

  return true;
}

double LocaleData::ByteStringToDouble(const char* str, int32 len, bool* ok)
{
  if (ok) {
    *ok = true;
  }

  if (len < 0) {
    len = CStringTraitsA::Strlen(str);
  }

  int32 processed = 0;
  bool is_non_null_ok = false;
  double result = AsciiToDouble(str, len, is_non_null_ok, processed);
  if (ok) {
    *ok = is_non_null_ok;
  }
  return result;
}

//TODO 함수 이름을 확실하게 변경해줄 필요가 있음.
int64 LocaleData::ByteStringToInt64(const char* str, int32 len, int32 base, bool* ok)
{
  if (ok) {
    *ok = true;
  }

  if (len < 0) {
    len = CStringTraitsA::Strlen(str);
  }

  //TODO
  fun_check(0);
  //return CStringTraitsA::Strtoi64(str, str + len, nullptr, base, ok);
  return 0;
}

//TODO 함수 이름을 확실하게 변경해줄 필요가 있음.
int64 LocaleData::ByteStringToUInt64(const char* str, int32 len, int32 base, bool* ok)
{
  if (ok) {
    *ok = true;
  }

  if (len < 0) {
    len = CStringTraitsA::Strlen(str);
  }

  //TODO
  fun_check(0);
  //return CStringTraitsA::Strtoui64(str, str + len, nullptr, base, ok);
  return 0;
}

double LocaleData::StringToDouble(const UNICHAR* str,
                                  int32 len,
                                  bool* ok,
                                  Locale::NumberOptions number_options) const
{
  if (ok) {
    *ok = true;
  }

  if (len < 0) {
    len = CStringTraitsU::Strlen(str);
  }

  AsciiBuffer buf;
  if (!NumberToCLocale(str, len, number_options, buf)) {
    if (ok) {
      *ok = false;
    }
    return 0.0;
  }

  int32 processed = 0;
  bool is_non_null_ok = false;
  const double result = AsciiToDouble(buf.ConstData(), buf.Count(), is_non_null_ok, processed);
  if (ok) {
    *ok = is_non_null_ok;
  }
  return result;
}

int64 LocaleData::StringToInt64(const UNICHAR* str,
                                int32 len,
                                int32 base,
                                bool* ok,
                                Locale::NumberOptions number_options) const
{
  if (ok) {
    *ok = true;
  }

  if (len < 0) {
    len = CStringTraitsU::Strlen(str);
  }

  AsciiBuffer buf;
  if (!NumberToCLocale(str, len, number_options, buf)) {
    if (ok) {
      *ok = false;
    }
    return 0;
  }

  return ByteStringToInt64(buf.ConstData(), buf.Count(), base, ok);
}

uint64 LocaleData::StringToUInt64(const UNICHAR* str,
                                  int32 len,
                                  int32 base,
                                  bool* ok,
                                  Locale::NumberOptions number_options) const
{
  if (ok) {
    *ok = true;
  }

  if (len < 0) {
    len = CStringTraitsU::Strlen(str);
  }

  AsciiBuffer buf;
  if (!NumberToCLocale(str, len, number_options, buf)) {
    if (ok) {
      *ok = false;
    }
    return 0;
  }

  return ByteStringToUInt64(buf.ConstData(), buf.Count(), base, ok);
}

//FIXME Variant 타입처리가 너무 민감함. 호환이 가능한 형태라면 가능하게 해주는게 좋을듯...?
UString Locale::GetCurrencySymbol(Locale::CurrencySymbolFormat format) const
{
#if !FUN_NO_SYSTEM_LOCALE
  if (impl_->data == GetSystemData()) {
    //FIXME
    //Variant result = GetSystemLocale()->Query(SystemLocale::CurrencySymbol, format); //FIXME: Locale::CurrencySymbolFormat이 Variant 형태로 넘어가지 않음.
    Variant result = GetSystemLocale()->Query(SystemLocale::CurrencySymbol, (int32)format);
    if (!result.IsNull()) {
      return result.Value<UString>();
    }
  }
#endif

  uint32 idx, len;
  switch (format) {
  case CSFCurrencySymbol:
    idx = impl_->data->currency_symbol_idx;
    len = impl_->data->currency_symbol_len;
    return GetLocaleData(_CURRENCY_SYMBOL_DATA + idx, len);

  //TODO 현재 적용이 안되고 있음.  NativeName만 테이블로 존재하는 상황임.
  case CSFCurrencyEnglishName:
    idx = impl_->data->currency_native_name_idx; //TODO 이름변경
    len = impl_->data->currency_native_name_len; //TODO 이름변경
    return GetLocaleListData(_CURRENCY_NATIVE_NAME_DATA + idx, len, 0);

  case CSFCurrencyNativeName:
    idx = impl_->data->currency_native_name_idx; //TODO 이름변경
    len = impl_->data->currency_native_name_len; //TODO 이름변경
    return GetLocaleListData(_CURRENCY_NATIVE_NAME_DATA + idx, len, 0);

  case CSFCurrencyIsoCode: {
    int32 len = 0;
    const LocaleData* data = impl_->data;
    for (; len < 3; ++len) {
      if (!data->currency_iso_code[len]) {
        break;
      }
    }

    UNICHAR ret[3 + 1];
    for (int32 i = 0; i < len; ++i) {
      ret[i] = data->currency_iso_code[i];
    }
    //null-term 필요없음!
    //ret[len] = '\0'; // null-term
    return UString(ret, len);
  }
  }
  return UString();
}

UString Locale::ToCurrency(int64 value, const UString& symbol) const
{
#if !FUN_NO_SYSTEM_LOCALE
  if (impl_->data == GetSystemData()) {
    SystemLocale::CurrencyToStringArgument arg(value, symbol);
    Variant result = GetSystemLocale()->Query(SystemLocale::CurrencyToString, Variant(arg));
    if (!result.IsNull()) {
      return result.Value<UString>();
    }
  }
#endif

  const LocaleImpl* impl = this->impl_.Get();
  uint8 idx = impl->data->currency_format_idx;
  uint8 len = impl->data->currency_format_len;
  if (impl->data->currency_negative_format_len && value < 0) {
    idx = impl->data->currency_negative_format_idx;
    len = impl->data->currency_negative_format_len;
    value = -value;
  }

  const UString str = ToString(value);
  UString Sym = symbol.IsEmpty() ? GetCurrencySymbol() : symbol;
  if (Sym.IsEmpty()) {
    Sym = GetCurrencySymbol(Locale::CSFCurrencyIsoCode);
  }

  //TODO 포맷을 어떤식으로 지정하는지 체크!
  //%1 -> {0} 치환해야함...
  //UString format = GetLocaleData(_CURRENCY_FORMAT_DATA + idx, len);
  //return format.arg(str, Sym);
  return UString(UTEXT("TODO UString formatting"));
}

UString Locale::ToCurrency(uint64 value, const UString& symbol) const
{
#if !FUN_NO_SYSTEM_LOCALE
  if (impl_->data == GetSystemData()) {
    SystemLocale::CurrencyToStringArgument arg(value, symbol);
    Variant result = GetSystemLocale()->Query(SystemLocale::CurrencyToString, Variant(arg));
    if (!result.IsNull()) {
      return result.Value<UString>();
    }
  }
#endif

  const LocaleData* data = impl_->data;
  const uint8 idx = data->currency_format_idx;
  const uint8 len = data->currency_format_len;
  const UString str = ToString(value);
  UString Sym = symbol.IsEmpty() ? GetCurrencySymbol() : symbol;
  if (Sym.IsEmpty()) {
    Sym = GetCurrencySymbol(Locale::CSFCurrencyIsoCode);
  }

  //TODO 포맷을 어떤식으로 지정하는지 체크!
  //UString format = GetLocaleData(_CURRENCY_FORMAT_DATA + idx, len);
  //return format.arg(str, Sym);
  return UString(UTEXT("TODO UString formatting"));
}

//TODO replace 해주어야함.
// %1 -> {0}
// %2 -> {1}
// %3 -> {2}
UString Locale::ToCurrency(double value, const UString& symbol, int32 precision) const
{
#if !FUN_NO_SYSTEM_LOCALE
  if (impl_->data == GetSystemData()) {
    SystemLocale::CurrencyToStringArgument arg(value, symbol);
    Variant result = GetSystemLocale()->Query(SystemLocale::CurrencyToString, Variant(arg));
    if (!result.IsNull()) {
      return result.Value<UString>();
    }
  }
#endif

  const LocaleData* data = impl_->data;
  uint8 idx = data->currency_format_idx;
  uint8 len = data->currency_format_len;
  if (data->currency_negative_format_len && value < 0) {
    idx = data->currency_negative_format_idx;
    len = data->currency_negative_format_len;
    value = -value;
  }

  UString str = ToString(value, 'f', precision == -1 ? impl_->data->currency_digits : precision);
  UString Sym = symbol.IsEmpty() ? GetCurrencySymbol() : symbol;
  if (Sym.IsEmpty()) {
    Sym = GetCurrencySymbol(Locale::CSFCurrencyIsoCode);
  }

  //TODO 포맷을 어떤식으로 지정하는지 체크!
  //UString format = GetLocaleData(_CURRENCY_FORMAT_DATA + idx, len);
  //return format.arg(str, Sym);
  return UString(UTEXT("TODO UString formatting"));
}

Array<UString> Locale::GetUiLanguages() const
{
#if !FUN_NO_SYSTEM_LOCALE
  if (impl_->data == GetSystemData()) {
    Variant result = GetSystemLocale()->Query(SystemLocale::UILanguages, Variant());
    if (!result.IsNull()) {
      Array<UString> value = result.Value<Array<UString>>();
      if (!value.IsEmpty()) {
        return value;
      }
    }
  }
#endif

  LocaleId id = LocaleId::FromIds(impl_->data->language_id, impl_->data->script_id, impl_->data->country_id);
  const LocaleId max = id.WithLikelySubtagsAdded();
  const LocaleId min = max.WithLikelySubtagsRemoved();

  Array<UString> ui_languages;
  ui_languages.Add(min.GetName());
  if (id.script_id) {
    id.script_id = 0;
    if (id != min && id.WithLikelySubtagsAdded() == max) {
      ui_languages.Add(id.GetName());
    }
  }

  if (max != min && max != id) {
    ui_languages.Add(max.GetName());
  }

  return ui_languages;
}

UString Locale::GetEnglishLanguageName() const
{
#if !FUN_NO_SYSTEM_LOCALE
  if (impl_->data == GetSystemData()) {
    Variant result = GetSystemLocale()->Query(SystemLocale::EnglishLanguageName, Variant());
    if (!result.IsNull()) {
      return result.Value<UString>();
    }
  }
#endif

  //TODO Latin1String으로 해야하지 않으려나...
  return AsciiString(_LANGUAGE_NAME_LIST + _LANGUAGE_NAME_INDEX[impl_->GetLanguageId()]);
}

UString Locale::GetNativeLanguageName() const
{
#if !FUN_NO_SYSTEM_LOCALE
  if (impl_->data == GetSystemData()) {
    Variant result = GetSystemLocale()->Query(SystemLocale::NativeLanguageName, Variant());
    if (!result.IsNull()) {
      return result.Value<UString>();
    }
  }
#endif

  return GetLocaleData(_ENDONYMS_DATA + impl_->data->language_endonym_idx, impl_->data->language_endonym_size);
}

UString Locale::GetNativeCountryName() const
{
#if !FUN_NO_SYSTEM_LOCALE
  if (impl_->data == GetSystemData()) {
    Variant result = GetSystemLocale()->Query(SystemLocale::NativeCountryName, Variant());
    if (!result.IsNull()) {
      return result.Value<UString>();
    }
  }
#endif

  return GetLocaleData(_ENDONYMS_DATA + impl_->data->country_endonym_idx, impl_->data->country_endonym_size);
}

uint32 HashOf(const Locale& locale)
{
  return HashOf(locale.GetName()) ^ uint32(locale.impl_->number_options);
}

Archive& operator & (Archive& ar, Locale& locale)
{
  if (ar.IsLoading()) {
    UString name;
    ar & name;

    locale = Locale(name);

    uint32 number_options;
    ar & number_options;

    locale.SetNumberOptions((Locale::NumberOptions)number_options);
  }
  else {
    UString name = locale.GetName();
    ar & name;

    uint32 number_options = uint32(locale.GetNumberOptions());
    ar & number_options;
  }
  return ar;
}

} // namespace fun
