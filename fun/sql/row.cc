#include "fun/sql/row.h"
#include "fun/sql/simple_row_formatter.h"
#include "fun/base/string.h"
#include "fun/base/exception.h"

namespace fun {
namespace sql {

std::ostream& operator << (std::ostream &os, const Row& row) {
  os << row.ValuesToString();
  return os;
}

Row::Row()
  : names_(0),
    sort_map_(new SortMap),
    formatter_(new SimpleRowFormatter) {}

Row::Row( NameVecPtr names,
          const RowFormatter::Ptr& formatter)
  : names_(names) {
  if (!names_) {
    throw NullPointerException();
  }

  init(0, formatter);
}

Row::Row( NameVecPtr names,
          const SortMapPtr& sort_map,
          const RowFormatter::Ptr& formatter)
  : names_(names) {
  if (!names_) {
    throw NullPointerException();
  }

  init(sort_map, formatter);
}

void Row::init(const SortMapPtr& sort_map, const RowFormatter::Ptr& formatter) {
  SetFormatter(formatter);
  SetSortMap(sort_map);

  NameVec::size_type sz = names_->size();
  if (sz) {
    values_.resize(sz);
    // Row sortability in the strict weak ordering sense is
    // an invariant, hence we must start with a zero here.
    // If null value is later retrieved from DB, the
    // Var::empty() call should be used to empty
    // the corresponding Row value.
    values_[0] = 0;
    AddSortField(0);
  }
}

Row::~Row() {}

fun::dynamic::Var& Row::Get(size_t col) {
  try {
    return values_.at(col);
  } catch (std::out_of_range& re) {
    throw RangeException(re.what());
  }
}

const fun::dynamic::Var& Row::Get(size_t col) const {
  try {
    return values_.at(col);
  } catch (std::out_of_range& re) {
    throw RangeException(re.what());
  }
}

size_t Row::GetPosition(const String& name) const {
  if (!names_) {
    throw NullPointerException();
  }

  NameVec::const_iterator it = names_->begin();
  NameVec::const_iterator end = names_->end();
  size_t col = 0;
  for (; it != end; ++it, ++col) {
    if (0 == icompare(name, *it)) {
      return col;
    }
  }

  throw NotFoundException(name);
}

void Row::AddSortField(size_t pos) {
  fun_check(pos <= values_.size());

  SortMap::iterator it = sort_map_->begin();
  SortMap::iterator end = sort_map_->end();
  for (; it != end; ++it) {
    if (it->Get<0>() == pos) {
      return;
    }
  }

  ComparisonType ct;
  if (values_[pos].IsEmpty()) {
    ct = COMPARE_AS_EMPTY;
  } else if ((values_[pos].type() == typeid(int8))   ||
    (values_[pos].type() == typeid(uint8))  ||
    (values_[pos].type() == typeid(int16))  ||
    (values_[pos].type() == typeid(uint16)) ||
    (values_[pos].type() == typeid(int32))  ||
    (values_[pos].type() == typeid(uint32)) ||
    (values_[pos].type() == typeid(int64))  ||
    (values_[pos].type() == typeid(uint64)) ||
    (values_[pos].type() == typeid(bool))) {
    ct = COMPARE_AS_INTEGER;
  } else if ((values_[pos].type() == typeid(float)) ||
            (values_[pos].type() == typeid(double))) {
    ct = COMPARE_AS_FLOAT;
  } else {
    ct = COMPARE_AS_STRING;
  }

  sort_map_->push_back(SortTuple(pos, ct));
}

void Row::AddSortField(const String& name) {
  AddSortField(GetPosition(name));
}

void Row::RemoveSortField(size_t pos) {
  SortMap::iterator it = sort_map_->begin();
  SortMap::iterator end = sort_map_->end();
  for (; it != end; ++it) {
    if (it->Get<0>() == pos) {
      sort_map_->erase(it);
      return;
    }
  }
}

void Row::RemoveSortField(const String& name) {
  RemoveSortField(GetPosition(name));
}

void Row::ReplaceSortField(size_t old_pos, size_t new_pos) {
  fun_check(old_pos <= values_.size());
  fun_check(new_pos <= values_.size());

  ComparisonType ct;

  if (values_[new_pos].IsEmpty()) {
    ct = COMPARE_AS_EMPTY;
  }
  else if ((values_[new_pos].type() == typeid(int8)) ||
    (values_[new_pos].type() == typeid(uint8))     ||
    (values_[new_pos].type() == typeid(int16))     ||
    (values_[new_pos].type() == typeid(uint16))    ||
    (values_[new_pos].type() == typeid(int32))     ||
    (values_[new_pos].type() == typeid(uint32))    ||
    (values_[new_pos].type() == typeid(int64))     ||
    (values_[new_pos].type() == typeid(uint64))    ||
    (values_[new_pos].type() == typeid(bool))) {
    ct = COMPARE_AS_INTEGER;
  }
  else if ((values_[new_pos].type() == typeid(float)) ||
            (values_[new_pos].type() == typeid(double))) {
    ct = COMPARE_AS_FLOAT;
  } else {
    ct = COMPARE_AS_STRING;
  }

  SortMap::iterator it = sort_map_->begin();
  SortMap::iterator end = sort_map_->end();
  for (; it != end; ++it)
  {
    if (it->Get<0>() == old_pos)
    {
      *it = SortTuple(new_pos, ct);
      return;
    }
  }

  throw NotFoundException("Field not found");
}

void Row::ReplaceSortField(const String& old_name, const String& new_name) {
  ReplaceSortField(GetPosition(old_name), GetPosition(new_name));
}

void Row::ResetSort() {
  sort_map_->clear();
  if (values_.size()) {
    AddSortField(0);
  }
}

bool Row::IsEqualSize(const Row& other) const {
  return (other.values_.size() == values_.size());
}

bool Row::IsEqualType(const Row& other) const {
  std::vector<fun::dynamic::Var>::const_iterator it = values_.begin();
  std::vector<fun::dynamic::Var>::const_iterator end = values_.end();
  for (int i = 0; it != end; ++it, ++i) {
    if (it->type() != other.values_[i].type()) {
      return false;
    }
  }

  return true;
}

bool Row::operator == (const Row& other) const {
  if (!IsEqualSize(other)) return false;
  if (!IsEqualType(other)) return false;

  std::vector<fun::dynamic::Var>::const_iterator it = values_.begin();
  std::vector<fun::dynamic::Var>::const_iterator end = values_.end();
  for (int i = 0; it != end; ++it, ++i) {
    if ((*it).convert<String>() != other.values_[i].convert<String>()) {
      return false;
    }
  }

  return true;
}

bool Row::operator != (const Row& other) const {
  return !(*this == other);
}

bool Row::operator < (const Row& other) const {
  if (*sort_map_ != *other.sort_map_) {
    throw InvalidAccessException("Rows compared have different sorting criteria.");
  }

  SortMap::const_iterator it = sort_map_->begin();
  SortMap::const_iterator end = sort_map_->end();
  for (; it != end; ++it) {
    switch (it->Get<1>()) {
      case COMPARE_AS_EMPTY:
        return false;

      case COMPARE_AS_INTEGER:
        if (values_[it->Get<0>()].convert<int64>() <
          other.values_[it->Get<0>()].convert<int64>())
          return true;
        else if (values_[it->Get<0>()].convert<int64>() !=
          other.values_[it->Get<0>()].convert<int64>())
          return false;
        break;

      case COMPARE_AS_FLOAT:
        if (values_[it->Get<0>()].convert<double>() <
          other.values_[it->Get<0>()].convert<double>())
          return true;
        else if (values_[it->Get<0>()].convert<double>() !=
          other.values_[it->Get<0>()].convert<double>())
          return false;
        break;

      case COMPARE_AS_STRING:
        if (values_[it->Get<0>()].convert<String>() <
          other.values_[it->Get<0>()].convert<String>())
          return true;
        else if (values_[it->Get<0>()].convert<String>() !=
          other.values_[it->Get<0>()].convert<String>())
          return false;
        break;

      default:
        throw IllegalStateException("Unknown comparison criteria.");
    }
  }

  return false;
}

void Row::SetFormatter(const RowFormatter::Ptr& formatter) {
  if (formatter.Get()) {
    formatter_ = formatter;
  } else {
    formatter_ = new SimpleRowFormatter;
  }
}

void Row::SetSortMap(const SortMapPtr& sort_map) {
  if (sort_map.Get()) {
    sort_map_ = sort_map;
  } else {
    sort_map_ = new SortMap;
  }
}

const String& Row::NamesToString() const {
  if (!names_) {
    throw NullPointerException();
  }

  return formatter_->FormatNames(names(), name_str_);
}

void Row::FormatNames() const {
  if (!names_) {
    throw NullPointerException();
  }

  return formatter_->FormatNames(names());
}

} // namespace sql
} // namespace fun
