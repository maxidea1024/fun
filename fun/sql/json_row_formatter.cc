#include "fun/sql/json_row_formatter.h"
#include "fun/base/string.h"
#include "fun/json_string.h"
#include "fun/base/format.h"

using fun::trimInPlace;
using fun::Format;
using fun::toJSON;

namespace fun {
namespace sql {

const int JsonRowFormatter::JSON_FMT_MODE_SMALL;
const int JsonRowFormatter::JSON_FMT_MODE_ROW_COUNT;
const int JsonRowFormatter::JSON_FMT_MODE_COLUMN_NAMES;
const int JsonRowFormatter::JSON_FMT_MODE_FULL;

JsonRowFormatter::JsonRowFormatter(int mode)
  : RowFormatter("{", "]}"), first_time_(true) {
  if (mode == JSON_FMT_MODE_FULL) {
    mode |= JSON_FMT_MODE_ROW_COUNT;
    mode |= JSON_FMT_MODE_COLUMN_NAMES;
  }

  SetJsonMode(mode);
}

JsonRowFormatter::~JsonRowFormatter() {}

void JsonRowFormatter::AdjustPrefix() {
  if (PrintRowCount()) {
    std::ostringstream ostr;
    ostr << "{\"count\":" << GetTotalRowCount() << ",";
    if (mode_ & JSON_FMT_MODE_FULL) {
      ostr << '[';
    }
    SetPrefix(ostr.str());
  }
}

void JsonRowFormatter::SetJsonMode(int mode) {
  if (mode < JSON_FMT_MODE_SMALL ||
      mode > (JSON_FMT_MODE_SMALL | JSON_FMT_MODE_ROW_COUNT | JSON_FMT_MODE_COLUMN_NAMES | JSON_FMT_MODE_FULL)) {
    throw fun::InvalidArgumentException(
          fun::Format("JsonRowFormatter mode must be between "
          "%d (JSON_FMT_MODE_SMALL) and %d (JSON_FMT_MODE_FULL)",
          JSON_FMT_MODE_SMALL,
          JSON_FMT_MODE_FULL));
  }

  mode_ = mode;
  if (!(mode_ & JSON_FMT_MODE_SMALL) && !(mode_ & JSON_FMT_MODE_FULL)) {
    mode_ |= JSON_FMT_MODE_SMALL;
  } else if (mode_ & JSON_FMT_MODE_FULL) {
    mode_ |= JSON_FMT_MODE_ROW_COUNT;
  }

  AdjustPrefix();
}

String& JsonRowFormatter::FormatValues(const ValueVec& vals, String& formatted_values) {
  std::ostringstream str;
  if (!first_time_) {
    str << ',';
  }
  if (IsSmall()) {
    if (first_time_) {
      if (PrintColumnNames()) {
        str << ",\"values\":";
      }

      str << '[';
    }

    str << '[';
    ValueVec::const_iterator it = vals.begin();
    ValueVec::const_iterator end = vals.end();
    for (; it != end;) {
      if (!it->IsEmpty()) {
        if (it->IsString() || it->IsDate() || it->IsTime()) {
          String val = it->convert<String>();
          trimInPlace(val);
          str << toJSON(val);
        } else {
          str << it->convert<String>();
        }
      } else {
        str << "null";
      }

      if (++it == end) {
        break;
      }

      str << ',';
    }
    str << ']';
  } else if (IsFull()) {
    str << '{';
    ValueVec::const_iterator it = vals.begin();
    ValueVec::const_iterator end = vals.end();
    NameVec::iterator nIt = names_->begin();
    NameVec::iterator nEnd = names_->end();
    for (; it != end && nIt != nEnd; ++nIt) {
      if (!it->IsEmpty()) {
        if (it->IsString() || it->IsDate() || it->IsTime()) {
          String val = it->convert<String>();
          trimInPlace(val);
          str << '"' << *nIt << "\":" << toJSON(val);
        } else {
          str << '"' << *nIt << "\":" << it->convert<String>();
        }
      } else {
        str << '"' << *nIt << "\":null";
      }

      if (++it != end) {
        str << ',';
      }
    }
    str << '}';
  }

  first_time_ = false;
  return formatted_values = str.str();
}

String& JsonRowFormatter::FormatNames(const NameVecPtr names, String& formatted_names) {
  if (IsFull()) {
    // names are used in FormatValues
    if (names && !names_) {
      names_ = names;
    }
    return formatted_names = "";
  } else if (PrintColumnNames()) {
    std::ostringstream ostr;
    ostr << "\"names\":[";
    for (NameVec::const_iterator it = names->begin(), end = names->end();;) {
      ostr << '"' << *it << '"';
      if (++it == end) {
        break;
      }
      ostr << ',';
    }
    ostr << "]";
    return formatted_names = ostr.str();
  }

  return formatted_names = "";
}

} // namespace sql
} // namespace fun
