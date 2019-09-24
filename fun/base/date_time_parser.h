//private에 위치해야함. 외부에서 직접적으로는 사용되지 않음.
#pragma once

#include "fun/base/base.h"
#include "fun/base/variant.h"
#include "fun/base/locale/locale.h"
#include "fun/base/date_time.h"

namespace fun {

#define DATETIMEEDIT_TIME_MIN               Time(0, 0, 0, 0)
#define DATETIMEEDIT_TIME_MAX               Time(23, 59, 59, 999)
#define DATETIMEEDIT_DATE_MIN               Date(100, 1, 1)
//#define DATETIMEEDIT_COMPAT_DATE_MIN      Date(1752, 9, 14)
#define DATETIMEEDIT_DATE_MAX               Date(7999, 12, 31)
#define DATETIMEEDIT_DATETIME_MIN           DateTime(DATETIMEEDIT_DATE_MIN, DATETIMEEDIT_TIME_MIN)
//#define DATETIMEEDIT_COMPAT_DATETIME_MIN  DateTime(DATETIMEEDIT_COMPAT_DATE_MIN, DATETIMEEDIT_TIME_MIN)
#define DATETIMEEDIT_DATETIME_MAX           DateTime(DATETIMEEDIT_DATE_MAX, DATETIMEEDIT_TIME_MAX)
//#define DATETIMEEDIT_DATE_INITIAL         Date(2000, 1, 1)

class DateTimeParser {
 public:
  enum Context {
    CONTEXT_FromString,
    CONTEXT_DateTimeEdit
  };

  DateTimeParser(/*VariantTypes*/int32 type, Context context)
    : current_section_index_(-1),
      display_(NoSection),
      cached_day_(-1),
      parser_type_(type),
      fix_day_(false),
      spec_(TimeSpec::Local),
      context_(context) {
    default_locale_ = Locale::System();

    first_.type = FirstSection;
    first_.pos = -1;
    first_.count = -1;
    first_.zeroes_added = 0;

    last_.type = LastSection;
    last_.pos = -1;
    last_.count = -1;
    last_.zeroes_added = 0;

    none_.type = NoSection;
    none_.pos = -1;
    none_.count = -1;
    none_.zeroes_added = 0;
  }
  virtual ~DateTimeParser();

  enum Section {
    NoSection     = 0x00000,
    AmPmSection   = 0x00001,
    MSecSection   = 0x00002,
    SecondSection = 0x00004,
    MinuteSection = 0x00008,
    Hour12Section   = 0x00010,
    Hour24Section   = 0x00020,
    HourSectionMask = (Hour12Section | Hour24Section),
    TimeSectionMask = (MSecSection | SecondSection | MinuteSection |
               HourSectionMask | AmPmSection),

    DaySection         = 0x00100,
    MonthSection       = 0x00200,
    YearSection        = 0x00400,
    YearSection2Digits = 0x00800,
    YearSectionMask = YearSection | YearSection2Digits,
    DayOfWeekSectionShort = 0x01000,
    DayOfWeekSectionLong  = 0x02000,
    DayOfWeekSectionMask = DayOfWeekSectionShort | DayOfWeekSectionLong,
    DaySectionMask = DaySection | DayOfWeekSectionMask,
    DateSectionMask = DaySectionMask | MonthSection | YearSectionMask,

    Internal             = 0x10000,
    FirstSection         = 0x20000 | Internal,
    LastSection          = 0x40000 | Internal,
    CalendarPopupSection = 0x80000 | Internal,

    NoSectionIndex = -1,
    FirstSectionIndex = -2,
    LastSectionIndex = -3,
    CalendarPopupIndex = -4
  };
  FUN_DECLARE_FLAGS_IN_CLASS(Sections, Section);

  struct SectionNode {
    Section type;
    mutable int32 pos;
    int32 count;
    int32 zeroes_added;

    static String GetName(Section s);
    String GetName() const { return GetName(type); }
    String GetFormat() const;
    int32 GetMaxChange() const;
  };

