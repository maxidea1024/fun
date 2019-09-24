#pragma once

#include <mysql.h>
#include <vector>
#include "fun/sql/meta_column.h"

namespace fun {
namespace sql {
namespace mysql {

/**
 * MySQL result metadata
 */
class ResultMetadata {
 public:
  /**
   * Resets the metadata.
   */
  void Reset();

  /**
   * Initializes the metadata.
   */
  void Init(MYSQL_STMT* stmt);

  /**
   * Returns the number of columns in resultset.
   */
  size_t ReturnedColumnCount() const;

  /**
   * Returns the reference to the specified metacolumn.
   */
  const MetaColumn& MetaColumnAt(size_t pos) const;

  /**
   * Returns pointer to native row.
   */
  MYSQL_BIND* Row();

  /**
   * Returns the length.
   */
  size_t length(size_t pos) const;

  /**
   * Returns raw data.
   */
  const unsigned char* rawData(size_t pos) const;

  /**
   * Returns true if value at pos is null.
   */
  bool IsNull(size_t pos) const;

 private:
  std::vector<MetaColumn> columns_;
  std::vector<MYSQL_BIND> row_;
  std::vector<char> buffer_;
  std::vector<unsigned long> lengths_;
  std::vector<my_bool> is_null_;
};

}  // namespace mysql
}  // namespace sql
}  // namespace fun
