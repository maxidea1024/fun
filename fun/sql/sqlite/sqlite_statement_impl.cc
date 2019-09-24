#include "fun/sql/sqlite/sqlite_statement_impl.h"
#include <cstdlib>
#include <cstring>
#include "fun/base/string.h"
#include "fun/sql/sqlite/sqlite_exception.h"
#include "fun/sql/sqlite/utility.h"

#if defined(FUN_UNBUNDLED)
#include <sqlite3.h>
#else
#include "sqlite3.h"
#endif

namespace fun {
namespace sql {
namespace sqlite {

const int SQLiteStatementImpl::FUN_SQLITE_INV_ROW_CNT = -1;

SQLiteStatementImpl::SQLiteStatementImpl(fun::sql::SessionImpl& session,
                                         sqlite3* db)
    : StatementImpl(session),
      db_(db),
      stmt_(0),
      step_called_(false),
      next_response_(0),
      affected_row_count_(FUN_SQLITE_INV_ROW_CNT),
      can_bind_(false),
      is_extracted_(false),
      can_compile_(true) {
  columns_.Resize(1);
}

SQLiteStatementImpl::~SQLiteStatementImpl() {
  try {
    Clear();
  } catch (...) {
    fun_unexpected();
  }
}

void SQLiteStatementImpl::CompileImpl() {
  if (!leftover_) {
    bind_begin_ = bindings().begin();
  }

  String statement(ToString());

  sqlite3_stmt* stmt = 0;
  const char* pSql = leftover_ ? leftover_->c_str() : statement.c_str();

  if (0 == std::strlen(pSql)) {
    throw InvalidSQLStatementException("Empty statements are illegal");
  }

  int rc = SQLITE_OK;
  const char* pLeftover = 0;
  bool queryFound = false;

  do {
    rc = sqlite3_prepare_v2(db_, pSql, -1, &stmt, &pLeftover);
    if (rc != SQLITE_OK) {
      if (stmt) sqlite3_finalize(stmt);
      stmt = 0;
      String errMsg = sqlite3_errmsg(db_);
      Utility::ThrowException(db_, rc, errMsg);
    } else if (rc == SQLITE_OK && stmt) {
      queryFound = true;
    } else if (rc == SQLITE_OK && !stmt) {  // comment/whitespace ignore
      pSql = pLeftover;
      if (std::strlen(pSql) == 0) {
        // empty statement or an conditional statement! like CREATE IF NOT
        // EXISTS this is valid
        queryFound = true;
      }
    }
  } while (rc == SQLITE_OK && !stmt && !queryFound);

  // Finalization call in Clear() invalidates the pointer, so the value is
  // remembered here. For last statement in a batch (or a single statement),
  // pLeftover == "", so the next call
  // to CompileImpl() shall return false immediately when there are no more
  // statements left.
  String leftOver(pLeftover);
  trimInPlace(leftOver);
  Clear();
  stmt_ = stmt;
  if (!leftOver.IsEmpty()) {
    leftover_ = new String(leftOver);
    can_compile_ = true;
  } else
    can_compile_ = false;

  binder_ = new Binder(stmt_);
  extractor_ = new Extractor(stmt_);

  if (SQLITE_DONE == next_response_ && is_extracted_) {
    // if this is not the first compile and there has already been extraction
    // during previous step, switch to the next set if there is one provided
    if (HasMoreDataSets()) {
      ActivateNextDataSet();
      is_extracted_ = false;
    }
  }

  int col_count = sqlite3_column_count(stmt_);

  if (col_count) {
    size_t curDataSet = currentDataSet();
    if (curDataSet >= columns_.size()) columns_.Resize(curDataSet + 1);
    for (int i = 0; i < col_count; ++i) {
      MetaColumn mc(i, sqlite3_column_name(stmt_, i),
                    Utility::GetColumnType(stmt_, i));
      columns_[curDataSet].push_back(mc);
    }
  }
}

void SQLiteStatementImpl::BindImpl() {
  step_called_ = false;
  next_response_ = 0;
  if (stmt_ == 0) {
    return;
  }

  sqlite3_reset(stmt_);

  int param_count = sqlite3_bind_parameter_count(stmt_);
  if (0 == param_count) {
    can_bind_ = false;
    return;
  }

  BindIt bind_end = bindings().end();
  size_t available_count = 0;
  Bindings::difference_type bind_count = 0;
  Bindings::iterator it = bind_begin_;
  for (; it != bind_end; ++it) {
    available_count += (*it)->HandledColumnsCount();
    if (available_count <= param_count) {
      ++bind_count;
    } else {
      break;
    }
  }

  if (available_count < param_count) {
    throw ParameterCountMismatchException();
  }

  Bindings::difference_type remaining_bind_count = bind_end - bind_begin_;
  if (bind_count < remaining_bind_count) {
    bind_end = bind_begin_ + bind_count;
    can_bind_ = true;
  } else if (bind_count > remaining_bind_count) {
    throw ParameterCountMismatchException();
  }

  size_t boundRowCount;
  if (bind_begin_ != bindings().end()) {
    boundRowCount = (*bind_begin_)->HandledRowsCount();

    Bindings::iterator old_begin = bind_begin_;
    for (size_t pos = 1; bind_begin_ != bind_end && (*bind_begin_)->CanBind();
         ++bind_begin_) {
      if (boundRowCount != (*bind_begin_)->HandledRowsCount()) {
        throw BindingException(
            "Size mismatch in Bindings. All Bindings MUST have the same size");
      }

      size_t named_bind_pos = 0;
      if (!(*bind_begin_)->name().IsEmpty()) {
        named_bind_pos = (size_t)sqlite3_bind_parameter_index(
            stmt_, (*bind_begin_)->name().c_str());
      }

      (*bind_begin_)->bind((named_bind_pos != 0) ? named_bind_pos : pos);
      pos += (*bind_begin_)->HandledColumnsCount();
    }

    if ((*old_begin)->CanBind()) {
      // container binding will come back for more, so we must rewind
      bind_begin_ = old_begin;
      can_bind_ = true;
    } else {
      can_bind_ = false;
    }
  }
}

void SQLiteStatementImpl::Clear() {
  columns_[currentDataSet()].Clear();
  affected_row_count_ = FUN_SQLITE_INV_ROW_CNT;

  if (stmt_) {
    sqlite3_finalize(stmt_);
    stmt_ = 0;
  }
  leftover_ = 0;
}

bool SQLiteStatementImpl::HasNext() {
  if (step_called_) {
    return (next_response_ == SQLITE_ROW);
  }

  // stmt_ is allowed to be null for conditional sql statements
  if (stmt_ == 0) {
    step_called_ = true;
    next_response_ = SQLITE_DONE;
    return false;
  }

  step_called_ = true;
  next_response_ = sqlite3_step(stmt_);

  if (affected_row_count_ == FUN_SQLITE_INV_ROW_CNT) {
    affected_row_count_ = 0;
  }

  if (!sqlite3_stmt_readonly(stmt_)) {
    affected_row_count_ += sqlite3_changes(db_);
  }

  if (next_response_ != SQLITE_ROW && next_response_ != SQLITE_OK &&
      next_response_ != SQLITE_DONE) {
    Utility::ThrowException(db_, next_response_);
  }

  extractor_->Reset();  // Clear the cached null indicators

  return (next_response_ == SQLITE_ROW);
}

size_t SQLiteStatementImpl::next() {
  if (SQLITE_ROW == next_response_) {
    fun_check(ReturnedColumnCount() == sqlite3_column_count(stmt_));

    Extractions& extracts = GetExtractions();
    Extractions::iterator it = extracts.begin();
    Extractions::iterator itEnd = extracts.end();
    size_t pos = 0;  // sqlite starts with pos 0 for results!
    for (; it != itEnd; ++it) {
      (*it)->Extract(pos);
      pos += (*it)->HandledColumnsCount();
      is_extracted_ = true;
    }
    step_called_ = false;
    if (affected_row_count_ == FUN_SQLITE_INV_ROW_CNT) {
      affected_row_count_ = 0;
    }

    if (extracts.size()) {
      affected_row_count_ +=
          static_cast<int>((*extracts.begin())->HandledRowsCount());
    } else {
      affected_row_count_ +=
          static_cast<int>((*extracts.begin())->HandledRowsCount());
    }
  } else if (SQLITE_DONE == next_response_) {
    throw fun::sql::SqlException("No data received");
  } else {
    Utility::ThrowException(
        db_, next_response_,
        String("Iterator Error: trying to access the next value"));
  }

  return 1u;
}

size_t SQLiteStatementImpl::ReturnedColumnCount() const {
  return (size_t)columns_[currentDataSet()].size();
}

const MetaColumn& SQLiteStatementImpl::metaColumn(size_t pos,
                                                  size_t data_set) const {
  fun_check(pos >= 0 && pos <= columns_[data_set].size());
  return columns_[data_set][pos];
}

int SQLiteStatementImpl::AffectedRowCount() const {
  if (affected_row_count_ != FUN_SQLITE_INV_ROW_CNT) {
    return affected_row_count_;
  }

  return stmt_ == 0 || sqlite3_stmt_readonly(stmt_) ? 0 : sqlite3_changes(db_);
}

}  // namespace sqlite
}  // namespace sql
}  // namespace fun
