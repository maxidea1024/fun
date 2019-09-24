#pragma once

#include "fun/sql/row_formatter.h"

namespace fun {
namespace sql {

/**
 * Class for JSON formatting of data rows.
 *
 * Formatter can be configured to operate in four modes (and
 * certain combinations thereof) :
 *
 *    - small (condensed mode, only array of values)
 *
 *      Example:
 *      {
 *       [["Simpson", "Bart", "Springfield", 12],
 *       ["Simpson", "Lisa", "Springfield", 10]]
 *      }
 *
 *    - row count (total row count provided)
 *
 *      Example:
 *      {
 *       "count":2,
 *       [["Simpson", "Bart", "Springfield", 12],
 *        ["Simpson", "Lisa", "Springfield", 10]]
 *      }
 *
 *    - column names (column names provided as a string array)
 *
 *      Example:
 *      {
 *       "names":["LastName", "FirstName", "Address", "Age"],
 *       [["Simpson", "Bart", "Springfield", 12],
 *       ["Simpson", "Lisa", "Springfield", 10]]
 *      }
 *
 *    - full (total row count, column names provided in every row of data)
 *
 *      Example:
 *      {
 *       "count":2,
 *        [
 *         {"LastName": "Simpson", "FirstName": "Bart", "Address": "Springfield", "Age": 12},
 *         {"LastName": "Simpson", "FirstName": "Lisa", "Address": "Springfield", "Age": 10}
 *        ]
 *      }
 *
 * Total row count will be specified by the fun::SQLRecordSet. Note, however, that this is
 * not possible to do accurately in case of result set paging. For those cases, there is
 * SetTotalRowCount() member function, which allows to explicitly set the total row count.
 * If the total row count is preset on the formatter, the Data framework shall not interfere.
 */
class FUN_SQL_API JsonRowFormatter : public fun::sql::RowFormatter {
 public:
  static const int JSON_FMT_MODE_SMALL        = 1;
  static const int JSON_FMT_MODE_ROW_COUNT    = 2;
  static const int JSON_FMT_MODE_COLUMN_NAMES = 4;
  static const int JSON_FMT_MODE_FULL         = 8;

  /**
   * Creates a new JsonRowFormatter.
   */
  JsonRowFormatter(int mode = (JSON_FMT_MODE_COLUMN_NAMES | JSON_FMT_MODE_SMALL));

  /**
   * Destroys the JsonRowFormatter.
   */
  ~JsonRowFormatter();

  /**
   * Formats names.
   */
  String& FormatNames(const NameVecPtr names, String& formatted_names);

  /**
    // Formats values.
   */
  String& FormatValues(const ValueVec& vals, String& formatted_values);

  /**
   * Sets the mode. Valid mode values are:
   *   JSON_FMT_MODE_SMALL
   *   JSON_FMT_MODE_ROW_COUNT
   *   JSON_FMT_MODE_COLUMN_NAMES
   *   JSON_FMT_MODE_FULL
   */
  void SetJsonMode(int mode);

  /**
   * Returns true if row count printing is enabled,
   * false otherwise.
   */
  bool PrintRowCount();

  /**
   * Returns true if column names printing is enabled,
   * false otherwise.
   */
  bool PrintColumnNames();

  /**
   * Returns true if compact mode formatting is enabled,
   * false otherwise.
   */
  bool IsSmall();

  /**
   * Returns true if full mode formatting is enabled,
   * false otherwise.
   */
  bool IsFull();

 private:
  void AdjustPrefix();

  NameVecPtr names_;
  int mode_;
  bool first_time_;
};


//
// inlines
//

inline bool JsonRowFormatter::PrintRowCount() {
  return (mode_ & JSON_FMT_MODE_ROW_COUNT) != 0;
}

inline bool JsonRowFormatter::PrintColumnNames() {
  return (mode_ & JSON_FMT_MODE_COLUMN_NAMES) != 0;
}

inline bool JsonRowFormatter::IsSmall() {
  return (mode_ & JSON_FMT_MODE_SMALL) != 0;
}

inline bool JsonRowFormatter::IsFull() {
  return (mode_ & JSON_FMT_MODE_FULL) != 0;
}

} // namespace sql
} // namespace fun
