#include "fun/sql/mysql/mysql_statement_impl.h"

namespace fun {
namespace sql {
namespace mysql {

MySqlStatementImpl::MySqlStatementImpl(SessionImpl& h)
  : fun::sql::StatementImpl(h),
    stmt_(h.handle()),
    binder_(new Binder),
    extractor_(new Extractor(stmt_, metadata_)),
    has_next_(NEXT_DONTKNOW) {}

MySqlStatementImpl::~MySqlStatementImpl() {}

size_t MySqlStatementImpl::ReturnedColumnCount() const {
  return metadata_.ReturnedColumnCount();
}

int MySqlStatementImpl::AffectedRowCount() const {
  return stmt_.AffectedRowCount();
}

const MetaColumn& MySqlStatementImpl::MetaColumnAt(size_t pos, size_t data_set) const {
  // mysql doesn't support multiple result sets
  fun_check_dbg(data_set == 0);
  return metadata_.MetaColumnAt(pos);
}

bool MySqlStatementImpl::HasNext() {
  if (has_next_ == NEXT_DONTKNOW) {
    if (metadata_.ReturnedColumnCount() == 0) {
      return false;
    }

    if (stmt_.Fetch()) {
      has_next_ = NEXT_TRUE;
      return true;
    }

    has_next_ = NEXT_FALSE;
    return false;
  } else if (has_next_ == NEXT_TRUE) {
    return true;
  }

  return false;
}

size_t MySqlStatementImpl::Next() {
  if (!HasNext()) {
    throw StatementException("No data received");
  }

  fun::sql::ExtractionBaseVec::iterator it = GetExtractions().begin();
  fun::sql::ExtractionBaseVec::iterator itEnd = GetExtractions().end();
  size_t pos = 0;

  for (; it != itEnd; ++it) {
    (*it)->Extract(pos);
    pos += (*it)->HandledColumnsCount();
  }

  has_next_ = NEXT_DONTKNOW;
  return 1;
}

bool MySqlStatementImpl::CanBind() const {
  bool ret = false;

  if ((stmt_.state() >= StatementExecutor::STMT_COMPILED) && !bindings().IsEmpty()) {
    ret = (*bindings().begin())->CanBind();
  }

  return ret;
}

bool MySqlStatementImpl::CanCompile() const {
  return (stmt_.state() < StatementExecutor::STMT_COMPILED);
}

void MySqlStatementImpl::CompileImpl() {
  metadata_.Reset();
  stmt_.Prepare(ToString());
  metadata_.Init(stmt_);

  if (metadata_.ReturnedColumnCount() > 0) {
    stmt_.BindResult(metadata_.Row());
  }
}

void MySqlStatementImpl::BindImpl() {
  fun::sql::BindingBaseVec& binds = GetBindings();
  size_t pos = 0;
  fun::sql::BindingBaseVec::iterator it = binds.begin();
  fun::sql::BindingBaseVec::iterator itEnd = binds.end();
  for (; it != itEnd && (*it)->CanBind(); ++it) {
    (*it)->Bind(pos);
    pos += (*it)->HandledColumnsCount();
  }

  stmt_.BindParams(binder_->GetBindArray(), binder_->size());
  stmt_.Execute();
  has_next_ = NEXT_DONTKNOW;
}

fun::sql::ExtractorBase::Ptr MySqlStatementImpl::GetExtractor() {
  return extractor_;
}

fun::sql::BinderBase::Ptr MySqlStatementImpl::GetBinder() {
  return binder_;
}

} // namespace mysql
} // namespace sql
} // namespace fun
