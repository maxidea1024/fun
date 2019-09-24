#pragma once

#include "fun/base/dynamic/var_holder.h"
#include "fun/base/exception.h"
#include "fun/sql/sql.h"

namespace fun {

namespace Dynamic {

class Var;

}  // namespace Dynamic

class DateTime;

namespace sql {

class Date;

/**
 * Time class wraps a DateTime and exposes time related interface.
 * The purpose of this class is binding/extraction support for time fields.
 */
class FUN_SQL_API Time {
 public:
  /**
   * Creates the Time
   */
  Time();

  /**
   * Creates the Time
   */
  Time(int32 hour, int32 minute, int32 second);

  /**
   * Creates the Time from DateTime
  Time(const DateTime& dt);

  /**
   * Destroys the Time.
   */
  ~Time();

  /**
   * Returns the hour.
   */
  int32 Hour() const;

  /**
   * Returns the minute.
   */
  int32 Minute() const;

  /**
   * Returns the second.
   */
  int32 Second() const;

  /**
   * Assigns time.
   */
  void Assign(int32 hour, int32 minute, int32 second);

  /**
   * Assignment operator for Time.
   */
  Time& operator=(const Time& t);

  /**
   * Assignment operator for DateTime.
   */
  Time& operator=(const DateTime& dt);

  /**
   * Assignment operator for Var.
   */
  Time& operator=(const fun::dynamic::Var& var);

  /**
   * Equality operator.
   */
  bool operator==(const Time& time) const;

  /**
   * Inequality operator.
   */
  bool operator!=(const Time& time) const;

  /**
   * Less then operator.
   */
  bool operator<(const Time& time) const;

  /**
   * Greater then operator.
   */
  bool operator>(const Time& time) const;

 private:
  int32 hour_;
  int32 minute_;
  int32 second_;
};

//
// inlines
//

inline int32 Time::Hour() const { return hour_; }

inline int32 Time::Minute() const { return minute_; }

inline int32 Time::Second() const { return second_; }

inline Time& Time::operator=(const Time& t) {
  Assign(t.Hour(), t.Minute(), t.Second());
  return *this;
}

inline Time& Time::operator=(const DateTime& dt) {
  Assign(dt.Hour(), dt.Minute(), dt.Second());
  return *this;
}

inline bool Time::operator==(const Time& time) const {
  return hour_ == time.Hour() && minute_ == time.Minute() &&
         second_ == time.Second();
}

inline bool Time::operator!=(const Time& time) const {
  return !(*this == time);
}

inline bool Time::operator>(const Time& time) const {
  return !(*this == time) && !(*this < time);
}

}  // namespace sql
}  // namespace fun

//
// VarHolderImpl<Time>
//

namespace fun {
namespace Dynamic {

template <>
class VarHolderImpl<fun::sql::Time> : public VarHolder {
 public:
  VarHolderImpl(const fun::sql::Time& val) : val_(val) {}
  ~VarHolderImpl() {}

  const std::type_info& Type() const { return typeid(fun::sql::Time); }

  void Convert(fun::Timestamp& val) const {
    fun::DateTime dt;
    dt.Assign(dt.Year(), dt.Month(), dt.Day(), val_.Hour(), val_.Minute(),
              val_.Second());
    val = dt.Timestamp();
  }

  void Convert(fun::DateTime& val) const {
    fun::DateTime dt;
    dt.Assign(dt.Year(), dt.Month(), dt.Day(), val_.Hour(), val_.Minute(),
              val_.Second());
    val = dt;
  }

  void Convert(String& val) const {
    DateTime dt(0, 1, 1, val_.Hour(), val_.Minute(), val_.Second());
    val = DateTimeFormatter::Format(dt, "%H:%M:%S");
  }

  VarHolder* Clone(Placeholder<VarHolder>* var_holder = nullptr) const {
    return CloneHolder(var_holder, val_);
  }

  const fun::sql::Time& Value() const { return val_; }

  bool IsDate() const { return false; }

  bool IsTime() const { return true; }

  bool IsDateTime() const { return false; }

 private:
  VarHolderImpl();
  fun::sql::Time val_;
};

}  // namespace Dynamic
}  // namespace fun
