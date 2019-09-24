#include "fun/sql/mysql/result_metadata.h"
#include <cstring>
#include "fun/sql/mysql/mysql_exception.h"

namespace {

/**
 * Simple exception-safe wrapper
 */
class ResultMetadataHandle {
 public:
  explicit ResultMetadataHandle(MYSQL_STMT* stmt) {
    h = mysql_stmt_result_metadata(stmt);
  }

  ~ResultMetadataHandle() {
    if (h) {
      mysql_free_result(h);
    }
  }

  operator MYSQL_RES*() { return h; }

 private:
  MYSQL_RES* h;
};

/**
 * Convert field MySQL-type and field MySQL-length to actual field length
 */
size_t GetFieldLength(const MYSQL_FIELD& field) {
  switch (field.type) {
    case MYSQL_TYPE_TINY:
      return sizeof(char);
    case MYSQL_TYPE_SHORT:
      return sizeof(short);
    case MYSQL_TYPE_INT24:
    case MYSQL_TYPE_LONG:
      return sizeof(int32);
    case MYSQL_TYPE_FLOAT:
      return sizeof(float);
    case MYSQL_TYPE_DOUBLE:
      return sizeof(double);
    case MYSQL_TYPE_LONGLONG:
      return sizeof(int64);

    case MYSQL_TYPE_DATE:
    case MYSQL_TYPE_TIME:
    case MYSQL_TYPE_DATETIME:
      return sizeof(MYSQL_TIME);

    case MYSQL_TYPE_DECIMAL:
    case MYSQL_TYPE_NEWDECIMAL:
    case MYSQL_TYPE_STRING:
    case MYSQL_TYPE_VAR_STRING:
    case MYSQL_TYPE_TINY_BLOB:
    case MYSQL_TYPE_MEDIUM_BLOB:
    case MYSQL_TYPE_LONG_BLOB:
    case MYSQL_TYPE_BLOB:
      return field.length;

    default:
      throw fun::sql::MySql::StatementException("unknown field type");
  }
}

/**
 * Convert field MySQL-type to fun-type
 */
fun::sql::MetaColumn::ColumnDataType GetFieldType(const MYSQL_FIELD& field) {
  bool unsig = ((field.flags & UNSIGNED_FLAG) == UNSIGNED_FLAG);

  switch (field.type) {
    case MYSQL_TYPE_TINY:
      if (unsig) return fun::sql::MetaColumn::FDT_UINT8;
      return fun::sql::MetaColumn::FDT_INT8;

    case MYSQL_TYPE_SHORT:
      if (unsig) return fun::sql::MetaColumn::FDT_UINT16;
      return fun::sql::MetaColumn::FDT_INT16;

    case MYSQL_TYPE_INT24:
    case MYSQL_TYPE_LONG:
      if (unsig) return fun::sql::MetaColumn::FDT_UINT32;
      return fun::sql::MetaColumn::FDT_INT32;

    case MYSQL_TYPE_FLOAT:
      return fun::sql::MetaColumn::FDT_FLOAT;

    case MYSQL_TYPE_DECIMAL:
    case MYSQL_TYPE_NEWDECIMAL:
    case MYSQL_TYPE_DOUBLE:
      return fun::sql::MetaColumn::FDT_DOUBLE;

    case MYSQL_TYPE_LONGLONG:
      if (unsig) return fun::sql::MetaColumn::FDT_UINT64;
      return fun::sql::MetaColumn::FDT_INT64;

    case MYSQL_TYPE_DATE:
      return fun::sql::MetaColumn::FDT_DATE;

    case MYSQL_TYPE_TIME:
      return fun::sql::MetaColumn::FDT_TIME;

    case MYSQL_TYPE_DATETIME:
      return fun::sql::MetaColumn::FDT_TIMESTAMP;

    case MYSQL_TYPE_STRING:
    case MYSQL_TYPE_VAR_STRING:
      return fun::sql::MetaColumn::FDT_STRING;

    case MYSQL_TYPE_TINY_BLOB:
    case MYSQL_TYPE_MEDIUM_BLOB:
    case MYSQL_TYPE_LONG_BLOB:
    case MYSQL_TYPE_BLOB:
      return fun::sql::MetaColumn::FDT_BLOB;
    default:
      return fun::sql::MetaColumn::FDT_UNKNOWN;
  }
}

}  // namespace

namespace fun {
namespace sql {
namespace mysql {

void ResultMetadata::Reset() {
  columns_.Resize(0);
  row_.Resize(0);
  buffer_.Resize(0);
  lengths_.Resize(0);
  is_null_.Resize(0);
}

void ResultMetadata::Init(MYSQL_STMT* stmt) {
  ResultMetadataHandle h(stmt);

  if (!h) {
    // all right, it is normal
    // querys such an "INSERT INTO" just does not have result at all
    Reset();
    return;
  }

  size_t count = mysql_num_fields(h);
  MYSQL_FIELD* fields = mysql_fetch_fields(h);

  size_t column_size = 0;
  columns_.Reserve(count);

  {
    for (size_t i = 0; i < count; i++) {
      size_t size = GetFieldLength(fields[i]);
      if (size == 0xFFFFFFFF) {
        size = 0;
      }

      columns_.push_back(MetaColumn(i,                        // position
                                    fields[i].name,           // name
                                    GetFieldType(fields[i]),  // type
                                    size,                     // length
                                    0,                        // TODO: precision
                                    !IS_NOT_NULL(fields[i].flags)  // nullable
                                    ));

      column_size += columns_[i].length();
    }
  }

  buffer_.Resize(column_size);
  row_.Resize(count);
  lengths_.Resize(count);
  is_null_.Resize(count);

  size_t offset = 0;

  for (size_t i = 0; i < count; i++) {
    UnsafeMemory::Memset(&row_[i], 0, sizeof(MYSQL_BIND));
    unsigned int len = static_cast<unsigned int>(columns_[i].length());
    row_[i].buffer_type = fields[i].type;
    row_[i].buffer_length = len;
    row_[i].buffer = (len > 0) ? (&buffer_[0] + offset) : 0;
    row_[i].length = &lengths_[i];
    row_[i].is_null = &is_null_[i];
    row_[i].is_unsigned = (fields[i].flags & UNSIGNED_FLAG) > 0;

    offset += row_[i].buffer_length;
  }
}

size_t ResultMetadata::ReturnedColumnCount() const {
  return static_cast<size_t>(columns_.size());
}

const MetaColumn& ResultMetadata::metaColumn(size_t pos) const {
  return columns_[pos];
}

MYSQL_BIND* ResultMetadata::row() { return &row_[0]; }

size_t ResultMetadata::length(size_t pos) const { return lengths_[pos]; }

const unsigned char* ResultMetadata::rawData(size_t pos) const {
  return reinterpret_cast<const unsigned char*>(row_[pos].buffer);
}

bool ResultMetadata::IsNull(size_t pos) const { return (is_null_[pos] != 0); }

}  // namespace mysql
}  // namespace sql
}  // namespace fun
