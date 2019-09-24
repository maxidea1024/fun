#pragma once

#include <sstream>
#include <vector>
#include "fun/base/dynamic/var.h"
#include "fun/base/shared_ptr.h"
#include "fun/ref_counted_object.h"
#include "fun/sql/sql.h"

namespace fun {
namespace sql {

/**
 * Row formatter is an abstract class providing definition for row formatting
 * functionality. For custom formatting strategies, inherit from this class and
 * override FormatNames() and FormatValues() member functions.
 *
 * Row formatter can be either passed to the RecordSet at construction time,
 * like in the following example:
 *
 * RecordSet rs(session, "SELECT * FROM Table", new MyRowFormater);
 *
 * or it can be supplied to the statement as in the following example:
 *
 * MyRowFormatter rf
 * session << "SELECT * FROM Table", format(rf);
 *
 * If no formatter is externally supplied to the statement, the
 * SimpleRowFormatter is used. Statement always has the ownership of the row
 * formatter and shares it with rows through RecordSet.
 *
 * To accommodate for various formatting needs, a formatter can operate in two
 * modes:
 *
 *    - progressive: formatted individual row strings are generated and returned
 * from each call to FormatValues; String& FormatNames(const NameVecPtr,
 * String&) and String& FormatValues(const ValueVec&, String&) member calls
 * should be used in this case; this is the default mode
 *
 *   - bulk: formatted resulting string is accumulated internally and obtained
 * at the end of iteration via ToString() member function; void
 * FormatNames(const NameVecPtr) and void FormatValues(const ValueVec&) member
 * calls should be used in this case
 *
 * When formatter is used in conjunction with Row/RecordSet, the formatting
 * members corresponding to the formatter mode are expected to be implemented.
 * If a call is propagated to this parent class, the functions do nothing or
 * silently return empty string respectively.
 */
class FUN_SQL_API RowFormatter {
 public:
  typedef SharedPtr<RowFormatter> Ptr;
  typedef std::vector<String> NameVec;
  typedef SharedPtr<std::vector<String> > NameVecPtr;
  typedef std::vector<fun::dynamic::Var> ValueVec;

  static const int32 INVALID_ROW_COUNT = -1;

  enum Mode { FORMAT_PROGRESSIVE, FORMAT_BULK };

  /**
   * Creates the RowFormatter and sets the prefix and postfix to specified
   * values.
   */
  RowFormatter(const String& prefix = "", const String& postfix = "",
               Mode mode = FORMAT_PROGRESSIVE);

  /**
   * Destroys the RowFormatter.
   */
  virtual ~RowFormatter();

  /**
   * Should be implemented to format the row fields names and return the
   * formatted string. The default implementation clears the names string and
   * returns it.
   */
  virtual String& FormatNames(const NameVecPtr names, String& formatted_names);

  /**
   * Should be implemented to format the row fields names.
   * The default implementation does nothing.
   */
  virtual void FormatNames(const NameVecPtr names);

  /**
   * Should be implemented to format the row fields values and return the
   * formatted string. The default implementation clears the values string and
   * returns it.
   */
  virtual String& FormatValues(const ValueVec& vals, String& formatted_values);

  /**
   * Should be implemented to format the row fields values.
   * The default implementation does nothing.
   */
  virtual void FormatValues(const ValueVec& vals);

  /**
   * Throws NotImplementedException. Formatters operating in bulk mode should
   * implement this member function to return valid pointer to the formatted
   * result.
   */
  virtual const String& ToString();

  /**
   * Returns INVALID_ROW_COUNT. Must be implemented by inheriting classes
   * which maintain count of processed rows.
   */
  virtual int32 RowCount() const;

  /**
   * Returns zero. Must be implemented by inheriting classes.
   * Typically, total row count shall be set up front through
   * SetTotalRowCount() call.
   */
  int32 GetTotalRowCount() const;

  /**
   * Sets total row count.
   */
  void SetTotalRowCount(int32 count);

  /**
   * Returns prefix string;
   */
  virtual const String& GetPrefix() const;

  /**
   * Returns postfix string;
   */
  virtual const String& GetPostfix() const;

  /**
   * Resets the formatter by setting prefix and postfix
   * to empty strings and row count to INVALID_ROW_COUNT.
   */
  void Reset();

  /**
   * Returns the formatter mode.
   */
  Mode GetMode() const;

  /**
   * Sets the formatter mode.
   */
  void SetMode(Mode mode);

 protected:
  /**
   * Sets the prefix for the formatter.
   */
  void SetPrefix(const String& prefix);

  /**
   * Sets the postfix for the formatter
   */
  void SetPostfix(const String& postfix);

  /**
   * Adjusts prefix after it has been set.
   * No-op here, called by SetTotalRowCount and
   * should be implemented by inheriting classes,
   * if needed.
   */
  virtual void AdjustPrefix();

 private:
  mutable String prefix_;
  mutable String postfix_;
  Mode mode_;
  int32 total_row_count_;
};

//
// inlines
//

inline int32 RowFormatter::RowCount() const { return INVALID_ROW_COUNT; }

inline int32 RowFormatter::GetTotalRowCount() const { return total_row_count_; }

inline void RowFormatter::SetTotalRowCount(int32 count) {
  total_row_count_ = count;
  AdjustPrefix();
}

inline void RowFormatter::SetPrefix(const String& prefix) { prefix_ = prefix; }

inline void RowFormatter::SetPostfix(const String& postfix) {
  postfix_ = postfix;
}

inline const String& RowFormatter::GetPrefix() const { return prefix_; }

inline const String& RowFormatter::GetPostfix() const { return postfix_; }

inline RowFormatter::Mode RowFormatter::GetMode() const { return mode_; }

inline void RowFormatter::SetMode(Mode mode) { mode_ = mode; }

inline void RowFormatter::AdjustPrefix() {}

namespace Keywords {

/**
 * Utility function used to pass formatter to the statement.
 */
template <typename T>
inline RowFormatter::Ptr format(const T& formatter) {
  return new T(formatter);
}

}  // namespace Keywords

}  // namespace sql
}  // namespace fun
