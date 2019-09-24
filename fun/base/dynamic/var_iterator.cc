#include "fun/base/dynamic/var_iterator.h"
#include "fun/base/dynamic/var.h"
//#include "fun/base/dynamic/struct.h"

#undef min
#undef max
#include <limits>

namespace fun {
namespace dynamic {

const size_t VarIterator::POSITION_END = std::numeric_limits<size_t>::max();

VarIterator::VarIterator(Var* var, bool position_end)
  : var_(var),
    position_(position_end ? POSITION_END : 0) {}

VarIterator::VarIterator(const VarIterator& other)
  : var_(other.var_),
    position_(other.position_) {}

VarIterator::~VarIterator() {}

VarIterator& VarIterator::operator = (const VarIterator& other) {
  VarIterator tmp(other);
  Swap(tmp);
  return *this;
}

void VarIterator::Swap(VarIterator& other) {
  using fun::Swap;

  Swap(var_, other.var_);
  Swap(position_, other.position_);
}

void VarIterator::Increment() const {
  if (POSITION_END == position_) {
    throw RangeException("End of iterator reached.");
  }

  if (position_ < var_->size() - 1) {
    ++position_;
  } else {
    position_ = POSITION_END;
  }
}

void VarIterator::Decrement() const {
  if (0 == position_) {
    throw RangeException("Beginning of iterator reached.");
  } else if (POSITION_END == position_) {
    position_ = var_->size() - 1;
  } else {
    --position_;
  }
}

void VarIterator::SetPosition(size_t pos) const {
  if (position_ == pos) {
    return;
  }

  if (pos < var_->size()) {
    position_ = pos;
  } else if (pos == var_->size()) {
    position_ = POSITION_END;
  } else {
    throw RangeException("Invalid position argument.");
  }
}

Var& VarIterator::operator * () const {
  if (POSITION_END == position_) {
    throw InvalidAccessException("End of iterator reached.");
  }

  return var_->operator[](position_);
}

Var* VarIterator::operator -> () const {
  if (POSITION_END == position_) {
    throw InvalidAccessException("End of iterator reached.");
  }

  return &var_->operator[](position_);
}

const VarIterator& VarIterator::operator ++ () const {
  Increment();
  return *this;
}

VarIterator VarIterator::operator ++ (int) const {
  VarIterator ret(*this);
  Increment();
  return ret;
}

const VarIterator& VarIterator::operator -- () const {
  Decrement();
  return *this;
}

VarIterator VarIterator::operator -- (int) const {
  VarIterator ret(*this);
  Decrement();
  return ret;
}

VarIterator VarIterator::operator + (size_t diff) const {
  VarIterator ret(*this);
  ret.SetPosition(position_ + diff);
  return ret;
}

VarIterator VarIterator::operator - (size_t diff) const {
  if (diff > position_) {
    throw RangeException("Invalid position argument.");
  }

  VarIterator ret(*this);
  ret.SetPosition(position_ - diff);
  return ret;
}

} // namespace dynamic
} // namespace fun
