// TODO accessor �̸��� �����ؾ��ұ�??

#pragma once

#include <cstddef>
#include "fun/sql/sql.h"

namespace fun {
namespace sql {

/**
 * MetaColumn class contains column metadata information.
 */
class FUN_SQL_API MetaColumn {
 public:
  enum ColumnDataType {
    FDT_BOOL,
    FDT_INT8,
    FDT_UINT8,
    FDT_INT16,
    FDT_UINT16,
    FDT_INT32,
    FDT_UINT32,
    FDT_INT64,
    FDT_UINT64,
    FDT_FLOAT,
    FDT_DOUBLE,
    FDT_STRING,
    FDT_WSTRING,
    FDT_BLOB,
    FDT_CLOB,
    FDT_DATE,
    FDT_TIME,
    FDT_TIMESTAMP,
    FDT_UNKNOWN
  };

  /**
   * Creates the MetaColumn.
   */
  MetaColumn();

  /**
   * Creates the MetaColumn.
   */
  explicit MetaColumn(size_t position, const String& name = "",
                      ColumnDataType type = FDT_UNKNOWN, size_t length = 0,
                      size_t precision = 0, bool nullable = false);

  /**
   * Destroys the MetaColumn.
   */
  virtual ~MetaColumn();

  /**
   * Returns column name.
   */
  const String& name() const;

  /**
   * Returns column maximum length.
   */
  size_t length() const;

  /**
   * Returns column precision.
   * Valid for floating point fields only
   * (zero for other data types).
   */
  size_t precision() const;

  /**
   * Returns column position.
   */
  size_t position() const;

  /**
   * Returns column type.
   */
  ColumnDataType type() const;

  /**
   * Returns true if column allows null values, false otherwise.
   */
  bool IsNullable() const;

 protected:
  /**
   * Sets the column name.
   */
  void SetName(const String& name);

  /**
   * Sets the column length.
   */
  void SetLength(size_t length);

  /**
   * Sets the column precision.
   */
  void SetPrecision(size_t precision);

  /**
   * Sets the column data type.
   */
  void SetType(ColumnDataType type);

  /**
   * Sets the column nullability.
   */
  void SetNullable(bool nullable);

 private:
  String name_;
  size_t length_;
  size_t precision_;
  size_t position_;
  ColumnDataType type_;
  bool nullable_;
};

//
// inlines
//

inline const String& MetaColumn::name() const { return name_; }

inline size_t MetaColumn::length() const { return length_; }

inline size_t MetaColumn::precision() const { return precision_; }

inline size_t MetaColumn::position() const { return position_; }

inline MetaColumn::ColumnDataType MetaColumn::type() const { return type_; }

inline bool MetaColumn::IsNullable() const { return nullable_; }

inline void MetaColumn::SetName(const String& name) { name_ = name; }

inline void MetaColumn::SetLength(size_t length) { length_ = length; }

inline void MetaColumn::SetPrecision(size_t precision) {
  precision_ = precision;
}

inline void MetaColumn::SetType(ColumnDataType type) { type_ = type; }

inline void MetaColumn::SetNullable(bool nullable) { nullable_ = nullable; }

}  // namespace sql
}  // namespace fun
