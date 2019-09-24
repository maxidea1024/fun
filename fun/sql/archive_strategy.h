#pragma once

#include "fun/sql/sql.h"
#include "fun/sql/session.h"
#include "fun/base/date_time.h"
#include "fun/Timespan.h"
#include "fun/base/dynamic/var.h"
#include "fun/base/shared_ptr.h"

namespace fun {
namespace sql {

/**
 * The ArchiveStrategy is used by SqlSink to archive log rows.
 */
class FUN_SQL_API ArchiveStrategy {
 public:
  static const String DEFAULT_ARCHIVE_DESTINATION;

  /**
   * Creates archive strategy.
   */
  ArchiveStrategy(const String& connector,
                  const String& connect,
                  const String& source,
                  const String& destination = DEFAULT_ARCHIVE_DESTINATION);

  /**
   * Destroys archive strategy.
   */
  virtual ~ArchiveStrategy();

  /**
   * Opens the session.
   */
  void Open();

  /**
   * Archives the rows.
   */
  virtual void Archive() = 0;

  /**
   * Returns the name of the source table containing rows to be archived.
   */
  const String& GetSource() const;

  /**
   * Sets the name of the source table.
   */
  void SetSource(const String& source);

  /**
   * Returns the name of the destination table for rows to be archived.
   */
  const String& GetDestination() const;

  /**
   * Sets the name of the destination table.
   */
  void SetDestination(const String& destination);

  /**
   * Returns the archive threshold.
   */
  virtual const String& GetThreshold() const = 0;

  /**
   * Sets the archive threshold.
   */
  virtual void SetThreshold(const String& threshold) = 0;

 protected:
  typedef fun::SharedPtr<Session> SessionPtr;
  typedef fun::SharedPtr<Statement> StatementPtr;

  Session& GetSession();

  void SetCopyStatement();
  void SetDeleteStatement();
  void SetCountStatement();

  Statement& GetCopyStatement();
  Statement& GetDeleteStatement();
  Statement& GetCountStatement();

 private:
  ArchiveStrategy() = delete;
  ArchiveStrategy(const ArchiveStrategy&) = delete;
  ArchiveStrategy& operator = (const ArchiveStrategy&) = delete;

  String connector_;
  String connect_;
  SessionPtr session_;
  StatementPtr copy_statement_;
  StatementPtr delete_statement_;
  StatementPtr count_statement_;
  String source_;
  String destination_;
};


//
// inlines
//

inline const String& ArchiveStrategy::GetSource() const {
  return source_;
}

inline void ArchiveStrategy::SetSource(const String& source) {
  source_ = source;
}

inline void ArchiveStrategy::SetDestination(const String& destination) {
  destination_ = destination;
}

inline const String& ArchiveStrategy::GetDestination() const {
  return destination_;
}

inline Session& ArchiveStrategy::GetSession() {
  return *session_;
}

inline void ArchiveStrategy::SetCopyStatement() {
  copy_statement_ = new Statement(*session_);
}

inline void ArchiveStrategy::SetDeleteStatement() {
  delete_statement_ = new Statement(*session_);
}

inline void ArchiveStrategy::SetCountStatement() {
  count_statement_ = new Statement(*session_);
}

inline Statement& ArchiveStrategy::GetCopyStatement() {
  return *copy_statement_;
}

inline Statement& ArchiveStrategy::GetDeleteStatement() {
  return *delete_statement_;
}

inline Statement& ArchiveStrategy::GetCountStatement() {
  return *count_statement_;
}


//
// ArchiveByAgeStrategy
//

/**
 * Archives rows scheduled for archiving.
 */
class FUN_SQL_API ArchiveByAgeStrategy : public ArchiveStrategy {
 public:
  ArchiveByAgeStrategy( const String& connector,
                        const String& connect,
                        const String& source_table,
                        const String& destination_table = DEFAULT_ARCHIVE_DESTINATION);

  ~ArchiveByAgeStrategy();

  void Archive() override;

  /**
   * Returns the archive threshold.
   */
  const String& GetThreshold() const;

  /**
   * Sets the archive threshold.
   */
  void SetThreshold(const String& threshold);

 private:
  void InitStatements();

  Timespan max_age_;
  String age_string_;
  DateTime archive_date_time_;
  fun::dynamic::Var archive_count_;

 public:
  ArchiveByAgeStrategy() = delete;
  ArchiveByAgeStrategy(const ArchiveByAgeStrategy&) = delete;
  ArchiveByAgeStrategy& operator = (const ArchiveByAgeStrategy&) = delete;
};


//
// inlines
//

inline const String& ArchiveByAgeStrategy::GetThreshold() const {
  return age_string_;
}

} // namespace sql
} // namespace fun
