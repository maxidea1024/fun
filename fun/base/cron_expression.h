#pragma once

#define FUN_WITH_CRON_EXPRESSION  0

#if FUN_WITH_CRON_EXPRESSION

#include <set>
#include <regex>

#include "fun/base/base.h"
#include "fun/base/date_time.h"
#include "fun/base/time_zone.h"
#include "fun/base/nullable.h"
#include "fun/base/string/string.h"
#include "fun/base/container/map.h"

namespace fun {

class FUN_BASE_API CronExpression {
 public:
  static const int32 SECOND = 0;
  static const int32 MINUTE = 1;
  static const int32 HOUR = 2;
  static const int32 DAY_OF_MONTH = 3;
  static const int32 MONTH = 4;
  static const int32 DAY_OF_WEEK = 5;
  static const int32 YEAR = 6;

  static const int32 AllSpecInt = 99; // '*'
  static const int32 NoSpecInt = 98; // '?'
  static const int32 AllSpec = AllSpecInt;
  static const int32 NoSpec = NoSpecInt;

  static Map<String, int32> month_map_;
  static Map<String, int32> day_map_;

  String cron_expression_string_;
  TimeZone time_zone_;

  struct ValueSet {
    int32 value;
    int32 pos;
  };

  //순서가 중요하기 때문에 stl의 set을 사용해야함.
  //ordered_set을 하나 만들면 좋으련만...
  std::set<int32> seconds_;
  std::set<int32> minutes_;
  std::set<int32> hours_;
  std::set<int32> days_of_month_;
  std::set<int32> months_;
  std::set<int32> days_of_week_;
  std::set<int32> years_;

  bool last_day_of_week_;
  int32 nth_day_of_week_;
  bool last_day_of_month_;
  bool nearest_weekday_; // 가까운 평일이어하는지 여부
  int32 last_day_offset_;
  bool calendar_day_of_week_;
  bool calendar_day_of_month_;
  bool expression_parsed_;

  static const int32 MaxYear = 2299;

  CronExpression();
  CronExpression(const String& cron_expression);

  void Init();
  void BuildExpression(const String& expression);
  int32 StoreExpressionValues(int32 pos, const String& s, int32 type);
  void CheckIncrementRange(int32 incr, int32 type) const;
  int32 CheckNext(int32 pos, const String& s, int32 val, int32 type);
  int32 SkipWhiteSpace(int32 i, const String& s) const;
  int32 FindNextWhiteSpace(int32 i, const String& s) const;

  ValueSet GetValue(int32 v, const String& s, int32 i) const;
  int32 GetNumericValue(const String& s, int32 i) const;
  int32 GetMonthNumber(const String& s) const;
  int32 GetDayOfWeekNumber(const String& s) const;
  void AddToSet(int32 val, int32 end, int32 incr, int32 type);
  std::set<int32>& GetSet(int32 type);
  const std::set<int32>& GetSet(int32 type) const;
  std::set<int32> TailSet(const std::set<int32>& set, int32 value) const;
  bool ContainsInSet(const std::set<int32>& set, int32 value) const;
  String GetExpressionSummary() const;
  String GetExpressionSetSummary(const std::set<int32>& set) const;
  bool IsSatisfiedBy(const DateTime& at_utc) const;
  Nullable<DateTime> GetNextValidTimeAfter(const DateTime& at_utc) const;
  Nullable<DateTime> GetNextInvalidTimeAfter(const DateTime& at_utc) const;
  Nullable<DateTime> GetTimeAfter(const DateTime& after_time_utc) const;
  DateTime CreateDateTimeWithoutMillis(const DateTime& date) const;
  DateTime SetCalendarHour(const DateTime& date, int32 hour) const;
};

} // namespace fun

#endif // FUN_WITH_CRON_EXPRESSION
