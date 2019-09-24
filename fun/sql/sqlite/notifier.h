#pragma once

#include "fun/sql/sqlite/sqlite.h"
#include "fun/sql/sqlite/utility.h"
#include "fun/sql/session.h"
#include "fun/base/mutex.h"
#include "fun/base/types.h"
#include "fun/base/dynamic/var.h"
#include "fun/base/basic_event.h"
#include <map>

namespace fun {
namespace sql {
namespace sqlite {

/**
 * Notifier is a wrapper for SQLite callback calls. It supports event callbacks
 * for insert, update, delete, commit and rollback events. While (un)registering
 * callbacks is thread-safe, execution of the callbacks themselves are not;
 * it is the user's responsibility to ensure the thread-safey of the functions
 * they provide as callback target. Additionally, commit callbacks may prevent
 * database transactions from succeeding (see SqliteCommitCallbackFn documentation
 * for details).
 *
 * There can be only one set of callbacks per session (i.e. registering a new
 * callback automatically unregisters the previous one). All callbacks are
 * registered and enabled at Notifier construction time and can be disabled
 * at a later point time.
 */
class FUN_SQLITE_API Notifier {
 public:
  /**
   * A type definition for events-enabled bitmap.
   */
  typedef unsigned char EnabledEventType;

  typedef fun::BasicEvent<void> Event;

  //
  // Events
  //
  Event update;
  Event insert;
  Event erase;
  Event commit;
  Event rollback;

  // Event types.
  static const EnabledEventType SQLITE_NOTIFY_UPDATE   = 1;
  static const EnabledEventType SQLITE_NOTIFY_COMMIT   = 2;
  static const EnabledEventType SQLITE_NOTIFY_ROLLBACK = 4;

  /**
   * Creates a Notifier and enables all callbacks.
   */
  Notifier( const Session& session,
            EnabledEventType enabled = SQLITE_NOTIFY_UPDATE | SQLITE_NOTIFY_COMMIT | SQLITE_NOTIFY_ROLLBACK);

  /**
   * Creates a Notifier, assigns the value to the internal storage and and enables all callbacks.
   */
  Notifier( const Session& session,
            const Any& value,
            EnabledEventType enabled = SQLITE_NOTIFY_UPDATE | SQLITE_NOTIFY_COMMIT | SQLITE_NOTIFY_ROLLBACK);

  /**
   * Disables all callbacks and destroys the Notifier.
   */
  ~Notifier();

  /**
   * Enables update callbacks.
   */
  bool EnableUpdate();

  /**
   * Disables update callbacks.
   */
  bool DisableUpdate();

  /**
   * Returns true if update callbacks are enabled, false otherwise.
   */
  bool UpdateEnabled() const;

  /**
   * Enables commit callbacks.
   */
  bool EnableCommit();

  /**
   * Disables commit callbacks.
   */
  bool DisableCommit();

  /**
   * Returns true if update callbacks are enabled, false otherwise.
   */
  bool CommitEnabled() const;

  /**
   * Enables rollback callbacks.
   */
  bool EnableRollback();

  /**
   * Disables rollback callbacks.
   */
  bool DisableRollback();

  /**
   * Returns true if rollback callbacks are enabled, false otherwise.
   */
  bool RollbackEnabled() const;

  /**
   * Enables all callbacks.
   */
  bool EnableAll();

  /**
   * Disables all callbacks.
   */
  bool DisableAll();

  /**
   * Update callback event dispatcher. Determines the type of the event, updates the row number
   * and triggers the event.
   */
  static void SqliteUpdateCallbackFn(void* pVal, int opCode, const char* db, const char* pTable, int64 row);

  /**
   * Commit callback event dispatcher. If an exception occurs, it is caught inside this function,
   * non-zero value is returned, which causes SQLite engine to turn commit into a rollback.
   * Therefore, callers should check for return value - if it is zero, callback completed successfully
   * and transaction was committed.
   */
  static int SqliteCommitCallbackFn(void* pVal);

  /**
   * Rollback callback event dispatcher.
   */
  static void SqliteRollbackCallbackFn(void* pVal);

  /**
   * Equality operator. Compares value, row and database handles and
   * returns true if all are equal.
   */
  bool operator == (const Notifier& other) const;

  /**
   * Returns the row number.
   */
  int64 GetRow() const;

  /**
   * Sets the row number.
   */
  void SetRow(int64 row);

  /**
   * Returns the table name.
   */
  const String& GetTable() const;

  /**
   * Returns the value.
   */
  const fun::dynamic::Var& GetValue() const;

  /**
   * Sets the value.
   */
  template <typename T>
  inline void SetValue(const T& val) {
    value_ = val;
  }

 private:
  Notifier();
  Notifier(const Notifier&);
  Notifier& operator=(const Notifier&);

  const Session& session_;
  fun::dynamic::Var value_;
  int64 row_;
  String table_;
  EnabledEventType enabled_events_;
  fun::Mutex mutex_;
};


//
// inlines
//

inline bool Notifier::operator == (const Notifier& other) const {
  return  value_ == other.value_ &&
          row_ == other.row_ &&
          table_ == other.table_ &&
          Utility::GetDbHandle(session_) == Utility::GetDbHandle(other.session_);
}

inline int64 Notifier::GetRow() const {
  return row_;
}

inline const String& Notifier::GetTable() const {
  return table_;
}

inline void Notifier::SetRow(int64 row) {
  row_ = row;
}

inline const fun::dynamic::Var& Notifier::GetValue() const {
  return value_;
}

} // namespace sqlite
} // namespace sql
} // namespace fun
