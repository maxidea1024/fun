#include "fun/sql/statement.h"
#include "fun/sql/sql_exception.h"
#include "fun/sql/extraction.h"
#include "fun/sql/session.h"
#include "fun/sql/bulk.h"
#include "fun/base/any.h"
#include "fun/base/tuple.h"
#include "fun/base/async_method.h"
#include <algorithm>

namespace fun {
namespace sql {

Statement::Statement(StatementImpl::Ptr impl)
  : impl_(impl), async_(false) {
  fun_check_ptr(impl);
}

Statement::Statement(Session& session)
  : async_(false) {
  Reset(session);
}

Statement::Statement(const Statement& stmt) {
  if (stmt.IsAsync() && !stmt.IsDone()) {
    Wait();
  }

  impl_ = stmt.impl_;
  async_ = stmt.async_;
  result_ = stmt.result_;
  async_exec_ = stmt.async_exec_;
  arguments_ = stmt.arguments_;
  row_formatter_ = stmt.row_formatter_;
}

Statement::Statement(Statement&& stmt) {
  this->Move(MoveTemp(stmt));
}

Statement::~Statement() {}

Statement& Statement::operator = (const Statement& stmt) {
  if (stmt.IsAsync() && !stmt.IsDone()) {
    stmt.Wait();
  }

  if (FUN_LIKELY(&stmt != this)) {
    Statement tmp(stmt);
    Swap(tmp);
  }
  return *this;
}

void Statement::Swap(Statement& other) {
  if (this != &other) {
    fun::Swap(impl_, other.impl_);
    fun::Swap(async_, other.async_);
    fun::Swap(async_exec_, other.async_exec_);
    fun::Swap(result_, other.result_);
    arguments_.Swap(other.arguments_);
    fun::Swap(row_formatter_, other.row_formatter_);
  }
}

Statement& Statement::operator = (Statement&& stmt) {
  this->Move(MoveTemp(stmt));
  return *this;
}

void Statement::Move(Statement&& stmt) {
  if (stmt.IsAsync() && !stmt.IsDone()) {
    stmt.Wait();
  }

  impl_ = stmt.impl_; stmt.impl_ = nullptr;
  async_ = stmt.async_; stmt.async_ = false;
  result_ = stmt.result_; stmt.result_ = nullptr;
  async_exec_ = stmt.async_exec_; stmt.async_exec_ = nullptr;
  arguments_ = MoveTemp(stmt.arguments_); stmt.arguments_.Clear();
  row_formatter_ = stmt.row_formatter_; stmt.row_formatter_ = nullptr;
}

Statement& Statement::Reset(Session& session) {
  Statement stmt(session.CreateStatementImpl());
  Swap(stmt);
  return *this;
}

size_t Statement::Execute(bool do_reset) {
  Mutex::ScopedLock lock(mutex_);
  bool is_done = IsDone();
  if (IsInitialized() || IsPaused() || is_done) {
    if (arguments_.size()) {
      impl_->FormatSql(arguments_);
      arguments_.Clear();
    }

    if (!IsAsync()) {
      if (is_done) {
        impl_->Reset();
      }
      return impl_->Execute(do_reset);
    } else {
      DoAsyncExec();
      return 0;
    }
  } else {
    throw InvalidAccessException("Statement still executing.");
  }
}

const Statement::Result& Statement::ExecuteAsync(bool do_reset) {
  Mutex::ScopedLock lock(mutex_);
  if (IsInitialized() || IsPaused() || IsDone()) {
    return DoAsyncExec(do_reset);
  } else {
    throw InvalidAccessException("Statement still executing.");
  }
}

const Statement::Result& Statement::DoAsyncExec(bool do_reset) {
  if (IsDone()) {
    impl_->Reset();
  }
  if (!async_exec_) {
    async_exec_ = new AsyncExecMethod(impl_, &StatementImpl::Execute);
  }
  result_ = new Result((*async_exec_)(do_reset));
  return *result_;
}

void Statement::SetAsync(bool async) {
  async_ = async;
  if (async_ && !async_exec_) {
    async_exec_ = new AsyncExecMethod(impl_, &StatementImpl::Execute);
  }
}

size_t Statement::Wait(long milliseconds) const {
  if (!result_) {
    return 0;
  }

  bool success = true;
  if (WAIT_FOREVER != milliseconds) {
    success = result_->TryWait(milliseconds);
  } else {
    result_->Wait();
  }

  if (result_->exception()) {
    throw *result_->exception();
  } else if (!success) {
    throw TimeoutException("Statement timed out.");
  }

  return result_->data();
}

const String& Statement::GetStorage() const {
  switch (storage()) {
    case STORAGE_VECTOR:
      return StatementImpl::VECTOR;
    case STORAGE_LIST:
      return StatementImpl::LIST;
    case STORAGE_DEQUE:
      return StatementImpl::DEQUE;
    case STORAGE_UNKNOWN:
      return StatementImpl::UNKNOWN;
  }

  throw IllegalStateException("Invalid storage setting.");
}

Statement& Statement::operator , (Manipulator manip) {
  manip(*this);
  return *this;
}

Statement& Statement::AddBind(BindingBase::Ptr bind) {
  if (bind->IsBulk()) {
    if (!impl_->IsBulkSupported()) {
      throw InvalidAccessException("Bulk not supported by this session.");
    }

    if(impl_->BulkBindingAllowed()) {
      impl_->SetBulkBinding();
    } else {
      throw InvalidAccessException("Bulk and non-bulk binding modes can not be mixed.");
    }
  } else {
    impl_->ForbidBulk();
  }

  impl_->AddBind(bind);
  return *this;
}

Statement& Statement::AddExtract(ExtractionBase::Ptr extract) {
  if (extract->IsBulk()) {
    if (!impl_->IsBulkSupported()) {
      throw InvalidAccessException("Bulk not supported by this session.");
    }

    if(impl_->BulkExtractionAllowed()) {
      Bulk b(extract->GetLimit());
      impl_->SetBulkExtraction(b);
    } else {
      throw InvalidAccessException("Bulk and non-bulk extraction modes can not be mixed.");
    }
  } else {
    impl_->ForbidBulk();
  }

  impl_->AddExtract(extract);
  return *this;
}

Statement& Statement::operator , (const Limit& extr_limit) {
  if (impl_->IsBulkExtraction() && impl_->extractionLimit() != extr_limit) {
    throw InvalidArgumentException("Limit for bulk extraction already set.");
  }

  impl_->SetExtractionLimit(extr_limit);
  return *this;
}

Statement& Statement::operator , (const Range& extr_range) {
  if (impl_->IsBulkExtraction()) {
    throw InvalidAccessException("Can not set range for bulk extraction.");
  }

  impl_->SetExtractionLimit(extr_range.Lower());
  impl_->SetExtractionLimit(extr_range.Upper());
  return *this;
}

Statement& Statement::operator , (const Bulk& bulk) {
  if (!impl_->IsBulkSupported()) {
      throw InvalidAccessException("Bulk not supported by this session.");
  }

  if (0 == impl_->extractions().size() &&
      0 == impl_->bindings().size() &&
      impl_->BulkExtractionAllowed() &&
      impl_->BulkBindingAllowed()) {
    impl_->SetBulkExtraction(bulk);
    impl_->SetBulkBinding();
  } else {
    throw InvalidAccessException("Can not set bulk operations.");
  }

  return *this;
}

Statement& Statement::operator , (BulkFnType) {
  const Limit& limit(impl_->GetExtractionLimit());
  if (limit.IsHardLimit() ||
      limit.IsLowerLimit() ||
      Limit::LIMIT_UNLIMITED == limit.Value()) {
    throw InvalidAccessException("Bulk is only allowed with limited extraction,"
      "non-hard and zero-based limits.");
  }

  Bulk bulk(limit);
  impl_->SetBulkExtraction(bulk);
  impl_->SetBulkBinding();

  return *this;
}

Session Statement::GetSession() {
  fun::RefCountedPtr<SessionImpl> ps(&GetImpl()->GetSession(), true);
  return Session(ps);
}

void Statement::SetTotalRowCount(const String& sql) {
  size_t count;
  GetSession() << sql,
      fun::sql::Keywords::into(count),
      fun::sql::Keywords::now;
  impl_->SetTotalRowCount(count);
}

} // namespace sql
} // namespace fun
