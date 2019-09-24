#include <mysql.h>
#include "fun/sql/mysql/statement_executor.h"
#include "fun/base/format.h"

namespace fun {
namespace sql {
namespace mysql {

StatementExecutor::StatementExecutor(MYSQL* mysql)
  : session_handle_(mysql),
    affected_row_count_(0) {
  if ((handle_ = mysql_stmt_init(mysql)) == 0) {
    throw StatementException("mysql_stmt_init error");
  }

  state_ = STMT_INITED;
}

StatementExecutor::~StatementExecutor() {
  mysql_stmt_close(handle_);
}

int StatementExecutor::GetState() const {
  return state_;
}

void StatementExecutor::Prepare(const String& query) {
  if (state_ >= STMT_COMPILED) {
    state_ = STMT_COMPILED;
    return;
  }

  int rc = mysql_stmt_prepare(handle_, query.c_str(), static_cast<unsigned int>(query.length()));
  if (rc != 0) {
    // retry if connection lost
    int err = mysql_errno(session_handle_);
    if (err == 2006 /* CR_SERVER_GONE_ERROR */ || err == 2013 /* CR_SERVER_LOST */) {
      rc = mysql_stmt_prepare(handle_, query.c_str(), static_cast<unsigned int>(query.length()));
    }
  }
  if (rc != 0) {
    throw StatementException("mysql_stmt_prepare error", handle_, query);
  }

  query_ = query;
  state_ = STMT_COMPILED;
}

void StatementExecutor::BindParams(MYSQL_BIND* params, size_t count) {
  if (state_ < STMT_COMPILED) {
    throw StatementException("Statement is not compiled yet");
  }

  if (count != mysql_stmt_param_count(handle_)) {
    throw StatementException("wrong bind parameters count", 0, query_);
  }

  if (count == 0) {
    return;
  }

  if (mysql_stmt_bind_param(handle_, params) != 0) {
    throw StatementException("mysql_stmt_bind_param() error ", handle_, query_);
  }
}

void StatementExecutor::BindResult(MYSQL_BIND* result) {
  if (state_ < STMT_COMPILED) {
    throw StatementException("Statement is not compiled yet");
  }

  if (mysql_stmt_bind_result(handle_, result) != 0) {
    throw StatementException("mysql_stmt_bind_result error ", handle_, query_);
  }
}

void StatementExecutor::Execute() {
  if (state_ < STMT_COMPILED) {
    throw StatementException("Statement is not compiled yet");
  }

  if (mysql_stmt_execute(handle_) != 0) {
    throw StatementException("mysql_stmt_execute error", handle_, query_);
  }

  state_ = STMT_EXECUTED;

  my_ulonglong affected_row_count = mysql_affected_rows(session_handle_);
  if (affected_row_count != ((my_ulonglong)-1)) {
    affected_row_count_ = static_cast<int>(affected_row_count); // Was really a DELETE, UPDATE or INSERT statement
  }
}

bool StatementExecutor::Fetch() {
  if (state_ < STMT_EXECUTED) {
    throw StatementException("Statement is not executed yet");
  }

  int res = mysql_stmt_fetch(handle_);

  // we have specified zero buffers for BLOBs, so DATA_TRUNCATED is normal in this case
  if ((res != 0) && (res != MYSQL_NO_DATA) && (res != MYSQL_DATA_TRUNCATED)) {
    throw StatementException("mysql_stmt_fetch error", handle_, query_);
  }

  return (res == 0) || (res == MYSQL_DATA_TRUNCATED);
}

bool StatementExecutor::FetchColumn(size_t n, MYSQL_BIND* bind) {
  if (state_ < STMT_EXECUTED) {
    throw StatementException("Statement is not executed yet");
  }

  int res = mysql_stmt_fetch_column(handle_, bind, static_cast<unsigned int>(n), 0);

  if ((res != 0) && (res != MYSQL_NO_DATA)) {
    throw StatementException(fun::Format("mysql_stmt_fetch_column(%z) error", n), handle_, query_);
  }

  return (res == 0);
}

int StatementExecutor::AffectedRowCount() const {
  return affected_row_count_;
}

} // namespace mysql
} // namespace sql
} // namespace fun
