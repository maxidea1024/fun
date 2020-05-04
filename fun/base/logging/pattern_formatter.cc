// TODO 오늘안에 필히 완료하도록 하자.
//좀더 긴 포맷으로 명확하게 하는게 좋으려나... 파이썬 로깅 부분을 참고해볼까..

#include "fun/base/logging/pattern_formatter.h"

#include "fun/base/logging/log_message.h"
//#include "fun/base/number_formatter.h"
//#include "fun/base/date_time_format.h"
//#include "fun/base/date_time_formatter.h"
#include "fun/base/date_time.h"
#include "fun/base/environment.h"
#include "fun/base/time_zone.h"
#include "fun/base/timestamp.h"
//#include "fun/base/number_parser.h"
//#include "fun/base/string_tokenizer.h"
#include "fun/base/exception.h"
#include "fun/base/str.h"

namespace fun {

const String PatternFormatter::PROP_PATTERN = "Pattern";
const String PatternFormatter::PROP_TIMES = "Times";
const String PatternFormatter::PROP_LEVEL_NAMES = "LevelNames";

PatternFormatter::PatternFormatter() : local_time_(false) {
  ParsePriorityNames();
}

PatternFormatter::PatternFormatter(const String& pattern)
    : local_time_(false), pattern_(pattern) {
  ParsePriorityNames();
  ParsePattern();
}

PatternFormatter::~PatternFormatter() {}

void PatternFormatter::Format(const LogMessage& msg, String& text) {
  Timestamp timestamp = msg.GetTime();
  bool local_time = local_time_;

  // TODO
  // if (local_time) {
  //  timestamp += Timezone::utcOffset()  * Timestamp::resolution();
  //  timestamp += Timezone::dst()        * Timestamp::resolution();
  //}

  DateTime date_time(timestamp);

  for (const auto& action : pattern_actions_) {
    text.Append(action.prepend);

    switch (action.key) {
      // message source
      case 's':
        text.Append(msg.GetSource());
        break;

      // message text
      case 't':
        text.Append(msg.GetText());
        break;

      // message priority level
      case 'l':
        text.AppendNumber(msg.GetLevel());
        break;

      // message priority name
      // (Fatal, Critical, Error, Warning, Notice, Information, Debug, Trace)
      case 'p':
        text.Append(GetLevelName(msg.GetLevel()));
        break;

      // abbreviated message priority name
      // (F, C, E, W, N, I, D, T)
      case 'q':
        text.Append(GetLevelName(msg.GetLevel()).First());
        break;

      // message process identifier (PID)
      case 'P':
        text.AppendNumber(static_cast<int64>(msg.GetPid()));
        break;

      // message thread identifier (TID)
      case 'I':
        text.AppendNumber(static_cast<int64>(msg.GetTid()));
        break;

      // message thread name
      case 'T':
        text.Append(msg.GetThread());
        break;

      // message thread identifier (TID)
      case 'O':
        text.AppendNumber(msg.GetOsTid());
        break;

      // node or host name
      case 'N':
        text.Append(Environment::GetNodeName());
        break;

      // message source file path (empty string if not set)
      case 'U':
        text.Append(msg.GetSourceFile() ? msg.GetSourceFile() : "");
        break;

      // message source line number (0 if not set)
      case 'u':
        text.AppendNumber(msg.GetSourceLine());
        break;

      // message date/time abbreviated weekday (Mon, Tue, ...)
      case 'w':
        text.Append(Date::GetShortDayName(date_time.DayOfWeek()));
        break;

      // message date/time full weekday (Monday, Tuesday, ...)
      case 'W':
        text.Append(Date::GetLongDayName(date_time.DayOfWeek()));
        break;

      // message date/time abbreviated month (Jan, Feb, ...)
      case 'b':
        text.Append(Date::GetShortMonthName(date_time.Month()));
        break;

      // message date/time full month (January, February, ...)
      case 'B':
        text.Append(Date::GetLongMonthName(date_time.Month()));
        break;

      // message date/time zero-padded day of month (01 .. 31)
      case 'd':
        text.Appendf("{0:02d}", date_time.Day());
        break;

      // message date/time day of month (1 .. 31)
      case 'e':
        text.AppendNumber(date_time.Day());
        break;

      // message date/time space-padded day of month ( 1 .. 31)
      case 'f':
        text.Appendf("{0:2d}", date_time.Day());
        break;

      // message date/time zero-padded month (01 .. 12)
      case 'm':
        text.Appendf("{0:02d}", date_time.Month());
        break;

      // message date/time month (1 .. 12)
      case 'n':
        text.Appendf("{0:d}", date_time.Month());
        break;

      // message date/time space-padded month ( 1 .. 12)
      case 'o':
        text.Appendf("{0:2d}", date_time.Month());
        break;

      // message date/time year without century (70)
      case 'y':
        text.Appendf("{0:02d}", date_time.Year() % 100);
        break;

      // message date/time year with century (1970)
      case 'Y':
        text.Appendf("{0:4d}", date_time.Year());
        break;

      // message date/time hour (00 .. 23)
      case 'H':
        text.Appendf("{0:02d}", date_time.Hour());
        break;

      // message date/time hour (00 .. 12)
      case 'h':
        text.Appendf("{0:02d}", date_time.HourAMPM());
        break;

      // message date/time am/pm
      case 'a':
        text.Append(date_time.IsAM() ? "am" : "pm");
        break;

      // message date/time AM/PM
      case 'A':
        text.Append(date_time.IsAM() ? "AM" : "PM");
        break;

      // message date/time minute (00 .. 59)
      case 'M':
        text.Appendf("{0:02d}", date_time.Minute());
        break;

      // message date/time second (00 .. 59)
      case 'S':
        text.Appendf("{0:02d}", date_time.Second());
        break;

      // message date/time millisecond (000 .. 999)
      case 'i':
        text.Appendf("{0:03d}", date_time.Millisecond());
        break;

      // message date/time centisecond (0 .. 9)
      case 'c':
        text.Appendf("{0}", date_time.Millisecond() / 100);
        break;

      // message date/time fractional seconds/microseconds (000000 - 999999)
      case 'F':
        text.Appendf("{0:06d}",
                     date_time.Millisecond() * 1000 + date_time.Microsecond());
        break;

      // TODO
      // time zone differential in ISO 8601 format (Z or +NN.NN)
      // case 'z':
      //  text.Append(DateTimeFormatter::TzdISO(local_time ? Timezone::tzd() :
      //  DateTimeFormatter::UTC)); break;

      // TODO
      // time zone differential in RFC format (GMT or +NNNN)
      // case 'Z':
      //  text.Append(DateTimeFormatter::TzdRFC(local_time ? Timezone::tzd() :
      //  DateTimeFormatter::UTC)); break;

      // convert time to local time
      // (must be specified before any date/time specifier;
      //  does not itself output anything)
      case 'L':
        // TODO
        // if (!local_time) {
        //  local_time = true;
        //  timestamp += Timezone::utcOffset() * Timestamp::resolution();
        //  timestamp += Timezone::dst()       * Timestamp::resolution();
        //  date_time = timestamp;
        //}
        break;

      // epoch time (UTC, seconds since midnight, January 1, 1970)
      case 'E':
        text.AppendNumber(static_cast<int64>(msg.GetTime().EpochTime()));
        break;

      // the message source (%s) but text length is padded/cropped to 'width'
      case 'v':
        if (action.length > msg.GetSource().Len()) {  // Append spaces
          text.Append(msg.GetSource())
              .Append(action.length - msg.GetSource().Len(), ' ');
        } else if (action.length &&
                   action.length < msg.GetSource().Len()) {  // crop
          // TODO
          // text.Append(msg.GetSource(), msg.GetSource().Len() - action.length,
          // action.length);
        } else {
          text.Append(msg.GetSource());
        }
        break;

      // %[property]
      case 'x':
        try {
          text.Append(msg[action.property]);
        } catch (...) {
        }
        break;
    }
  }
}

void PatternFormatter::ParsePattern() {
  pattern_actions_.Clear();

  String::ConstIterator it = pattern_.begin();
  String::ConstIterator end = pattern_.end();
  PatternAction end_act;
  while (it != end) {
    if (*it == '%') {
      if (++it != end) {
        PatternAction act;
        act.prepend = end_act.prepend;
        end_act.prepend.Clear();

        // %[xyz] 와 같은 형태일 경우에 key를 'x'로 해주어서 프로퍼티임을
        // 표시함.
        if (*it == '[') {
          act.key = 'x';
          ++it;
          String prop;
          while (it != end && *it != ']') {
            prop += *it++;
          }
          if (it == end) {
            --it;
          }
          act.property = prop;
        } else {
          act.key = *it;
          if ((it + 1) != end && *(it + 1) == '[') {
            it += 2;
            String number;
            while (it != end && *it != ']') {
              number += *it++;
            }
            if (it == end) {
              --it;
            }
            try {
              bool ok = false;
              act.length = number.ToInt32(&ok);
              if (!ok) {
                act.length = 0;  // 그냥 값으로 0으로...
              }
            } catch (...) {
            }
          }
        }
        pattern_actions_.Add(act);
        ++it;
      }
    } else {
      end_act.prepend += *it++;
    }
  }

  if (end_act.prepend.Len()) {
    pattern_actions_.Add(end_act);
  }
}

void PatternFormatter::SetProperty(const String& name, const String& value) {
  if (name == PROP_PATTERN) {
    pattern_ = value;
    ParsePattern();
  } else if (name == PROP_TIMES) {
    local_time_ = icompare(value, "Local") == 0;
  } else if (name == PROP_LEVEL_NAMES) {
    level_names_ = value;
    ParsePriorityNames();
  } else {
    LogFormatter::SetProperty(name, value);
  }
}

String PatternFormatter::GetProperty(const String& name) const {
  if (name == PROP_PATTERN) {
    return pattern_;
  } else if (name == PROP_TIMES) {
    return local_time_ ? "Local" : "UTC";
  } else if (name == PROP_LEVEL_NAMES) {
    return level_names_;
  } else {
    return LogFormatter::GetProperty(name);
  }
}

namespace {

static String LEVEL_NAMES[] = {"",        "Fatal",  "Critical",    "Error",
                               "Warning", "Notice", "Information", "Debug",
                               "Trace"};

}  // namespace

void PatternFormatter::ParsePriorityNames() {
  for (int32 i = 0; i < 9; ++i) {
    levels_[i] = LEVEL_NAMES[i];
  }

  if (!level_names_.IsEmpty()) {
    Array<String> st = level_names_.Split(",;", 0, StringSplitOption::Trimming);

    if (st.Count() == 8) {
      for (int32 i = 1; i < 9; ++i) {
        levels_[i] = st[i - 1];
      }
    } else {
      throw SyntaxException(
          "LogLevelNames property must specify a comma-separated list of 8 "
          "property names");
    }
  }
}

const String& PatternFormatter::GetLevelName(LogLevel::Type level) {
  fun_check(1 <= level && level < 9);
  return levels_[level];
}

}  // namespace fun
