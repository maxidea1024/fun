//TODO Unicode를 사용해야할지 아닐지 결정해야함.
//Unicode 베이스로 파싱 처리하는게 바람직할까?? 아무래도???

#include "fun/base/date_time_parser.h"
#include "fun/base/date_time.h"
#include "fun/base/locale/locale.h"

namespace fun {

DateTimeParser::~DateTimeParser() {
  // NOOP
}

int32 DateTimeParser::GetDigit(const DateTime& dt, int32 index) const {
  if (index < 0 || index >= section_nodes_.Count()) {
#if !FUN_NO_DATESTRING
    //fun_log(Warning, "DateTimeParser::GetDigit() Internal error (%s %d)", qPrintable(t.ToString()), index);
#else
    //fun_log(Warning, "DateTimeParser::GetDigit() Internal error (%d)", index);
#endif
    return -1;
  }

  const SectionNode& node = section_nodes_[index];
  switch (node.type) {
    case Hour24Section:
    case Hour12Section:
      return dt.Hour();

    case MinuteSection:
      return dt.Minute();

    case SecondSection:
      return dt.Second();

    case MSecSection:
      return dt.Millisecond();

    case YearSection2Digits:
    case YearSection:
      return dt.Year();

    case MonthSection:
      return dt.Month();

    case DaySection:
    case DayOfWeekSectionShort:
    case DayOfWeekSectionLong:
      return dt.Day();

    case AmPmSection:
      return dt.Hour() > 11 ? 1 : 0;

    // not reachable
    default: return -1; break;
  }
}

bool DateTimeParser::SetDigit(DateTime& dt, int32 index, int32 new_value) const {
  if (index < 0 || index >= section_nodes_.Count()) {
#if !FUN_NO_DATESTRING
    //fun_log(Warning, "DateTimeParser::SetDigit() Internal error (%s %d %d)", qPrintable(dt.ToString()), index, new_value);
#else
    //fun_log(Warning, "DateTimeParser::SetDigit() Internal error (%d %d)", index, new_value);
#endif
    return false;
  }

  const SectionNode& node = section_nodes_[index];

  int32 year = dt.Year();
  int32 month = dt.Month();
  int32 day = dt.Day();
  int32 hour = dt.Hour();
  int32 minute = dt.Minute();
  int32 second = dt.Second();
  int32 millisecond = dt.Millisecond();

  switch (node.type) {
    case Hour24Section:
    case Hour12Section:
      hour = new_value;
      break;

    case MinuteSection:
      minute = new_value;
      break;

    case SecondSection:
      second = new_value;
      break;

    case MSecSection:
      millisecond = new_value;
      break;

    case YearSection2Digits:
    case YearSection:
      year = new_value;
      break;

    case MonthSection:
      month = new_value;
      break;

    case DaySection:
    case DayOfWeekSectionShort:
    case DayOfWeekSectionLong:
      if (new_value > 31) {
        // have to keep legacy behavior. setting the
        // Date to 32 should return false. Setting it
        // to 31 for february should return true
        return false;
      }
      day = new_value;
      break;

    case AmPmSection:
      hour = (new_value == 0 ? hour % 12 : (hour % 12) + 12);
      break;

    default:
      //fun_log(Warning, "DateTimeParser::SetDigit() Internal error (%s)", qPrintable(node.GetName()));
      break;
  }

  if (!(node.type & DaySectionMask)) {
    if (day < cached_day_) {
      day = cached_day_;
    }

    const int32 max = Date::DaysInMonth(year, month);
    if (day > max) {
      day = max;
    }
  }

  if (Date::IsValid(year, month, day) && Time::IsValid(hour, minute, second, millisecond)) {
    dt = DateTime(Date(year, month, day), Time(hour, minute, second, millisecond), spec_);
    return true;
  }

  return false;
}

int32 DateTimeParser::GetAbsoluteMax(int32 s, const DateTime& cur) const {
  const SectionNode& node = GetSectionNode(s);
  switch (node.type) {
    case Hour24Section:
    case Hour12Section:
      return 23;

    case MinuteSection:
    case SecondSection:
      return 59;

    case MSecSection:
      return 999;

    case YearSection2Digits:
    case YearSection:
      return 9999;

    case MonthSection:
      return 12;

    case DaySection:
    case DayOfWeekSectionShort:
    case DayOfWeekSectionLong:
      return cur.IsValid() ? Date::DaysInMonth(cur.Year(), cur.Month()) : 31;

    case AmPmSection:
      return 1;

    default: break;
  }

  //fun_log(Warning, "DateTimeParser::GetAbsoluteMax() Internal error (%s)", qPrintable(node.GetName()));
  return -1;
}

int32 DateTimeParser::GetAbsoluteMin(int32 s) const {
  const SectionNode& node = GetSectionNode(s);
  switch (node.type) {
    case Hour24Section:
    case Hour12Section:
    case MinuteSection:
    case SecondSection:
    case MSecSection:
    case YearSection2Digits:
    case YearSection:
      return 0;

    case MonthSection:
      return 1;

    case DaySection:
      return 1;

    //0이 맞는건지 1이 맞는건지?
    case DayOfWeekSectionShort:
    case DayOfWeekSectionLong:
      return 1;

    case AmPmSection:
      return 0;

    default: break;
  }
  //fun_log(Warning, "DateTimeParser::GetAbsoluteMin() Internal error (%s, %0x)", qPrintable(node.GetName()), node.type);
  return -1;
}

const DateTimeParser::SectionNode&
DateTimeParser::GetSectionNode(int32 section_index) const {
  if (section_index < 0) {
    switch (section_index) {
      case FirstSectionIndex:
        return first_;

      case LastSectionIndex:
        return last_;

      case NoSectionIndex:
        return none_;
    }
  } else if (section_index < section_nodes_.Count()) {
    return section_nodes_[section_index];
  }

  //fun_log(Warning, "DateTimeParser::sectionNode() Internal error (%d)", section_index);
  return none_;
}

DateTimeParser::Section
DateTimeParser::GetSectionType(int32 section_index) const {
  return GetSectionNode(section_index).type;
}

int32 DateTimeParser::GetSectionPos(int32 section_index) const {
  return GetSectionPos(GetSectionNode(section_index));
}

int32 DateTimeParser::GetSectionPos(const SectionNode& node) const {
  switch (node.type) {
    case FirstSection:
      return 0;

    case LastSection:
      return GetDisplayText().Len() - 1;

    default:
      break;
  }

  if (node.pos == -1) {
    //fun_log(Warning, "DateTimeParser::sectionPos Internal error (%s)", qPrintable(node.GetName()));
    return -1;
  }

  return node.pos;
}

static String Unquote(const String& str) {
  const char quote('\'');
  const char slash('\\');
  const char zero('0');
  String ret;
  char status(zero);
  const int32 max = str.Len();
  for (int32 i = 0; i < max; ++i) {
    if (str[i] == quote) {
      if (status != quote) {
        status = quote;
      } else if (!ret.IsEmpty() && str[i - 1] == slash) {
        ret[ret.Len() - 1] = quote;
      } else {
        status = zero;
      }
    } else {
      ret += str[i];
    }
  }
  return ret;
}

static inline int32 CountRepeat(const String& str, int32 index, int32 max_count) {
  int32 count = 1;
  const char ch(str[index]);
  const int32 max = MathBase::Min(index + max_count, str.Len());
  while (index + count < max && str[index + count] == ch) {
    ++count;
  }
  return count;
}

static inline void AppendSeparator(Array<String>* list, const String& string, int32 from, int32 len, int32 last_quote) {
  const String separator = string.Mid(from, len);
  list->Add(last_quote >= from ? Unquote(separator) : separator);
}

bool DateTimeParser::ParseFormat(const String& new_format) {
  const char quote('\'');
  const char slash('\\');
  const char zero('0');

  if (new_format == display_format_ && !new_format.IsEmpty()) {
    return true;
  }

  //QDTPDEBUGN("ParseFormat: %s", new_format.ToLatin1().ConstData());

  Array<SectionNode> new_section_nodes;
  Sections new_display = NoSection;
  Array<String> new_separators;
  int32 i, index = 0;
  int32 add = 0;
  char status(zero);
  const int32 max = new_format.Len();
  int32 last_quote = -1;
  for (i = 0; i < max; ++i) {
    if (new_format[i] == quote) {
      last_quote = i;

      ++add;

      if (status != quote) {
        status = quote;
      } else if (i > 0 && new_format[i - 1] != slash) {
        status = zero;
      }
    } else if (status != quote) {
      const char sect = new_format[i];
      switch (sect) {
        case 'H':
        case 'h':
          if (parser_type_ != VariantTypes::Date) {
            const Section hour = (sect == 'h') ? Hour12Section : Hour24Section;
            const SectionNode node = { hour, i - add, CountRepeat(new_format, i, 2), 0 };
            new_section_nodes.Add(node);
            AppendSeparator(&new_separators, new_format, index, i - index, last_quote);
            i += node.count - 1;
            index = i + 1;
            new_display |= hour;
          }
          break;

        case 'm':
          if (parser_type_ != VariantTypes::Date) {
            const SectionNode node = { MinuteSection, i - add, CountRepeat(new_format, i, 2), 0 };
            new_section_nodes.Add(node);
            AppendSeparator(&new_separators, new_format, index, i - index, last_quote);
            i += node.count - 1;
            index = i + 1;
            new_display |= MinuteSection;
          }
          break;

        case 's':
          if (parser_type_ != VariantTypes::Date) {
            const SectionNode node = { SecondSection, i - add, CountRepeat(new_format, i, 2), 0 };
            new_section_nodes.Add(node);
            AppendSeparator(&new_separators, new_format, index, i - index, last_quote);
            i += node.count - 1;
            index = i + 1;
            new_display |= SecondSection;
          }
          break;

        case 'z':
          if (parser_type_ != VariantTypes::Date) {
            const SectionNode node = { MSecSection, i - add, CountRepeat(new_format, i, 3) < 3 ? 1 : 3, 0 };
            new_section_nodes.Add(node);
            AppendSeparator(&new_separators, new_format, index, i - index, last_quote);
            i += node.count - 1;
            index = i + 1;
            new_display |= MSecSection;
          }
          break;

        case 'A':
        case 'a':
          if (parser_type_ != VariantTypes::Date) {
            const bool cap = (sect == 'A');
            const SectionNode node = { AmPmSection, i - add, (cap ? 1 : 0), 0 };
            new_section_nodes.Add(node);
            AppendSeparator(&new_separators, new_format, index, i - index, last_quote);
            new_display |= AmPmSection;
            if (i + 1 < new_format.Len() && new_format[i+1] == (cap ? char('P') : char('p'))) {
              ++i;
            }
            index = i + 1;
          }
          break;

        case 'y':
          if (parser_type_ != VariantTypes::Time) {
            const int32 repeat = CountRepeat(new_format, i, 4);
            if (repeat >= 2) {
              const SectionNode node = { repeat == 4 ? YearSection : YearSection2Digits, i - add, repeat == 4 ? 4 : 2, 0 };
              new_section_nodes.Add(node);
              AppendSeparator(&new_separators, new_format, index, i - index, last_quote);
              i += node.count - 1;
              index = i + 1;
              new_display |= node.type;
            }
          }
          break;

        case 'M':
          if (parser_type_ != VariantTypes::Time) {
            const SectionNode node = { MonthSection, i - add, CountRepeat(new_format, i, 4), 0 };
            new_section_nodes.Add(node);
            new_separators.Add(Unquote(new_format.Mid(index, i - index)));
            i += node.count - 1;
            index = i + 1;
            new_display |= MonthSection;
          }
          break;

        case 'd':
          if (parser_type_ != VariantTypes::Time) {
            const int32 repeat = CountRepeat(new_format, i, 4);
            const Section section_type = (repeat == 4 ? DayOfWeekSectionLong : (repeat == 3 ? DayOfWeekSectionShort : DaySection));
            const SectionNode node = { section_type, i - add, repeat, 0 };
            new_section_nodes.Add(node);
            AppendSeparator(&new_separators, new_format, index, i - index, last_quote);
            i += node.count - 1;
            index = i + 1;
            new_display |= node.type;
          }
          break;

        default:
          break;
      }
    }
  }

  if (new_section_nodes.IsEmpty() && context_ == CONTEXT_DateTimeEdit) {
    return false;
  }

  if ((new_display & (AmPmSection|Hour12Section)) == Hour12Section) {
    const int32 count = new_section_nodes.Count();
    for (int32 i = 0; i < count; ++i) {
      SectionNode& node = new_section_nodes[i];
      if (node.type == Hour12Section) {
        node.type = Hour24Section;
      }
    }
  }

  if (index < max) {
    AppendSeparator(&new_separators, new_format, index, index - max, last_quote);
  } else {
    new_separators.Add(String());
  }

  display_format_ = new_format;
  separators_ = new_separators;
  section_nodes_ = new_section_nodes;
  display_ = new_display;
  last_.pos = -1;

  //for (int32 i = 0; i < section_nodes_.Count(); ++i) {
  //  QDTPDEBUG << section_nodes_[i].GetName() << section_nodes_[i].Count;
  //}

  //QDTPDEBUG << new_format << DisplayFormat;
  //QDTPDEBUGN("separators_:\n'%s'", separators_.join(QLatin1String("\n")).ToLatin1().ConstData());

  return true;
}

int32 DateTimeParser::GetSectionSize(int32 section_index) const {
  if (section_index < 0) {
    return 0;
  }

  if (section_index >= section_nodes_.Count()) {
    //fun_log(Warning, "DateTimeParser::sectionSize Internal error (%d)", section_index);
    return -1;
  }

  if (section_index == section_nodes_.Count() - 1) {
    // In some cases there is a difference between GetDisplayText() and text.
    // e.g. when text is 2000/01/31 and GetDisplayText() is "2000/2/31" - text
    // is the previous value and GetDisplayText() is the new value.
    // The Size difference is always due to leading zeroes.
    int32 size_adjustment = 0;
    const int32 display_text_size = GetDisplayText().Len();
    if (display_text_size != text_.Len()) {
      // Any zeroes added before this Section will affect our Size.
      int32 preceding_zeroes_added = 0;
      if (section_nodes_.Count() > 1 && context_ == CONTEXT_DateTimeEdit) {
        const auto begin = section_nodes_.ConstData();
        const auto end = begin + section_index;
        for (auto section_it = begin; section_it != end; ++section_it) {
          preceding_zeroes_added += section_it->zeroes_added;
        }
      }
      size_adjustment = preceding_zeroes_added;
    }

    return display_text_size + size_adjustment - GetSectionPos(section_index) - separators_.Last().Len();
  } else {
    return GetSectionPos(section_index + 1) - GetSectionPos(section_index) - separators_[section_index + 1].Len();
  }
}

int32 DateTimeParser::GetSectionMaxSize(Section section, int32 count) const {
#if !FUN_NO_TEXTDATE
  int32 m_count = 12;
#endif

  switch (section) {
    case FirstSection:
    case NoSection:
    case LastSection: return 0;

    case AmPmSection: {
      const int32 lower_max = MathBase::Min(GetAmPmText(AmText, LowerCase).Len(), GetAmPmText(PmText, LowerCase).Len());
      const int32 upper_max = MathBase::Min(GetAmPmText(AmText, UpperCase).Len(), GetAmPmText(PmText, UpperCase).Len());
      return MathBase::Min(4, MathBase::Min(lower_max, upper_max));
    }

    case Hour24Section:
    case Hour12Section:
    case MinuteSection:
    case SecondSection:
    case DaySection:
      return 2;

    //day of week와 month를 아예 분리해서 따로 처리해야함.

    case DayOfWeekSectionShort:
    case DayOfWeekSectionLong:
  #if FUN_NO_TEXTDATE
      return 2;
  #else
      m_count = 7;
      FUN_FALLTHROUGH
  #endif
    case MonthSection:
  #if FUN_NO_TEXTDATE
      return 2;
  #else
      if (count <= 2) {
        return 2;
      }

      {
        int32 ret = 0;
        const Locale locale = GetLocale();
        const Locale::FormatType format = count == 4 ? Locale::LongFormat : Locale::ShortFormat;
        for (int32 i = 1; i <= m_count; ++i) {
          const String str = (section == MonthSection
                     ? locale.GetMonthName(i, format).ToUtf8()
                     : locale.GetDayName((DayOfWeekType)i, format).ToUtf8());
          ret = MathBase::Max(str.Len(), ret);
        }
        return ret;
      }
  #endif

    // Millisecond
    case MSecSection:
      return 3;

    // 네자리 년도
    case YearSection:
      return 4;

    // 두자리 년도
    case YearSection2Digits:
      return 2;

    case CalendarPopupSection:
    case Internal:
    case TimeSectionMask:
    case DateSectionMask:
    case HourSectionMask:
    case YearSectionMask:
    case DayOfWeekSectionMask:
    case DaySectionMask:
      //fun_log(Warning, "DateTimeParser::GetSectionMaxSize: Invalid Section %s", SectionNode::name(s).ToLatin1().ConstData());
      fun_unexpected();

    case NoSectionIndex:
    case FirstSectionIndex:
    case LastSectionIndex:
    case CalendarPopupIndex:
      // these cases can't happen
      fun_unexpected();
  }

  return -1;
}

int32 DateTimeParser::GetSectionMaxSize(int32 index) const {
  const SectionNode& node = GetSectionNode(index);
  return GetSectionMaxSize(node.type, node.count);
}

String DateTimeParser::GetSectionText(const String& text, int32 section_index, int32 index) const {
  const SectionNode& node = GetSectionNode(section_index);
  switch (node.type) {
    case NoSectionIndex:
    case FirstSectionIndex:
    case LastSectionIndex:
      return String();

    default:
      break;
  }

  return text.Mid(index, GetSectionSize(section_index));
}

String DateTimeParser::GetSectionText(int32 section_index) const {
  const SectionNode& node = GetSectionNode(section_index);
  return GetSectionText(GetDisplayText(), section_index, node.pos);
}


#if !FUN_NO_TEXTDATE

int32 DateTimeParser::ParseSection( const DateTime& current_value,
                                    int32 section_index,
                                    String& text,
                                    int32& get_cursor_position,
                                    int32 index,
                                    State& state,
                                    int32* used_ptr) const {
  state = Invalid;
  int32 num = 0;
  const SectionNode& node = GetSectionNode(section_index);
  if (node.type & Internal) {
    //fun_log(Warning, "DateTimeParser::ParseSection Internal error (%s %d)", qPrintable(node.GetName()), section_index);
    return -1;
  }

  const int32 section_max_size = GetSectionMaxSize(section_index);
  String section_text = text.Mid(index, section_max_size);

  //QDTPDEBUG << "sectionValue for" << node.GetName() << "with text" << text << "and st" << section_text << text.Mid(index, section_max_size) << index;

  int32 used = 0;
  switch (node.type) {
    case AmPmSection: {
      const int32 ampm = FindAmPm(section_text, section_index, &used);
      switch (ampm) {
        case AM: // section_text == AM
        case PM: // section_text == PM
          num = ampm;
          state = Acceptable;
          break;

        case PossibleAM: // section_text => AM
        case PossiblePM: // section_text => PM
          num = ampm - 2;
          state = Intermediate;
          break;

        case PossibleBoth: // section_text => AM|PM
          num = 0;
          state = Intermediate;
          break;

        case Neither:
          state = Invalid;
          //QDTPDEBUG << "invalid because FindAmPm(" << section_text << ") returned -1";
          break;

        default:
          //QDTPDEBUGN("This should never happen (FindAmPm returned %d)", ampm);
          break;
      }

      if (state != Invalid) {
        //TODO
        //text.replace(index, used, section_text.ConstData(), used);
      }
      break;
    }

    case MonthSection:
    case DayOfWeekSectionShort:
    case DayOfWeekSectionLong:
      if (node.count >= 3) {
        String section_text2 = section_text;
        if (node.type == MonthSection) {
          const Date min_date = GetMinimum().GetDate();
          const int32 min = (current_value.GetDate().Year() == min_date.Year()) ? min_date.Month() : 1;
          num = FindMonth(section_text2.ToLower(), min, section_index, &section_text2, &used);
        } else {
          num = FindDay(section_text2.ToLower(), 1, section_index, &section_text2, &used);
        }

        if (num != -1) {
          state = (used == section_text2.Len() ? Acceptable : Intermediate);
          //TODO
          //text.replace(index, used, section_text2.ConstData(), used);
        } else {
          state = Intermediate;
        }
        break;
      }
      FUN_FALLTHROUGH

    case DaySection:
    case YearSection:
    case YearSection2Digits:
    case Hour12Section:
    case Hour24Section:
    case MinuteSection:
    case SecondSection:
    case MSecSection: {
      int32 section_text_size = section_text.Len();
      if (section_text_size == 0) {
        num = 0;
        used = 0;
        state = Intermediate;
      } else {
        for (int32 i = 0; i < section_text_size; ++i) {
          if (CharTraitsA::IsWhitespace(section_text[i])) {
            section_text_size = i; // which exits the loop
          }
        }

        const int32 abs_max = GetAbsoluteMax(section_index);
        Locale locale;
        bool ok = true;
        int32 last = -1;
        used = -1;

        const int32 max = MathBase::Min(section_max_size, section_text_size);
        String digit_str = section_text.Left(max);
        for (int32 digits = max; digits >= 1; --digits) {
          digit_str.Truncate(digits);
          int32 tmp = (int32)locale.ToUInt32(UString::FromUtf8(digit_str), &ok);
          if (ok && node.type == Hour12Section) {
            if (tmp > 12) {
              tmp = -1;
              ok = false;
            } else if (tmp == 12) {
              tmp = 0;
            }
          }
          if (ok && tmp <= abs_max) {
            //QDTPDEBUG << section_text.Left(digits) << tmp << digits;
            last = tmp;
            used = digits;
            break;
          }
        }

        if (last == -1) {
          char first(section_text[0]);
          if (separators_[section_index + 1].StartsWith(first)) {
            used = 0;
            state = Intermediate;
          } else {
            state = Invalid;
            //QDTPDEBUG << "invalid because" << section_text << "can't become a uint" << last << ok;
          }
        } else {
          num += last;
          const FieldInfo fi = GetFieldInfo(section_index);
          const bool done = (used == section_max_size);
          if (!done && fi & Fraction) { // typing 2 in a zzz field should be .200, not .002
            for (int32 i = used; i < section_max_size; ++i) {
              num *= 10;
            }
          }
          const int32 abs_min = GetAbsoluteMin(section_index);
          if (num < abs_min) {
            state = done ? Invalid : Intermediate;
            if (done) {
              //QDTPDEBUG << "invalid because" << num << "is less than GetAbsoluteMin" << abs_min;
            }
          } else if (num > abs_max) {
            state = Intermediate;
          } else if (!done && (fi & (FixedWidth|Numeric)) == (FixedWidth|Numeric)) {
            if (SkipToNextSection(section_index, current_value, digit_str)) {
              state = Acceptable;
              const int32 missing_zeroes = section_max_size - digit_str.Len();
              text.Insert(index, String(missing_zeroes, char('0')));
              used = section_max_size;
              get_cursor_position += missing_zeroes;
              ++(const_cast<DateTimeParser*>(this)->section_nodes_[section_index].zeroes_added);
            } else {
              state = Intermediate;;
            }
          } else {
            state = Acceptable;
          }
        }
      }
      break;
    }

    default:
      //fun_log(Warning, "DateTimeParser::ParseSection Internal error (%s %d)", qPrintable(node.GetName()), section_index);
      return -1;
  }

  if (used_ptr) {
    *used_ptr = used;
  }

  return (state != Invalid ? num : -1);
}
#endif //!FUN_NO_TEXTDATE

#if !FUN_NO_DATESTRING
DateTimeParser::StateNode
DateTimeParser::Parse(String& input,
                      int32& cursor_position,
                      const DateTime& default_value,
                      bool fixup) const {
  const DateTime minimum = GetMinimum();
  const DateTime maximum = GetMaximum();

  State state = Acceptable;

  DateTime final_value;
  bool conflicts = false;
  const int32 section_node_count = section_nodes_.Count();

  //QDTPDEBUG << "Parse" << input;
  {
    int32 pos = 0;
    const Date default_date = default_value.GetDate();
    const Time default_time = default_value.GetTime();
    int32 year = default_date.Year();
    int32 month = default_date.Month();
    int32 day = default_date.Day();
    int32 year_2digits = year % 100;
    int32 hour = default_time.Hour();
    int32 hour12 = -1;
    int32 minute = default_time.Minute();
    int32 second = default_time.Second();
    int32 millisecond = default_time.Millisecond();
    int32 day_of_week = (int32)default_date.DayOfWeek();

    int32 ampm = -1;
    Sections is_set = NoSection;
    int32 num;
    State tmp_state;

    for (int32 index = 0; state != Invalid && index < section_node_count; ++index) {
      if (input.Mid(pos, separators_[index].Len()) != separators_[index]) {
        //QDTPDEBUG << "invalid because" << input.Mid(pos, separators_[index].Len()) << "!=" << separators_[index] << index << pos << current_section_index_;
        state = Invalid;
        goto end;
      }

      pos += separators_[index].Len();
      section_nodes_[index].pos = pos;
      int32* current = 0;
      const SectionNode node = section_nodes_[index];
      int32 used;

      num = ParseSection(default_value, index, input, cursor_position, pos, tmp_state, &used);
      //QDTPDEBUG << "sectionValue" << node.GetName() << input << "pos" << pos << "used" << used << GetStateName(tmp_state);
      if (fixup && tmp_state == Intermediate && used < node.count) {
        const FieldInfo fi = GetFieldInfo(index);
        if ((fi & (Numeric|FixedWidth)) == (Numeric|FixedWidth)) {
          //TODO
          //const String newText = String::fromLatin1("%1").arg(num, node.count, 10, char('0'));
          //input.replace(pos, used, newText);
          //used = node.count;
          fun_check(0);
        }
      }
      pos += MathBase::Max(0, used);

      state = MathBase::Min<State>(state, tmp_state);
      if (state == Intermediate && context_ == CONTEXT_FromString) {
        state = Invalid;
        break;
      }

      //QDTPDEBUG << index << node.GetName() << "is set to" << pos << "State is" << GetStateName(state);

      if (state != Invalid) {
        switch (node.type) {
          case Hour24Section:
            current = &hour;
            break;

          case Hour12Section:
            current = &hour12;
            break;

          case MinuteSection:
            current = &minute;
            break;

          case SecondSection:
            current = &second;
            break;

          case MSecSection:
            current = &millisecond;
            break;

          case YearSection:
            current = &year;
            break;

          case YearSection2Digits:
            current = &year_2digits;
            break;

          case MonthSection:
            current = &month;
            break;

          case DayOfWeekSectionShort:
          case DayOfWeekSectionLong:
            current = &day_of_week;
            break;

          case DaySection:
            current = &day;
            num = MathBase::Max<int32>(1, num);
            break;

          case AmPmSection:
            current = &ampm;
            break;

          default:
            //fun_log(Warning, "DateTimeParser::Parse Internal error (%s)", qPrintable(node.GetName()));
            break;
        }

        if (!current) {
          //fun_log(Warning, "DateTimeParser::Parse Internal error 2");
          return StateNode();
        }

        if (is_set & node.type && *current != num) {
          //QDTPDEBUG << "CONFLICT " << node.GetName() << *current << num;
          conflicts = true;
          if (index != current_section_index_ || num == -1) {
            continue;
          }
        }

        if (num != -1) {
          *current = num;
        }

        is_set |= node.type;
      }
    }

    if (state != Invalid && input.Mid(pos) != separators_.Last()) {
      //QDTPDEBUG << "invalid because" << input.MidRef(pos) << "!=" << separators_.Last() << pos;
      state = Invalid;
    }

    if (state != Invalid) {
      if (parser_type_ != VariantTypes::Time) {
        if ((year % 100) != year_2digits && (is_set & YearSection2Digits)) {
          if (!(is_set & YearSection)) {
            year = (year / 100) * 100;
            year += year_2digits;
          } else {
            conflicts = true;
            const SectionNode& node = GetSectionNode(current_section_index_);
            if (node.type == YearSection2Digits) {
              year = (year / 100) * 100;
              year += year_2digits;
            }
          }
        }

        const Date date(year, month, day);
        const int32 diff = day_of_week - (int32)date.DayOfWeek();
        if (diff != 0 && state == Acceptable && is_set & DayOfWeekSectionMask) {
          if (is_set & DaySection) {
            conflicts = true;
          }

          const SectionNode& node = GetSectionNode(current_section_index_);
          if (node.type & DayOfWeekSectionMask || current_section_index_ == -1) {
            //TODO 아래 교정하는 코드가 과연 맞는건지??

            // DayOfWeek should be preferred
            day += diff;
            if (day < 0) {
              day += 7;
            } else if (day > date.DaysInMonth()) {
              day -= 7; //이게 과연??
            }
            //QDTPDEBUG << year << month << day << day_of_week << diff << DateTime::Date(year, month, day).DayOfWeek();
          }
        }

        bool need_fix_day = false;
        if (GetSectionType(current_section_index_) & DaySectionMask) {
          cached_day_ = day;
        } else if (cached_day_ > day) {
          day = cached_day_;
          need_fix_day = true;
        }

        if (!Date::IsValid(year, month, day)) {
          if (day < 32) {
            cached_day_ = day;
          }

          if (day > 28 && Date::IsValid(year, month, 1)) {
            need_fix_day = true;
          }
        }

        if (need_fix_day) {
          if (context_ == CONTEXT_FromString) {
            state = Invalid;
            goto end;
          }

          if (state == Acceptable && fix_day_) {
            day = MathBase::Min<int32>(day, Date(year, month, 1).DaysInMonth());

            const Locale locale = GetLocale();
            for (int32 i = 0; i < section_node_count; ++i) {
              const SectionNode node = GetSectionNode(i);

              if (!!(node.type & DaySection)) {
                //TODO
                fun_check(0);
                //input.replace(GetSectionPos(node), GetSectionSize(i), locale.ToString(day));
              } else if (!!(node.type & DayOfWeekSectionMask)) {
                //TODO
                const int32 day_of_week = (int32)Date(year, month, day).DayOfWeek();
                const Locale::FormatType day_format = (node.type == DayOfWeekSectionShort ? Locale::ShortFormat : Locale::LongFormat);
                const String day_name(locale.GetDayName((DayOfWeekType)day_of_week, day_format).ToUtf8());
                //input.replace(GetSectionPos(node), GetSectionSize(i), day_name);
              }
            }
          } else if (state > Intermediate) {
            state = Intermediate;
          }
        }
      }

      if (parser_type_ != VariantTypes::Date) {
        if (!!(is_set & Hour12Section)) {
          const bool has_hour = !!(is_set & Hour24Section);
          if (ampm == -1) {
            if (has_hour) {
              ampm = (hour < 12 ? 0 : 1);
            } else {
              ampm = 0; // no way to tell if this is am or pm so I assume am
            }
          }

          hour12 = (ampm == 0 ? hour12 % 12 : (hour12 % 12) + 12);

          if (!has_hour) {
            hour = hour12;
          } else if (hour != hour12) {
            conflicts = true;
          }
        } else if (ampm != -1) {
          if (!(is_set & (Hour24Section))) {
            hour = (12 * ampm); // special case. Only ap Section
          } else if ((ampm == 0) != (hour < 12)) {
            conflicts = true;
          }
        }
      }

      final_value = DateTime(Date(year, month, day), Time(hour, minute, second, millisecond), spec_);

      //QDTPDEBUG << year << month << day << hour << minute << second << millisecond;
    }
    //QDTPDEBUGN("'%s' => '%s'(%s)", input.ToLatin1().ConstData(), final_value.ToString(QLatin1String("yyyy/MM/dd hh:mm:ss.zzz")).ToLatin1().ConstData(), GetStateName(state).ToLatin1().ConstData());
  }

end:
  if (final_value.IsValid()) {
    if (context_ != CONTEXT_FromString && state != Invalid && final_value < minimum) {
      const char space(' ');
      if (final_value >= minimum) {
        //fun_log(Warning, "DateTimeParser::Parse Internal error 3 (%s %s)", qPrintable(final_value.ToString()), qPrintable(minimum.ToString()));
      }

      bool done = false;
      state = Invalid;
      for (int32 i = 0; i < section_node_count && !done; ++i) {
        const SectionNode& node = section_nodes_[i];
        String tmp = GetSectionText(input, i, node.pos).ToLower();
        if ((tmp.Len() < GetSectionMaxSize(i) && (((int32)GetFieldInfo(i) & (FixedWidth|Numeric)) != Numeric)) || tmp.Contains(space)) {
          switch (node.type) {
          case AmPmSection:
            switch (FindAmPm(tmp, i)) {
            case AM:
            case PM:
              state = Acceptable;
              done = true;
              break;

            case Neither:
              state = Invalid;
              done = true;
              break;

            case PossibleAM:
            case PossiblePM:
            case PossibleBoth: {
              const DateTime copy(final_value.AddSeconds(12 * 60 * 60));
              if (copy >= minimum && copy <= maximum) {
                state = Intermediate;
                done = true;
              }
              break;
              }
            }
            FUN_FALLTHROUGH

            case MonthSection:
              if (node.count >= 3) {
                const int32 final_month = final_value.GetDate().Month();
                int32 tmp2 = final_month;
                // I know the First possible Month makes the Date too early
                while ((tmp2 = FindMonth(tmp, tmp2 + 1, i)) != -1) {
                  const DateTime copy(final_value.AddMonths(tmp2 - final_month));
                  if (copy >= minimum && copy <= maximum) {
                    break; // break out of while
                  }
                }
                if (tmp2 == -1) {
                  break;
                }
                state = Intermediate;
                done = true;
                break;
              }
              FUN_FALLTHROUGH

            default: {
              int32 to_min;
              int32 to_max;

              if (node.type & TimeSectionMask) {
                if (final_value.DaysTo(minimum) != 0) {
                  break;
                }

                const Time time = final_value.GetTime();
                to_min = (int32)time.MillisecondsTo(minimum.GetTime());
                if (final_value.DaysTo(maximum) > 0) {
                  to_max = -1; // can't get to Max
                } else {
                  to_max = (int32)time.MillisecondsTo(maximum.GetTime());
                }
              } else {
                to_min = (int32)final_value.DaysTo(minimum);
                to_max = (int32)final_value.DaysTo(maximum);
              }

              const int32 max_change = node.GetMaxChange();
              if (to_min > max_change) {
                //QDTPDEBUG << "invalid because to_min > max_change" << to_min << max_change << t << final_value << minimum;
                state = Invalid;
                done = true;
                break;
              } else if (to_max > max_change) {
                to_max = -1; // can't get to Max
              }

              const int32 min = GetDigit(minimum, i);
              if (min == -1) {
                //fun_log(Warning, "DateTimeParser::Parse Internal error 4 (%s)", qPrintable(node.GetName()));
                state = Invalid;
                done = true;
                break;
              }

              int32 max = to_max != -1 ? GetDigit(maximum, i) : GetAbsoluteMax(i, final_value);
              int32 pos = cursor_position - node.pos;
              if (pos < 0 || pos >= tmp.Len()) {
                pos = -1;
              }

              if (!GetPotentialValue(tmp.Simplified(), min, max, i, final_value, pos)) {
                //QDTPDEBUG << "invalid because GetPotentialValue(" << tmp.simplified() << min << max << node.GetName() << "returned" << to_max << to_min << pos;
                state = Invalid;
                done = true;
                break;
              }
              state = Intermediate;
              done = true;
              break;
            }
          }
        }
      }
    } else {
      if (context_ == CONTEXT_FromString) {
        // optimization
        //fun_check(maximum.GetDate().ToJulianDay() == 4642999);
        //if (final_value.GetDate().ToJulianDay() > 4642999)
        if (!final_value.IsValid()) {
          state = Invalid;
        }
      } else {
        if (final_value > maximum) {
          state = Invalid;
        }
      }

      //QDTPDEBUG << "not checking intermediate because final_value is" << final_value << minimum << maximum;
    }
  }

  StateNode node;
  node.input = input;
  node.state = state;
  node.conflicts = conflicts;
  node.value = final_value.ToTimeSpec(spec_); //Spec에 맞추어서 변환하는건데, Spec은 어디서 지정하는건지??
  node.value = final_value;
  text_ = input;
  return node;
}
#endif //!FUN_NO_DATESTRING

#if !FUN_NO_TEXTDATE
static int32 FindTextEntry(
    const String& text, const Array<String>& entries, String* used_text, int32* used) {
  if (text.IsEmpty()) {
    return -1;
  }

  int32 best_match = -1;
  int32 best_count = 0;
  for (int32 n = 0; n < entries.Count(); ++n) {
    const String& name = entries[n];

    const int32 limit = MathBase::Min(text.Len(), name.Len());
    int32 i = 0;
    while (i < limit && text[i] == CharTraitsA::ToLower(name[i])) {
      ++i;
    }

    // Full match beats an equal prefix match:
    if (i > best_count || (i == best_count && i == name.Len())) {
      best_count = i;
      best_match = n;
      if (i == name.Len() && i == text.Len()) {
        break; // Exact match, name == text, wins.
      }
    }
  }

  if (used_text && best_match != -1) {
    *used_text = entries[best_match];
  }
  if (used) {
    *used = best_count;
  }

  return best_match;
}

int32 DateTimeParser::FindMonth(
      const String& str,
      int32 start_month,
      int32 section_index,
      String* used_month,
      int32* used) const {
  const SectionNode& node = GetSectionNode(section_index);
  if (node.type != MonthSection) {
    //fun_log(Warning, "DateTimeParser::FindMonth Internal error");
    return -1;
  }

  const Locale::FormatType type = node.count == 3 ? Locale::ShortFormat : Locale::LongFormat;
  Locale locale = GetLocale();
  Array<String> month_names;
  month_names.Reserve(13 - start_month);
  for (int32 month = start_month; month <= 12; ++month) {
    month_names.Add(locale.GetMonthName(month, type).ToUtf8());
  }

  const int32 index = FindTextEntry(str, month_names, used_month, used);
  return index < 0 ? index : index + start_month;
}

int32 DateTimeParser::FindDay(
    const String& str,
    int32 start_day,
    int32 section_index,
    String* used_day,
    int32* used) const {
  const SectionNode& node = GetSectionNode(section_index);
  if (!(node.type & DaySectionMask)) {
    //fun_log(Warning, "DateTimeParser::FindDay Internal error");
    return -1;
  }

  const Locale::FormatType type = node.count == 4 ? Locale::LongFormat : Locale::ShortFormat;
  Locale locale = GetLocale();
  Array<String> days_of_week;
  days_of_week.Reserve(8 - start_day);
  for (int32 day = start_day; day <= 7; ++day) {
    days_of_week.Add(locale.GetDayName((DayOfWeekType)day, type).ToUtf8());
  }

  const int32 index = FindTextEntry(str, days_of_week, used_day, used);
  return index < 0 ? index : index + start_day;
}
#endif //!FUN_NO_TEXTDATE

DateTimeParser::AmPmFinder
DateTimeParser::FindAmPm(String& str, int32 section_index, int32* used) const {
  const SectionNode& section_node = GetSectionNode(section_index);
  if (section_node.type != AmPmSection) {
    //fun_log(Warning, "DateTimeParser::FindAmPm Internal error");
    return Neither;
  }

  if (used) {
    *used = str.Len();
  }

  if (str.Trimmed().IsEmpty()) {
    return PossibleBoth;
  }

  const char space(' ');
  int32 size = GetSectionMaxSize(section_index);

  enum { AMIndex = 0, PMIndex = 1 };
  String ampm[2];
  ampm[AMIndex] = GetAmPmText(AmText, section_node.count == 1 ? UpperCase : LowerCase);
  ampm[PMIndex] = GetAmPmText(PmText, section_node.count == 1 ? UpperCase : LowerCase);
  for (int32 i = 0; i < 2; ++i) {
    ampm[i].Truncate(size);
  }

  //QDTPDEBUG << "FindAmPm" << str << ampm[0] << ampm[1];

  if (str.IndexOf(ampm[AMIndex], CaseSensitivity::IgnoreCase) == 0) {
    str = ampm[AMIndex];
    return AM;
  }

  if (str.IndexOf(ampm[PMIndex], CaseSensitivity::IgnoreCase) == 0) {
    str = ampm[PMIndex];
    return PM;
  } else if (context_ == CONTEXT_FromString || (str.Count(space) == 0 && str.Len() >= size)) {
    return Neither;
  }

  size = MathBase::Min(size, str.Len());

  bool broken[2] = {false, false};
  for (int32 i = 0; i < size; ++i) {
    if (str[i] != space) {
      for (int32 j = 0; j < 2; ++j) {
        if (!broken[j]) {
          int32 index = ampm[j].IndexOf(str[i]);
          if (index == -1) {
            if (CharTraitsA::IsUpper(str[i])) {
              index = ampm[j].IndexOf(CharTraitsA::ToLower(str[i]));
            } else if (CharTraitsA::IsLower(str[i])) {
              index = ampm[j].IndexOf(CharTraitsA::ToUpper(str[i]));
            }
            if (index == -1) {
              broken[j] = true;
              if (broken[AMIndex] && broken[PMIndex]) {
                return Neither;
              }
              continue;
            } else {
              str[i] = ampm[j][index]; // fix case
            }
          }
          ampm[j].Remove(index, 1);
        }
      }
    }
  }

  if (!broken[PMIndex] && !broken[AMIndex]) {
    return PossibleBoth;
  }

  return (!broken[AMIndex] ? PossibleAM : PossiblePM);
}

int32 DateTimeParser::SectionNode::GetMaxChange() const {
  switch (type) {
    // Time. unit is Millisecond
    case MSecSection:
      return 999;

    case SecondSection:
      return 59 * 1000;

    case MinuteSection:
      return 59 * 60 * 1000;

    case Hour24Section:
    case Hour12Section:
      return 59 * 60 * 60 * 1000;

    // Date. unit is Day
    case DayOfWeekSectionShort:
    case DayOfWeekSectionLong:
      return 7;

    case DaySection:
      return 30;

    case MonthSection:
      return 365 - 31;

    case YearSection:
      return 9999 * 365;

    case YearSection2Digits:
      return 100 * 365;

    default:
      //fun_log(Warning, "DateTimeParser::GetMaxChange() Internal error (%s)", qPrintable(GetName()));
      break;
  }

  return -1;
}

DateTimeParser::FieldInfo
DateTimeParser::GetFieldInfo(int32 index) const {
  FieldInfo ret;

  const SectionNode& node = GetSectionNode(index);
  switch (node.type) {
    case MSecSection:
      ret |= Fraction;
      FUN_FALLTHROUGH

    case SecondSection:
    case MinuteSection:
    case Hour24Section:
    case Hour12Section:
    case YearSection2Digits:
      ret |= AllowPartial;
      FUN_FALLTHROUGH

    case YearSection:
      ret |= Numeric;
      if (node.count != 1) {
        ret |= FixedWidth;
      }
      break;

    case MonthSection:
    case DaySection:
      switch (node.count) {
      case 2:
        ret |= FixedWidth;
        FUN_FALLTHROUGH

      case 1:
        //TODO 왜 에러가?
        //ret |= (Numeric|AllowPartial);
        ret |= Numeric;
        ret |= AllowPartial;
        break;
      }
      break;

    case DayOfWeekSectionShort:
    case DayOfWeekSectionLong:
      if (node.count == 3) {
        ret |= FixedWidth;
      }
      break;

    case AmPmSection:
      ret |= FixedWidth;
      break;

    default:
      //fun_log(Warning, "DateTimeParser::GetFieldInfo Internal error 2 (%d %s %d)", index, qPrintable(node.GetName()), node.count);
      break;
  }

  return ret;
}

String DateTimeParser::SectionNode::GetFormat() const {
  char fill_char = 0;
  switch (type) {
    case AmPmSection:
      return count == 1 ? "AP" : "ap";

    case MSecSection:
      fill_char = 'z';
      break;

    case SecondSection:
      fill_char = 's';
      break;

    case MinuteSection:
      fill_char = 'm';
      break;

    case Hour24Section:
      fill_char = 'H';
      break;

    case Hour12Section:
      fill_char = 'h';
      break;

    case DayOfWeekSectionShort:
    case DayOfWeekSectionLong:
    case DaySection:
      fill_char = 'd';
      break;

    case MonthSection:
      fill_char = 'M';
      break;

    case YearSection2Digits:
    case YearSection:
      fill_char = 'y';
      break;

    default:
      //fun_log(Warning, "DateTimeParser::sectionFormat Internal error (%s)", qPrintable(name(Type)));
      return String();
  }

  if (fill_char == 0) {
    //fun_log(Warning, "DateTimeParser::sectionFormat Internal error 2");
    return String();
  }

  return String(count, fill_char);
}

bool DateTimeParser::GetPotentialValue(
    const String& str,
    int32 min,
    int32 max,
    int32 index,
    const DateTime& current_value,
    int32 insert) const {
  if (str.IsEmpty()) {
    return true;
  }

  const int32 size = GetSectionMaxSize(index);
  int32 val = (int32)GetLocale().ToUInt32(UString::FromUtf8(str));
  const SectionNode& node = GetSectionNode(index);
  if (node.type == YearSection2Digits) {
    const int32 year = current_value.GetDate().Year();
    val += year - (year % 100);
  }

  if (val >= min && val <= max && str.Len() == size) {
    return true;
  } else if (val > max) {
    return false;
  } else if (str.Len() == size && val < min) {
    return false;
  }

  const int32 len = size - str.Len();
  for (int32 i = 0; i < len; ++i) {
    for (int32 j = 0; j < 10; ++j) {
      if (GetPotentialValue(str + char('0' + j), min, max, index, current_value, insert)) {
        return true;
      } else if (insert >= 0) {
        const String tmp = str.Left(insert) + char('0' + j) + str.Mid(insert);
        if (GetPotentialValue(tmp, min, max, index, current_value, insert)) {
          return true;
        }
      }
    }
  }

  return false;
}

bool DateTimeParser::SkipToNextSection(int32 index, const DateTime& current, const String& text) const {
  const SectionNode& node = GetSectionNode(index);
  fun_check(text.Len() < GetSectionMaxSize(index));

  const DateTime maximum = GetMaximum();
  const DateTime minimum = GetMinimum();
  fun_check(current >= minimum && current <= maximum);

  DateTime tmp = current;
  int32 min = GetAbsoluteMin(index);
  SetDigit(tmp, index, min);
  if (tmp < minimum) {
    min = GetDigit(minimum, index);
  }

  int32 max = GetAbsoluteMax(index, current);
  SetDigit(tmp, index, max);
  if (tmp > maximum) {
    max = GetDigit(maximum, index);
  }

  int32 pos = GetCursorPosition() - node.pos;
  if (pos < 0 || pos >= text.Len()) {
    pos = -1;
  }

  // If the value potentially can become another valid entry we don't want to
  // skip to the next. E.g. In a M field (Month without leading 0) if you Type
  // 1 we don't want to autoskip (there might be [012] following) but if you
  // Type 3 we do.
  return !GetPotentialValue(text, min, max, index, current, pos);
}

String DateTimeParser::SectionNode::GetName(DateTimeParser::Section section_type) {
  switch (section_type) {
    case DateTimeParser::AmPmSection: return "AmPmSection";
    case DateTimeParser::DaySection: return "DaySection";
    case DateTimeParser::DayOfWeekSectionShort: return "DayOfWeekSectionShort";
    case DateTimeParser::DayOfWeekSectionLong: return "DayOfWeekSectionLong";
    case DateTimeParser::Hour24Section: return "Hour24Section";
    case DateTimeParser::Hour12Section: return "Hour12Section";
    case DateTimeParser::MSecSection: return "MSecSection";
    case DateTimeParser::MinuteSection: return "MinuteSection";
    case DateTimeParser::MonthSection: return "MonthSection";
    case DateTimeParser::SecondSection: return "SecondSection";
    case DateTimeParser::YearSection: return "YearSection";
    case DateTimeParser::YearSection2Digits: return "YearSection2Digits";
    case DateTimeParser::NoSection: return "NoSection";
    case DateTimeParser::FirstSection: return "FirstSection";
    case DateTimeParser::LastSection: return "LastSection";
    default: return "Unknown Section " + String::FromNumber(int32(section_type));
  }
}

String DateTimeParser::GetStateName(State state) const {
  switch (state) {
    case Invalid: return "Invalid";
    case Intermediate: return "Intermediate";
    case Acceptable: return "Acceptable";
    default: return "Unknown state " + String::FromNumber((int32)state);
  }
}

#if !FUN_NO_DATESTRING
bool DateTimeParser::FromString(const String& str, Date* date, Time* time) const {
  DateTime val(Date(1900, 1, 1), DATETIMEEDIT_TIME_MIN);
  String text = str;
  int32 copy = -1;
  const StateNode tmp = Parse(text, copy, val, false);
  if (tmp.state != Acceptable || tmp.conflicts) {
    return false;
  }

  if (time) {
    const Time time_part = tmp.value.GetTime();
    if (!time_part.IsValid()) {
      return false;
    }
    *time = time_part;
  }

  if (date) {
    const Date date_part = tmp.value.GetDate();
    if (!date_part.IsValid()) {
      return false;
    }
    *date = date_part;
  }
  return true;
}
#endif // !FUN_NO_DATESTRING

DateTime DateTimeParser::GetMinimum() const {
  // Cache the most common case
  if (spec_ == TimeSpec::Local) {
    static const DateTime local_time_min(DATETIMEEDIT_DATE_MIN, DATETIMEEDIT_TIME_MIN, TimeSpec::Local);
    return local_time_min;
  }

  return DateTime(DATETIMEEDIT_DATE_MIN, DATETIMEEDIT_TIME_MIN, spec_);
}

DateTime DateTimeParser::GetMaximum() const {
  // Cache the most common case
  if (spec_ == TimeSpec::Local) {
    static const DateTime local_time_max(DATETIMEEDIT_DATE_MAX, DATETIMEEDIT_TIME_MAX, TimeSpec::Local);
    return local_time_max;
  }

  return DateTime(DATETIMEEDIT_DATE_MAX, DATETIMEEDIT_TIME_MAX, spec_);
}

String DateTimeParser::GetAmPmText(AmPm ap, Case casesense) const {
  const Locale locale = GetLocale();
  const UString raw = ap == AmText ? locale.GetAMText() : locale.GetPMText();
  return casesense == UpperCase ? raw.ToUpper().ToUtf8() : raw.ToLower().ToUtf8();
}

bool operator == (const DateTimeParser::SectionNode& node1,
                  const DateTimeParser::SectionNode& node2) {
  return node1.type == node2.type && node1.pos == node2.pos && node1.count == node2.count;
}

} // namespace fun
