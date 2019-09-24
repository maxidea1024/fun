#include "fun/base/base.h"
#include "fun/http/status_code.h"

namespace fun {

/*

  UpdateCounter("server", "item_count", 100);
  UpdateCounter("server", "monster_count", "The number of monsters", 150);

  IncreaseCounterBy("server", "item_count", 1);
  DecreaseCounterBy("server", "item_count", 1);

  int64 item_count = ReadCounterAsInteger("server", "item_count");
  TT_CHECK(item_count == 150);

  UpdateCounter("server", "connection_per_second", 77.7);
  UpdateCounter("billing", "purchase_per_second", 7.1);




[카운터 변화 감지]

  void OnResetGoldCounterTimerExpired(const Timer::Id&, const WallClock::Value&)
  {
    //매크로화 한다면 문자열을 상수화할 수 있을듯...
    UpdateCounter("game", "gold_per_hour", 0);
  }

  void Install()
  {
    UpdateCounter("game", "gold_per_hour", 0);

  }
*/


//TODO Counter 클래스로 묶어준다면 좀더 깔끔해지지 않을까??

typedef Function<http::StatusCode(const String&,const String&,json::JValue*) CounterCallback;

void UpdateCounter(const String& counter_type, const String& counter_id, const int32 value) {
  ScopedLock guard(mutex_);
  CounterGroup* group = FindOrAddGroup(counter_type);
  group->SetValue(counter_id, value);
}

void UpdateCounter(const String& counter_type, const String& counter_id, const int64 value) {
  ScopedLock guard(mutex_);
  CounterGroup* group = FindOrAddGroup(counter_type);
  group->SetValue(counter_id, value);
}

void UpdateCounter(const String& counter_type, const String& counter_id, const double value) {
  ScopedLock guard(mutex_);
  CounterGroup* group = FindOrAddGroup(counter_type);
  group->SetValue(counter_id, value);
}

void UpdateCounter(const String& counter_type, const String& counter_id, const String& value) {
  ScopedLock guard(mutex_);
  CounterGroup* group = FindOrAddGroup(counter_type);
  group->SetValue(counter_id, value);
}


void UpdateCounter(const String& counter_type, const String& counter_id, const String& description, const int32 value) {
  ScopedLock guard(mutex_);
  CounterGroup* group = FindOrAddGroup(counter_type);
  group->SetValue(counter_id, description, value);
}

void UpdateCounter(const String& counter_type, const String& counter_id, const String& description, const int64 value) {
  ScopedLock guard(mutex_);
  CounterGroup* group = FindOrAddGroup(counter_type);
  group->SetValue(counter_id, description, value);
}

void UpdateCounter(const String& counter_type, const String& counter_id, const String& description, const double value) {
  ScopedLock guard(mutex_);
  CounterGroup* group = FindOrAddGroup(counter_type);
  group->SetValue(counter_id, description, value);
}

void UpdateCounter(const String& counter_type, const String& counter_id, const String& description, const String& value) {
  ScopedLock guard(mutex_);
  CounterGroup* group = FindOrAddGroup(counter_type);
  group->SetValue(counter_id, description, value);
}



// 아래 함수는 카운터가 등록되어야함.

void IncreaseCounterBy(const String& counter_type, const String& counter_id, const int32 value);
void DecreaseCounterBy(const String& counter_type, const String& counter_id, const int32 value);
void IncreaseCounterBy(const String& counter_type, const String& counter_id, const int64 value);
void DecreaseCounterBy(const String& counter_type, const String& counter_id, const int64 value);
void IncreaseCounterBy(const String& counter_type, const String& counter_id, const double value);
void DecreaseCounterBy(const String& counter_type, const String& counter_id, const double value);

int64 ReadCounterAsInteger(const String& counter_type, const String& counter_id);
double ReadCounterAsDouble(const String& counter_type, const String& counter_id);
String ReadCounterAsString(const String& counter_type, const String& counter_id);

void RegisterCallableCounter(const String& counter_type, const String& counter_id, const String& desc, const CounterCallback& value_cb);

void MonitorCounter(const String& counter_type, const String& counter_id, double threshold);

} // namespace fun
