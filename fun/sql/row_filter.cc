#include "fun/sql/row_filter.h"
#include <functional>
#include "fun/base/exception.h"
#include "fun/base/string.h"
#include "fun/sql/record_set.h"

namespace fun {
namespace sql {

RowFilter::RowFilter(RecordSet& recordSet)
    : record_set_(&recordSet), not_(false) {
  Init();
  Ptr this_ptr(this, true);
  record_set_->Filter(this_ptr);
}

RowFilter::RowFilter(Ptr parent, LogicOperator op)
    : record_set_(nullptr), parent_(parent), not_(false) {
  fun_check_ptr(parent_.Get());
  Init();
  Ptr this_ptr(this, true);
  parent_->AddFilter(this_ptr, op);
}

void RowFilter::Init() {
  comparisons_.insert(Comparisons::value_type("<", VALUE_LESS_THAN));
  comparisons_.insert(Comparisons::value_type("<=", VALUE_LESS_THAN_OR_EQUAL));
  comparisons_.insert(Comparisons::value_type("=", VALUE_EQUAL));
  comparisons_.insert(Comparisons::value_type("==", VALUE_EQUAL));
  comparisons_.insert(Comparisons::value_type(">", VALUE_GREATER_THAN));
  comparisons_.insert(
      Comparisons::value_type(">=", VALUE_GREATER_THAN_OR_EQUAL));
  comparisons_.insert(Comparisons::value_type("<>", VALUE_NOT_EQUAL));
  comparisons_.insert(Comparisons::value_type("!=", VALUE_NOT_EQUAL));
  comparisons_.insert(Comparisons::value_type("IS NULL", VALUE_IS_NULL));
}

RowFilter::~RowFilter() {}

bool RowFilter::IsAllowed(size_t row) const {
  fun::dynamic::Var ret;
  const RecordSet& rs = GetRecordSet();

  size_t columns = rs.columnCount();
  ComparisonMap::const_iterator it = comparison_map_.begin();
  ComparisonMap::const_iterator end = comparison_map_.end();
  for (; it != end; ++it) {
    for (size_t col = 0; col < columns; ++col) {
      const String name =
          toUpper(rs.MetaColumnAt(static_cast<uint32>(col)).name());
      if (comparison_map_.find(name) == comparison_map_.end()) {
        continue;
      }

      fun::dynamic::Var ret;
      CompT comp_op = 0;
      fun::dynamic::Var val = rs.value(col, row, false);

      switch (it->second.Get<1>()) {
        case VALUE_LESS_THAN:
          comp_op = less;
          break;
        case VALUE_LESS_THAN_OR_EQUAL:
          comp_op = lessOrEqual;
          break;
        case VALUE_EQUAL:
          comp_op = equal;
          break;
        case VALUE_GREATER_THAN:
          comp_op = greater;
          break;
        case VALUE_GREATER_THAN_OR_EQUAL:
          comp_op = greaterOrEqual;
          break;
        case VALUE_NOT_EQUAL:
          comp_op = notEqual;
          break;
        case VALUE_IS_NULL:
          comp_op = IsNull;
          break;
        default:
          throw IllegalStateException("Unsupported comparison criteria.");
      }

      DoCompare(ret, val, comp_op, it->second);
      if (ret.IsEmpty()) {
        ret = ret;
      } else {
        ret = ret || ret;
      }
    }
  }

  // iterate through children
  FilterMap::const_iterator fIt = fileter_map_.begin();
  FilterMap::const_iterator fEnd = fileter_map_.end();
  for (; fIt != fEnd; ++fIt) {
    if (OP_OR == fIt->second) {
      if (ret.IsEmpty()) {
        ret = fIt->first->IsAllowed(row);
      } else {
        ret = ret || fIt->first->IsAllowed(row);
      }
    } else if (OP_AND == fIt->second) {
      if (ret.IsEmpty()) {
        ret = fIt->first->IsAllowed(row);
      } else {
        ret = ret && fIt->first->IsAllowed(row);
      }
    } else {
      throw IllegalStateException("Unknown logical operation.");
    }
  }

  if (ret.IsEmpty()) {
    ret = true;  // no filtering found
  }

  return (!not_) && ret.extract<bool>();
}

int RowFilter::Remove(const String& name) {
  fun_check_ptr(record_set_);
  record_set_->MoveFirst();
  return static_cast<int>(comparison_map_.erase(toUpper(name)));
}

RowFilter::Comparison RowFilter::GetComparison(const String& comp) const {
  Comparisons::const_iterator it = comparisons_.find(toUpper(comp));
  if (it == comparisons_.end()) {
    throw NotFoundException("Comparison not found", comp);
  }

  return it->second;
}

void RowFilter::AddFilter(Ptr filter, LogicOperator comparison) {
  fun_check_ptr(record_set_);

  filter->record_set_ = record_set_;
  record_set_->MoveFirst();
  fileter_map_.insert(FilterMap::value_type(filter, comparison));
}

void RowFilter::removeFilter(Ptr filter) {
  fun_check_ptr(record_set_);

  record_set_->MoveFirst();
  fileter_map_.erase(filter);
  filter->record_set_ = 0;
  filter->parent_ = 0;
}

void RowFilter::DoCompare(fun::dynamic::Var& ret, fun::dynamic::Var& val,
                          CompT comp, const ComparisonEntry& ce) {
  if (ret.IsEmpty()) {
    ret = comp(val, ce.Get<0>());
  } else {
    if (ce.Get<2>() == OP_OR) {
      ret = ret || comp(val, ce.Get<0>());
    } else if (ce.Get<2>() == OP_AND) {
      ret = ret && comp(val, ce.Get<0>());
    } else {
      throw IllegalStateException("Unknown logical operation.");
    }
  }
}

RecordSet& RowFilter::GetRecordSet() const {
  if (!record_set_) {
    Ptr parent = parent_.lock();
    while (parent && !record_set_) {
      record_set_ = parent->record_set_;
    }
  }
  fun_check_ptr(record_set_);
  return *record_set_;
}

void RowFilter::RewindRecordSet() {
  if (record_set_) {
    record_set_->MoveFirst();
  }
}

}  // namespace sql
}  // namespace fun
