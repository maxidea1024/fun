#include "fun/sql/odbc/odbc_statement_impl.h"
#include "fun/base/exception.h"
#include "fun/sql/odbc/connection_handle.h"
#include "fun/sql/odbc/odbc_exception.h"
#include "fun/sql/odbc/utility.h"
#include "fun/sql/preparation_base.h"

#ifdef FUN_PLATFORM_WINDOWS_FAMILY
#pragma warning(disable : 4312)  // 'type cast' : conversion from 'size_t' to
                                 // 'SQLPOINTER' of greater size
#endif

using fun::DataFormatException;

namespace fun {
namespace sql {
namespace odbc {

const String OdbcStatementImpl::INVALID_CURSOR_STATE = "24000";

OdbcStatementImpl::OdbcStatementImpl(SessionImpl& session)
    : fun::sql::StatementImpl(session),
      connection_(session.dbc()),
      stmt_(session.dbc()),
      step_called_(false),
      next_response_(0),
      prepared_(false),
      affected_row_count_(0),
      can_compile_(true),
      insert_hint_(false) {
  int query_timeout = session.GetQueryTimeout();
  if (query_timeout >= 0) {
    SQLULEN uqt = static_cast<SQLULEN>(query_timeout);
    SQLSetStmtAttr(stmt_, SQL_ATTR_QUERY_TIMEOUT, (SQLPOINTER)uqt, 0);
  }

  SQLSMALLINT t;
  SQLRETURN r =
      fun::sql::odbc::SQLGetInfo(connection_, SQL_DRIVER_NAME, NULL, 0, &t);
  if (!Utility::IsError(r) && t > 0) {
    String server_string;
    server_string.resize(static_cast<size_t>(t) + 2);
    r = fun::sql::odbc::SQLGetInfo(
        connection_, SQL_DRIVER_NAME, &server_string[0],
        SQLSMALLINT((server_string.length() - 1) * sizeof(server_string[0])),
        &t);
  }
}

OdbcStatementImpl::~OdbcStatementImpl() {
  ColumnPtrVecVec::iterator it = column_ptrs_.begin();
  ColumnPtrVecVec::iterator end = column_ptrs_.end();
  for (; it != end; ++it) {
    ColumnPtrVec::iterator itC = it->begin();
    ColumnPtrVec::iterator endC = it->end();
    for (; itC != endC; ++itC) {
      delete *itC;
    }
  }
}

void OdbcStatementImpl::CompileImpl() {
  if (!can_compile_) {
    return;
  }

  step_called_ = false;
  next_response_ = 0;

  if (preparations_.size()) {
    PreparatorVec().Swap(preparations_);
  }

  AddPreparator();

  Binder::ParameterBinding bind = GetSession().GetFeature("autoBind")
                                      ? Binder::PB_IMMEDIATE
                                      : Binder::PB_AT_EXEC;

  TypeInfo* pDT = nullptr;
  try {
    fun::Any dti = GetSession().GetProperty("dataTypeInfo");
    pDT = AnyCast<TypeInfo*>(dti);
  } catch (NotSupportedException&) {
  }

  const size_t maxFieldSize =
      AnyCast<size_t>(GetSession().GetProperty("maxFieldSize"));

  binder_ = new Binder(stmt_, maxFieldSize, bind, pDT, insert_hint_);

  can_compile_ = false;
}

bool OdbcStatementImpl::CanMakeExtractors() {
  return StatementImpl::CanMakeExtractors() &&
         GetSession().GetFeature("MakeExtractorsBeforeExecute");
}

void OdbcStatementImpl::MakeExtractors(size_t count) {
  if (HasData() && GetExtractions().IsEmpty()) {
    try {
      FillColumns(currentDataSet());
    } catch (DataFormatException&) {
      if (IsStoredProcedure()) {
        return;
      }
      throw;
    }

    StatementImpl::MakeExtractors(count);
    FixupExtraction();
  }
}

bool OdbcStatementImpl::AddPreparator(bool add_always) {
  prepared_ = false;
  Preparator* prep = nullptr;
  if (0 == preparations_.size()) {
    String statement(ToString());
    if (statement.IsEmpty())
      throw OdbcException("Empty statements are illegal");

    Preparator::DataExtraction ext = GetSession().GetFeature("autoExtract")
                                         ? Preparator::DE_BOUND
                                         : Preparator::DE_MANUAL;

    size_t maxFieldSize =
        AnyCast<size_t>(GetSession().GetProperty("maxFieldSize"));

    prep = new Preparator(stmt_, statement, maxFieldSize, ext);
  } else {
    prep = new Preparator(*preparations_[0]);
  }
  if (add_always || prep->columns() > 0) {
    preparations_.push_back(prep);
    extractors_.push_back(new Extractor(stmt_, preparations_.back()));
    return true;
  }

  delete prep;
  return false;
}

void OdbcStatementImpl::DoPrepare() {
  if (!prepared_ && GetSession().GetFeature("autoExtract") && HasData()) {
    size_t current_data_set = GetCurrentDataSet();
    fun_check_ptr(preparations_[current_data_set]);

    Extractions& extracts = GetExtractions();
    Extractions::iterator it = extracts.begin();
    Extractions::iterator itEnd = extracts.end();

    if (it != itEnd && (*it)->IsBulk()) {
      size_t limit = GetExtractionLimit();
      if (limit == Limit::LIMIT_UNLIMITED) {
        throw InvalidArgumentException(
            "Bulk operation not allowed without limit.");
      }
      CheckError(fun::sql::odbc::SQLSetStmtAttr(stmt_, SQL_ATTR_ROW_ARRAY_SIZE,
                                                (SQLPOINTER)limit, 0),
                 "SQLSetStmtAttr(SQL_ATTR_ROW_ARRAY_SIZE)");
    }

    PreparationBase::Ptr pAP = 0;
    fun::sql::PreparatorBase::Ptr pP = preparations_[current_data_set];
    for (size_t pos = 0; it != itEnd; ++it) {
      pAP = (*it)->CreatePreparation(pP, pos);
      pAP->prepare();
      pos += (*it)->HandledColumnsCount();
    }

    prepared_ = true;
  }
}

bool OdbcStatementImpl::CanBind() const {
  if (!GetBindings().IsEmpty()) {
    return (*bindings().begin())->CanBind();
  }

  return false;
}

void OdbcStatementImpl::DoBind() {
  this->Clear();
  Bindings& binds = GetBindings();
  if (!binds.IsEmpty()) {
    Bindings::iterator it = binds.begin();
    Bindings::iterator itEnd = binds.end();

    if (it != itEnd && 0 == affected_row_count_) {
      affected_row_count_ = static_cast<size_t>((*it)->HandledRowsCount());
    }

    for (size_t pos = 0; it != itEnd && (*it)->CanBind(); ++it) {
      (*it)->Bind(pos);
      pos += (*it)->HandledColumnsCount();
    }
  }
}

void OdbcStatementImpl::BindImpl() {
  DoBind();
  SQLRETURN rc = SQLExecute(stmt_);

  if (SQL_NEED_DATA == rc)
    PutData();
  else
    CheckError(rc, "SQLExecute()");

  binder_->synchronize();
}

void OdbcStatementImpl::PutData() {
  SQLPOINTER param = 0;
  SQLINTEGER dataSize = 0;
  SQLRETURN rc;

  while (SQL_NEED_DATA == (rc = SQLParamData(stmt_, &param))) {
    if (param) {
      dataSize = (SQLINTEGER)binder_->GetParameterSize(param);

      if (Utility::IsError(SQLPutData(stmt_, param, dataSize))) {
        throw StatementException(stmt_, "SQLPutData()");
      }
    } else {  // if param is null pointer, do a dummy call
      char dummy = 0;
      if (Utility::IsError(SQLPutData(stmt_, &dummy, 0))) {
        throw StatementException(stmt_, "SQLPutData()");
      }
    }
  }

  CheckError(rc, "SQLParamData()");
}

void OdbcStatementImpl::Clear() {
  SQLRETURN rc = SQLCloseCursor(stmt_);
  step_called_ = false;
  affected_row_count_ = 0;

  if (Utility::IsError(rc)) {
    StatementError err(stmt_);
    bool ignore_error = false;

    const StatementDiagnostics& diagnostics = err.diagnostics();
    // ignore "Invalid cursor state" error
    //(returned by 3.x drivers when cursor is not opened)
    for (int i = 0; i < diagnostics.count(); ++i) {
      if ((ignore_error =
               (INVALID_CURSOR_STATE == String(diagnostics.sqlState(i))))) {
        break;
      }
    }

    if (!ignore_error) {
      throw StatementException(stmt_, "SQLCloseCursor()");
    }
  }
}

bool OdbcStatementImpl::NextResultSet() {
  SQLRETURN ret = SQLMoreResults(stmt_);

  if (SQL_NO_DATA == ret) {
    return false;
  }

  if (Utility::IsError(ret)) {
    throw StatementException(stmt_, "SQLMoreResults()");
  }

  // need to remove old bindings, as Sybase doesn't like old ones
  if (Utility::IsError(SQLFreeStmt(stmt_, SQL_UNBIND))) {
    throw StatementException(stmt_, "SQLFreeStmt(SQL_UNBIND)");
  }

  return true;
}

bool OdbcStatementImpl::HasNext() {
  if (HasData()) {
    if (GetExtractions().IsEmpty()) {
      MakeExtractors(ReturnedColumnCount());
    }

    if (!prepared_) {
      DoPrepare();
    }

    if (step_called_) {
      return step_called_ = NextRowReady();
    }

    MakeStep();

    if (!NextRowReady()) {
      // have a loop here, as there could be one or more empty results
      do {
        if (HasMoreDataSets()) {
          ActivateNextDataSet();
          if (!NextResultSet()) {
            return false;
          }
          AddPreparator();
        } else {
          if (NextResultSet()) {
            if (!AddPreparator(
                    false)) {  // skip the result set if it has no columns
              continue;
            }

            FillColumns(GetCurrentDataSet() + 1);
            StatementImpl::MakeExtractors(
                preparations_.back()->columns(),
                static_cast<Position::Type>(GetCurrentDataSet() + 1));
            ActivateNextDataSet();
          } else {
            return false;
          }
        }
        DoPrepare();
        FixupExtraction();
        MakeStep();
      } while (!NextRowReady());
    } else if (Utility::IsError(static_cast<SQLRETURN>(next_response_))) {
      CheckError(static_cast<SQLRETURN>(next_response_), "SQLFetch()");
    }

    return true;
  }

  return false;
}

void OdbcStatementImpl::MakeStep() {
  extractors_[GetCurrentDataSet()]->reset();
  next_response_ = SQLFetch(stmt_);
  CheckError(static_cast<SQLRETURN>(next_response_));
  step_called_ = true;
}

size_t OdbcStatementImpl::Next() {
  size_t count = 0;

  if (NextRowReady()) {
    Extractions& extracts = GetExtractions();
    Extractions::iterator it = extracts.begin();
    Extractions::iterator itEnd = extracts.end();
    size_t prev_count = 0;
    for (size_t pos = 0; it != itEnd; ++it) {
      count = (*it)->Extract(pos);
      if (prev_count && count != prev_count) {
        throw IllegalStateException("Different extraction counts");
      }
      prev_count = count;
      pos += (*it)->HandledColumnsCount();
    }
    step_called_ = false;
  } else {
    throw StatementException(stmt_, String("Next row not available."));
  }

  return count;
}

String OdbcStatementImpl::GetNativeSQL() {
  String statement = ToString();

  SQLINTEGER length = (SQLINTEGER)statement.size() * 2;

  char* native_ptr = nullptr;
  SQLINTEGER retlen = length;
  do {
    delete[] native_ptr;
    native_ptr = new char[retlen];
    UnsafeMemory::Memset(pNative, 0, retlen);
    length = retlen;
    if (Utility::IsError(SQLNativeSql(connection_, (SQLCHAR*)statement.c_str(),
                                      (SQLINTEGER)statement.size(),
                                      (SQLCHAR*)native_ptr, length, &retlen))) {
      delete[] native_ptr;
      throw ConnectionException(connection_, "SQLNativeSql()");
    }
    ++retlen;  // accommodate for terminating '\0'
  } while (retlen > length);

  String sql(native_ptr);
  delete[] native_ptr;
  return sql;
}

void OdbcStatementImpl::CheckError(SQLRETURN rc, const String& msg) {
  if (SQL_NO_DATA == rc) {
    return;
  }

  if (Utility::IsError(rc)) {
    std::ostringstream os;
    os << std::endl << "Requested sql statement: " << ToString() << std::endl;
    os << "Native sql statement: " << GetNativeSQL() << std::endl;
    String str(msg);
    str += os.str();

    throw StatementException(stmt_, str);
  }
}

void OdbcStatementImpl::FillColumns(size_t data_set_pos) {
  fun_check_dbg(data_set_pos < preparations_.size());
  fun_check_dbg(preparations_[data_set_pos]);
  size_t col_count =
      static_cast<size_t>(preparations_[data_set_pos]->columns());
  if (data_set_pos >= column_ptrs_.size()) {
    column_ptrs_.resize(data_set_pos + 1);
  }

  for (int i = 0; i < col_count; ++i) {
    column_ptrs_[data_set_pos].push_back(new OdbcMetaColumn(stmt_, i));
  }
}

bool OdbcStatementImpl::IsStoredProcedure() const {
  static const std::set<String> tagsProc = {"exec "};
  static const std::set<String> tagsNonProc = {"select ", "insert ",  "update ",
                                               "delete ", "create ",  "drop ",
                                               "alter ",  "truncate "};

  String str = ToString();
  if (trimInPlace(str).size() < 2) {
    return false;
  }

  if ('{' == str.front() && '}' == str.back()) {
    return true;
  }

  for (const String& tag : tagsProc) {
    if (icompare(str, tag.length(), tag) == 0) {
      return true;
    }
  }

  for (const String& tag : tagsNonProc) {
    if (icompare(str, tag.length(), tag) == 0) {
      return false;
    }
  }

  return true;
}

const MetaColumn& OdbcStatementImpl::metaColumn(size_t pos,
                                                size_t data_set) const {
  fun_check_dbg(data_set < column_ptrs_.size());

  size_t sz = column_ptrs_[data_set].size();

  if (0 == sz || pos > sz - 1) {
    throw InvalidAccessException(format("Invalid column number: %u", pos));
  }

  return *column_ptrs_[data_set][pos];
}

int OdbcStatementImpl::AffectedRowCount() const {
  if (0 == affected_row_count_) {
    SQLLEN rows = 0;
    if (!Utility::IsError(SQLRowCount(stmt_, &rows))) {
      affected_row_count_ = static_cast<size_t>(rows);
    }
  }

  return static_cast<int>(affected_row_count_);
}

void OdbcStatementImpl::InsertHint() { insert_hint_ = true; }

}  // namespace odbc
}  // namespace sql
}  // namespace fun
