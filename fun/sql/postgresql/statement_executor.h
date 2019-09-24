#pragma once

#include "fun/sql/postgresql/postgresql_exception.h"
#include "fun/sql/postgresql/postgresql_types.h"
#include "fun/sql/postgresql/session_handle.h"
#include "fun/sql/meta_column.h"

#include <libpq-fe.h>

#include <string>
#include <vector>

namespace fun {
namespace sql {
namespace postgresql {

/**
 * PostgreSQL statement executor.
 */
class StatementExecutor {
 public:
  enum State {
    STMT_INITED,
    STMT_COMPILED,
    STMT_EXECUTED
  };

  /**
   * Creates the StatementExecutor.
   */
  explicit StatementExecutor(SessionHandle& handle);

  /**
   * Destroys the StatementExecutor.
   */
  ~StatementExecutor();

  /**
   * Returns the current state.
   */
  State GetState() const;

  /**
   * Prepares the statement for execution.
   */
  void Prepare(const String& sql_statement);

  /**
   * Binds the params - REQUIRED if the statement has input parameters/placeholders
   * Pointer and list elements must stay valid for the lifetime of the StatementExecutor!
   */
  void BindParams(const InputParameterVector& input_parameter_vector);

  /**
   * Binds the params ONLY for COPY IN feature of PostgreSQL
   * Pointer and list elements must stay valid for the lifetime of the StatementExecutor!
   */
  void BindBulkParams(const InputParameterVector& input_bulk_parameter_vector);

  /**
   * Executes the statement.
   */
  void Execute();

  /**
   * Fetches the data for the current row
   */
  bool Fetch();

  /**
   * get the count of rows affected by the statement
   */
  size_t AffectedRowCount() const;

  /**
   * get the count of columns returned by the statement
   */
  size_t ReturnedColumnCount() const;

  /**
   * Returns the reference to the specified metacolumn - 0 based
   */
  const MetaColumn& MetaColumnAt(size_t position) const;

  /**
   * Returns the reference to the specified result - 0 based
   */
  const OutputParameter& ResultColumnAt(size_t position) const;

  /**
   * Cast operator to native result handle type.
   */
  operator PGresult* ();

 private:
  void clearResults();

  StatementExecutor(const StatementExecutor&) = delete;
  StatementExecutor& operator= (const StatementExecutor&) = delete;

 private:
  typedef std::vector<MetaColumn> ColVec;

  SessionHandle& session_handle_;
  State state_;
  PGresult* result_handle_;
  String sql_statement_;
  String prepared_statement_name_; // UUID based to allow multiple prepared statements per transaction.
  size_t count_placeholders_in_sql_statement_;
  ColVec result_columns_;

  InputParameterVector input_bulk_parameter_vector_;

  InputParameterVector  input_parameter_vector_;
  OutputParameterVector output_parameter_vector_;
  size_t current_row_; // current row of the result
  size_t affected_row_count_;
};


//
// inlines
//

inline StatementExecutor::operator PGresult* () {
  return result_handle_;
}

} // namespace postgresql
} // namespace sql
} // namespace fun
