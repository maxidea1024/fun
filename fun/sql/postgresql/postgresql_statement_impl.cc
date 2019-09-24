#include "fun/sql/postgresql/postgresql_statement_impl.h"

namespace fun {
namespace sql {
namespace postgresql {

PostgreSqlStatementImpl::PostgreSqlStatementImpl(SessionImpl& session_impl)
    : fun::sql::StatementImpl(session_impl),
      statement_executor_(session_impl.GetHandle()),
      binder_(new Binder),
      bulk_binder_(new Binder),
      extractor_(new Extractor(statement_executor_)),
      has_next_(NEXT_DONTKNOW) {}

PostgreSqlStatementImpl::~PostgreSqlStatementImpl() {}

size_t PostgreSqlStatementImpl::ReturnedColumnCount() const {
  return statement_executor_.ReturnedColumnCount();
}

int PostgreSqlStatementImpl::AffectedRowCount() const {
  return (int)statement_executor_.AffectedRowCount();
}

const MetaColumn& PostgreSqlStatementImpl::metaColumn(size_t position,
                                                      size_t data_set) const {
  // PostgreSql doesn't support multiple result sets
  fun_check_dbg(data_set == 0);

  return statement_executor_.MetaColumnAt(position);
}

bool PostgreSqlStatementImpl::HasNext() {
  if (NEXT_DONTKNOW == has_next_) {
    if (ReturnedColumnCount() == 0) {
      return false;
    }

    if (statement_executor_.Fetch()) {
      has_next_ = NEXT_TRUE;
      return true;
    }

    has_next_ = NEXT_FALSE;
    return false;
  } else {
    if (NEXT_TRUE == has_next_) {
      return true;
    }
  }

  return false;
}

size_t PostgreSqlStatementImpl::Next() {
  if (!HasNext()) {
    throw StatementException("No data received");
  }

  fun::sql::ExtractionBaseVec::iterator it = extractions().begin();
  fun::sql::ExtractionBaseVec::iterator itEnd = extractions().end();

  size_t position = 0;

  for (; it != itEnd; ++it) {
    (*it)->Extract(position);
    position += (*it)->HandledColumnsCount();
  }

  has_next_ = NEXT_DONTKNOW;

  return 1;
}

bool PostgreSqlStatementImpl::CanBind() const {
  bool ret = false;

  if ((statement_executor_.GetState() >= StatementExecutor::STMT_COMPILED) &&
      !GetBindings().IsEmpty()) {
    ret = (*GetBindings().begin())->CanBind();
  }

  return ret;
}

bool PostgreSqlStatementImpl::CanCompile() const {
  return (statement_executor_.GetState() < StatementExecutor::STMT_COMPILED);
}

void PostgreSqlStatementImpl::CompileImpl() {
  statement_executor_.Prepare(ToString());
}

void PostgreSqlStatementImpl::BindImpl() {
  fun::sql::BindingBaseVec& binds = GetBindings();

  size_t position = 0;
  fun::sql::BindingBaseVec::iterator it = binds.begin();
  fun::sql::BindingBaseVec::iterator itEnd = binds.end();

  for (; it != itEnd && (*it)->CanBind(); ++it) {
    if ((*it)->IsBulk()) {
      (*it)->SetBinder(bulk_binder_);
    }

    (*it)->Bind(position);
    position += (*it)->HandledColumnsCount();
  }

  binder_->UpdateBindVectorToCurrentValues();

  statement_executor_.BindParams(binder_->BindVector());
  statement_executor_.BindBulkParams(bulk_binder_->BindVector());

  statement_executor_.Execute();

  has_next_ = NEXT_DONTKNOW;
}

fun::sql::ExtractorBase::Ptr PostgreSqlStatementImpl::GetExtractor() {
  return extractor_;
}

fun::sql::BinderBase::Ptr PostgreSqlStatementImpl::GetBinder() {
  return binder_;
}

}  // namespace postgresql
}  // namespace sql
}  // namespace fun