  enum State { // duplicated from QValidator
    Invalid,
    Intermediate,
    Acceptable
  };

  struct StateNode {
    String input;
    State state;
    bool conflicts;
    DateTime value;

    StateNode() : state(Invalid), conflicts(false) {}
  };

  enum AmPm {
    AmText,
    PmText
  };

  enum Case {
    UpperCase,
    LowerCase
  };

#if !FUN_NO_DATESTRING
  StateNode Parse(String& input, int32& cursor_position, const DateTime& default_value, bool fixup) const;
#endif
  bool ParseFormat(const String& format);
#if !FUN_NO_DATESTRING
  bool FromString(const String& text, Date* Date, Time* time) const;
#endif

  enum FieldInfoFlag {
    Numeric = 0x01,
    FixedWidth = 0x02,
    AllowPartial = 0x04,
    Fraction = 0x08
  };
  FUN_DECLARE_FLAGS_IN_CLASS(FieldInfo, FieldInfoFlag);

  FieldInfo GetFieldInfo(int32 index) const;

  void SetDefaultLocale(const Locale& locale) { default_locale_ = locale; }
  virtual String GetDisplayText() const { return text_; }

 private:
  int32 GetSectionMaxSize(Section section, int32 count) const;
  String GetSectionText(const String& text, int32 section_index, int32 index) const;
  int32 ParseSection(const DateTime& current_value, int32 section_index, String& text, int32& cursor_position, int32 index, DateTimeParser::State& state, int32* used = nullptr) const;
#if !FUN_NO_TEXTDATE
  int32 FindMonth(const String& str, int32 month_start, int32 section_index, String* month_name = nullptr, int32* used = nullptr) const;
  int32 FindDay(const String& str, int32 day_start, int32 section_index, String* day_name = nullptr, int32* used = nullptr) const;
#endif

  enum AmPmFinder {
    Neither = -1,
    AM = 0,
    PM = 1,
    PossibleAM = 2,
    PossiblePM = 3,
    PossibleBoth = 4
  };
  AmPmFinder FindAmPm(String& str, int32 index, int32* used = nullptr) const;
  bool GetPotentialValue(const String& str, int32 min, int32 max, int32 index, const DateTime& current_value, int32 insert) const;

 protected: // for the benefit of QDateTimeEditPrivate
  int32 GetSectionSize(int32 index) const;
  int32 GetSectionMaxSize(int32 index) const;
  int32 GetSectionPos(int32 index) const;
  int32 GetSectionPos(const SectionNode& node) const;

  const SectionNode& GetSectionNode(int32 index) const;
  Section GetSectionType(int32 index) const;
  String GetSectionText(int32 section_index) const;
  int32 GetDigit(const DateTime& dt, int32 index) const;
  bool SetDigit(DateTime& dt, int32 index, int32 new_value) const;

  int32 GetAbsoluteMax(int32 index, const DateTime& value = DateTime()) const;
  int32 GetAbsoluteMin(int32 index) const;

  bool SkipToNextSection(int32 section, const DateTime& current, const String& section_text) const;
  String GetStateName(State state) const;
  virtual DateTime GetMinimum() const;
  virtual DateTime GetMaximum() const;
  virtual int32 GetCursorPosition() const { return -1; }
  virtual String GetAmPmText(AmPm ap, Case casesense) const;
  virtual Locale GetLocale() const { return default_locale_; }

  mutable int32 current_section_index_;
  Sections display_;
  mutable int32 cached_day_;
  mutable String text_;
  Array<SectionNode> section_nodes_;
  SectionNode first_;
  SectionNode last_;
  SectionNode none_;
  SectionNode popup_;
  Array<String> separators_;
  String display_format_;
  Locale default_locale_;
  /*VariantTypes*/int32 parser_type_;
  bool fix_day_;
  TimeSpec spec_; // Spec if used by CDateTimeEdit
  Context context_;
};

bool operator == (const DateTimeParser::SectionNode& node1, const DateTimeParser::SectionNode& node2);

} // namespace fun
