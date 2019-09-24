#pragma once

#include "fun/sql/sql.h"
#include "fun/sql/connector.h"
#include "fun/sql/session.h"
#include "fun/sql/statement.h"
#include "fun/sql/archive_strategy.h"
#include "fun/log_sink.h"
#include "fun/logging/log_message.h"
#include "fun/ref_counted_ptr.h"
#include "fun/base/string.h"

namespace fun {
namespace sql {

/**
 * This LogSink implements logging to a sql database.
 * The sink is dependent on the schema. The DDL for
 * table creation (subject to target DDL dialect dependent
 * modifications) is:
 *
 * "CREATE TABLE FUN_LOG (Source VARCHAR,
 *        Name VARCHAR,
 *        ProcessId INTEGER,
 *        Thread VARCHAR,
 *        ThreadId INTEGER,
 *        Priority INTEGER,
 *        Text VARCHAR,
 *        DateTime DATE)"
 *
 * The table name is configurable through "table" property.
 * Other than DateTime filed name used for optional time-based archiving purposes, currently the
 * field names are not mandated. However, it is recommended to use names as specified above.
 *
 * To provide as non-intrusive operation as possible, the log entries are cached and
 * inserted into the target database asynchronously by default . The blocking, however, will occur
 * before the next entry insertion with default timeout of 1 second. The default settings can be
 * overriden (see async, timeout and throw properties for details).
 * If throw property is false, insertion timeouts are ignored, otherwise a TimeoutException is thrown.
 * To force insertion of every entry, set timeout to 0. This setting, however, introduces
 * a risk of long blocking periods in case of remote server communication delays.
 */
class FUN_SQL_API SqlSink : public fun::Logsink {
 public:
  typedef RefCountedPtr<SqlSink> Ptr;

  /**
   * Creates SqlSink.
   */
  SqlSink();

  /**
   * Creates a SqlSink with the given connector, connect string, timeout, table and name.
   * The connector must be already registered.
   */
  SqlSink(const String& connector, const String& connect, const String& name = "-");

  /**
   * Opens the SqlSink.
   */
  void Open() override;

  /**
   * Closes the SqlSink.
   */
  void Close() override;

  /**
   * Sends the message's text to the syslog service.
   */
  void Log(const LogMessage& msg) override;

  /**
   * Sets the property with the given value.
   *
   * The following properties are supported:
   *     * name:      The name used to identify the source of log messages.
   *                  Defaults to "-".
   *
   *     * connector: The target data storage connector name ("SQLite", "ODBC", ...).
   *
   *     * connect:   The target data storage connection string.
   *
   *     * table:     Destination log table name. Defaults to "FUN_LOG".
   *                  Table must exist in the target database.
   *
   *     * keep:      Max row age for the log table. To disable archiving,
   *                  set this property to empty string or "forever".
   *
   *     * archive:   Archive table name. Defaults to "FUN_LOG_ARCHIVE".
   *                  Table must exist in the target database. To disable archiving,
   *                  set this property to empty string.
   *
   *     * async:     Indicates asynchronous execution. When executing asynchronously,
   *                  messages are sent to the target using asynchronous execution.
   *                  However, prior to the next message being processed and sent to
   *                  the target, the previous operation must have been either completed
   *                  or timed out (see timeout and throw properties for details on
   *                  how abnormal conditions are handled).
   *
   *     * timeout:   Timeout (ms) to wait for previous log operation completion.
   *                  Values "0" and "" mean no timeout. Only valid when logging
   *                  is asynchronous, otherwise ignored.
   *
   *     * throw:     Boolean value indicating whether to throw in case of timeout.
   *                  Setting this property to false may result in log entries being lost.
   *                  True values are (case insensitive) "true", "t", "yes", "y".
   *                  Anything else yields false.
   */
  void SetProperty(const String& name, const String& value) override;

  /**
   * Returns the value of the property with the given name.
   */
  String GetProperty(const String& name) const override;

  /**
   * Waits for the completion of the previous operation and returns
   * the result. If channel is in synchronous mode, returns 0 immediately.
   */
  size_t Wait();

  /**
   * Registers the channel with the global LoggingFactory.
   */
  static void RegisterSink();

  static const String PROP_CONNECT;
  static const String PROP_CONNECTOR;
  static const String PROP_NAME;
  static const String PROP_TABLE;
  static const String PROP_ARCHIVE_TABLE;
  static const String PROP_MAX_AGE;
  static const String PROP_ASYNC;
  static const String PROP_TIMEOUT;
  static const String PROP_THROW;

 protected:
  ~SqlSink();

 private:
  typedef fun::SharedPtr<Session> SessionPtr;
  typedef fun::SharedPtr<Statement> StatementPtr;
  typedef fun::LogMessage::Priority Priority;
  typedef fun::SharedPtr<ArchiveStrategy> StrategyPtr;

  /**
   * Initializes the log statement.
   */
  void InitLogStatement();

  /**
   * Initializes the archive statement.
   */
  void InitArchiveStatements();

  /**
   * Waits for previous operation completion and
   * calls LogSync(). If the previous operation times out,
   * and throw_ is true, TimeoutException is thrown, otherwise
   * the timeout is ignored and log entry is lost.
   */
  void LogAsync(const LogMessage& msg);

  /**
   * Inserts the message in the target database.
   */
  void LogSync(const LogMessage& msg);

  /**
   * Returns true is value is "true", "t", "yes" or "y".
   * Case insensitive.
   */
  bool IsTrue(const String& value) const;

  String connector_;
  String connect_;
  SessionPtr session_;
  StatementPtr log_statement_;
  String name_;
  String table_;
  int timeout_;
  bool throw_;
  bool async_;

  // members for log entry cache (needed for async mode)
  String source_;
  long pid_;
  String thread_;
  long tid_;
  int level_;
  String text_;
  DateTime datetime_;
  StrategyPtr archive_strategy_;
};


//
// inlines
//

inline size_t SqlSink::Wait() {
  if (async_ && log_statement_) {
    return log_statement_->Wait(timeout_);
  }

  return 0;
}

inline bool SqlSink::IsTrue(const String& value) const {
  return ((0 == icompare(value, "true")) ||
          (0 == icompare(value, "t")) ||
          (0 == icompare(value, "yes")) ||
          (0 == icompare(value, "y")));
}

} // namespace sql
} // namespace fun
