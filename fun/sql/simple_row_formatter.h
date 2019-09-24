#pragma once

#include "fun/sql/row_formatter.h"
#include "fun/sql/sql.h"

namespace fun {
namespace sql {

/**
 * A simple row formatting class.
 */
class FUN_SQL_API SimpleRowFormatter : public RowFormatter {
 public:
  static const int32 DEFAULT_COLUMN_WIDTH = 16;
  static const int32 DEFAULT_SPACING = 1;

  /**
   * Creates the SimpleRowFormatter and sets the column width to specified
   * value.
   */
  SimpleRowFormatter(std::streamsize column_width = DEFAULT_COLUMN_WIDTH,
                     std::streamsize spacing = DEFAULT_SPACING);

  /**
   * Creates the copy of the supplied SimpleRowFormatter.
   */
  SimpleRowFormatter(const SimpleRowFormatter& other);

  /**
   * Assignment operator.
   */
  SimpleRowFormatter& operator=(const SimpleRowFormatter& row);

  /**
   * Destroys the SimpleRowFormatter.
   */
  ~SimpleRowFormatter();

  /**
   * Swaps the row formatter with another one.
   */
  void Swap(SimpleRowFormatter& other);

  /**
   * Formats the row field names.
   */
  String& FormatNames(const NameVecPtr names, String& formatted_names);

  /**
   * Formats the row values.
   */
  String& FormatValues(const ValueVec& vals, String& formatted_values);

  /**
   * Returns row count.
   */
  int32 RowCount() const;

  /**
   * Sets the column width.
   */
  void SetColumnWidth(std::streamsize width);

  /**
   * Returns the column width.
   */
  std::streamsize GetColumnWidth() const;

  /**
   * Returns the spacing.
   */
  std::streamsize GetSpacing() const;

 private:
  std::streamsize col_width_;
  std::streamsize spacing_;
  int32 row_count_;
};

//
// inlines
//

inline int32 SimpleRowFormatter::RowCount() const { return row_count_; }

inline void SimpleRowFormatter::SetColumnWidth(std::streamsize column_width) {
  col_width_ = column_width;
}

inline std::streamsize SimpleRowFormatter::GetColumnWidth() const {
  return col_width_;
}

inline std::streamsize SimpleRowFormatter::GetSpacing() const {
  return spacing_;
}

}  // namespace sql
}  // namespace fun

namespace std {

/**
 * Full template specialization of std:::Swap for SimpleRowFormatter
 */
template <>
inline void Swap<fun::sql::SimpleRowFormatter>(
    fun::sql::SimpleRowFormatter& s1, fun::sql::SimpleRowFormatter& s2) {
  s1.Swap(s2);
}

}  // namespace std
