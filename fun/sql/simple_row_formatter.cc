#include "fun/sql/simple_row_formatter.h"
#include <iomanip>
#include "fun/base/exception.h"

namespace fun {
namespace sql {

SimpleRowFormatter::SimpleRowFormatter(std::streamsize column_width,
                                       std::streamsize spacing)
    : col_width_(column_width), spacing_(spacing), row_count_(0) {}

SimpleRowFormatter::SimpleRowFormatter(const SimpleRowFormatter& other)
    : RowFormatter(other.prefix(), other.postfix()),
      col_width_(other.col_width_),
      spacing_(other.spacing_),
      row_count_(0) {}

SimpleRowFormatter::~SimpleRowFormatter() {}

SimpleRowFormatter& SimpleRowFormatter::operator=(
    const SimpleRowFormatter& row) {
  SimpleRowFormatter tmp(row);
  Swap(tmp);
  return *this;
}

void SimpleRowFormatter::Swap(SimpleRowFormatter& other) {
  SetPrefix(other.prefix());
  SetPostfix(other.postfix());
  fun::Swap(col_width_, other.col_width_);
  fun::Swap(spacing_, other.spacing_);
}

String& SimpleRowFormatter::FormatNames(const NameVecPtr names,
                                        String& formatted_names) {
  row_count_ = 0;

  std::ostringstream str;
  String line(String::size_type(names->size() * col_width_ +
                                (names->size() - 1) * spacing_),
              '-');
  String space(static_cast<size_t>(spacing_), ' ');
  NameVec::const_iterator it = names->begin();
  NameVec::const_iterator end = names->end();
  for (; it != end; ++it) {
    if (it != names->begin()) {
      str << space;
    }
    str << std::left << std::setw(col_width_) << *it;
  }
  str << std::endl << line << std::endl;

  return formatted_names = str.str();
}

String& SimpleRowFormatter::FormatValues(const ValueVec& vals,
                                         String& formatted_values) {
  std::ostringstream str;
  String space(static_cast<size_t>(spacing_), ' ');
  ValueVec::const_iterator it = vals.begin();
  ValueVec::const_iterator end = vals.end();
  for (; it != end; ++it) {
    if (it != vals.begin()) str << space;
    if (it->IsNumeric()) {
      str << std::right << std::fixed << std::setprecision(2);
    } else {
      str << std::left;
    }

    if (!it->IsEmpty()) {
      str << std::setw(col_width_) << it->Convert<String>();
    } else {
      str << std::setw(col_width_) << "null";
    }
  }
  str << std::endl;

  ++row_count_;

  return formatted_values = str.str();
}

}  // namespace sql
}  // namespace fun
