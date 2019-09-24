#include "fun/sql/odbc/preparator.h"
#include "fun/base/exception.h"
#include "fun/sql/odbc/odbc_meta_column.h"

using fun::InvalidArgumentException;

namespace fun {
namespace sql {
namespace odbc {

Preparator::Preparator(const StatementHandle& stmt, const String& statement,
                       size_t maxFieldSize, DataExtraction dataExtraction)
    : stmt_(stmt),
      max_field_size_(maxFieldSize),
      data_extraction_(dataExtraction) {
  SQLCHAR* pStr = (SQLCHAR*)statement.c_str();
  if (Utility::IsError(fun::sql::odbc::SQLPrepare(
          stmt_, pStr, (SQLINTEGER)statement.length()))) {
    throw StatementException(stmt_);
  }

  // PostgreSQL error swallowing workaround:
  // Postgres may execute a statement with syntax error fine,
  // but will return error later
  {
    SQLSMALLINT t = 0;
    SQLRETURN r = SQLNumResultCols(stmt, &t);
    if (r != SQL_NO_DATA && Utility::IsError(r)) {
      throw StatementException(stmt, "Failed to Get number of columns");
    }
  }
}

Preparator::Preparator(const Preparator& other)
    : stmt_(other.stmt_),
      max_field_size_(other.max_field_size_),
      data_extraction_(other.data_extraction_) {
  Resize();
}

Preparator::~Preparator() {
  try {
    FreeMemory();
  } catch (...) {
    fun_unexpected();
  }
}

void Preparator::FreeMemory() const {
  IndexMap::iterator it = var_length_arrays_.begin();
  IndexMap::iterator end = var_length_arrays_.end();
  for (; it != end; ++it) {
    switch (it->second) {
      case DT_BOOL:
        DeleteCachedArray<bool>(it->first);
        break;

      case DT_CHAR:
        DeleteCachedArray<char>(it->first);
        break;

      case DT_WCHAR:
        DeleteCachedArray<UString::value_type>(it->first);
        break;

      case DT_UCHAR:
        DeleteCachedArray<unsigned char>(it->first);
        break;

      case DT_CHAR_ARRAY: {
        char** pc = AnyCast<char*>(&values_[it->first]);
        if (pc) {
          std::free(*pc);
        }
        break;
      }

      case DT_WCHAR_ARRAY: {
        UString::value_type** pc =
            AnyCast<UString::value_type*>(&values_[it->first]);
        if (pc) {
          std::free(*pc);
        }
        break;
      }

      case DT_UCHAR_ARRAY: {
        unsigned char** pc = AnyCast<unsigned char*>(&values_[it->first]);
        if (pc) {
          std::free(*pc);
        }
        break;
      }

      case DT_BOOL_ARRAY: {
        bool** pb = AnyCast<bool*>(&values_[it->first]);
        if (pb) {
          std::free(*pb);
        }
        break;
      }

      default:
        throw InvalidArgumentException("Unknown data type.");
    }
  }
}

size_t Preparator::columns() const {
  if (values_.IsEmpty()) {
    Resize();
  }
  return values_.size();
}

void Preparator::Resize() const {
  SQLSMALLINT col_count = 0;
  int rc = SQLNumResultCols(stmt_, &col_count);
  if (!Utility::IsError(static_cast<SQLRETURN>(rc)) && (0 != col_count)) {
    values_.Resize(col_count, 0);
    huge_flags_.Resize(col_count, false);
    lengths_.Resize(col_count, 0);
    len_lengths_.Resize(col_count);
    if (var_length_arrays_.size()) {
      FreeMemory();
      var_length_arrays_.clear();
    }
  }
}

size_t Preparator::maxDataSize(size_t pos) const {
  fun_check_dbg(pos < values_.size());

  size_t sz = 0;
  size_t maxsz = GetMaxFieldSize();

  try {
    OdbcMetaColumn mc(stmt_, pos);
    sz = mc.length();

    // accommodate for terminating zero (non-bulk only!)
    MetaColumn::ColumnDataType type = mc.type();
    if (!IsBulk() && ((OdbcMetaColumn::FDT_WSTRING == type) ||
                      (OdbcMetaColumn::FDT_STRING == type))) {
      ++sz;
    }
  } catch (StatementException&) {
  }

  if (!sz || sz > maxsz) {
    sz = maxsz;
  }

  return sz;
}

size_t Preparator::actualDataSize(size_t col, size_t row) const {
  SQLLEN size = (FUN_DATA_INVALID_ROW == row) ? lengths_.at(col)
                                              : len_lengths_.at(col).at(row);

  // workaround for drivers returning negative length
  if (size < 0 && SQL_NULL_DATA != size) {
    size *= -1;
  }

  return size;
}

void Preparator::PrepareBoolArray(size_t pos, SQLSMALLINT valueType,
                                  size_t length) {
  fun_check_dbg(DE_BOUND == data_extraction_);
  fun_check_dbg(pos < values_.size());
  fun_check_dbg(pos < lengths_.size());
  fun_check_dbg(pos < len_lengths_.size());

  bool* pArray = (bool*)std::calloc(length, sizeof(bool));

  values_[pos] = Any(pArray);
  lengths_[pos] = 0;
  len_lengths_[pos].Resize(length);
  var_length_arrays_.insert(IndexMap::value_type(pos, DT_BOOL_ARRAY));

  if (Utility::IsError(SQLBindCol(stmt_, (SQLUSMALLINT)pos + 1, valueType,
                                  (SQLPOINTER)pArray, (SQLINTEGER)sizeof(bool),
                                  &len_lengths_[pos][0]))) {
    throw StatementException(stmt_, "SQLBindCol()");
  }
}

}  // namespace odbc
}  // namespace sql
}  // namespace fun
