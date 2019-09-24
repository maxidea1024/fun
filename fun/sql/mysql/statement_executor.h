#pragma once

#include <mysql.h>
#include "fun/sql/mysql/mysql_exception.h"

namespace fun {
namespace sql {
namespace mysql {

/**
 * MySQL statement executor.
 */
class StatementExecutor {
 public:
  enum State { STMT_INITED, STMT_COMPILED, STMT_EXECUTED };

  /**
   * Creates the StatementExecutor.
   */
  explicit StatementExecutor(MYSQL* mysql);

  /**
   * Destroys the StatementExecutor.
   */
  ~StatementExecutor();

  /**
   * Returns the current state.
   */
  int GetState() const;

  /**
   * Prepares the statement for execution.
   */
  void Prepare(const String& query);

  /**
   * Binds the params.
   */
  void BindParams(MYSQL_BIND* params, size_t count);

  /**
   * Binds result.
   */
  void BindResult(MYSQL_BIND* result);

  /**
   * Executes the statement.
   */
  void Execute();

  /**
   * Fetches the data.
   */
  bool Fetch();

  /**
   * Fetches the column.
   */
  bool FetchColumn(size_t n, MYSQL_BIND* bind);

  /**
   * Returns number of affected rows.
   */
  int AffectedRowCount() const;

  /**
   * Cast operator to native handle type.
   */
  operator MYSQL_STMT*();

 private:
  MYSQL* session_handle_;
  MYSQL_STMT* handle_;
  int state_;
  int affected_row_count_;
  String query_;

 public:
  StatementExecutor(const StatementExecutor&) = delete;
  StatementExecutor& operator=(const StatementExecutor&) = delete;
};

//
// inlines
//

inline StatementExecutor::operator MYSQL_STMT*() { return handle_; }

}  // namespace mysql
}  // namespace sql
}  // namespace fun
