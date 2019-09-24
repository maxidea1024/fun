#include "fun/sql/row_iterator.h"
#include "fun/sql/record_set.h"
#undef min
#undef max
#include <limits>

namespace fun {
namespace sql {

const size_t RowIterator::POSITION_END = std::numeric_limits<size_t>::max();

RowIterator::RowIterator(RecordSet* record_set, bool position_end)
    : record_set_(record_set), position_(position_end ? POSITION_END : 0) {}

RowIterator::RowIterator(RecordSet& record_set, bool position_end)
    : record_set_(&record_set), position_(position_end ? POSITION_END : 0) {}

RowIterator::RowIterator(const RowIterator& other)
    : record_set_(other.record_set_), position_(other.position_) {}

RowIterator::~RowIterator() {}

RowIterator& RowIterator::operator=(const RowIterator& other) {
  RowIterator tmp(other);
  Swap(tmp);
  return *this;
}

void RowIterator::Swap(RowIterator& other) {
  fun::Swap(record_set_, other.record_set_);
  fun::Swap(position_, other.position_);
}

void RowIterator::Increment() const {
  if (POSITION_END == position_) {
    throw RangeException("End of iterator reached.");
  }

  if (position_ < record_set_->StorageRowCount() - 1) {
    ++position_;
  } else {
    position_ = POSITION_END;
  }

  if (record_set_->GetFilter() && POSITION_END != position_) {
    while (!record_set_->IsAllowed(position_)) {
      Increment();
      if (POSITION_END == position_) {
        break;
      }
    }
  }
}

void RowIterator::Decrement() const {
  if (0 == position_) {
    throw RangeException("Beginning of iterator reached.");
  } else if (POSITION_END == position_) {
    position_ = record_set_->StorageRowCount() - 1;
  } else {
    --position_;
  }

  if (record_set_->GetFilter() && 0 != position_) {
    while (!record_set_->IsAllowed(position_)) {
      Decrement();
      if (0 == position_) {
        break;
      }
    }
  }
}

void RowIterator::SetPosition(size_t pos) const {
  if (position_ == pos) {
    return;
  }

  if (record_set_->GetFilter()) {
    size_t start = position_;
    if (position_ > pos) {
      size_t end = position_ - pos;
      for (; start > end; --start) {
        if (pos) {
          --pos;
        } else {
          throw RangeException("Invalid position argument.");
        }
      }
    } else {
      size_t end = pos - position_;
      for (; start < end; ++start) {
        if (record_set_->StorageRowCount() != pos) {
          ++pos;
        } else {
          throw RangeException("Invalid position argument.");
        }
      }
    }
  }

  if (pos < record_set_->StorageRowCount()) {
    position_ = pos;
  } else if (pos == record_set_->StorageRowCount()) {
    position_ = POSITION_END;
  } else {
    throw RangeException("Invalid position argument.");
  }
}

Row& RowIterator::operator*() const {
  if (POSITION_END == position_) {
    throw InvalidAccessException("End of iterator reached.");
  }

  return record_set_->row(position_);
}

Row* RowIterator::operator->() const {
  if (POSITION_END == position_) {
    throw InvalidAccessException("End of iterator reached.");
  }

  return &record_set_->row(position_);
}

const RowIterator& RowIterator::operator++() const {
  Increment();
  return *this;
}

RowIterator RowIterator::operator++(int) const {
  RowIterator old(*this);
  Increment();
  return old;
}

const RowIterator& RowIterator::operator--() const {
  Decrement();
  return *this;
}

RowIterator RowIterator::operator--(int) const {
  RowIterator old(*this);
  Decrement();
  return old;
}

RowIterator RowIterator::operator+(size_t diff) const {
  RowIterator ri(*this);
  ri.SetPosition(position_ + diff);
  return ri;
}

RowIterator RowIterator::operator-(size_t diff) const {
  if (diff > position_) {
    throw RangeException("Invalid position argument.");
  }

  RowIterator ri(*this);
  ri.SetPosition(position_ - diff);
  return ri;
}

}  // namespace sql
}  // namespace fun
