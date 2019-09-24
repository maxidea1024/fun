#pragma once

#include "fun/sql/sql.h"
#include "fun/base/dynamic/var_holder.h"
#include "fun/base/exception.h"

namespace fun {

class DateTime;

namespace Dynamic {

class Var;

}

namespace sql {

class Time;

/**
 * Date class wraps a DateTime and exposes date related interface.
 * The purpose of this class is binding/extraction support for date fields.
 */
class FUN_SQL_API Date {
 public:
  /**
   * Creates the Date
   */
  Date();

  /**
   * Creates the Date
   */
  Date(int32 year, int32 month, int32 day);

  /**
   * Creates the Date from DateTime
   */
  Date(const DateTime& dt);

  /**
   * Destroys the Date.
   */
  ~Date();

  /**
   * Returns the year.
   */
  int32 Year() const;

  /**
   * Returns the month.
   */
  int32 Month() const;

  /**
   * Returns the day.
   */
  int32 Day() const;

  /**
   * Assigns date.
   */
  void Assign(int32 year, int32 month, int32 day);

  /**
   * Assignment operator for Date.
   */
  Date& operator = (const Date& d);

  /**
   * Assignment operator for DateTime.
   */
  Date& operator = (const DateTime& dt);

  /**
   * Assignment operator for Var.
   */
  Date& operator = (const fun::dynamic::Var& var);

  /**
   * Equality operator.
   */
  bool operator == (const Date& date) const;

  /**
   * Inequality operator.
   */
  bool operator != (const Date& date) const;

  /**
   * Less then operator.
   */
  bool operator < (const Date& date) const;

  /**
   * Greater then operator.
   */
  bool operator > (const Date& date) const;

 private:
  int32 year_;
  int32 month_;
  int32 day_;
};


//
// inlines
//

inline int32 Date::Year() const {
  return year_;
}

inline int32 Date::Month() const {
  return month_;
}

inline int32 Date::Day() const {
  return day_;
}

inline Date& Date::operator = (const Date& d) {
  Assign(d.Year(), d.Month(), d.Day());
  return *this;
}

inline Date& Date::operator = (const DateTime& dt) {
  Assign(dt.Year(), dt.Month(), dt.Day());
  return *this;
}

inline bool Date::operator == (const Date& date) const {
  return  year_ == date.Year() &&
          month_ == date.Month() &&
          day_ == date.Day();
}

inline bool Date::operator != (const Date& date) const {
  return !(*this == date);
}

inline bool Date::operator > (const Date& date) const {
  return !(*this == date) && !(*this < date);
}

} // namespace sql
} // namespace fun


//
// VarHolderImpl<Date>
//

namespace fun {
namespace Dynamic {

template <>
class VarHolderImpl<fun::sql::Date> : public VarHolder {
 public:
  VarHolderImpl(const fun::sql::Date& val) : val_(val) {}

  ~VarHolderImpl() {}

  const std::type_info& Type() const {
    return typeid(fun::sql::Date);
  }

  void Convert(fun::Timestamp& val) const {
    DateTime dt;
    dt.Assign(val_.Year(), val_.Month(), val_.Day());
    val = dt.timestamp();
  }

  void Convert(fun::DateTime& val) const {
    val.Assign(val_.Year(), val_.Month(), val_.Day());
  }

  void Convert(String& val) const {
    DateTime dt(val_.Year(), val_.Month(), val_.Day());
    val = DateTimeFormatter::Format(dt, "%Y/%m/%d");
  }

  VarHolder* Clone(Placeholder<VarHolder>* var_holder = nullptr) const {
    return CloneHolder(var_holder, val_);
  }

  const fun::sql::Date& Value() const {
    return val_;
  }

  bool IsDate() const {
    return true;
  }

  bool IsTime() const {
    return false;
  }

  bool IsDateTime() const {
    return false;
  }

 private:
  VarHolderImpl();

  fun::sql::Date val_;
};

} // namespace Dynamic
} // namespace fun
