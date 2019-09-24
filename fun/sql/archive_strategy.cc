#include "fun/Ascii.h"
#include "fun/sql/ArchiveStrategy.h"

namespace fun {
namespace sql {

using namespace Keywords;

//
// ArchiveStrategy
//

const String ArchiveStrategy::DEFAULT_ARCHIVE_DESTINATION = "FUN_LOG_ARCHIVE";

ArchiveStrategy::ArchiveStrategy(const String& connector, const String& connect,
                                 const String& source,
                                 const String& destination)
    : connector_(connector),
      connect_(connect),
      source_(source),
      destination_(destination) {
  Open();
}

ArchiveStrategy::~ArchiveStrategy() {}

void ArchiveStrategy::Open() {
  if (connector_.IsEmpty() || connect_.IsEmpty()) {
    throw IllegalStateException(
        "Connector and connect string must be non-empty.");
  }

  session_ = new Session(connector_, connect_);
}

//
// ArchiveByAgeStrategy
//

ArchiveByAgeStrategy::ArchiveByAgeStrategy(const String& connector,
                                           const String& connect,
                                           const String& source_table,
                                           const String& destination_table)
    : ArchiveStrategy(connector, connect, source_table, destination_table) {
  InitStatements();
}

ArchiveByAgeStrategy::~ArchiveByAgeStrategy() {}

void ArchiveByAgeStrategy::Archive() {
  if (!GetSession().IsConnected()) {
    Open();
  }

  DateTime now;
  archive_date_time_ = now - max_age_;
  GetCountStatement().Execute();
  if (archive_count_ > 0) {
    GetCopyStatement().Execute();
    GetDeleteStatement().Execute();
  }
}

void ArchiveByAgeStrategy::InitStatements() {
  String src = GetSource();
  String dest = GetDestination();

  SetCountStatement();
  archive_count_ = 0;
  String sql;
  fun::Format(sql, "SELECT COUNT(*) FROM %s WHERE DateTime < ?", src);
  GetCountStatement() << sql, into(archive_count_), use(archive_date_time_);

  SetCopyStatement();
  sql.Clear();
  fun::Format(sql, "INSERT INTO %s SELECT * FROM %s WHERE DateTime < ?", dest,
              src);
  GetCopyStatement() << sql, use(archive_date_time_);

  SetDeleteStatement();
  sql.Clear();
  fun::Format(sql, "DELETE FROM %s WHERE DateTime < ?", src);
  GetDeleteStatement() << sql, use(archive_date_time_);
}

void ArchiveByAgeStrategy::SetThreshold(const String& age) {
  String::const_iterator it = age.begin();
  String::const_iterator end = age.end();
  int n = 0;
  while (it != end && Ascii::isSpace(*it)) ++it;
  while (it != end && Ascii::IsDigit(*it)) {
    n *= 10;
    n += *it++ - '0';
  }
  while (it != end && Ascii::isSpace(*it)) ++it;
  String unit;
  while (it != end && Ascii::isAlpha(*it)) unit += *it++;

  Timespan::TimeDiff factor = Timespan::SECONDS;
  if (unit == "minutes") {
    factor = Timespan::MINUTES;
  } else if (unit == "hours") {
    factor = Timespan::HOURS;
  } else if (unit == "days") {
    factor = Timespan::DAYS;
  } else if (unit == "weeks") {
    factor = 7 * Timespan::DAYS;
  } else if (unit == "months") {
    factor = 30 * Timespan::DAYS;
  } else if (unit != "seconds") {
    throw InvalidArgumentException("setMaxAge", age);
  }

  max_age_ = factor * n;
}

}  // namespace sql
}  // namespace fun
