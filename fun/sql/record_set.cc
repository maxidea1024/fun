#include "fun/sql/record_set.h"
#include "fun/sql/date.h"
#include "fun/sql/time.h"
#include "fun/sql/sql_exception.h"
#include "fun/base/date_time.h"
#include "fun/base/utf_string.h"

using namespace fun::sql::Keywords;
using fun::DateTime;
using fun::UString;

namespace fun {
namespace sql {

RecordSet::RecordSet( const Statement& statement,
                      RowFormatter::Ptr row_formatter)
  : Statement(statement),
    current_row_(0),
    begin_(new RowIterator(this, 0 == ExtractedRowCount())),
    end_(new RowIterator(this, true)) {
  if (row_formatter) {
    SetRowFormatter(row_formatter);
  }
}

RecordSet::RecordSet( Session& session,
                      const String& query,
                      RowFormatter::Ptr row_formatter)
  : Statement((session << query, now)),
    current_row_(0),
    begin_(new RowIterator(this, 0 == ExtractedRowCount())),
    end_(new RowIterator(this, true)) {
  if (row_formatter) {
    SetRowFormatter(row_formatter);
  }
}

RecordSet::RecordSet(const RecordSet& other)
  : Statement(other),
    current_row_(other.current_row_),
    begin_(new RowIterator(this, 0 == ExtractedRowCount())),
    end_(new RowIterator(this, true)),
    filter_(other.filter_) {}

RecordSet::RecordSet(RecordSet&& other)
  : Statement(MoveTemp(other)) {
  current_row_ = other.current_row_; other.current_row_ = 0;
  begin_ = other.begin_; other.begin_ = nullptr;
  end_ = other.end_; other.end_ = nullptr;
  filter_ = other.filter_; other.filter_ = nullptr;
  row_map_ = MoveTemp(other.row_map_); other.row_map_.clear();
}

RecordSet::~RecordSet() {
  try {
    delete begin_;
    delete end_;

    RowMap::iterator it = row_map_.begin();
    RowMap::iterator itEnd = row_map_.end();
    for (; it != itEnd; ++it) {
      delete it->second;
    }
  } catch (...) {
    fun_unexpected();
  }
}

RecordSet& RecordSet::Reset(const Statement& stmt) {
  delete begin_;
  begin_ = 0;
  delete end_;
  end_ = 0;
  current_row_ = 0;
  Statement::SetTotalRowCount(StatementImpl::UNKNOWN_TOTAL_ROW_COUNT);

  RowMap::iterator it = row_map_.begin();
  RowMap::iterator end = row_map_.end();
  for (; it != end; ++it) delete it->second;
  row_map_.clear();

  Statement::operator = (stmt);

  begin_ = new RowIterator(this, 0 == ExtractedRowCount());
  end_ = new RowIterator(this, true);

  return *this;
}

fun::dynamic::Var RecordSet::value(size_t col, size_t data_row, bool use_filter) const {
  if (use_filter && IsFiltered() && !IsAllowed(data_row)) {
    throw InvalidAccessException("Row not allowed");
  }

  if (IsNull(col, data_row)) {
    return fun::dynamic::Var();
  }

  switch (columnType(col)) {
    case MetaColumn::FDT_BOOL:      return value<bool>(col, data_row, use_filter);
    case MetaColumn::FDT_INT8:      return value<int8>(col, data_row, use_filter);
    case MetaColumn::FDT_UINT8:     return value<uint8>(col, data_row, use_filter);
    case MetaColumn::FDT_INT16:     return value<int16>(col, data_row, use_filter);
    case MetaColumn::FDT_UINT16:    return value<uint16>(col, data_row, use_filter);
    case MetaColumn::FDT_INT32:     return value<int32>(col, data_row, use_filter);
    case MetaColumn::FDT_UINT32:    return value<uint32>(col, data_row, use_filter);
    case MetaColumn::FDT_INT64:     return value<int64>(col, data_row, use_filter);
    case MetaColumn::FDT_UINT64:    return value<uint64>(col, data_row, use_filter);
    case MetaColumn::FDT_FLOAT:     return value<float>(col, data_row, use_filter);
    case MetaColumn::FDT_DOUBLE:    return value<double>(col, data_row, use_filter);
    case MetaColumn::FDT_STRING:    return value<String>(col, data_row, use_filter);
    case MetaColumn::FDT_WSTRING:   return value<UString>(col, data_row, use_filter);
    case MetaColumn::FDT_BLOB:      return value<BLOB>(col, data_row, use_filter);
    case MetaColumn::FDT_CLOB:      return value<CLOB>(col, data_row, use_filter);
    case MetaColumn::FDT_DATE:      return value<Date>(col, data_row, use_filter);
    case MetaColumn::FDT_TIME:      return value<Time>(col, data_row, use_filter);
    case MetaColumn::FDT_TIMESTAMP: return value<DateTime>(col, data_row);
    default:
      throw UnknownTypeException("Data type not supported.");
  }
}

fun::dynamic::Var RecordSet::value(const String& name, size_t data_row, bool use_filter) const {
  if (use_filter && IsFiltered() && !IsAllowed(data_row)) {
    throw InvalidAccessException("Row not allowed");
  }

  if (IsNull(metaColumn(name).Position(), data_row)) {
    return fun::dynamic::Var();
  }

  switch (columnType(name)) {
    case MetaColumn::FDT_BOOL:      return Value<bool>(name, data_row, use_filter);
    case MetaColumn::FDT_INT8:      return Value<int8>(name, data_row, use_filter);
    case MetaColumn::FDT_UINT8:     return Value<uint8>(name, data_row, use_filter);
    case MetaColumn::FDT_INT16:     return Value<int16>(name, data_row, use_filter);
    case MetaColumn::FDT_UINT16:    return Value<uint16>(name, data_row, use_filter);
    case MetaColumn::FDT_INT32:     return Value<int32>(name, data_row, use_filter);
    case MetaColumn::FDT_UINT32:    return Value<uint32>(name, data_row, use_filter);
    case MetaColumn::FDT_INT64:     return Value<int64>(name, data_row, use_filter);
    case MetaColumn::FDT_UINT64:    return Value<uint64>(name, data_row, use_filter);
    case MetaColumn::FDT_FLOAT:     return Value<float>(name, data_row, use_filter);
    case MetaColumn::FDT_DOUBLE:    return Value<double>(name, data_row, use_filter);
    case MetaColumn::FDT_STRING:    return Value<String>(name, data_row, use_filter);
    case MetaColumn::FDT_WSTRING:   return Value<UString>(name, data_row, use_filter);
    case MetaColumn::FDT_BLOB:      return Value<BLOB>(name, data_row, use_filter);
    case MetaColumn::FDT_DATE:      return Value<Date>(name, data_row, use_filter);
    case MetaColumn::FDT_TIME:      return Value<Time>(name, data_row, use_filter);
    case MetaColumn::FDT_TIMESTAMP: return Value<DateTime>(name, data_row, use_filter);
    default:
      throw UnknownTypeException("Data type not supported.");
  }
}

Row& RecordSet::row(size_t pos) {
  size_t rowCnt = RowCount();
  if (0 == rowCnt || pos > rowCnt - 1) {
    throw RangeException("Invalid recordset row requested.");
  }

  RowMap::const_iterator it = row_map_.find(pos);
  Row* row_ptr = nullptr;
  size_t columns = ColumnCount();
  if (it == row_map_.end()) {
    if (row_map_.size()) {
      //reuse first row column names and sorting fields to save some memory
      row_ptr = new Row(row_map_.begin()->second->names(),
        row_map_.begin()->second->GetSortMap(),
        GetRowFormatter());

      for (size_t col = 0; col < columns; ++col) {
        row_ptr->set(col, value(col, pos));
      }
    } else {
      row_ptr = new Row;
      row_ptr->SetFormatter(GetRowFormatter());
      for (size_t col = 0; col < columns; ++col) {
        row_ptr->append(metaColumn(static_cast<uint32>(col)).name(), value(col, pos));
      }
    }

    row_map_.insert(RowMap::value_type(pos, row_ptr));
  } else {
    row_ptr = it->second;
    fun_check_ptr(row_ptr);
  }

  return *row_ptr;
}

size_t RecordSet::RowCount() const {
  if (0 == extractions().size() && 0 == ExtractedColumnCount()) {
    return 0;
  }

  fun_check(extractions().size());
  size_t rc = StorageRowCount();
  if (!IsFiltered()) {
    return rc;
  }

  size_t counter = 0;
  for (int data_row = 0; data_row < rc; ++data_row) {
    if (IsAllowed(data_row)) ++counter;
  }

  return counter;
}

bool RecordSet::IsAllowed(size_t data_row) const {
  if (!IsFiltered()) {
    return true;
  }

  return filter_->IsAllowed(data_row);
}

bool RecordSet::MoveFirst() {
  const size_t rc = StorageRowCount();
  if (rc > 0) {
    if (!IsFiltered()) {
      current_row_ = 0;
      return true;
    }

    size_t current_row = 0;
    while (!IsAllowed(current_row)) {
      if (current_row >= rc - 1) {
        return false;
      }
      ++current_row;
    }

    current_row_ = current_row;
    return true;
  } else {
    return false;
  }
}

bool RecordSet::MoveNext()
{
  size_t current_row = current_row_;
  do {
    if (current_row >= StorageRowCount() -1) {
      return false;
    }
    ++current_row;
  } while (IsFiltered() && !IsAllowed(current_row));

  current_row_ = current_row;
  return true;
}

bool RecordSet::MovePrevious() {
  size_t current_row = current_row_;
  do {
    if (current_row <= 0) {
      return false;
    }
    --current_row;
  } while (IsFiltered() && !IsAllowed(current_row));

  current_row_ = current_row;
  return true;
}

bool RecordSet::MoveLast() {
  if (StorageRowCount() > 0) {
    size_t current_row = SubTotalRowCount() - 1;
    if (!IsFiltered()) {
      current_row_ = current_row;
      return true;
    }

    while (!IsAllowed(current_row)) {
      if (current_row <= 0) {
        return false;
      }
      --current_row;
    }

    current_row_ = current_row;
    return true;
  }
  else {
    return false;
  }
}

void RecordSet::SetRowFormatter(RowFormatter::Ptr row_formatter) {
  if (row_formatter) {
    if (row_formatter->GetTotalRowCount() == RowFormatter::INVALID_ROW_COUNT) {
      row_formatter->SetTotalRowCount(static_cast<int>(GetTotalRowCount()));
    }

    Statement::SetRowFormatter(row_formatter);
    RowMap::iterator it = row_map_.begin();
    RowMap::iterator itEnd = row_map_.end();
    for (; it != itEnd; ++it) it->second->SetFormatter(GetRowFormatter());
  } else {
    throw NullPointerException("Null RowFormatter in RecordSet.");
  }
}

std::ostream& RecordSet::CopyNames(std::ostream& os) const {
  if (begin() == end()) return os;

  String names = (*begin_)->NamesToString();
  if (!names.IsEmpty()) os << names;
  return os;
}

std::ostream& RecordSet::CopyValues(std::ostream& os, size_t offset, size_t length) const {
  if (begin() == end()) return os;

  RowIterator it = *begin_ + offset;
  RowIterator itEnd = (RowIterator::POSITION_END != length) ? it + length : *end_;
  std::copy(it, itEnd, std::ostream_iterator<Row>(os));
  return os;
}

void RecordSet::FormatValues(size_t offset, size_t length) const {
  if (begin() == end()) return;

  RowIterator it = *begin_ + offset;
  RowIterator itEnd = (RowIterator::POSITION_END != length) ? it + length : *end_;
  String val;
  for (; it != itEnd; ++it) it->FormatValues();
}

std::ostream& RecordSet::Copy(std::ostream& os, size_t offset, size_t length) const {
  if (begin() == end()) {
    return os;
  }

  RowFormatter& rf = const_cast<RowFormatter&>((*begin_)->GetFormatter());
  rf.SetTotalRowCount(static_cast<int>(GetTotalRowCount()));
  if (RowFormatter::FORMAT_PROGRESSIVE == rf.GetMode()) {
    os << rf.prefix();
    CopyNames(os);
    CopyValues(os, offset, length);
    os << rf.postfix();
  } else {
    FormatNames();
    FormatValues(offset, length);
    os << rf.ToString();
  }

  return os;
}

void RecordSet::Filter(const fun::RefCountedPtr<RowFilter>& filter) {
  filter_ = filter;
}

bool RecordSet::IsFiltered() const {
  return filter_ && !filter_->IsEmpty();
}

} // namespace sql
} // namespace fun
