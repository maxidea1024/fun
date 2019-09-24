#include "fun/base/cron_expression.h"
#include "fun/base/date_time.h"
#include "fun/base/time_zone.h"

#if FUN_WITH_CRON_EXPRESSION

namespace fun {

Map<String, int32> CronExpression::month_map_;
Map<String, int32> CronExpression::day_map_;


/*

struct CIntegerSet {
  Array<int32> Values;

  void Clear() {
    Values.Clear();
  }

  void Add(int32 value) {
    int32 Index = 0;
    for (; Index < Values.Count() && value < Values[Index]; ++Index);
    if (Values.Count() && value == Values[Index]) {
      return;
    }
    Values.Insert(value, Index);
  }

  int32 FirstOrDefault(int32 Default) const {
    return !Values.IsEmpty() ? Values[0] : Default;
  }

  int32 LastOrDefault(int32 Default) const {
    return !Values.IsEmpty() ? Values[Values.Count()-1] : Default;
  }

  bool Contains(int32 value) const {
    return Values.Contains(value);
  }
};

*/


CronExpression::CronExpression() {
  Init();
}

CronExpression::CronExpression(const String& cron_expression) {
  Init();

  cron_expression_string_ = cron_expression;
  cron_expression_string_.MakeUpper();

  BuildExpression(cron_expression_string_);
}

void CronExpression::Init() {
  time_zone_ = TimeZone::Local();

  last_day_of_week_ = false;
  nth_day_of_week_ = 0;
  last_day_of_month_ = false;
  nearest_weekday_ = false;
  last_day_offset_ = 0;
  calendar_day_of_week_ = false;
  calendar_day_of_month_ = false;
  expression_parsed_ = false;

  //TODO solve thread safe issue.
  if (month_map_.Count() == 0) {
    month_map_.Add("JAN", 0);
    month_map_.Add("FEB", 1);
    month_map_.Add("MAR", 2);
    month_map_.Add("APR", 3);
    month_map_.Add("MAY", 4);
    month_map_.Add("JUN", 5);
    month_map_.Add("JUL", 6);
    month_map_.Add("AUG", 7);
    month_map_.Add("SEP", 8);
    month_map_.Add("OCT", 9);
    month_map_.Add("NOV", 10);
    month_map_.Add("DEC", 11);

    day_map_.Add("SUN", 1);
    day_map_.Add("MON", 2);
    day_map_.Add("TUE", 3);
    day_map_.Add("WED", 4);
    day_map_.Add("THU", 5);
    day_map_.Add("FRI", 6);
    day_map_.Add("SAT", 7);
  }
}

void CronExpression::BuildExpression(const String& expression) {
  expression_parsed_ = true;

  seconds_.clear();
  minutes_.clear();
  hours_.clear();
  days_of_month_.clear();
  months_.clear();
  days_of_week_.clear();
  years_.clear();

  int32 expr_on = SECOND;

  Array<String> exprs_tok;
  expression.Simplified().Split(exprs_tok, " ", StringSplitOption::TrimmingAndCullEmpty);

  try {
    for (const auto& token : exprs_tok) {
      if (token.Len() == 0) {
        continue;
      }

      if (expr_on > YEAR) {
        break;
      }

      // throw an exception if L is used with other days of the month
      if (expr_on == DAY_OF_MONTH && token.Contains('L') && token.Len() > 1 && token.Contains(',')) {
        throw SyntaxException("Support for specifying 'L' and 'LW' with other days of the month is not implemented");
      }

      // throw an exception if L is used with other days of the week
      if (expr_on == DAY_OF_WEEK && token.Contains('L') && token.Len() > 1 && token.Contains(',')) {
        throw SyntaxException("Support for specifying 'L' with other days of the week is not implemented");
      }

      if (expr_on == DAY_OF_WEEK && token.Contains('#')/*TODO*/) {
        throw SyntaxException("Support for specifying multiple \"nth\" days is not implemented.");
      }

      Array<String> value_tokens;
      token.Split(value_tokens, ",");
      for (const auto& value : value_tokens) {
        StoreExpressionValues(0, value, expr_on);
      }

      ++expr_on;
    }

    if (expr_on <= DAY_OF_WEEK) {
      throw SyntaxException("Unexpected end of expression.");
    }

    if (expr_on <= YEAR) {
      StoreExpressionValues(0, "*", YEAR);
    }

    const auto& day_of_week = GetSet(DAY_OF_WEEK);
    const auto& day_of_month = GetSet(DAY_OF_MONTH);

    const bool day_of_month_spec = !ContainsInSet(day_of_month, NoSpec);
    const bool day_of_week_spec = !ContainsInSet(day_of_week, NoSpec);

    if (day_of_month_spec && !day_of_week_spec) {
      // skip
    }
    else if (!day_of_month_spec && day_of_week_spec) {
      // skip
    }
    else {
      throw SyntaxException("Support for specifying both a day-of-week and a day-of-month parameter is not implemented.");
    }
  }
  catch (SyntaxException&) {
    throw;
  }
  catch (Exception& e) {
    throw SyntaxException(String::Format("Illegal cron expression format (%s)", e.GetMessage()));
  }
}

int32 CronExpression::StoreExpressionValues(int32 pos, const String& s, int32 type) {
  int32 incr = 0;
  int32 i = SkipWhiteSpace(pos, s);
  if (i >= s.Len()) {
    return i;
  }

  char c = s[i];
  //if (c >= 'A' && c <= 'Z' && (s != "L" && s != "LW")/*TODO && (!Regex.IsMatch(s, "^L-[0-9]*[W]?"))*/)
  if (c >= 'A' && c <= 'Z' && (s != "L" && s != "LW") && !std::regex_match(*s, std::regex("^L-[0-9]*[W]?"))) {
    String sub = s.Mid(i, 3);
    int32 start_val;
    int32 end_val = -1;

    if (type == MONTH) {
      start_val = GetMonthNumber(sub) + 1;
      if (start_val <= 0) {
        throw SyntaxException(String::Format("Invalid month value: '%s'", sub));
      }

      if (s.Len() > (i+3)) {
        c = s[i+3];
        if (c == '-') {
          i += 4;
          sub = s.Mid(i, 3);
          end_val = GetMonthNumber(sub) + 1;
          if (end_val <= 0) {
            throw SyntaxException(String::Format("Invalid month value: '%s'", sub));
          }
        }
      }
    }
    else if (type == DAY_OF_WEEK) {
      start_val = GetDayOfWeekNumber(sub);
      if (start_val < 0) {
        throw SyntaxException(String::Format("Invalid day-of-week value: '%s'", sub));
      }

      if (s.Len() > (i + 3)) {
        c = s[i+3];
        if (c == '-') {
          i += 4;
          sub = s.Mid(i,3);
          end_val = GetDayOfWeekNumber(sub);
          if (end_val < 0) {
            throw SyntaxException(String::Format("Invalid day-of-week value: '%s'", sub));
          }
        }
        else if (c == '#') {
          i += 4;
          try {
            nth_day_of_week_ = s.Mid(i).ToInt32();
            if (nth_day_of_week_ < 1 || nth_day_of_week_ > 5) {
              //TODO 구체화 시켜주는게 좋을듯함..
              throw SyntaxException(String::Format("Invalid day-of-week value: '%s'", sub));
            }
          }
          catch (Exception&) {
            throw SyntaxException("A numeric value between 1 and 5 must follow the '#' option");
          }
        }
        else if (c == 'L') {
          last_day_of_week_ = true;
          i++;
        }
      }
    }
    else {
      throw SyntaxException(String::Format("Illegal characters for this position: '%s'", sub));
    }

    if (end_val != -1) {
      incr = 1;
    }

    AddToSet(start_val, end_val, incr, type);
    return i + 3;
  }

  if (c == '?') {
    i++; // skip '?'

    if ((i + 1) < s.Len() && (s[i + 1] != ' ' && s[i + 1] != '\t')) {
      throw SyntaxException(String::Format("Illegal character after '?': %c", s[i]));
    }

    if (type != DAY_OF_WEEK && type != DAY_OF_MONTH) {
      throw SyntaxException("'?' can only be specified for day-of-month or day-of-week.");
    }

    if (type == DAY_OF_WEEK && !last_day_of_month_) {
      //int val = daysOfMonth.LastOrDefault();
      const int32 val = days_of_month_.empty() ? 0 : *(days_of_month_.rbegin());
      if (val == NoSpecInt) {
        throw SyntaxException("'?' can only be specified for day-of-month or day-of-week.");
      }
    }

    AddToSet(NoSpecInt, -1, 0, type);
    return i;
  }

  if (c == '*' || c == '/') {
    if (c == '*' && (i + 1) >= s.Len()) {
      AddToSet(AllSpecInt, -1, incr, type);
      return i + 1;
    }
    else if (c == '/' && ((i + 1) >= s.Len() || s[i + 1] == ' ' || s[i + 1] == '\t')) {
      throw SyntaxException("'/' must be followed by an integer.");
    }
    else if (c == '*') {
      i++;
    }

    c = s[i];
    if (c == '/') {
      // is an increment specified?
      i++;
      if (i >= s.Len()) {
        throw SyntaxException("Unexpected end of String.");
      }

      incr = GetNumericValue(s, i);

      i++;
      if (incr >= 10) { // 원 소스에서는 'incr > 10' 이었지만, 두자리읽었을 경우에 대한 처리일테니까 'incr >= 10'이 맞을듯 한데?
        i++;
      }
      CheckIncrementRange(incr, type);
    }
    else {
      incr = 1;
    }

    AddToSet(AllSpecInt, -1, incr, type);
    return i;
  }
  else if (c == 'L') {
    i++;

    if (type == DAY_OF_MONTH) {
      last_day_of_month_ = true;
    }
    else if (type == DAY_OF_WEEK) {
      AddToSet(7, 7, 0, type);
    }

    if (type == DAY_OF_MONTH && s.Len() > i) {
      c = s[i];
      if (c == '-') {
        ValueSet value_set = GetValue(0, s, i+1);
        last_day_offset_ = value_set.value;
        if (last_day_offset_ > 30) {
          throw SyntaxException("Offset from last day must be <= 30");
        }
        i = value_set.pos;
      }

      if (s.Len() > i) {
        c = s[i];
        if (c == 'W') {
          i++; // skip 'W'
          nearest_weekday_ = true;
        }
      }
    }

    return i;
  }
  else if (c >= '0' && c <= '9') { // digit(s)
    int32 val = (int32)(c - '0');
    i++;
    if (i >= s.Len()) {
      AddToSet(val, -1, -1, type);
    }
    else {
      c = s[i];
      if (c >= '0' && c <= '9') { // 2 digits
        auto value_set = GetValue(val, s, i);
        val = value_set.value;
        i = value_set.pos;
      }
      return CheckNext(i, s, val, type);
    }
  }
  else {
    throw SyntaxException(String::Format("Unexpected character: %c", c));
  }

  // unreachable!
  return i;
}

void CronExpression::CheckIncrementRange(int32 incr, int32 type) const {
  if (incr > 59 && (type == SECOND || type == MINUTE)) {
    throw SyntaxException(String::Format("Increment > 59 : %d", incr));
  }

  if (incr > 23 && type == HOUR) {
    throw SyntaxException(String::Format("Increment > 24 : %d", incr));
  }

  if (incr > 31 && type == DAY_OF_MONTH) {
    throw SyntaxException(String::Format("Increment > 31 : %d", incr));
  }

  if (incr > 7 && type == DAY_OF_WEEK) {
    throw SyntaxException(String::Format("Increment > 7 : %d", incr));
  }

  if (incr > 12 && type == MONTH) {
    throw SyntaxException(String::Format("Increment > 12 : %d", incr));
  }
}

int32 CronExpression::CheckNext(int32 pos, const String& s, int32 val, int32 type) {
  int32 end = -1;
  int32 i = pos;

  if (i >= s.Len()) {
    AddToSet(val, end, -1, type);
    return i;
  }

  char c = s[pos];

  if (c == 'L') {
    if (type == DAY_OF_WEEK) {
      if (val < 1 || val > 7) {
        throw SyntaxException("Day-of-Week values must be between 1 and 7");
      }
      last_day_of_week_ = true;
    }
    else {
      throw SyntaxException(String::Format("'L' option is not valid here. (pos=%d)", i));
    }

    i++; // skip 'L'

    std::set<int32>& data = GetSet(type);
    data.insert(val);
    return i;
  }

  if (c == 'W') {
    if (type == DAY_OF_MONTH) {
      nearest_weekday_ = true;
    }
    else {
      throw SyntaxException(String::Format("'W' option is not valid here. (pos=%d)", i));
    }
    if (val > 31) {
      throw SyntaxException("The 'W' option does not make sense with values larger than 31 (max number of days in a month)");
    }

    i++; // skip 'W'

    std::set<int32>& data = GetSet(type);
    data.insert(val);
    return i;
  }

  if (c == '#') {
    if (type != DAY_OF_WEEK) {
      throw SyntaxException(String::Format("'#' option is not valid here. (pos=%d)", i));
    }

    try {
      nth_day_of_week_ = s.Mid(i).ToInt32();
      if (nth_day_of_week_ < 1 || nth_day_of_week_ > 5) {
        //TODO 메시지를 구체화 하는게 어떨런지?
        throw SyntaxException();
      }
    }
    catch (Exception&) {
      throw SyntaxException("A numeric value between 1 and 5 must follow the '#' option");
    }

    i++; // skip '#'

    std::set<int32>& data = GetSet(type);
    data.insert(val);
    return i;
  }

  if (c == 'C') {
    if (type == DAY_OF_WEEK) {
      calendar_day_of_week_ = true;
    }
    else if (type == DAY_OF_MONTH) {
      calendar_day_of_month_ = true;
    }
    else {
      throw SyntaxException(String::Format("'C' option is not valid here. (pos=%d)", i));
    }

    i++; // skip 'C'

    std::set<int32>& data = GetSet(type);
    data.insert(val);
    return i;
  }

  if (c == '-') {
    i++; // skip '-'

    c = s[i];
    if (c < '0' || c > '9') {
      throw SyntaxException(String::Format("The range value specification position must be followed by a number. However, the letter %c is specified.(pos=%d)", c, i));
    }
    i++; // skip 1 digit

    const int32 v = (int32)(c - '0');
    end = v;
    if (i >= s.Len()) {
      AddToSet(val, end, 1, type);
      return i;
    }

    c = s[i];
    if (c >= '0' && c <= '9') { // 2 digits
      auto value_set = GetValue(v, s, i);
      end = value_set.value;
      i = value_set.pos;
    }

    if (i < s.Len() && ((c = s[i]) == '/')) { // incr is specified
      i++; // skip '/'

      c = s[i];
      const int32 v2 = (int32)(c - '0');
      i++;
      if (i >= s.Len()) { // 1 digit
        AddToSet(val, end, v2, type);
        return i;
      }

      c = s[i];
      if (c >= '0' && c <= '9') { // 2 digits
        auto value_set = GetValue(v2, s, i);
        AddToSet(val, end, value_set.value, type);
        return value_set.pos;
      }
      else { //1 digit
        AddToSet(val, end, v2, type);
        return i;
      }
    }
    else { // no incr is specified
      AddToSet(val, end, 1, type);
      return i;
    }
  }

  if (c == '/') {
    if ((i + 1) >= s.Len() || s[i + 1] == ' ' || s[i + 1] == '\t') {
      throw SyntaxException("\'/\' must be followed by an integer.");
    }

    i++; // skip '/'

    c = s[i];
    const int32 v2 = (int32)(c - '0');
    i++; // skip 1 digit
    if (i >= s.Len()) {
      CheckIncrementRange(v2, type);
      AddToSet(val, end, v2, type);
      return i;
    }

    c = s[i];
    if (c >= '0' && c <= '9') { // 2 digits
      auto value_set = GetValue(v2, s, i);
      CheckIncrementRange(value_set.value, type);
      AddToSet(val, end, value_set.value, type);
      return value_set.pos;
    }
    else {
      throw SyntaxException(String::Format("Unexpected character '%c' after '/'", c));
    }
  }

  AddToSet(val, end, 0, type);
  return i + 1;
}

int32 CronExpression::SkipWhiteSpace(int32 i, const String& s) const {
  for (; i < s.Len() && (s[i] == ' ' || s[i] == '\t'); ++i);
  return i;
}

int32 CronExpression::FindNextWhiteSpace(int32 i, const String& s) const {
  for (; i < s.Len() && (s[i] != ' ' && s[i] == '\t'); ++i);
  return i;
}

CronExpression::ValueSet CronExpression::GetValue(int32 v, const String& s, int32 i) const {
  //String value_str = String::Format("%d"), v); // start with leading digit
  String value_str = String::FromNumber(v); // start with leading digit
  char c = s[i];
  while (c >= '0' && c <= '9') {
    value_str += c;
    i++;
    if (i >= s.Len()) {
      break;
    }
    c = s[i];
  }

  ValueSet result;
  if (i < s.Len()) {
    result.pos = i;
  }
  else {
    result.pos = i + 1;
  }
  result.value = value_str.ToInt32();
  return result;
}

int32 CronExpression::GetNumericValue(const String& s, int32 i) const {
  const int32 end_of_val = FindNextWhiteSpace(i, s);
  const String result = s.Mid(i, end_of_val - i);
  return result.ToInt32();
}

int32 CronExpression::GetMonthNumber(const String& s) const {
  int32 result = -1;
  month_map_.TryGetValue(s, result);
  return result;
}

int32 CronExpression::GetDayOfWeekNumber(const String& s) const {
  int32 result = -1;
  day_map_.TryGetValue(s, result);
  return result;
}

void CronExpression::AddToSet(int32 val, int32 end, int32 incr, int32 type) {
  auto& data = GetSet(type);

  if (type == SECOND || type == MINUTE) {
    if ((val < 0 || val > 59 || end > 59) && (val != AllSpecInt)) {
      throw SyntaxException("Minute and Second values must be between 0 and 59");
    }
  }
  else if (type == HOUR) {
    if ((val < 0 || val > 23 || end > 23) && (val != AllSpecInt)) {
      throw SyntaxException("Hour values must be between 0 and 23");
    }
  }
  else if (type == DAY_OF_MONTH) {
    if ((val < 1 || val > 31 || end > 31) && (val != AllSpecInt) && (val != NoSpecInt)) {
      throw SyntaxException("Day of month values must be between 1 and 31");
    }
  }
  else if (type == MONTH) {
    if ((val < 1 || val > 12 || end > 12) && (val != AllSpecInt)) {
      throw SyntaxException("Month values must be between 1 and 12");
    }
  }
  else if (type == DAY_OF_WEEK) {
    if ((val == 0 || val > 7 || end > 7) && (val != AllSpecInt) && (val != NoSpecInt)) {
      throw SyntaxException("Day-of-Week values must be between 1 and 7");
    }
  }

  if ((incr == 0 || incr == -1) && val != AllSpecInt) {
    if (val != -1) {
      data.insert(val);
    }
    else {
      data.insert(NoSpec);
    }
    return;
  }


  int32 start_at = val;
  int32 stop_at = end;

  if (val == AllSpecInt && incr <= 0) {
    incr = 1;
    data.insert(AllSpec); // put in a marker, but also fill values
  }

  if (type == SECOND || type == MINUTE) {
    if (stop_at == -1) {
      stop_at = 59;
    }

    if (start_at == -1 || start_at == AllSpecInt) {
      start_at = 0;
    }
  }
  else if (type == HOUR) {
    if (stop_at == -1) {
      stop_at = 23;
    }

    if (start_at == -1 || start_at == AllSpecInt) {
      start_at = 0;
    }
  }
  else if (type == DAY_OF_MONTH) {
    if (stop_at == -1) {
      stop_at = 31;
    }

    if (start_at == -1 || start_at == AllSpecInt) {
      start_at = 1;
    }
  }
  else if (type == MONTH) {
    if (stop_at == -1) {
      stop_at = 12;
    }

    if (start_at == -1 || start_at == AllSpecInt) {
      start_at = 1;
    }
  }
  else if (type == DAY_OF_WEEK) {
    if (stop_at == -1) {
      stop_at = 7;
    }

    if (start_at == -1 || start_at == AllSpecInt) {
      start_at = 1;
    }
  }
  else if (type == YEAR) {
    if (stop_at == -1) {
      stop_at = MaxYear;
    }

    if (start_at == -1 || start_at == AllSpecInt) {
      start_at = 1970;
    }
  }

  // if the end of the range is before the start, then we need to overflow into
  // the next day, month etc. This is done by adding the maximum amount for that
  // type, and using modulus Max to determine the value being added.
  int32 max = -1;
  if (stop_at < start_at) {
    switch (type) {
    case SECOND: max = 60; break;
    case MINUTE: max = 60; break;
    case HOUR: max = 24; break;
    case MONTH: max = 12; break;
    case DAY_OF_WEEK: max = 7; break;
    case DAY_OF_MONTH: max = 31; break;
    case YEAR: throw InvalidArgumentException("Start year must be less than stop year");
    default: throw InvalidArgumentException("Unexpected type encountered");
    }

    stop_at += max;
  }

  for (int32 i = start_at; i <= stop_at; i += incr) {
    if (max == -1) {
      // ie: there's no Max to overflow over
      data.insert(i);
    }
    else {
      // take the modulus to get the real value
      int32 i2 = i % max;

      // 1-indexed ranges should not include 0, and should include their Max
      if (i2 == 0 && (type == MONTH || type == DAY_OF_WEEK || type == DAY_OF_MONTH)) {
        i2 = max;
      }

      data.insert(i2);
    }
  }
}

std::set<int32>& CronExpression::GetSet(int32 type) {
  switch (type) {
  case SECOND: return seconds_;
  case MINUTE: return minutes_;
  case HOUR: return hours_;
  case DAY_OF_MONTH: return days_of_month_;
  case MONTH: return months_;
  case DAY_OF_WEEK: return days_of_week_;
  case YEAR: return years_;
  }
  fun_unexpected();
}

const std::set<int32>& CronExpression::GetSet(int32 type) const {
  switch (type) {
  case SECOND: return seconds_;
  case MINUTE: return minutes_;
  case HOUR: return hours_;
  case DAY_OF_MONTH: return days_of_month_;
  case MONTH: return months_;
  case DAY_OF_WEEK: return days_of_week_;
  case YEAR: return years_;
  }
  fun_unexpected();
}

std::set<int32> CronExpression::TailSet(const std::set<int32>& set, int32 value) const {
  std::set<int32> result;
  for (const auto& val : set) {
    if (val >= value) {
      result.insert(val);
    }
  }
  return result;
}

bool CronExpression::ContainsInSet(const std::set<int32>& set, int32 value) const {
  return set.find(value) != set.end();
}

/** cron 표현식의 요약 정보를 구함. */
String CronExpression::GetExpressionSummary() const {
  String summary;
  //TODO
  //summary.Append("seconds: ");
  //summary.Append(GetExpressionSetSummary(seconds_));
  //summary.Append("\n");
  //summary.Append("minutes: ");
  //summary.Append(GetExpressionSetSummary(minutes_));
  //summary.Append("\n");
  //summary.Append("hours: ");
  //summary.Append(GetExpressionSetSummary(hours_));
  //summary.Append("\n");
  //summary.Append("days_of_month: ");
  //summary.Append(GetExpressionSetSummary(days_of_month_));
  //summary.Append("\n");
  //summary.Append("months: ");
  //summary.Append(GetExpressionSetSummary(months_));
  //summary.Append("\n");
  //summary.Append("days_of_week: ");
  //summary.Append(GetExpressionSetSummary(days_of_week_));
  //summary.Append("\n");
  //summary.Append("last_day_of_week: ");
  //summary.Append(ToString(last_day_of_week_));
  //summary.Append("\n");
  //summary.Append("nearest_weekday: ");
  //summary.Append(ToString(nearest_weekday_));
  //summary.Append("\n");
  //summary.Append("nth_day_of_week: ");
  //summary.Append(ToString(nth_day_of_week_));
  //summary.Append("\n");
  //summary.Append("last_day_of_month: ");
  //summary.Append(ToString(last_day_of_month_));
  //summary.Append("\n");
  //summary.Append("calendar_day_of_week: ");
  //summary.Append(ToString(calendar_day_of_week_));
  //summary.Append("\n");
  //summary.Append("calendar_day_of_month: ");
  //summary.Append(ToString(calendar_day_of_month_));
  //summary.Append("\n");
  //summary.Append("years: ");
  //summary.Append(GetExpressionSetSummary(years_));
  //summary.Append("\n");
  return summary;
}

/** 주어진 set의 요약 정보를 구함. */
String CronExpression::GetExpressionSetSummary(const std::set<int32>& set) const {
  if (ContainsInSet(set, NoSpec)) {
    return "?";
  }

  if (ContainsInSet(set, AllSpec)) {
    return "*";
  }

  String summary;
  bool first = true;
  for (const auto& value : set) {
    if (!first) {
      summary += ",";
    }
    //summary += String::Format("%d"), value);
    summary += String::FromNumber(value);
    first = false;
  }
  return summary;
}


bool CronExpression::IsSatisfiedBy(const DateTime& at_utc) const {
  //TODO test 값은 왜 안쓰나???
  auto test = at_utc.AddSeconds(-1);
  auto time_after = GetTimeAfter(at_utc);
  if (!time_after.IsNull() && time_after.Value() == at_utc) {
    return true;
  }
  return false;
}

/** 유효한 다음 시간을 구함. */
Nullable<DateTime>
CronExpression::GetNextValidTimeAfter(const DateTime& at_utc) const {
  return GetTimeAfter(at_utc);
}

/** 유효하지 않은 다음 시간을 구함. */
Nullable<DateTime>
CronExpression::GetNextInvalidTimeAfter(const DateTime& at_utc) const {
  auto last_date = at_utc.AddSeconds(-1);

  int64 difference = 1000;
  while (difference == 1000) {
    auto new_date = GetTimeAfter(last_date);
    if (new_date == nullptr) {
      break;
    }

    difference = (int64)(new_date.Value() - last_date).TotalMilliseconds();

    if (difference == 1000) {
      last_date = new_date.Value();
    }
  }

  return last_date.AddSeconds(1);
}

/** 주어진 시간에서 다음으로 이벤트가 발생할 시간을 구하기. */
Nullable<DateTime> CronExpression::GetTimeAfter(const DateTime& in_after_time_utc) const {
  auto after_time_utc = in_after_time_utc.AddSeconds(1);
  after_time_utc = CreateDateTimeWithoutMillis(after_time_utc);

  auto time = TimeZone::ConvertTime(after_time_utc, time_zone_);

  bool got_one = false;
  while (!got_one) {
    std::set<int32> set;
    int32 t;
    int32 second = time.Second();

    // Get second.
    set = TailSet(seconds_, second);
    if (set.size() > 0) {
      second = *set.begin();
    }
    else {
      second = *seconds_.begin();
      time = time.AddMinutes(1);
    }

    time = DateTime(time.Year(), time.Month(), time.Day(), time.Hour(), time.Minute(), second, time.Millisecond(), time.Offset());


    int32 minute = time.Minute();
    int32 hour = time.Hour();
    t = -1;

    // Get minute
    set = TailSet(minutes_, minute);
    if (set.size() > 0) {
      t = minute;
      minute = *set.begin();
    }
    else {
      minute = *minutes_.begin();
      hour++;
    }

    if (minute != t) {
      time = DateTime(time.Year(), time.Month(), time.Day(), time.Hour(), minute, 0, time.Millisecond(), time.Offset());
      time = SetCalendarHour(time, hour);
      continue;
    }

    time = DateTime(time.Year(), time.Month(), time.Day(), time.Hour(), minute, time.Second(), time.Millisecond(), time.Offset());


    hour = time.Hour();
    int32 day = time.Day();
    t = -1;

    // Get hour
    set = TailSet(hours_, hour);
    if (set.size() > 0) {
      t = hour;
      hour = *set.begin();
    }
    else {
      hour = *hours_.begin();
      day++;
    }

    if (hour != t) {
      const int32 days_in_month = Date::DaysInMonth(time.Year(), time.Month());
      if (day > days_in_month) {
        time = DateTime(time.Year(), time.Month(), days_in_month, time.Hour(), 0, 0, time.Millisecond(), time.Offset());
        time.AddDaysInPlace(day - days_in_month);
      }
      else {
        time = DateTime(time.Year(), time.Month(), day, time.Hour(), 0, 0, time.Millisecond(), time.Offset());
      }
      time = SetCalendarHour(time, hour);
      continue;
    }

    time = DateTime(time.Year(), time.Month(), time.Day(), hour, time.Minute(), time.Second(), time.Millisecond(), time.Offset());

    day = time.Day();
    int32 month = time.Month();
    t = -1;
    int32 tmonth = month;

    // Get day
    const bool day_of_month_spec = !ContainsInSet(days_of_month_, NoSpec);
    const bool day_of_week_spec = !ContainsInSet(days_of_week_, NoSpec);
    if (day_of_month_spec && !day_of_week_spec) {
      // get day by day of month rule

      set = TailSet(days_of_month_, day);
      bool found = set.size() != 0;
      if (last_day_of_month_) {
        if (!nearest_weekday_) {
          t = day;
          day = Date::DaysInMonth(time.Year(), month);
          day -= last_day_offset_;

          if (t > day) {
            month++;
            if (month > 12) {
              month = 1;
              tmonth = 3333;
              time.AddYearsInPlace(1);
            }
            day = 1;
          }
        }
        else {
          t = day;
          day = Date::DaysInMonth(time.Year(), month);
          day -= last_day_offset_;

          DateTime tcal(time.Year(), month, day, 0,0,0, time.Offset());
          const int32 last_day_of_month = Date::DaysInMonth(time.Year(), month);
          const DayOfWeekType day_of_week = tcal.DayOfWeek();

          if (day_of_week == DayOfWeekType::Saturday && day == 1) { // 월의 시작임과 동시에 토요일일 경우에는 2일을 건너뛰어야 평일이 될터? (월요일)
            // select monday
            day += 2;
          }
          else if (day_of_week == DayOfWeekType::Saturday) { // 금요일
            // select friday
            day -= 1;
          }
          else if (day_of_week == DayOfWeekType::Sunday && day == last_day_of_month) { // 월의 마지막 날이 일요일인 경우, 이틀전이면 평일 금요일일터...
            // select friday
            day -= 2;
          }
          else if (day_of_week == DayOfWeekType::Sunday) { // 월요일 선택
            // select monday
            day += 1;
          }

          DateTime ntime(tcal.Year(), month, day, hour, minute, second, time.Millisecond(), time.Offset());

          if (ntime.ToUniversalTime() < after_time_utc) {
            day = 1;
            month++;
          }
        }
      }
      else if (nearest_weekday_) {
        t = day;
        day = *days_of_month_.begin();

        DateTime tcal(time.Year(), month, day, 0,0,0, time.Offset());

        const int32 last_day_of_month = Date::DaysInMonth(time.Year(), month);
        const DayOfWeekType day_of_week = tcal.DayOfWeek();

        if (day_of_week == DayOfWeekType::Saturday && day == 1) { // 월의 시작임과 동시에 토요일일 경우에는 2일을 건너뛰어야 평일이 될터? (월요일)
          // select monday
          day += 2;
        }
        else if (day_of_week == DayOfWeekType::Saturday) { // 금요일
          // select friday
          day -= 1;
        }
        else if (day_of_week == DayOfWeekType::Sunday && day == last_day_of_month) { // 월의 마지막 날이 일요일인 경우, 이틀전이면 평일 금요일일터...
          // select friday
          day -= 2;
        }
        else if (day_of_week == DayOfWeekType::Sunday) { // 월요일 선택
          // select monday
          day += 1;
        }

        DateTime ntime(tcal.Year(), month, day, hour, minute, second, time.Millisecond(), time.Offset());

        if (ntime.ToUniversalTime() < after_time_utc) {
          day = *days_of_month_.begin();
          month++;
        }
      }
      else if (found) {
        t = day;
        day = *set.begin();

        // make sure we don't over-run a short month, such as february
        const int32 last_day = Date::DaysInMonth(time.Year(), month);

        if (day > last_day) {
          day = *days_of_month_.begin();
          month++;
        }
      }
      else {
        day = *days_of_month_.begin();
        month++;
      }

      if (day != t || month != tmonth) {
        if (month > 12) {
          time = DateTime(time.Year(), 12, day, 0,0,0, time.Offset());
          time.AddMonthsInPlace(month - 12);
        }
        else {
          // This is to avoid a bug when moving from a month
          // with 30 or 31 days to a month with less. Causes an invalid datetime to be instantiated.
          // ex. 0 29 0 30 1 ? 2009 with clock set to 1/30/2009
          const int32 last_day = Date::DaysInMonth(time.Year(), month);

          if (day <= last_day) {
            time = DateTime(time.Year(), month, day, 0,0,0, time.Offset());
          }
          else {
            time = DateTime(time.Year(), month, last_day, 0,0,0, time.Offset());
            time.AddDaysInPlace(day - last_day);
          }
        }

        continue;
      }
    }
    else if (!day_of_month_spec && day_of_week_spec) {
      // get day by day of week rule
      if (last_day_of_week_) {
        // are we looking for the last XXX day of the month?
        const int32 day_of_week = *days_of_week_.begin(); // desired
        const int32 current_day_of_week = ((int32)time.DayOfWeek() + 1); // Current day of week
        int32 days_to_add = 0;
        if (current_day_of_week < day_of_week) {
          days_to_add = day_of_week - current_day_of_week;
        }
        else if (current_day_of_week > day_of_week) {
          days_to_add = day_of_week + (7 - current_day_of_week);
        }

        const int32 last_day = Date::DaysInMonth(time.Year(), month);

        // Promote month
        if ((day + days_to_add) > last_day) {
          // did we already miss the last one?
          if (month == 12) {
            //will we pass the end of the year?
            time = DateTime(time.Year(), 1, 1, 0,0,0, time.Offset());
            time.AddYearsInPlace(1);
          }
          else {
            time = DateTime(time.Year(), month + 1, 1, 0,0,0, time.Offset());
          }

          // we are promoting the month
          continue;
        }

        // find date of last occurrence of this day in this month.
        while ((day + days_to_add + 7) <= last_day) {
          days_to_add += 7;
        }

        day += days_to_add;

        if (days_to_add > 0) {
          time = DateTime(time.Year(), month, day, 0,0,0, time.Offset());

          // we are not promoting the month.
          continue;
        }
      }
      else if (nth_day_of_week_ != 0) {
        // are we looking for the Nth XXX day in the month?

        const int32 day_of_week = *days_of_week_.begin();
        const int32 current_day_of_week = ((int32)time.DayOfWeek() + 1);
        int32 days_to_add = 0;
        if (current_day_of_week < day_of_week) {
          days_to_add = day_of_week - current_day_of_week;
        }
        else if (current_day_of_week > day_of_week) {
          days_to_add = day_of_week + (7 - current_day_of_week);
        }

        const bool day_shifted = days_to_add > 0;

        day += days_to_add;
        int32 week_of_month = day / 7;
        if (day % 7 != 0) { // If it does not fall by 7 (one week), it increases by one week.
          week_of_month++;
        }

        days_to_add = (nth_day_of_week_ - week_of_month) * 7;
        day += days_to_add;
        if (days_to_add < 0 || day > Date::DaysInMonth(time.Year(), month)) {
          if (month == 12) {
            time = DateTime(time.Year(), 1,1, 0,0,0, time.Offset());
            time.AddYearsInPlace(1);
          }
          else {
            time = DateTime(time.Year(), month+1,1, 0,0,0, time.Offset());
          }

          // we are promoting the month.
          continue;
        }
        else if (days_to_add > 0 || day_shifted) {
          time = DateTime(time.Year(), month, day, 0,0,0, time.Offset());

          // we are NOT promoting the month.
          continue;
        }
      }
      else {
        const int32 current_day_of_week = ((int32)time.DayOfWeek() + 1);
        int32 day_of_week = *days_of_week_.begin(); // desired
        set = TailSet(days_of_week_, current_day_of_week);
        if (set.size() > 0) {
          day_of_week = *set.begin();
        }

        int32 days_to_add = 0;

        if (current_day_of_week < day_of_week) {
          days_to_add = day_of_week - current_day_of_week;
        }
        else if (current_day_of_week > day_of_week) {
          days_to_add = day_of_week + (7 - current_day_of_week);
        }

        const int32 last_day = Date::DaysInMonth(time.Year(), month);

        if ((day + days_to_add) > last_day) {
          // will we pass the end of the month?

          if (month == 12) {
            //will we pass the end of the year?
            time = DateTime(time.Year(),1,1, 0,0,0, time.Offset());
            time.AddYearsInPlace(1);
          }
          else {
            time = DateTime(time.Year(),month+1,1, 0,0,0, time.Offset());
          }

          // we are promoting the month
          continue;
        }
        else if (days_to_add > 0) {
          // are we switching days?
          time = DateTime(time.Year(), month, day + days_to_add, 0,0,0, time.Offset());
          continue;
        }
      }
    }
    else {
      throw Exception("Support for specifying both a day-of-week AND a day-of-month parameter is not implemented.");
    }


    time = DateTime(time.Year(), time.Month(), day, time.Hour(), time.Minute(), time.Second(), time.Millisecond(), time.Offset());
    month = time.Month();
    int32 year = time.Year();
    t = -1;

    if (year > MaxYear) {
      return nullptr;
    }


    // Get month
    set = TailSet(months_, month);
    if (set.size() > 0) {
      t = month;
      month = *set.begin();
    }
    else {
      month = *months_.begin();
      year++;
    }

    if (month != t) {
      time = DateTime(year, month, 1, 0, 0, 0, time.Offset());
      continue;
    }

    time = DateTime(time.Year(), month, time.Day(), time.Hour(), time.Minute(), time.Second(), time.Millisecond(), time.Offset());
    year = time.Year();
    t = -1;


    // Get year
    set = TailSet(years_, year);
    if (set.size() > 0) {
      t = year;
      year = *set.begin();
    }
    else {
      return nullptr;
    }

    if (year != t) {
      time = DateTime(year, 1, 1, 0, 0, 0, time.Offset());
      continue;
    }

    time = DateTime(year, time.Month(), time.Day(), time.Hour(), time.Minute(), time.Second(), time.Millisecond(), time.Offset());

    // Apply the proper offset for this date
    time = DateTime(time.DateTime(), time_zone_.GetOffsetFromUtc(time.DateTime()));

    got_one = true;
  }

  return time.ToUniversalTime();
}

/** cron 표현식에서는 millisecond 단위를 취급하지 않으므로, 오동작을 방지하기 위해서, millisecond 값을 제함. */
DateTime CronExpression::CreateDateTimeWithoutMillis(const DateTime& date) const {
  return DateTime(date.Year(), date.Month(), date.Day(), date.Hour(), date.Minute(), date.Second(), 0, date.Offset());
}

/** 24시에 대한 처리. 시간이 24가 되면 0으로 wrap하고 하루를 증가시킴. */
DateTime CronExpression::SetCalendarHour(const DateTime& date, int32 hour) const {
  if (hour == 24) {
    DateTime result(date.Year(), date.Month(), date.Day(), 0, date.Minute(), date.Second(), date.Millisecond(), date.Offset());
    result.AddDaysInPlace(1);
    return result;
  }
  else {
    return date;
  }
}

} // namespace fun

#endif // FUN_WITH_CRON_EXPRESSION
