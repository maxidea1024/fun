#include "fun/sql/row_formatter.h"
#include <iomanip>

namespace fun {
namespace sql {

RowFormatter::RowFormatter(const String& prefix, const String& postfix,
                           Mode mode)
    : prefix_(prefix), postfix_(postfix), mode_(mode), total_row_count_(0) {}

RowFormatter::~RowFormatter() {}

String& RowFormatter::FormatNames(const NameVecPtr names,
                                  String& formatted_names) {
  formatted_names.clear();
  return formatted_names;
}

void RowFormatter::FormatNames(const NameVecPtr names) {}

String& RowFormatter::FormatValues(const ValueVec& /*vals*/,
                                   String& formatted_values) {
  formatted_values.clear();
  return formatted_values;
}

void RowFormatter::FormatValues(const ValueVec& /*vals*/) {}

const String& RowFormatter::ToString() {
  throw NotImplementedException("RowFormatter::ToString()");
}

void RowFormatter::Reset() {
  prefix_ = "";
  postfix_ = "";
  total_row_count_ = INVALID_ROW_COUNT;
}

}  // namespace sql
}  // namespace fun
