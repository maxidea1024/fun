#include "fun/sql/postgresql/statement_executor.h"
#include "fun/base/format.h"
#include "fun/base/number_parser.h"
#include "fun/base/regular_expression.h"  // TODO: remove after C++ 11 implementation
#include "fun/base/uuid.h"
#include "fun/base/uuid_generator.h"
#include "fun/sql/postgresql/postgresql_types.h"
//#include <regex> // saved for C++ 11 implementation
#include <algorithm>
#include <set>

namespace {
size_t countOfPlaceHoldersInSQLStatement(const String& aSQLStatement) {
  // Find unique placeholders.
  // Unique placeholders allow the same placeholder to be used multiple times in
  // the same statement.

  // NON C++11 implementation

  // if (aSQLStatement.IsEmpty())
  //{
  // return 0;
  //}

  // set to hold the unique placeholders ($1, $2, $3, etc.).
  // A set is used because the same placeholder can be used muliple times
  std::set<String> placeholderSet;

  fun::RegularExpression placeholderRE("[$][0-9]+");
  fun::RegularExpression::Match match = {
      0, 0};  // Match is a struct, not a class :-(

  size_t startingPosition = 0;

  while (match.offset != String::npos) {
    try {
      if (placeholderRE.match(aSQLStatement, startingPosition, match)) {
        placeholderSet.insert(aSQLStatement.substr(match.offset, match.length));
        startingPosition = match.offset + match.length;
      }
    } catch (fun::RegularExpressionException&) {
      break;
    }
  }

  /*  C++ 11 implementation

    std::regex const expression("[$][0-9]+");  // match literal dollar signs
    followed directly by one or more digits

    std::sregex_iterator itr(aSQLStatement.begin(), aSQLStatement.end(),
    expression); std::sregex_iterator eItr;

    // set to hold the unique placeholders ($1, $2, $3, etc.).
    // A set is used because the same placeholder can be used muliple times
    std::set<String> placeholderSet;

    while (itr != eItr)
    {
      placeholderSet.insert(itr->str());
      ++itr;
    }
  */
  return placeholderSet.size();
}

}  // namespace

