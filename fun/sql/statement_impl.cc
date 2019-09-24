#include "fun/sql/statement_impl.h"
#include "fun/base/date_time.h"
#include "fun/base/exception.h"
#include "fun/base/shared_ptr.h"
#include "fun/sql/binder_base.h"
#include "fun/sql/date.h"
#include "fun/sql/extraction.h"
#include "fun/sql/lob.h"
#include "fun/sql/session_impl.h"
#include "fun/sql/sql_exception.h"
#include "fun/sql/time.h"

using fun::icompare;

namespace fun {
namespace sql {

using namespace Keywords;

const String StatementImpl::VECTOR = "vector";
const String StatementImpl::LIST = "list";
const String StatementImpl::DEQUE = "deque";
const String StatementImpl::UNKNOWN = "unknown";

const size_t StatementImpl::UNKNOWN_TOTAL_ROW_COUNT =
    std::numeric_limits<size_t>::max();

StatementImpl::StatementImpl(SessionImpl& session)
    : state_(ST_INITIALIZED),
      extr_limit_(upperLimit(Limit::LIMIT_UNLIMITED, false)),
      lower_limit_(0),
      session_(session),
      storage_(STORAGE_UNKNOWN_IMPL),
      ostr_(),
      cur_data_set_(0),
      bulk_binding_(BULK_UNDEFINED),
      bulk_extraction_(BULK_UNDEFINED),
      total_row_count_(UNKNOWN_TOTAL_ROW_COUNT) {
  if (!session_.IsConnected()) {
    throw NotConnectedException(session_.GetConnectionString());
  }

  extractors_.Resize(1);
  columns_extracted_.Resize(1, 0);
  sub_total_row_count_.Resize(1, 0);
}

StatementImpl::~StatementImpl() {}

size_t StatementImpl::Execute(const bool& reset_ref) {
  if (reset_ref) {
    ResetExtraction();
  }

  if (!session_.IsConnected()) {
    state_ = ST_DONE;
    throw NotConnectedException(session_.connection_string());
  }

  size_t lim = 0;
  if (lower_limit_ > extr_limit_.Value()) {
    throw LimitException(
        "Illegal Statement state. Upper limit must not be smaller than the "
        "lower limit.");
  }

  do {
    Compile();

    if (extr_limit_.Value() == Limit::LIMIT_UNLIMITED) {
      lim += ExecuteWithoutLimit();
      AssignSubTotal(true);
    } else {
      lim += ExecuteWithLimit();
      AssignSubTotal(false);
    }
  } while (CanCompile());

  if (extr_limit_.Value() == Limit::LIMIT_UNLIMITED) {
    state_ = ST_DONE;
  }

  if (lim < lower_limit_) {
    throw LimitException("Did not receive enough data.");
  }

  return lim;
}

void StatementImpl::AssignSubTotal(bool do_reset) {
  if (extractors_.size() == sub_total_row_count_.size()) {
    CountVec::iterator it = sub_total_row_count_.begin();
    CountVec::iterator end = sub_total_row_count_.end();
    for (size_t counter = 0; it != end; ++it, ++counter) {
      if (extractors_[counter].size()) {
        if (do_reset) {
          *it =
              CountVec::value_type(extractors_[counter][0]->HandledRowsCount());
        } else {
          *it +=
              CountVec::value_type(extractors_[counter][0]->HandledRowsCount());
        }
      }
    }
  }
}

size_t StatementImpl::ExecuteWithLimit() {
  fun_check(state_ != ST_DONE);
  size_t count = 0;
  size_t limit = extr_limit_.Value();

  do {
    Bind();
    while (count < limit && HasNext()) {
      count += Next();
    }
  } while (count < limit && CanBind());

  if (!CanBind() && (!HasNext() || limit == 0)) {
    state_ = ST_DONE;
  } else if (limit == count && extr_limit_.IsHardLimit() && HasNext()) {
    throw LimitException(
        "HardLimit reached (retrieved more data than requested).");
  } else {
    state_ = ST_PAUSED;
  }

  int affected_row_count = AffectedRowCount();
  if (count == 0) {
    if (affected_row_count > 0) {
      return affected_row_count;
    }
  }

  return count;
}

size_t StatementImpl::ExecuteWithoutLimit() {
  fun_check(state_ != ST_DONE);
  size_t count = 0;

  do {
    Bind();
    while (HasNext()) {
      count += Next();
    }
  } while (CanBind());

  int affected_row_count = AffectedRowCount();
  if (count == 0) {
    if (affected_row_count > 0) {
      return affected_row_count;
    }
  }

  return count;
}

void StatementImpl::Compile() {
  if (state_ == ST_INITIALIZED || state_ == ST_RESET || state_ == ST_BOUND) {
    CompileImpl();
    state_ = ST_COMPILED;

    if (CanMakeExtractors()) {
      size_t cols = ReturnedColumnCount();
      if (cols) MakeExtractors(cols);
    }

    FixupExtraction();
    FixupBinding();
  }
}

void StatementImpl::Bind() {
  if (state_ == ST_COMPILED) {
    BindImpl();
    state_ = ST_BOUND;
  } else if (state_ == ST_BOUND) {
    if (!HasNext()) {
      if (CanBind()) {
        BindImpl();
      } else {
        state_ = ST_DONE;
      }
    }
  }
}

void StatementImpl::Reset() {
  ResetBinding();
  ResetExtraction();
  state_ = ST_RESET;
}

void StatementImpl::SetExtractionLimit(const Limit& extrLimit) {
  if (!extrLimit.isLowerLimit()) {
    extr_limit_ = extrLimit;
  } else {
    lower_limit_ = extrLimit.Value();
  }
}

void StatementImpl::SetBulkExtraction(const Bulk& b) {
  Limit::SizeT limit = GetExtractionLimit();
  if (Limit::LIMIT_UNLIMITED != limit && b.size() != limit) {
    throw InvalidArgumentException("Can not set limit for statement.");
  }

  SetExtractionLimit(b.limit());
  bulk_extraction_ = BULK_EXTRACTION;
}

void StatementImpl::FixupExtraction() {
  if (cur_data_set_ >= columns_extracted_.size()) {
    columns_extracted_.Resize(cur_data_set_ + 1, 0);
    sub_total_row_count_.Resize(cur_data_set_ + 1, 0);
  }

  fun::sql::ExtractionBaseVec::iterator it = extractions().begin();
  fun::sql::ExtractionBaseVec::iterator itEnd = extractions().end();
  for (; it != itEnd; ++it) {
    (*it)->SetExtractor(extractor());
    (*it)->SetLimit(extr_limit_.Value()),
        columns_extracted_[cur_data_set_] += (int)(*it)->HandledColumnsCount();
  }
}

void StatementImpl::FixupBinding() {
  // no need to call binder().Reset(); here will be called before each Bind
  // anyway
  BindingBaseVec::iterator it = bindings().begin();
  BindingBaseVec::iterator itEnd = bindings().end();
  for (; it != itEnd; ++it) (*it)->SetBinder(binder());
}

void StatementImpl::ResetBinding() {
  BindingBaseVec::iterator it = bindings().begin();
  BindingBaseVec::iterator itEnd = bindings().end();
  for (; it != itEnd; ++it) (*it)->Reset();
}

void StatementImpl::ResetExtraction() {
  fun::sql::ExtractionBaseVec::iterator it = extractions().begin();
  fun::sql::ExtractionBaseVec::iterator itEnd = extractions().end();
  for (; it != itEnd; ++it) (*it)->Reset();

  fun_check(cur_data_set_ < columns_extracted_.size());
  columns_extracted_[cur_data_set_] = 0;
}

void StatementImpl::SetStorage(const String& storage) {
  if (0 == icompare(DEQUE, storage)) {
    storage_ = STORAGE_DEQUE_IMPL;
  } else if (0 == icompare(VECTOR, storage)) {
    storage_ = STORAGE_VECTOR_IMPL;
  } else if (0 == icompare(LIST, storage)) {
    storage_ = STORAGE_LIST_IMPL;
  } else if (0 == icompare(UNKNOWN, storage)) {
    storage_ = STORAGE_UNKNOWN_IMPL;
  } else {
    throw NotFoundException();
  }
}

bool StatementImpl::CanMakeExtractors() {
  return extractions().IsEmpty() && !IsStoredProcedure();
}

void StatementImpl::MakeExtractors(size_t count) {
  // type cast is needed when size_t is 64 bit
  MakeExtractors(count, static_cast<Position::Type>(GetCurrentDataSet()));
}

void StatementImpl::MakeExtractors(size_t count, const Position& position) {
  for (int i = 0; i < count; ++i) {
    const MetaColumn& mc = metaColumn(i, position.Value());
    switch (mc.Type()) {
      case MetaColumn::FDT_BOOL:
        AddInternalExtract<bool>(mc, position.Value());
        break;
      case MetaColumn::FDT_INT8:
        AddInternalExtract<int8>(mc, position.Value());
        break;
      case MetaColumn::FDT_UINT8:
        AddInternalExtract<uint8>(mc, position.Value());
        break;
      case MetaColumn::FDT_INT16:
        AddInternalExtract<int16>(mc, position.Value());
        break;
      case MetaColumn::FDT_UINT16:
        AddInternalExtract<uint16>(mc, position.Value());
        break;
      case MetaColumn::FDT_INT32:
        AddInternalExtract<int32>(mc, position.Value());
        break;
      case MetaColumn::FDT_UINT32:
        AddInternalExtract<uint32>(mc, position.Value());
        break;
      case MetaColumn::FDT_INT64:
        AddInternalExtract<int64>(mc, position.Value());
        break;
      case MetaColumn::FDT_UINT64:
        AddInternalExtract<uint64>(mc, position.Value());
        break;
      case MetaColumn::FDT_FLOAT:
        AddInternalExtract<float>(mc, position.Value());
        break;
      case MetaColumn::FDT_DOUBLE:
        AddInternalExtract<double>(mc, position.Value());
        break;
      case MetaColumn::FDT_STRING:
        AddInternalExtract<String>(mc, position.Value());
        break;
      case MetaColumn::FDT_WSTRING:
        AddInternalExtract<fun::UString>(mc, position.Value());
        break;
      case MetaColumn::FDT_BLOB:
        AddInternalExtract<BLOB>(mc, position.Value());
        break;
      case MetaColumn::FDT_CLOB:
        AddInternalExtract<CLOB>(mc, position.Value());
        break;
      case MetaColumn::FDT_DATE:
        AddInternalExtract<Date>(mc, position.Value());
        break;
      case MetaColumn::FDT_TIME:
        AddInternalExtract<Time>(mc, position.Value());
        break;
      case MetaColumn::FDT_TIMESTAMP:
        AddInternalExtract<DateTime>(mc, position.Value());
        break;
      default:
        throw fun::InvalidArgumentException("Data type not supported.");
    }
  }
}

const MetaColumn& StatementImpl::metaColumn(const String& name) const {
  size_t cols = ReturnedColumnCount();
  for (size_t i = 0; i < cols; ++i) {
    const MetaColumn& column = metaColumn(i, currentDataSet());
    if (0 == icompare(column.name(), name)) {
      return column;
    }
  }

  throw NotFoundException(format("Invalid column name: %s", name));
}

size_t StatementImpl::ActivateNextDataSet() {
  if (cur_data_set_ + 1 < DataSetCount()) {
    return ++cur_data_set_;
  } else {
    throw NoSqlException("End of data sets reached.");
  }
}

size_t StatementImpl::ActivatePreviousDataSet() {
  if (cur_data_set_ > 0) {
    return --cur_data_set_;
  } else {
    throw NoSqlException("Beginning of data sets reached.");
  }
}

void StatementImpl::AddExtract(ExtractionBase::Ptr extraction) {
  fun_check_ptr(extraction);
  size_t pos = extraction->position();
  if (pos >= extractors_.size()) {
    extractors_.Resize(pos + 1);
  }

  extraction->SetEmptyStringIsNull(session_.GetFeature("empty_string_is_null"));

  extraction->SetForceEmptyString(session_.GetFeature("force_empty_string"));

  extractors_[pos].push_back(extraction);
}

void StatementImpl::RemoveBind(const String& name) {
  bool found = false;

  BindingBaseVec::iterator it = bindings_.begin();
  for (; it != bindings_.end();) {
    if ((*it)->name() == name) {
      it = bindings_.erase(it);
      found = true;
    } else {
      ++it;
    }
  }

  if (!found) {
    throw NotFoundException(name);
  }
}

size_t StatementImpl::ExtractedColumnCount(int data_set) const {
  if (USE_CURRENT_DATA_SET == data_set) {
    data_set = static_cast<int>(cur_data_set_);
  }

  if (columns_extracted_.size() > 0) {
    fun_check(data_set >= 0 && data_set < columns_extracted_.size());
    return columns_extracted_[data_set];
  }

  return 0;
}

size_t StatementImpl::ExtractedRowCount(int data_set) const {
  if (USE_CURRENT_DATA_SET == data_set) {
    data_set = static_cast<int>(cur_data_set_);
  }

  if (extractions().size() > 0) {
    fun_check(data_set >= 0 && data_set < extractors_.size());
    if (extractors_[data_set].size() > 0) {
      return extractors_[data_set][0]->HandledRowsCount();
    }
  }

  return 0;
}

size_t StatementImpl::SubTotalRowCount(int data_set) const {
  if (USE_CURRENT_DATA_SET == data_set) {
    data_set = static_cast<int>(cur_data_set_);
  }

  if (sub_total_row_count_.size() > 0) {
    fun_check(data_set >= 0 && data_set < sub_total_row_count_.size());
    return sub_total_row_count_[data_set];
  }

  return 0;
}

void StatementImpl::FormatSql(std::vector<Any>& arguments) {
  String sql;
  fun::Format(sql, ostr_.str(), arguments);
  ostr_.str("");
  ostr_ << sql;
}

void StatementImpl::InsertHint() {
  // NOOP BY DEFAULT..
}

}  // namespace sql
}  // namespace fun