namespace fun {
namespace sql {
namespace postgresql {

StatementExecutor::StatementExecutor(SessionHandle& handle)
    : session_handle_(handle),
      state_(STMT_INITED),
      result_handle_(0),
      count_placeholders_in_sql_statement_(0),
      current_row_(0),
      affected_row_count_(0) {}

StatementExecutor::~StatementExecutor() {
  try {
    // remove the prepared statement from the session
    if (session_handle_.IsConnected() && state_ >= STMT_COMPILED) {
      session_handle_.DeallocatePreparedStatement(prepared_statement_name_);
    }

    PQResultClear result_clearer(result_handle_);
  } catch (...) {
  }
}

StatementExecutor::State StatementExecutor::GetState() const { return state_; }

void StatementExecutor::Prepare(const String& aSQLStatement) {
  if (!session_handle_.IsConnected()) throw NotConnectedException();
  if (state_ >= STMT_COMPILED) return;

  // clear out the metadata.  One way or another it is now obsolete.
  count_placeholders_in_sql_statement_ = 0;
  sql_statement_ = String();
  prepared_statement_name_ = String();
  result_columns_.clear();

  // clear out any result data.  One way or another it is now obsolete.
  clearResults();

  // prepare parameters for the call to PQprepare
  const char* ptrCSQLStatement = aSQLStatement.c_str();
  size_t countPlaceholdersInSQLStatement =
      countOfPlaceHoldersInSQLStatement(aSQLStatement);

  fun::UUIDGenerator& generator = fun::UUIDGenerator::defaultGenerator();
  fun::UUID uuid(generator.create());  // time based
  String statementName = uuid.ToString();
  statementName.insert(
      0, 1, 'p');  // prepared statement names can't start with a number
  std::replace(
      statementName.begin(), statementName.end(), '-',
      'p');  // PostgreSQL doesn't like dashes in prepared statement names
  const char* pStatementName = statementName.c_str();

  PGresult* ptrPGResult = 0;

  {
    fun::FastMutex::ScopedLock mutexLocker(session_handle_.mutex());

    // prepare the statement - temporary PGresult returned
    ptrPGResult = PQprepare(session_handle_, pStatementName, ptrCSQLStatement,
                            (int)countPlaceholdersInSQLStatement, 0);
  }

  {
    // setup to clear the result from PQprepare
    PQResultClear result_clearer(ptrPGResult);

    if (!ptrPGResult || PQresultStatus(ptrPGResult) != PGRES_COMMAND_OK) {
      throw StatementException(String("postgresql_stmt_prepare error: ") +
                               PQresultErrorMessage(ptrPGResult) + " " +
                               aSQLStatement);
    }
  }

  // Determine what the structure of a statement result will look like
  {
    fun::FastMutex::ScopedLock mutexLocker(session_handle_.mutex());
    ptrPGResult = PQdescribePrepared(session_handle_, pStatementName);
  }

  {
    PQResultClear result_clearer(ptrPGResult);
    if (!ptrPGResult || PQresultStatus(ptrPGResult) != PGRES_COMMAND_OK) {
      throw StatementException(String("postgresql_stmt_describe error: ") +
                               PQresultErrorMessage(ptrPGResult) + " " +
                               aSQLStatement);
    }

    // remember the structure of the statement result
    int FieldCount = PQnfields(ptrPGResult);
    if (FieldCount < 0) FieldCount = 0;

    for (int i = 0; i < FieldCount; ++i) {
      result_columns_.push_back(
          MetaColumn(i, PQfname(ptrPGResult, i),
                     OidToColumnDataType(PQftype(ptrPGResult, i)), 0, 0, true));
    }
  }

  sql_statement_ = aSQLStatement;
  prepared_statement_name_ = statementName;
  count_placeholders_in_sql_statement_ = countPlaceholdersInSQLStatement;
  state_ = STMT_COMPILED;  // must be last
}

void StatementExecutor::BindParams(
    const InputParameterVector& input_parameter_vector) {
  if (!session_handle_.IsConnected()) {
    throw NotConnectedException();
  }

  if (state_ < STMT_COMPILED) {
    throw StatementException("Statement is not compiled yet");
  }

  if (input_parameter_vector.size() != count_placeholders_in_sql_statement_) {
    throw StatementException(
        String("incorrect bind parameters count for sql Statement: ") +
        sql_statement_);
  }

  // Just record the input vector for later execution
  input_parameter_vector_ = input_parameter_vector;
}

void StatementExecutor::BindBulkParams(
    const InputParameterVector& input_bulk_parameter_vector) {
  if (!session_handle_.IsConnected()) {
    throw NotConnectedException();
  }

  if (state_ < STMT_COMPILED) {
    throw StatementException("Statement is not compiled yet");
  }

  // Just record the input vector for later execution
  input_bulk_parameter_vector_ = input_bulk_parameter_vector;
}

void StatementExecutor::Execute() {
  if (!session_handle_.IsConnected()) {
    throw NotConnectedException();
  }

  if (state_ < STMT_COMPILED) {
    throw StatementException("Statement is not compiled yet");
  }

  if (count_placeholders_in_sql_statement_ != 0 &&
      input_parameter_vector_.size() != count_placeholders_in_sql_statement_) {
    throw StatementException(
        "Count of Parameters in Statement different than supplied parameters");
  }

  // "transmogrify" the input_parameter_vector_ to the C format required by
  // PQexecPrepared

  /* - from example
    const char *paramValues[1];
    int paramLengths[1];
    int paramFormats[1];
  */

  std::vector<const char*> pParameterVector;
  std::vector<int> parameterLengthVector;
  std::vector<int> parameterFormatVector;

  InputParameterVector::const_iterator cItr = input_parameter_vector_.begin();
  InputParameterVector::const_iterator cItrEnd = input_parameter_vector_.end();

  for (; cItr != cItrEnd; ++cItr) {
    try {
      pParameterVector.push_back(
          static_cast<const char*>(cItr->pInternalRepresentation()));
      parameterLengthVector.push_back((int)cItr->size());
      parameterFormatVector.push_back((int)cItr->IsBinary() ? 1 : 0);
    } catch (std::bad_alloc&) {
      throw StatementException("Memory Allocation Error");
    }
  }

  // clear out any result data.  One way or another it is now obsolete.
  clearResults();

  PGresult* ptrPGResult = 0;
  {
    fun::FastMutex::ScopedLock mutexLocker(session_handle_.mutex());

    ptrPGResult = PQexecPrepared(
        session_handle_, prepared_statement_name_.c_str(),
        (int)count_placeholders_in_sql_statement_,
        input_parameter_vector_.size() != 0 ? &pParameterVector[0] : 0,
        input_parameter_vector_.size() != 0 ? &parameterLengthVector[0] : 0,
        input_parameter_vector_.size() != 0 ? &parameterFormatVector[0] : 0, 0);
  }

  // Don't setup to auto clear the result (ptrPGResult).  It is required to
  // retrieve the results later.

  if (!ptrPGResult || (PQresultStatus(ptrPGResult) == PGRES_COPY_IN)) {
    InputParameterVector::const_iterator cItr =
        input_bulk_parameter_vector_.begin();
    InputParameterVector::const_iterator cItrEnd =
        input_bulk_parameter_vector_.end();

    for (; cItr != cItrEnd; ++cItr) {
      const char* bulkBuffer =
          static_cast<const char*>(cItr->pInternalRepresentation());
      if (PQputCopyData(session_handle_, bulkBuffer, strlen(bulkBuffer)) != 1) {
        ptrPGResult = PQgetResult(session_handle_);
      }
    }

    if (PQputCopyEnd(session_handle_, nullptr) != 1) {
      ptrPGResult = PQgetResult(session_handle_);
    }
  }

  if (!ptrPGResult || (PQresultStatus(ptrPGResult) != PGRES_COMMAND_OK &&
                       PQresultStatus(ptrPGResult) != PGRES_TUPLES_OK &&
                       PQresultStatus(ptrPGResult) != PGRES_COPY_IN)) {
    PQResultClear result_clearer(ptrPGResult);

    const char* pSeverity = PQresultErrorField(ptrPGResult, PG_DIAG_SEVERITY);
    const char* pSQLState = PQresultErrorField(ptrPGResult, PG_DIAG_SQLSTATE);
    const char* pDetail =
        PQresultErrorField(ptrPGResult, PG_DIAG_MESSAGE_DETAIL);
    const char* pHint = PQresultErrorField(ptrPGResult, PG_DIAG_MESSAGE_HINT);
    const char* pConstraint =
        PQresultErrorField(ptrPGResult, PG_DIAG_CONSTRAINT_NAME);

    throw StatementException(
        String("postgresql_stmt_execute error: ") +
        PQresultErrorMessage(ptrPGResult) +
        " Severity: " + (pSeverity ? pSeverity : "N/A") +
        " State: " + (pSQLState ? pSQLState : "N/A") + " Detail: " +
        (pDetail ? pDetail : "N/A") + " Hint: " + (pHint ? pHint : "N/A") +
        " Constraint: " + (pConstraint ? pConstraint : "N/A"));
  }

  result_handle_ = ptrPGResult;

  // are there any results?

  int affected_row_count = 0;

  if (PGRES_TUPLES_OK == PQresultStatus(result_handle_)) {
    affected_row_count = PQntuples(result_handle_);

    if (affected_row_count >= 0) {
      affected_row_count_ = static_cast<size_t>(affected_row_count);
    }
  } else {  // non Select DML statments also have an affected row count.
    // unfortunately PostgreSQL offers up this count as a char * - go figure!
    const char* pNonSelectAffectedRowCountString = PQcmdTuples(result_handle_);
    if (0 != pNonSelectAffectedRowCountString) {
      if (fun::NumberParser::tryParse(pNonSelectAffectedRowCountString,
                                      affected_row_count) &&
          affected_row_count >= 0) {
        affected_row_count_ = static_cast<size_t>(affected_row_count);
        current_row_ = affected_row_count_;  // no fetching on these statements!
      }
    }
  }

  state_ = STMT_EXECUTED;
}

bool StatementExecutor::Fetch() {
  if (!session_handle_.IsConnected()) {
    throw NotConnectedException();
  }

  if (state_ < STMT_EXECUTED) {
    throw StatementException("Statement is not yet executed");
  }

  size_t column_count = ReturnedColumnCount();

  // first time to fetch?
  if (0 == output_parameter_vector_.size()) {
    // setup a output vector for the results
    output_parameter_vector_.resize(column_count);
  }

  // already retrieved last row?
  if (current_row_ == affected_row_count()) {
    return false;
  }

  if (0 == column_count || PGRES_TUPLES_OK != PQresultStatus(result_handle_)) {
    return false;
  }

  for (int i = 0; i < column_count; ++i) {
    int fieldLength = PQgetlength(
        result_handle_, static_cast<int>(current_row_), static_cast<int>(i));

    Oid columnInternalDataType = PQftype(result_handle_, i);  // Oid of column

    output_parameter_vector_.At(i).setValues(
        OidToColumnDataType(
            columnInternalDataType),  // fun::sql::MetaData version of the
                                      // Column Data Type
        columnInternalDataType,       // Postgres Version
        current_row_,                 // the row number of the result
        PQgetvalue(result_handle_, (int)current_row_,
                   i),  // a pointer to the data
        (-1 == fieldLength ? 0
                           : fieldLength),  // the length of the data returned
        PQgetisnull(result_handle_, (int)current_row_, i) == 1
            ? true
            : false);  // is the column value null?
  }

  ++current_row_;
  return true;
}

size_t StatementExecutor::AffectedRowCount() const {
  return affected_row_count_;
}

size_t StatementExecutor::ReturnedColumnCount() const {
  return static_cast<size_t>(result_columns_.size());
}

const MetaColumn& StatementExecutor::metaColumn(size_t position) const {
  if (position >= ReturnedColumnCount()) {
    throw StatementException("Invalid column number for metaColumn");
  }

  return result_columns_.At(position);
}

const OutputParameter& StatementExecutor::resultColumn(size_t position) const {
  if (position >= ReturnedColumnCount()) {
    throw StatementException("Invalid column number for resultColumn");
  }

  return output_parameter_vector_.At(position);
}

void StatementExecutor::ClearResults() {
  // clear out any old result first
  { PQResultClear result_clearer(result_handle_); }

  output_parameter_vector_.Clear();
  affected_row_count_ = 0;
  current_row_ = 0;
}

}  // namespace postgresql
}  // namespace sql
}  // namespace fun
